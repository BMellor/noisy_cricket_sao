/// Opens the make dependency file passed
///
/// Returns a tuple with the object file, and a list of dependencies
/// This supports most special chars escaped [or not] the way [gcc thinks]
/// make wants them, including spaces, unicode [utf8], and backslashes.
/// # Panics
/// On parse error of the dep file, but *NOT* if the file does not exist.
/// # Expected Format
/// build/obj/arm_src_main_cpp.o: arm/src/main.cpp arm/src/main.hpp \
/// device/arm/ST/STM32F0xx/inc/stm32f0xx.h \
/// device/arm/ST/STM32F0xx/inc/stm32f051x8.h cmsis/arm/Include/core_cm0.h \
/// cmsis/arm/Include/core_cmInstr.h cmsis/arm/Include/cmsis_gcc.h \
/// cmsis/arm/Include/core_cmFunc.h
pub fn parse_dependency(dep_file_path: &str) -> Option<(String, Vec<String>)> {
    if let Some(data_str) = std::fs::read_to_string(dep_file_path).ok() {
        let (object, mut deps, _state, partial) = data_str.chars().fold(
            (
                String::new(),
                Vec::new(),
                State::LookingForObject,
                String::new(),
            ),
            parser,
        );

        if partial != "" {
            deps.push(partial);
        }
        Some((object.replace(".o.temp", ".o"), deps))
    } else {
        None
    }
}
enum State {
    LookingForObject,
    Normal,
    EscapeVar,
    EscapeBack,
}

fn parser(
    (mut object, mut deps, mut state, mut partial): (String, Vec<String>, State, String),
    ch: char,
) -> (String, Vec<String>, State, String) {
    match state {
        State::LookingForObject => {
            if ch == ':' {
                state = State::Normal;
                object = partial;
                partial = String::new();
            } else {
                partial.push(ch);
            }
        }
        State::Normal => {
            if ch == '\\' {
                state = State::EscapeBack;
            } else if ch == '$' {
                state = State::EscapeVar;
            } else if ch.is_whitespace() || ch.is_ascii_control() {
                if partial != "" {
                    deps.push(partial);
                }
                partial = String::new();
            } else {
                partial.push(ch);
            }
        }
        State::EscapeBack => {
            if ch == '\\' {
                partial.push('/'); // Transform backslash to forward slash
            } else if ch == '\n' || ch == '\r' {
                // Swallow
            } else if ch == ' ' {
                partial.push(' ');
            } else if ch == '#' {
                partial.push('#');
            } else if ch == ':' {
                partial.push(':');
            } else {
                // Invalid escape generated, treat as / and char as per gcc/docs
                partial.push('/'); // Transform backslash to forward slash
                partial.push(ch);
            }
            state = State::Normal;
        }
        State::EscapeVar => {
            if ch == '$' {
                partial.push('$');
            } else {
                partial.push('$');
                partial.push(ch);
            }
            state = State::Normal;
        }
    }
    (object, deps, state, partial)
}

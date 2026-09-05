use build_system::*;

pub fn add(state: &mut State) {
    add_include_dir!(state, "inc");
    add_c_file!(state, "main.c");
    add_c_file!(state, "timebase.c");
    add_c_file!(state, "led.c");
}

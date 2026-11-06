use build_system::*;

pub fn add(state: &mut State) {
    add_include_dir!(state, "inc");
    add_c_file!(state, "main.c");
    add_c_file!(state, "timebase.c");
    add_c_file!(state, "led.c");
    add_c_file!(state, "audio.c");
    add_c_file!(state, "mic.c");
    add_c_file!(state, "fft.c");
    add_c_file!(state, "i2c_target.c");
    add_c_file!(state, "settings.c");
    add_c_file!(state, "cricket.c");
    add_c_file!(state, "selftest.c");
}

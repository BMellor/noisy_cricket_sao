use build_system::*;

mod mspm0;
mod src;

fn main() {
    let mut state = new_build_system!();

    state.set_project("mspm0");

    state.set_opt_level(Optimize::O2);
    // C11 and C++17 are now the default
    state.set_default_warnings();
    workarounds::gcc_12_2_linker_rwx_warning(&mut state);
    workarounds::newlib_nosys_stub_warnings(&mut state);

    state.add_linker_opt("--specs=nano.specs");
    state.add_linker_opt("--specs=nosys.specs");

    mspm0::add(&mut state, mspm0::Device::MSPM0C1104);

    src::add(&mut state);

    state.write_vscode_metadata(None);
    build_system::build(state);
}

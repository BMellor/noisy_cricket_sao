use crate::{add_c_file_sys, Optimize, State};

/// LD bundled with GCC 12.2.rel1 (and likely older and newer going forward) will complain
/// if you have any segments that are listed as read/write/execute.
/// This is a semi-reasonable idea on full OS system and that have an MMU/OS
/// that can enforce this. However, on the embedded systems that the
/// build system targets, we do not have any of that, and so it's just useless noise.
/// Disable this, but optionally, since older linkers do not understand this flag and die.
pub fn gcc_12_2_linker_rwx_warning(state: &mut State) {
    state.add_linker_opt("-Wl,--no-warn-rwx-segments");
}
/// Newer releases of gcc-arm ship compiled libnosys that has a warning for every stub that is provided.
/// Despite that the folks using libnosys are using it specifically because they want the stubs,
/// this has been the case since GCC11. As such, your options are to hack your c libraries on disk
/// to remove the `gnu.warning` symbols, or to just provide your own implementations without that garbage.
/// This goes with the latter solution. Unlike the shipped libnosys, this will silently just do it's job,
/// but will let you override anything that you wish to actually implement as needed.
///
/// This is safe to call on all compiler versions, including those that do not need this work around.
/// Note that libnosys is still needed unless you have a custom malloc, or your own implementation of `sbrk()`.
///
/// ```
/// state.add_linker_opt("--specs=nano.specs");
/// state.add_linker_opt("--specs=nosys.specs");
/// workarounds::newlib_nosys_stub_warnings(&mut state);
/// ```
pub fn newlib_nosys_stub_warnings(state: &mut State) {
    add_c_file_sys!(state, "nosys.c", Optimize::Os);
}

//! The buildsystem is a mostly feature complete replacement for "make".
//!
//! The primary flow for building a project is to create an object,
//! set the project name, CPU, and stadard versions, as well as warnings etc.,
//! and set the linker script, and start adding files to compile.
//!
//! Behavior is currently similar to make, in that it will only recompile files if they have changed
//! since the last compile (or rust binary was changed describing the project).
//! Dependencies are tracked with the gcc dependency output information.
//!
//! By default, the files are built in parallel without the compiler arguments printed,
//! but this can be surpressed by calling ```state.set_verbose(true)``` or cargo run -- verbose
//!
//! Several Internal Utilities are exposed under the util module for external scripts
//!
//! # Example
//! ```
//!     let mut state = State::new();
//!
//!     state.set_project("ProjectName");
//!     state.set_cpu(CPU::CortexM0);
//!     state.set_opt_level(Optimize::Og);
//!     state.set_c_standard(CStandard::C11);
//!     state.set_cpp_standard(CPPStandard::CPP14);
//!
//!     state.add_define("HSE_VALUE=8000000");
//!     state.add_define("STM32F051x8");
//!
//!     state.add_c_warning("double-promotion");
//!     state.add_cpp_warning("double-promotion");
//!     state.add_c_error("all");
//!     state.add_cpp_error("all");
//!
//!     state.add_linker_opt("--specs=nano.specs");
//!     state.add_linker_opt("--specs=nosys.specs");
//!
//!     cmsis::add(&mut state); // From a external crate
//!
//!     add_asm_file!(state, "ST/STM32F0xx/src/startup_stm32f051x8.s");
//!     linker_script!(state, "ST/STM32F0xx/src/STM32F051R8_FLASH.ld");
//!     add_include_dir!(state, "Drivers/");
//!     add_cpp_file!(state, "src/main.cpp");
//!
//!     add_c_file_opts!(state,
//!                      "DSP_Lib/Source/BasicMathFunctions/arm_add_f32.c",
//!                      false,
//!                      Opt::O3,
//!                      "");
//!
//!     build_system::build(state);
//! ```

mod builder;
mod command_args;
mod file_names;
mod options;
mod parse_makefile;
mod src_dependency;
mod state;

pub mod util;
pub mod workarounds;

pub use builder::*;
pub use options::*;
pub use state::*;

/// Create a new State. Equal to State::new(file!(), line!())
#[macro_export]
macro_rules! new_build_system {
    () => {
        State::new(file!(), line!())
    };
}

/// Adds the passed in &str to the list of includes in state
///
/// The path is relative to $src_file
/// # Examples
/// ```
/// add_include_dir!(state, "inc");
/// add_include_dir!(state, "foo/bar");
/// ```
#[macro_export]
macro_rules! add_include_dir {
    ($state:expr, $path:expr) => {
        $state.add_include_dir_macro_(file!(), $path);
    };
}
/// Adds the passed in &str to the list of system includes in state
/// Specifically, gcc does not warn on system includes
///
/// The path is relative to $src_file
/// # Examples
/// ```
/// add_system_include_dir!(state, "inc");
/// add_system_include_dir!(state, "foo/bar");
/// ```
#[macro_export]
macro_rules! add_system_include_dir {
    ($state:expr, $path:expr) => {
        $state.add_system_include_dir_macro_(file!(), $path);
    };
}
/// Adds the passed in &str to the list of c files in state
///
/// The path is relative to $src_file
/// # Examples
/// ```
/// add_c_file!(state, "src/foo.c");
/// ```
#[macro_export]
macro_rules! add_c_file {
    ($state:expr, $path:expr) => {
        $state.add_c_file_macro_(file!(), $path, None);
    };
}
/// Adds the passed in &str to the list of cpp files in state
///
/// The path is relative to $src_file
/// # Examples
/// ```
/// add_cpp_file!(state, "src/foo.cpp");
/// ```
#[macro_export]
macro_rules! add_cpp_file {
    ($state:expr, $path:expr) => {
        $state.add_cpp_file_macro_(file!(), $path, None);
    };
}
/// Adds the passed in &str to the list of c files in state with a modification to how it is compiled
///
/// The path is relative to $src_file
///
/// In particular, you may set a different optimization level, disable all warnings, or set additional compiler flags
/// # Examples
/// ```
/// add_c_file_opts!(state, "DSP_Lib/Source/BasicMathFunctions/arm_add_f32.c",
///                      false, // Warnings off
///                      Opt::O3, // Fully Optimized, even if system at -O0
///                      "");); // No additional flags
/// ```
#[macro_export]
macro_rules! add_c_file_opts {
    ($state:expr, $path:expr, $enable_warn:expr, $opt_level:expr, $args:expr) => {
        $state.add_c_file_macro_(
            file!(),
            $path,
            Some($crate::FileOptions::new($enable_warn, $opt_level, $args)),
        );
    };
}
/// Adds the passed in &str to the list of cpp files in state with a modification to how it is compiled
///
/// The path is relative to $src_file
///
/// In particular, you may set a different optimization level, disable all warnings, or set additional compiler flags
/// # Examples
/// ```
/// add_cpp_file_opts!(state, "src/foo.cpp",
///                      false, // Warnings off
///                      Opt::O3, // Fully Optimized, even if system at -O0
///                      "");); // No additional flags
/// ```
#[macro_export]
macro_rules! add_cpp_file_opts {
    ($state:expr, $path:expr, $enable_warn:expr, $opt_level:expr, $args:expr) => {
        $state.add_cpp_file_macro_(
            file!(),
            $path,
            Some($crate::FileOptions::new($enable_warn, $opt_level, $args)),
        );
    };
}
/// Adds the passed in &str to the list of c files in state with no warnings, and a specified opt level
///
/// The path is relative to $src_file
///
/// # Examples
/// ```
/// add_c_file_sys!(state, "DSP_Lib/Source/BasicMathFunctions/arm_add_f32.c",
///                      Opt::O3, // Fully Optimized, even if system at -O0);
/// ```
#[macro_export]
macro_rules! add_c_file_sys {
    ($state:expr, $path:expr, $opt_level:expr) => {
        $state.add_c_file_macro_(
            file!(),
            $path,
            Some($crate::FileOptions::new(false, $opt_level, "")),
        );
    };
}
/// Adds the passed in &str to the list of cpp files in state with no warnings, and a specified opt level
///
/// The path is relative to $src_file
///
/// # Examples
/// ```
/// add_cpp_file_sys!(state, "src/foo.cpp",
///                      Opt::O3, // Fully Optimized, even if system at -O0);
/// ```
#[macro_export]
macro_rules! add_cpp_file_sys {
    ($state:expr, $path:expr, $opt_level:expr) => {
        $state.add_cpp_file_macro_(
            file!(),
            $path,
            Some($crate::FileOptions::new(false, $opt_level, "")),
        );
    };
}
/// Adds the passed in &str to the list of assembler files in state
///
/// The path is relative to $src_file
/// # Examples
/// ```
/// add_asm_file!(state, "src/foo.s");
/// ```
#[macro_export]
macro_rules! add_asm_file {
    ($state:expr, $path:expr) => {
        $state.add_asm_file_macro_(file!(), $path);
    };
}
/// Adds the passed in name and object
/// matching a ManualObject to the list
/// of manually created object files in state
///
/// The path is relative to $src_file
/// # Examples
/// ```
/// add_manual_object!(state, object);
/// ```
#[macro_export]
macro_rules! add_manual_object {
    ($state:expr, $path:expr, $obj:expr) => {
        $state.add_manual_object_macro_(file!(), $path, Box::new($obj));
    };
}
/// Set the &str to the path for the linker script in state
///
/// The path is relative to $src_file
/// # Examples
/// ```
/// linker_script!(state, "link/STM32F051R8.ld");
/// ```
#[macro_export]
macro_rules! linker_script {
    ($state:expr, $path:expr) => {
        $state.set_linker_script_macro_(file!(), $path);
    };
}

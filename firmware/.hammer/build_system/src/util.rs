use std::process::Command;
use std::process::Stdio;

/// Firmware Image Modification tools
pub mod image;
// May be useful to use something like https://docs.rs/elf/latest/elf/ in the future to replace need for objcopy

/// Project packaging tools
pub mod package;

/// Re-export the rust path to src folder converter
pub use crate::file_names::get_target_src_path;
/// Re-export the Dependency Searcher under util
pub use crate::src_dependency::DependencySearcher;

/// This is a helper function to convert Intel HEX files to Raw Binary
///  Example (using arm-objcopy):
/// ```
/// convert_intel_hex_to_binary("arm-none-eabi-objcopy", in_file_path, out_file_path)
/// ```
pub fn convert_intel_hex_to_binary(
    objcopy_name: &str,
    input_ihex_path: &str,
    output_bin_path: &str,
) {
    let mut cmd = Command::new(objcopy_name);
    cmd.arg("-I");
    cmd.arg("ihex");
    cmd.arg("-O");
    cmd.arg("binary");
    cmd.arg(input_ihex_path);
    cmd.arg(output_bin_path);
    run_command(
        cmd,
        &format!(
            "convert_intel_hex_to_binary({}, {}, {})",
            objcopy_name, input_ihex_path, output_bin_path
        ),
    );
}
/// This is a helper function to convert Raw Binary into linked object files
///
/// Specifying None for section flags will use: "contents,alloc,load,data,readonly,rom"
///
///  Examples (for Cortex-M):
/// ```
/// // Load into normal flash
/// convert_binary_to_object(
///   "arm-none-eabi-objcopy",
///   "elf32-littlearm",
///   "arm",
///   "rodata",
///   None,
///   in_file_path,
///   out_obj_path,
///   name,
///   )
/// convert_binary_to_object(
///   "arm-none-eabi-objcopy",
///   "elf32-littlearm",
///   "arm",
///   "rodata",
///   Some("contents,alloc,load,data,readonly,rom"),
///   in_file_path,
///   out_obj_path,
///   name,
///   )
/// ```
/// Both of those examples will work the same way.
/// The produced symbol table will have:
///
/// void *name_data -> data buffer
///
/// void *name_end -> byte after the end of data buffer
///
/// name_size -> (uintptr_t)&name_size == file_length(in_file_path)
///
pub fn convert_binary_to_object(
    objcopy_name: &str,
    target: &str,
    arch: &str,
    destination_section: &str,
    section_flags: Option<&str>,
    input_file_path: &str,
    output_obj_path: &str,
    linked_symbol: &str,
) {
    let mut cmd = Command::new(objcopy_name);
    cmd.arg("-I");
    cmd.arg("binary");
    cmd.arg("-O");
    cmd.arg(target);
    cmd.arg("-B");
    cmd.arg(arch);
    cmd.arg("--rename-section");
    cmd.arg(&format!(
        ".data=.{},{}",
        destination_section,
        section_flags.unwrap_or("contents,alloc,load,data,readonly,rom")
    ));
    cmd.arg("--redefine-sym");
    cmd.arg(&format!(
        "_binary_{}_start={}_data",
        fix_object_path_for_symbol(input_file_path),
        linked_symbol
    ));
    cmd.arg("--redefine-sym");
    cmd.arg(&format!(
        "_binary_{}_end={}_end",
        fix_object_path_for_symbol(input_file_path),
        linked_symbol
    ));
    cmd.arg("--redefine-sym");
    cmd.arg(&format!(
        "_binary_{}_size={}_size",
        fix_object_path_for_symbol(input_file_path),
        linked_symbol
    ));
    cmd.arg(input_file_path);
    cmd.arg(output_obj_path);
    run_command(
        cmd,
        &format!(
            "convert_binary_to_object({}, {}, {}, {}, {:?}, {}, {})",
            objcopy_name,
            target,
            arch,
            destination_section,
            section_flags,
            input_file_path,
            output_obj_path
        ),
    );
}

/// Enumerates all the arguments passed to this program at exec() time to see what matches
pub fn does_argument_flag_exist<S: AsRef<str>>(flag: S) -> bool {
    let args: std::collections::BTreeSet<_> = std::env::args().collect();
    args.contains(flag.as_ref())
}

/// Write the target file only if it doesn't have the contents provided
/// This will read the whole file into memory
/// Returns true if it had to write
pub fn write_file_if_not_equal<P: AsRef<std::path::Path>, B: AsRef<[u8]>>(
    path: P,
    data: B,
) -> std::io::Result<bool> {
    use std::fs;
    let path = path.as_ref();
    if !path.exists() || fs::read(path)? != data.as_ref() {
        fs::create_dir_all(path.parent().unwrap())?;
        fs::write(path, data)?;
        Ok(true)
    } else {
        Ok(false)
    }
}

/// Returns a generated files folder base string
/// Use via `util::get_generated_file_path() + "file_name"`
// It's expected that you will also use `state.add_include_dir(&get_generated_file_path(), "")`
pub fn get_generated_file_path() -> String {
    "target/build/generated/".into()
}

/// Write a templated string at the path
pub fn write_template<P: AsRef<std::path::Path>, S: serde::Serialize>(
    path: P,
    template: &str,
    template_data: S,
) -> std::io::Result<()> {
    let map_e = |e: tinytemplate::error::Error| std::io::Error::new(std::io::ErrorKind::Other, e);
    let mut tt = tinytemplate::TinyTemplate::new();
    tt.add_template("template", template).map_err(map_e)?;
    write_file_if_not_equal(path, tt.render("template", &template_data).map_err(map_e)?)?;
    Ok(())
}

/// Runs the Provided Command Object with output piped to the console,
/// printing an error such that most IDE will recognize it as a build failure
pub fn run_command(mut cmd: Command, error: &str) {
    // Making this error handling friendlier is blocked on https://github.com/rust-lang/rust/issues/44434
    // If we can easily get the command being called, this can be printed all the time
    cmd.stdout(Stdio::inherit());
    cmd.stderr(Stdio::inherit());
    let status = match cmd.status() {
        Ok(s) => s,
        Err(e) => panic!(
            "build.error:0:0: error: failed to execute process: \"{:?}\" because: {} and {}",
            &cmd, error, e
        ),
    };

    if !status.success() {
        panic!("build.error:0:0: error: {}", error);
    }
}

/// Return true if this build is being run in a CI/CD system (at least via GitLab)
pub fn is_build_in_ci() -> bool {
    std::option_env!("CI").is_some()
}

/// Version Information
#[derive(serde::Serialize, Debug)]
pub struct VersionInfo {
    /// Derived from CARGO_PKG_VERSION passed in
    pub version: Option<semver::Version>,
    /// Only present if run in a git repo (w/ git tools installed)
    pub hash: Option<String>,
    /// true if git says the repo is dirty
    pub dirty: Option<bool>,
}
/// Version Information (Unwrapped)
#[derive(serde::Serialize, Debug)]
pub struct VersionInfoUnwrapped {
    /// Derived from CARGO_PKG_VERSION passed in
    /// Notice, when serialized for template engine, it's already a string
    pub version: semver::Version,
    /// Only present if run in a git repo (w/ git tools installed)
    pub hash: String,
    /// true if git says the repo is dirty
    pub dirty: bool,
}

/// Get the version information of the project being built
/// Required to be called with the top-level project's `env!("CARGO_PKG_VERSION")`
/// This completely ignores git tags, etc, and assumes that you have accurate project versions in Cargo.
/// Simple Example: (for more complex situations, you may use the full features of the template engine)
/// ```
/// util::write_template(
///     util::get_generated_file_path() + "git_hash_str.h",
///     r#"#pragma once
/// static const char * const FW_VERSION = "{@root}";
/// "#,
///     util::get_project_version(env!("CARGO_PKG_VERSION"), 8).to_string(),
/// )
/// .unwrap();
/// ```
pub fn get_project_version(env_version: &str, hash_digits: usize) -> VersionInfo {
    let vers = semver::Version::parse(env_version).ok();
    let mut cmd = Command::new("git");
    cmd.arg("describe");
    cmd.arg("--always");
    cmd.arg("--dirty");
    cmd.arg("--match");
    cmd.arg("NEVER NAME A TAG THIS");
    cmd.arg(&format!("--abbrev={}", hash_digits));
    let git = cmd
        .output()
        .ok()
        .map(|out| {
            if out.status.success() {
                std::str::from_utf8(&out.stdout).ok().map(|s| {
                    if let Some(hash) = s.trim().strip_suffix("-dirty") {
                        (hash.to_string(), true)
                    } else {
                        (s.trim().to_string(), false)
                    }
                })
            } else {
                None
            }
        })
        .flatten();

    if let Some((hash, dirty)) = git {
        VersionInfo {
            version: vers,
            hash: Some(hash),
            dirty: Some(dirty),
        }
    } else {
        VersionInfo {
            version: vers,
            hash: None,
            dirty: None,
        }
    }
}

fn fix_object_path_for_symbol(path: &str) -> String {
    path.replace(".", "_")
        .replace("-", "_")
        .replace("/", "_")
        .to_owned()
}

impl VersionInfo {
    /// Convert to a version of this w/o any internal Option<T>
    pub fn unwrap(self) -> VersionInfoUnwrapped {
        VersionInfoUnwrapped {
            version: self.version.unwrap(),
            hash: self.hash.unwrap(),
            dirty: self.dirty.unwrap(),
        }
    }
}
impl std::fmt::Display for VersionInfoUnwrapped {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let dirty = if self.dirty { "-dirty" } else { "" };
        write!(f, "{}-g{}{}", self.version, self.hash, dirty)
    }
}
impl std::fmt::Display for VersionInfo {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        if let Some(v) = &self.version {
            write!(f, "{}", v)?;
        }
        if let Some(v) = &self.hash {
            write!(f, "-g{}", v)?;
        }
        if let Some(d) = self.dirty {
            let dirty = if d { "-dirty" } else { "" };
            write!(f, "{}", dirty)?;
        }
        Ok(())
    }
}

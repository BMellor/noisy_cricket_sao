use std::collections::HashMap;
use std::collections::HashSet;

use crate::options::*;

use crate::file_names::{get_hasher_key, HasherKey};

/// Manual Object Configuration Entry
///
/// To generate a manual object, implement this trait in your own struct
/// and pass it to the add_manual_object! macro
/// needs_update will be called during every build,
/// it is suggested that you use the DependencyTracker exported in the library
/// to determine the need to a build. Return true to signal that generate should be called.
/// fn needs_update will *always* be called, and so you may depends on it to do code generation, or nested build systems.
/// In any case, the output_obj_path is set to the path of the object file that chould be created.
/// If you need intermediate files, you may safely extend the path given with further extensions.
pub trait ManualObject {
    fn needs_update(&self, output_obj_path: &str) -> bool;
    fn generate(&self, output_obj_path: &str);
}

/// Override global behavior for this file. Use the add_file_with_opts macro
#[derive(Clone, Debug, PartialEq, Eq, Hash)]
pub struct FileOptions {
    pub warnings_enabled: bool,
    pub opt_level: Optimize,
    pub extra_flags: String,
}
impl FileOptions {
    pub fn new(warn_en: bool, opt_l: Optimize, flags: &str) -> FileOptions {
        FileOptions {
            warnings_enabled: warn_en,
            opt_level: opt_l,
            extra_flags: String::from(flags),
        }
    }
}
/// Build System State
///
/// This object fully describes the state of the system to be built
pub struct State {
    include_dirs: HashSet<String>,
    sys_include_dirs: HashSet<String>,
    c_files: HashSet<(String, Option<FileOptions>)>,
    cpp_files: HashSet<(String, Option<FileOptions>)>,
    asm_files: HashSet<String>,
    manual_objects: HashMap<String, Box<dyn ManualObject>>,

    c_compiler_name: String,
    cpp_compiler_name: String,
    linker_name: String,
    prefix: String,

    defines: HashSet<String>,
    special_compiler_arguments: HashSet<String>,
    c_warnings: HashSet<String>,
    c_errors: HashSet<String>,
    cpp_warnings: HashSet<String>,
    cpp_errors: HashSet<String>,
    linker_opts: HashSet<String>,

    opt_level: Optimize,
    cpu: CPU,
    c_standard: CStandard,
    cpp_standard: CPPStandard,

    linker_script: String,

    project: String,

    verbose: bool,

    build_directory: String,

    local_config_map: HashMap<String, String>,

    hasher_seed: HasherKey,
}
impl State {
    /// New empty State
    /// call as new(file!(), line!())
    pub fn new(file: &str, line: u32) -> State {
        let mut s = State {
            include_dirs: HashSet::new(),
            sys_include_dirs: HashSet::new(),
            c_files: HashSet::new(),
            cpp_files: HashSet::new(),
            asm_files: HashSet::new(),
            manual_objects: HashMap::new(),
            c_compiler_name: "gcc".to_owned(),
            cpp_compiler_name: "g++".to_owned(),
            linker_name: "g++".to_owned(),
            prefix: "arm-none-eabi-".to_owned(),
            defines: HashSet::new(),
            special_compiler_arguments: HashSet::new(),
            c_warnings: HashSet::new(),
            c_errors: HashSet::new(),
            cpp_warnings: HashSet::new(),
            cpp_errors: HashSet::new(),
            linker_opts: HashSet::new(),
            opt_level: Optimize::Og,
            cpu: CPU::CortexM0,
            c_standard: CStandard::C11,
            cpp_standard: CPPStandard::CPP17,
            linker_script: String::new(),
            project: String::new(),
            verbose: false,
            build_directory: "./target/build".to_owned(),
            local_config_map: HashMap::new(),
            hasher_seed: get_hasher_key(file, line),
        };
        if crate::util::is_build_in_ci() {
            s.add_define("AEREL_BUILD_IS_IN_CI");
        }
        s
    }
    /// Gets the include directories in the form gcc expects
    pub fn get_include_dir_args(&self) -> Vec<String> {
        self.include_dirs
            .iter()
            .map(|s| format!("-I{}", s))
            .chain(
                self.sys_include_dirs
                    .iter()
                    .map(|s| format!("-isystem{}", s)),
            )
            .collect::<Vec<_>>()
    }
    /// Returns the set of c files and their possible options
    pub fn get_c_files(&self) -> &HashSet<(String, Option<FileOptions>)> {
        &self.c_files
    }
    /// Returns the set of cpp files and their possible options
    pub fn get_cpp_files(&self) -> &HashSet<(String, Option<FileOptions>)> {
        &self.cpp_files
    }
    /// Returns the set of assembly files
    pub fn get_asm_files(&self) -> &HashSet<String> {
        &self.asm_files
    }
    /// Gets the list of manual objects
    pub fn get_manual_objects(&self) -> &HashMap<String, Box<dyn ManualObject>> {
        &self.manual_objects
    }
    /// Sets the name of the c compiler to use. Default: "gcc"
    pub fn set_c_compiler_name(&mut self, name: &str) {
        self.c_compiler_name = String::from(name);
    }
    /// Sets the name of the c++ compiler to use. Default: "g++"
    pub fn set_cpp_compiler_name(&mut self, name: &str) {
        self.cpp_compiler_name = String::from(name);
    }
    /// Sets the name of the linker to use. Default: "gcc"
    pub fn set_linker_name(&mut self, name: &str) {
        self.linker_name = String::from(name);
    }
    /// Sets the prefix of the compiler/linker to use. Default: "arm-none-eabi-"
    pub fn set_prefix(&mut self, name: &str) {
        self.prefix = String::from(name);
    }
    /// Sets the output build directory. Default: "./target/build"
    /// This puts it in the same folder that rust uses for build output
    pub fn set_build_dir(&mut self, dir: &str) {
        self.build_directory = String::from(dir);
    }
    /// Returns the compiler name with the prefix prepended
    pub fn get_build_dir(&self) -> String {
        self.build_directory.clone()
    }
    /// Returns the compiler name with the prefix prepended
    pub fn get_c_compiler_name(&self) -> String {
        format!("{}{}", self.prefix, self.c_compiler_name)
    }
    /// Returns the compiler name with the prefix prepended
    pub fn get_cpp_compiler_name(&self) -> String {
        format!("{}{}", self.prefix, self.cpp_compiler_name)
    }
    /// Returns the linker name with the prefix prepended
    pub fn get_linker_name(&self) -> String {
        format!("{}{}", self.prefix, self.linker_name)
    }
    /// Returns the prefix for the compiler/linker
    pub fn get_prefix(&self) -> String {
        self.prefix.clone()
    }
    /// Sets the desired global optimization level
    pub fn set_opt_level(&mut self, opt: Optimize) {
        self.opt_level = opt;
    }
    /// Gets the desired global optimization level
    pub fn get_opt_level(&self) -> Optimize {
        self.opt_level.clone()
    }
    /// Sets the desired global cpu type
    pub fn set_cpu(&mut self, cpu: CPU) {
        self.cpu = cpu;
    }
    /// Gets the desired global cpu type
    pub fn get_cpu(&self) -> CPU {
        self.cpu.clone()
    }
    /// Sets the desired global c standard
    pub fn set_c_standard(&mut self, st: CStandard) {
        self.c_standard = st;
    }
    /// Gets the desired global c standard
    pub fn get_c_standard(&self) -> CStandard {
        self.c_standard.clone()
    }
    /// Sets the desired global c++ standard
    pub fn set_cpp_standard(&mut self, st: CPPStandard) {
        self.cpp_standard = st;
    }
    /// Gets the desired global c++ standard
    pub fn get_cpp_standard(&self) -> CPPStandard {
        self.cpp_standard.clone()
    }
    /// Add a preprocessor define
    ///
    /// # Examples
    /// ```
    /// state.add_define("DEBUG");
    /// state.add_define("DEVICE=STM32F051");
    /// ```
    pub fn add_define(&mut self, mc: &str) {
        self.defines.insert(mc.to_owned());
    }
    /// Adds a custom linker option
    ///
    /// Mostly used to add a .spec file to the linker
    /// # Common Additional Options
    /// ```
    /// state.add_linker_opt("--specs=nano.specs");
    /// state.add_linker_opt("--specs=nosys.specs");
    /// ```
    pub fn add_linker_opt(&mut self, s: &str) {
        self.linker_opts.insert(s.to_owned());
    }
    /// Adds a special compiler argument
    pub fn add_special_compiler_argument(&mut self, s: &str) {
        self.special_compiler_arguments.insert(s.to_owned());
    }

    /// Add a c warning
    pub fn add_c_warning(&mut self, s: &str) {
        self.c_warnings.insert(s.to_owned());
    }
    /// Add a c error
    pub fn add_c_error(&mut self, s: &str) {
        self.c_errors.insert(s.to_owned());
    }
    /// Add a c++ warning
    pub fn add_cpp_warning(&mut self, s: &str) {
        self.cpp_warnings.insert(s.to_owned());
    }
    /// Add a c++ error
    pub fn add_cpp_error(&mut self, s: &str) {
        self.cpp_errors.insert(s.to_owned());
    }
    /// Get the list of cpp define arguments
    pub fn get_defines(&self) -> &HashSet<String> {
        &self.defines
    }
    /// Gets the list of special arguments
    pub fn get_special_compiler_arguments(&self) -> &HashSet<String> {
        &self.special_compiler_arguments
    }
    /// Get the additional linker arguments
    pub fn get_linker_opts(&self) -> &HashSet<String> {
        &self.linker_opts
    }
    /// Gets the list of C warnings
    pub fn get_c_warnings(&self) -> &HashSet<String> {
        &self.c_warnings
    }
    /// Gets the list of C errors
    pub fn get_c_errors(&self) -> &HashSet<String> {
        &self.c_errors
    }
    /// Gets the list of C++ warnings
    pub fn get_cpp_warnings(&self) -> &HashSet<String> {
        &self.cpp_warnings
    }
    /// Gets the list of C++ errors
    pub fn get_cpp_errors(&self) -> &HashSet<String> {
        &self.cpp_errors
    }
    /// Sets the project name
    pub fn set_project(&mut self, name: &str) {
        self.project = name.to_owned();
    }
    /// Gets the project name
    pub fn get_project(&self) -> String {
        self.project.clone()
    }
    /// Set the build system in verbose build mode.
    ///
    /// Implies a single threaded build
    pub fn set_verbose(&mut self, v: bool) {
        self.verbose = v;
    }
    /// Get the verbose mode
    pub fn get_verbose(&self) -> bool {
        self.verbose
    }
    /// Get the path of the linker script
    pub fn get_linker_script(&self) -> String {
        self.linker_script.clone()
    }
    /// Set the default warning set
    /// Notice: Changes in the default warning set are *NOT* breaking changes per semver
    pub fn set_default_warnings(&mut self) {
        self.add_c_warning("unused-parameter");
        self.add_c_warning("float-equal");
        self.add_c_warning("double-promotion");
        self.add_cpp_warning("unused-parameter");
        self.add_cpp_warning("cpp");
        self.add_cpp_warning("double-promotion");
        self.add_c_error("all");
        self.add_c_error("extra");
        self.add_c_error("strict-prototypes");
        self.add_c_error("old-style-definition");
        self.add_c_error("format=2");
        self.add_c_error("write-strings");
        self.add_cpp_error("all");
        self.add_cpp_error("extra");
        self.add_cpp_error("format=2");
        self.add_cpp_error("float-equal");
        self.add_cpp_error("cast-align");
        self.add_cpp_error("write-strings");
        self.add_cpp_error("no-pmf-conversions");
    }

    /// Use the similarly named macro at top level or the non-suffixed function
    #[doc(hidden)]
    pub fn add_include_dir_macro_(&mut self, src_path: &str, path: &str) {
        self.include_dirs
            .insert(crate::file_names::get_target_src_path(src_path, path));
    }
    /// Use the similarly named macro at top level or the non-suffixed function
    #[doc(hidden)]
    pub fn add_system_include_dir_macro_(&mut self, src_path: &str, path: &str) {
        self.sys_include_dirs
            .insert(crate::file_names::get_target_src_path(src_path, path));
    }
    /// Use the similarly named macro at top level or the non-suffixed function
    #[doc(hidden)]
    pub fn add_c_file_macro_(&mut self, src_path: &str, path: &str, opts: Option<FileOptions>) {
        self.c_files
            .insert((crate::file_names::get_target_src_path(src_path, path), opts));
    }
    /// Use the similarly named macro at top level or the non-suffixed function
    #[doc(hidden)]
    pub fn add_cpp_file_macro_(&mut self, src_path: &str, path: &str, opts: Option<FileOptions>) {
        self.cpp_files
            .insert((crate::file_names::get_target_src_path(src_path, path), opts));
    }
    /// Use the similarly named macro at top level or the non-suffixed function
    #[doc(hidden)]
    pub fn add_asm_file_macro_(&mut self, src_path: &str, path: &str) {
        self.asm_files
            .insert(crate::file_names::get_target_src_path(src_path, path));
    }
    /// Use the similarly named macro at top level or the non-suffixed function
    #[doc(hidden)]
    pub fn set_linker_script_macro_(&mut self, src_path: &str, path: &str) {
        self.linker_script = crate::file_names::get_target_src_path(src_path, path);
    }
    /// Use the similarly named macro at top level or the non-suffixed function
    #[doc(hidden)]
    pub fn add_manual_object_macro_(
        &mut self,
        src_path: &str,
        name: &str,
        obj: Box<dyn ManualObject>,
    ) {
        self.manual_objects
            .insert(crate::file_names::get_target_src_path(src_path, name), obj);
    }
    fn combine_normal_path(prefix: &str, postfix: &str) -> String {
        let mut base_path = prefix.to_owned();
        base_path.push('/');
        base_path.push_str(postfix);
        base_path
    }
    /// Add an include directory using a relative or absolute path from the current directory
    /// This is most useful with an absolute path, since the relative case is better covered with the macro
    pub fn add_include_dir(&mut self, src_path: &str, path: &str) {
        self.include_dirs
            .insert(Self::combine_normal_path(src_path, path));
    }
    /// Add a system include directory using a relative or absolute path from the current directory
    /// This is most useful with an absolute path, since the relative case is better covered with the macro
    pub fn add_system_include_dir(&mut self, src_path: &str, path: &str) {
        self.sys_include_dirs
            .insert(Self::combine_normal_path(src_path, path));
    }
    /// Add c file using a relative or absolute path from the current directory
    /// This is most useful with an absolute path, since the relative case is better covered with the macro
    pub fn add_c_file(&mut self, src_path: &str, path: &str) {
        self.c_files
            .insert((Self::combine_normal_path(src_path, path), None));
    }
    /// Add c++ file using a relative or absolute path from the current directory
    /// This is most useful with an absolute path, since the relative case is better covered with the macro
    pub fn add_cpp_file(&mut self, src_path: &str, path: &str) {
        self.cpp_files
            .insert((Self::combine_normal_path(src_path, path), None));
    }
    /// Add c file using a relative or absolute path from the current directory
    /// This is most useful with an absolute path, since the relative case is better covered with the macro
    pub fn add_c_file_opts(&mut self, src_path: &str, path: &str, opts: FileOptions) {
        self.c_files
            .insert((Self::combine_normal_path(src_path, path), Some(opts)));
    }
    /// Add c++ file using a relative or absolute path from the current directory
    /// This is most useful with an absolute path, since the relative case is better covered with the macro
    pub fn add_cpp_file_opts(&mut self, src_path: &str, path: &str, opts: FileOptions) {
        self.cpp_files
            .insert((Self::combine_normal_path(src_path, path), Some(opts)));
    }
    /// Add an assembly file using a relative or absolute path from the current directory
    /// This is most useful with an absolute path, since the relative case is better covered with the macro
    pub fn add_asm_file(&mut self, src_path: &str, path: &str) {
        self.asm_files
            .insert(Self::combine_normal_path(src_path, path));
    }
    /// Set the linker script using a relative or absolute path from the current directory
    /// This is most useful with an absolute path, since the relative case is better covered with the macro
    pub fn set_linker_script(&mut self, src_path: &str, path: &str) {
        self.linker_script = Self::combine_normal_path(src_path, path);
    }

    /// Returns a copy of the HasherKey for this build state
    /// If any external tools use file name manipulation, this is the place to get that variable
    pub fn get_hasher_key(&self) -> HasherKey {
        self.hasher_seed.clone()
    }

    /// Load a local configuration file
    /// This file is in (roughly) Makefile format, and should consist of solely variable declarations and comments
    /// Example:
    /// # Comment about the file
    /// SDK_ROOT := /path/to/sdk # SDK path
    ///
    /// Note that this implementation considers whitespace to be valid in keys and values,
    /// except at the start or end of the string, which is removed
    /// The internal configuration store will be extended with the contents of the given file
    pub fn local_configuration_load(&mut self, file: &str) {
        use std::io::prelude::*;
        let reader = std::io::BufReader::new(match std::fs::File::open(file) {
            Ok(f) => f,
            Err(e) => panic!(
                "Unable to open config file \"{}\" with error \"{}\"",
                file, e
            ),
        });
        self.local_config_map
            .extend(
                reader
                    .lines()
                    .map(|s| s.unwrap())
                    .enumerate()
                    .filter_map(|(ln, s)| {
                        let s = match s.find('#') {
                            Some(ofs) => s.split_at(ofs).0.trim(),
                            None => s.trim(),
                        };
                        if s.len() == 0 {
                            return None;
                        }
                        let split: Vec<_> = s.split(":=").collect();
                        if split.len() == 2 {
                            Some((split[0].trim().to_owned(), split[1].trim().to_owned()))
                        } else {
                            panic!("Invalid syntax at line {}", ln + 1);
                        }
                    }),
            );
    }
    /// Retrieve a key from the local configuration. Panics if no key
    pub fn local_configuration_for_key(&self, idx: &str) -> String {
        match self.local_config_map.get(idx) {
            Some(s) => s.to_owned(),
            None => panic!("No such key as \"{}\"", idx),
        }
    }
    /// Insert a key to the local configuration
    pub fn local_configuration_insert_key(&mut self, idx: &str, value: &str) {
        self.local_config_map
            .insert(idx.to_owned(), value.to_owned());
    }
    /// Checks if the given key exists in the local config store
    pub fn local_configuration_for_key_exists(&self, idx: &str) -> bool {
        self.local_config_map.contains_key(idx)
    }
    /// Write out vscode metadata files. Custom folder base if Some(path), otherwise None => ".vscode"
    pub fn write_vscode_metadata(&self, path: Option<&str>) {
        let path = std::path::PathBuf::from(path.unwrap_or(".vscode"));
        self.write_vscode_cpp_properties(path.join("c_cpp_properties.json"));
    }
    /// Write out a c_cpp_properties.json file that vscode can use to setup the code completions
    pub fn write_vscode_cpp_properties<P: AsRef<std::path::Path>>(&self, output_path: P) {
        #[derive(serde::Serialize)]
        struct VsCode {
            include_paths: Vec<String>,
            defines: Vec<String>,
            compiler_args: Vec<String>,
            compiler_path: String,
            c_standard: String,
            cpp_standard: String,
        }
        let mut include_paths: Vec<_> = self
            .include_dirs
            .iter()
            .chain(self.sys_include_dirs.iter())
            .map(|partial_path| {
                match std::fs::canonicalize(partial_path) {
                    Ok(pbuf) => pbuf.to_string_lossy().into_owned().to_owned(),
                    Err(_) => partial_path.to_owned(),
                }
                .replace('\\', "\\\\")
            })
            .map(|full_path| {
                if cfg!(windows) {
                    // Work around VSCode bug not supporting cannonical paths on windows
                    full_path.replacen("\\\\\\\\?\\\\", "", 1) // Doubled '\' due to processiong post-escaped data
                } else {
                    full_path
                }
            })
            .collect();

        let mut defines: Vec<_> = self
            .defines
            .iter()
            .chain(self.cpu.defs().iter())
            .map(String::from)
            .collect();
        let mut compiler_args: Vec<_> = self
            .cpu
            .mcpu_flags()
            .split_whitespace()
            .map(String::from)
            .collect();
        let compiler_path = which::which(format!("{}{}", self.prefix, self.cpp_compiler_name))
            .expect(&format!(
                "Failed to find a required build tool: {}{}",
                self.prefix, self.cpp_compiler_name
            ))
            .as_path()
            .to_str()
            .unwrap()
            .to_owned()
            .replace('\\', "/");

        include_paths.sort_unstable();
        defines.sort_unstable();
        compiler_args.sort_unstable();

        let info = VsCode {
            include_paths: include_paths,
            defines: defines,
            compiler_args: compiler_args,
            compiler_path: compiler_path,
            c_standard: self.c_standard.to_str(),
            cpp_standard: self.cpp_standard.to_str(),
        };
        // Yes, the right thing to do would be to use the json engine directly, but I already have the template engine, and this way I don't need to model everything.
        crate::util::write_template(
            output_path,
            include_str!("c_cpp_properties.json_template"),
            info,
        )
        .unwrap();
    }
}

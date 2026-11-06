use rayon::prelude::*;

use std::collections::HashSet;
use std::fs;
use std::io::{BufWriter, Write};
use std::process::Command;
use std::process::Stdio;

use crate::command_args::*;
use crate::file_names::*;
use crate::src_dependency::DependencySearcher;
use crate::state::State;
use crate::util;

/// This function consumes the state object and builds the project.
///
/// It takes all the information contained in the state object and runs the compilers and linker as directed.
/// Also will produce .hex as well as run "size" on the .elf.
///
/// Returns: true if a build was preformed
///
/// # Panics
/// This will panic! if anything goes wrong.
/// This is likely to be a build failure along the way.
pub fn build(mut state: State) -> bool {
    let build_dir = state.get_build_dir();
    if util::does_argument_flag_exist("clean") {
        fs::remove_dir_all(&build_dir).ok();
    }
    if util::does_argument_flag_exist("verbose") {
        state.set_verbose(true);
    }
    let state = state;

    fs::create_dir_all(&format!("{}/obj", build_dir)).ok();
    fs::create_dir_all(&format!("{}/.dep", build_dir)).ok();

    let gcc = state.get_c_compiler_name();
    let gpp = state.get_cpp_compiler_name();

    let compile_args = &[
        "-c",
        "-fomit-frame-pointer",
        "-ggdb3",
        "-fomit-frame-pointer",
        "-falign-functions=16",
        "-ffunction-sections",
        "-fdata-sections",
        "-fno-common",
    ];

    let asm_args = &["-x", "assembler-with-cpp"];
    let cpp_args = &["-fno-rtti", "-fno-exceptions"];
    let elf_name = format!("{}/{}.elf", build_dir, state.get_project());
    let linker_args = &[
        "-fomit-frame-pointer",
        "-Wl,--start-group",
        "-Wl,--end-group",
        &format!("-T{}", state.get_linker_script()),
        &format!(
            "-Wl,-Map={}/{}.map,--cref,--gc-sections",
            build_dir,
            state.get_project()
        ),
        // "-mno-thumb-interwork",
        // "-mthumb",
        "-o",
        &format!("{}.temp.elf", elf_name),
    ];

    let include_args = state.get_include_dir_args();
    let cpu_arg = state.get_cpu().mcpu_flags();
    let opt_arg = state.get_opt_level().arg();
    let c_std_arg = state.get_c_standard().arg();
    let cpp_std_arg = state.get_cpp_standard().arg();
    let define_args = state
        .get_defines()
        .iter()
        .chain(state.get_cpu().defs().iter())
        .map(|mc| format!("-D{}", mc))
        .collect::<Vec<_>>();
    let special_compiler_args = state.get_special_compiler_arguments();
    let c_warn_args = build_warning_args(state.get_c_warnings(), state.get_c_errors());
    let cpp_warn_args = build_warning_args(state.get_cpp_warnings(), state.get_cpp_errors());

    let hash_seed = state.get_hasher_key();

    // command, file name, final path
    let mut command_list: Vec<(FakeCommand, &str, String)> = Vec::new();
    let mut object_list: HashSet<String> = HashSet::new();

    let mut dep_lookup = DependencySearcher::new();

    for (object_name, manual_obj) in state.get_manual_objects() {
        let obj_path = get_obj_path(&object_name, &build_dir, &hash_seed);
        if manual_obj.needs_update(&obj_path) {
            manual_obj.generate(&obj_path);
        }
        insert_into_object_list_or_die(&mut object_list, obj_path);
    }

    for (file, opts) in state.get_c_files() {
        insert_into_object_list_or_die(
            &mut object_list,
            get_obj_path(&file, &build_dir, &hash_seed),
        );
        if dep_lookup.should_recompile(&file, &build_dir, &hash_seed) {
            let mut prog = FakeCommand::new(&gcc);
            prog.args(define_args.iter());
            prog.args_from_str(&cpu_arg);
            prog.args(compile_args.iter());
            prog.args(special_compiler_args.iter());
            prog.args_from_str(&c_std_arg);
            if opts.is_some() && !opts.as_ref().unwrap().warnings_enabled {
                prog.arg("-w"); // Disable all warnings
            } else {
                prog.args(c_warn_args.iter());
            }
            if opts.is_some() {
                prog.args_from_str(&opts.as_ref().unwrap().opt_level.arg());
                prog.args_from_str(&opts.as_ref().unwrap().extra_flags);
            } else {
                prog.args_from_str(&opt_arg);
            }
            prog.args(include_args.iter());
            prog.args_from_str(&get_dep_args(&file, &build_dir, &hash_seed));
            let (obj_args, real_path) = get_output_obj_args(&file, &build_dir, &hash_seed);
            prog.args_from_str(&obj_args);
            prog.arg(&file);
            command_list.push((prog, file, real_path));
        }
    }
    for (file, opts) in state.get_cpp_files() {
        insert_into_object_list_or_die(
            &mut object_list,
            get_obj_path(&file, &build_dir, &hash_seed),
        );
        if dep_lookup.should_recompile(&file, &build_dir, &hash_seed) {
            let mut prog = FakeCommand::new(&gpp);
            prog.args(define_args.iter());
            prog.args_from_str(&cpu_arg);
            prog.args(compile_args.iter());
            prog.args(special_compiler_args.iter());
            prog.args_from_str(&cpp_std_arg);
            prog.args(cpp_args.iter());
            if opts.is_some() && !opts.as_ref().unwrap().warnings_enabled {
                prog.arg("-w"); // Disable all warnings
            } else {
                prog.args(cpp_warn_args.iter());
            }
            if opts.is_some() {
                prog.args_from_str(&opts.as_ref().unwrap().opt_level.arg());
                prog.args_from_str(&opts.as_ref().unwrap().extra_flags);
            } else {
                prog.args_from_str(&opt_arg);
            }
            prog.args(include_args.iter());
            prog.args_from_str(&get_dep_args(&file, &build_dir, &hash_seed));
            let (obj_args, real_path) = get_output_obj_args(&file, &build_dir, &hash_seed);
            prog.args_from_str(&obj_args);
            prog.arg(&file);
            command_list.push((prog, file, real_path));
        }
    }
    for file in state.get_asm_files() {
        insert_into_object_list_or_die(
            &mut object_list,
            get_obj_path(&file, &build_dir, &hash_seed),
        );
        if dep_lookup.should_recompile(&file, &build_dir, &hash_seed) {
            let mut prog = FakeCommand::new(&gcc);
            prog.args(define_args.iter());
            prog.args_from_str(&cpu_arg);
            prog.args(compile_args.iter());
            prog.args(asm_args.iter());
            prog.args(include_args.iter());
            prog.args_from_str(&get_dep_args(&file, &build_dir, &hash_seed));
            let (obj_args, real_path) = get_output_obj_args(&file, &build_dir, &hash_seed);
            prog.args_from_str(&obj_args);
            prog.arg(&file);
            command_list.push((prog, file, real_path));
        }
    }

    if state.get_verbose() {
        command_list.sort();
        for (cmd, file, real_obj) in command_list.into_iter() {
            let mut c = cmd.to_cmd();
            c.stdout(Stdio::inherit());
            c.stderr(Stdio::inherit());
            println!("Compiling {} as {:?}", file, c);
            util::run_command(c, "Compilation Failed");
            std::fs::rename(format!("{}.temp", &real_obj), &real_obj)
                .expect("Failed to rename temporary file");
        }
    } else {
        command_list
            .into_par_iter()
            .for_each(|(cmd, file, real_obj)| {
                let mut c = cmd.to_cmd();
                c.stdout(Stdio::inherit());
                c.stderr(Stdio::inherit());
                println!("Compiling {}", file);
                util::run_command(c, "Compilation Failed");
                std::fs::rename(format!("{}.temp", &real_obj), &real_obj)
                    .expect("Failed to rename temporary file");
            });
    }

    // Purposly use a new DepSearcher as we have changed the fs at this point
    let build_was_run =
        if DependencySearcher::new().is_source_newer_than_output_list(&elf_name, &object_list) {
            let mut cmd = Command::new(&state.get_linker_name());

            {
                let mut args = BufWriter::new(
                    std::fs::File::create(&format!("{}/{}.objlst", build_dir, state.get_project()))
                        .unwrap(),
                );
                for obj in object_list.iter() {
                    writeln!(args, "{}", obj).unwrap();
                }
            }
            cmd.arg("-Xlinker");
            cmd.arg(&format!("@{}/{}.objlst", build_dir, state.get_project()));

            cmd.args_from_str(&state.get_cpu().link_arg());
            cmd.args(linker_args.iter());
            cmd.args(state.get_linker_opts().iter());

            cmd.stdout(Stdio::inherit());
            cmd.stderr(Stdio::inherit());

            if state.get_verbose() {
                println!("Linking {:?}", cmd);
            } else {
                println!("Linking...");
            }
            util::run_command(cmd, "Linking Failed");

            {
                let mut cmd = Command::new(format!("{}objcopy", state.get_prefix()));
                cmd.arg("-O");
                cmd.arg("ihex");
                cmd.arg(format!("{}.temp.elf", &elf_name));
                cmd.arg(format!("{1}/{0}.hex", state.get_project(), build_dir));
                util::run_command(cmd, "Objcopy Failed");
            }

            {
                let mut cmd = Command::new(format!("{}objdump", state.get_prefix()));
                cmd.arg("-x");
                cmd.arg("--syms");
                cmd.arg(format!("{}.temp.elf", &elf_name));
                cmd.stderr(Stdio::inherit());
                let output = cmd
                    .output()
                    .expect("build.error:0:0: error: failed to execute objdump");

                let mut f =
                    fs::File::create(format!("{}/{}.dmp", build_dir, state.get_project())).unwrap();
                f.write_all(&output.stdout).ok();
            }
            std::fs::rename(format!("{}.temp.elf", &elf_name), &elf_name)
                .expect("Failed to rename temporary file");
            true
        } else {
            println!("Files are up to date");
            false
        };
    // Always print the file size
    {
        let mut cmd = Command::new(format!("{}size", state.get_prefix()));
        cmd.arg(&elf_name);
        util::run_command(cmd, "Print Size Failed");
    }

    build_was_run
}

/// Builds the warning argument list for gcc
fn build_warning_args(warn: &HashSet<String>, err: &HashSet<String>) -> Vec<String> {
    let mut v: Vec<String> = Vec::new();
    v.push("-Wno-psabi".to_owned()); // note: parameter passing for argument of type 'class' changed in GCC 7.1 (when passed by value)
    v.push("-Werror".to_owned());
    for w in warn {
        v.push(format!("-W{}", w));
        v.push(format!("-Wno-error={}", w));
    }
    for e in err {
        v.push(format!("-W{}", e));
    }
    v
}

fn insert_into_object_list_or_die(list: &mut HashSet<String>, object: String) {
    if let Some(exists) = list.replace(object) {
        panic!("Object \"{}\" already exists. Most likely you named the same file in two places. Very unlikely, you could have an actual hash collision.", exists);
    }
}

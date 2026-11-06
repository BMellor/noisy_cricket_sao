/// Holds the Hasher State used to determine the object names
#[derive(Debug, Clone)]
pub struct HasherKey {
    a: u64,
    b: u64,
    c: u64,
    d: u64,
}
/// Makes a hasher key from a file path and line number
/// Expects to be called with file!() and line!()
pub fn get_hasher_key(path: &str, line: u32) -> HasherKey {
    let mut pbytes = path.to_owned().into_bytes();
    pbytes.push((line >> 24) as u8);
    pbytes.push((line >> 16) as u8);
    pbytes.push((line >> 8) as u8);
    pbytes.push((line >> 0) as u8);
    HasherKey {
        a: seahash::hash(&pbytes),
        // Use PATH as a key for the build
        b: seahash::hash(&std::env::var("PATH").expect("BAD $PATH").into_bytes()),
        c: 0x2bdf322e89df2070u64, // Random Data
        d: 0xbf8037ce468161fau64,
    }
}
/// Returns a path fragment that needs ".o" appended to it
/// The actual name returned is an implementation detail
pub fn get_obj_name(file: &str, key: &HasherKey) -> String {
    let end = file.rsplit('/').next().unwrap().replace(
        |c: char| c == '/' || c == '.' || c == ':' || c.is_whitespace() || c == '\\',
        "_",
    );
    // Seahash is fast enough on (fairly) short input to not be the bottleneck,
    // even on un-optimized builds, compared to the rest of the work
    format!(
        "{}-{:08x}",
        end,
        seahash::hash_seeded(file.as_bytes(), key.a, key.b, key.c, key.d)
    )
}

/// Returns the full arguments to gcc to generate dependency files
pub fn get_dep_args(file: &str, build_dir: &str, key: &HasherKey) -> String {
    format!("-MD -MF {}", get_dep_path(file, build_dir, key))
}

/// Returns the full arguments to gcc to save the object files
/// The second argument is the actual final path to save the object.
/// Allows for autonic rename after command completion to fix a compiler being killed.
pub fn get_output_obj_args(file: &str, build_dir: &str, key: &HasherKey) -> (String, String) {
    let obj_real = get_obj_path(file, build_dir, key);
    let dep_str = format!("-o {}.temp", &obj_real);

    (dep_str, obj_real)
}
/// Returns the object path for a file
pub fn get_obj_path(file: &str, build_dir: &str, key: &HasherKey) -> String {
    format!("{}/obj/{}.o", build_dir, get_obj_name(file, key))
}

/// Returns the dep path for a file
pub fn get_dep_path(file: &str, build_dir: &str, key: &HasherKey) -> String {
    format!("{}/.dep/{}.d", build_dir, get_obj_name(file, key))
}
/// Mutate the rust source file path into the path to the target source
pub fn get_target_src_path(rust_path: &str, target_path: &str) -> String {
    let mut filter_slash = rust_path
        .split(|c| c == '/' || c == '\\')
        .collect::<Vec<_>>();
    let len = filter_slash.len().saturating_sub(1); // Strip off the file
    filter_slash.truncate(len);
    filter_slash
        .iter()
        .map(|x| format!("{}/", *x))
        .chain(std::iter::once(target_path.to_string()))
        .collect()
}

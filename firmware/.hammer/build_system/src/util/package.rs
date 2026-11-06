use flate2::{Compression, GzBuilder};
use ignore::overrides::OverrideBuilder;
use ignore::WalkBuilder;
use std::fs::File;
use std::process::Command;
use tar::Builder;

pub struct ToolVersion {
    tool: &'static str,
    version_argument: &'static str,
}
impl ToolVersion {
    /// Create a tool version with the standard "--version" argument style
    pub fn new(tool: &'static str) -> Self {
        Self::custom(tool, "--version")
    }
    /// Create a tool version with a non-standard "--version" argument
    pub fn custom(tool: &'static str, version_argument: &'static str) -> Self {
        Self {
            tool,
            version_argument: version_argument,
        }
    }
    /// Create the default set of tools. Currently it's arm-none-eabi-gcc, the rust host version, gcc host version, and clang host version.
    /// Return array size is non contractual
    pub fn default_set_arm() -> [Self; 4] {
        [
            Self::new("arm-none-eabi-gcc"),
            Self::new("cargo"),
            Self::new("clang"),
            Self::new("gcc"),
        ]
    }
}

/// Create a source code archive, including all needed information, *if* this is being run from a GitLab CI pipeline.
pub fn make_source_archive_if_ci(tools: &[ToolVersion]) -> std::io::Result<()> {
    if !super::is_build_in_ci() {
        return Ok(());
    }
    let files: Vec<_> = WalkBuilder::new("./")
        .overrides(
            OverrideBuilder::new("")
                .add("*")
                .unwrap()
                .add("!target/")
                .unwrap()
                .add("!.git/")
                .unwrap()
                .build()
                .unwrap(),
        )
        .hidden(false)
        .git_ignore(true)
        .build()
        .filter_map(|e| e.ok().map(|e| e.path().to_str().unwrap().to_owned()))
        .collect();

    let file = GzBuilder::new().filename("source_release.tar").write(
        File::create("target/build/source_release.tar.gz")?,
        Compression::best(),
    );
    let mut a = Builder::new(file);

    for file in files {
        a.append_path(&file)?;
    }

    {
        let mut output = Vec::new();
        for tool in tools {
            output.extend_from_slice(tool.tool.as_bytes());
            output.push(b' ');
            output.extend_from_slice(tool.version_argument.as_bytes());
            output.extend_from_slice(":\n".as_bytes());
            let mut cmd = Command::new(tool.tool)
                .arg(tool.version_argument)
                .output()
                .expect("failed to execute process");
            output.append(&mut cmd.stdout);
            output.append(&mut cmd.stderr);
            output.extend_from_slice("\n\n".as_bytes());
        }
        let size = output.len();
        let data = std::io::Cursor::new(output);
        let mut header = tar::Header::new_gnu();
        header.set_size(size as u64);
        header.set_mode(0o644);
        a.append_data(&mut header, "COMPILER_VERSIONS", data)?;
    }
    {
        let mut output = Vec::new();
        for var in [
            "CI_COMMIT_AUTHOR",
            "CI_COMMIT_BRANCH",
            "CI_COMMIT_REF_NAME",
            "CI_COMMIT_SHA",
            "CI_COMMIT_TAG",
            "CI_COMMIT_TITLE",
            "CI_JOB_ID",
            "CI_JOB_STARTED_AT",
            "CI_PIPELINE_ID",
            "CI_PROJECT_NAME",
            "CI_PROJECT_TITLE",
            "CI_SERVER_HOST",
        ] {
            if let Ok(value) = std::env::var(var) {
                output.extend_from_slice(format!("{}={}\n", var, value).as_bytes());
            }
        }
        let size = output.len();
        let data = std::io::Cursor::new(output);
        let mut header = tar::Header::new_gnu();
        header.set_size(size as u64);
        header.set_mode(0o644);
        a.append_data(&mut header, "CI_METADATA", data)?;
    }

    a.into_inner()?.finish()?;

    Ok(())
}

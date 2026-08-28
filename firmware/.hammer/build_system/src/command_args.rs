use std::ffi::{OsStr, OsString};
use std::process::Command;
pub trait CommandArgs {
    fn args_from_str<S: AsRef<str>>(&mut self, args: S);
}
impl CommandArgs for Command {
    fn args_from_str<S: AsRef<str>>(&mut self, args: S) {
        if !args.as_ref().is_empty() {
            self.args(args.as_ref().split(' '));
        }
    }
}
/// A "Fake" Command object, that can be converted to one as Command is not Send on unix
#[derive(PartialEq, Eq, PartialOrd, Ord, Debug)]
pub struct FakeCommand {
    name: String,
    args: Vec<OsString>,
}
impl FakeCommand {
    pub fn new(nm: &str) -> FakeCommand {
        FakeCommand {
            name: nm.to_owned(),
            args: Vec::new(),
        }
    }
    pub fn arg<S: AsRef<OsStr>>(&mut self, ag: S) {
        self.args.push(ag.as_ref().to_owned());
    }
    pub fn args<I, S: AsRef<OsStr>>(&mut self, args: I)
    where
        I: Iterator<Item = S>,
    {
        for argument in args {
            self.arg(argument.as_ref());
        }
    }
    pub fn args_from_str<S: AsRef<str>>(&mut self, args: S) {
        if !args.as_ref().is_empty() {
            self.args(args.as_ref().split(' '));
        }
    }
    pub fn to_cmd(self) -> Command {
        let mut cmd = Command::new(self.name);
        // for arg in self.args {
        //     cmd.arg(&arg);
        // }
        cmd.args(self.args);
        cmd
    }
}
impl CommandArgs for FakeCommand {
    fn args_from_str<S: AsRef<str>>(&mut self, args: S) {
        if !args.as_ref().is_empty() {
            self.args(args.as_ref().split(' '));
        }
    }
}

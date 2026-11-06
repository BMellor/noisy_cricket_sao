/// Set the CPU to build for
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum CPU {
    /// Build for Cortex-M0
    CortexM0,
    /// Build for Cortex-M0+
    CortexM0Plus,
    /// Build for Cortex-M3
    CortexM3,
    /// Build for Cortex-M4 (no FPU)
    CortexM4,
    /// Build for Cortex-M4 (FPU)
    CortexM4F,
    /// Build for Cortex-M7 (no FPU) (Untested)
    CortexM7,
    /// Build for Cortex-M7 (FPU) (Untested, FPU Single Precision)
    CortexM7F,
    /// Build for Cortex-M7 (FPU) (Untested, FPU Double Precision)
    CortexM7FDP,
    /// Build for Cortex-M23
    CortexM23,
    /// Build for Cortex-M33 (no FPU)
    CortexM33,
    /// Build for Cortex-M33 (FPU)
    CortexM33F,
    /// Build for Cortex-A7, AFAIK all have NEON-VFP4
    CortexA7,
}
/// Set compiler optimization level
#[derive(Clone, Copy, PartialEq, Eq, Hash, Debug)]
pub enum Optimize {
    /// Level 0
    O0,
    /// Level 1
    O1,
    /// Level 2
    O2,
    /// Level 3
    O3,
    /// Optimize Size
    Os,
    /// All optimizations that don't harm debugging
    Og,
}
/// C Stanrd version to build against
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum CStandard {
    /// c1990
    C90,
    /// c1999
    C99,
    /// c2011
    C11,
    /// c2017
    C17,
    /// gnu1990
    GNU90,
    /// gnu1999
    GNU99,
    /// gnu2011
    GNU11,
    /// gnu2017
    GNU17,
}
/// C++ Standard version to build against
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum CPPStandard {
    /// c++1998
    CPP98,
    /// c++2003
    CPP03,
    /// c++2014
    CPP11,
    /// c++2014
    CPP14,
    /// c++2017
    CPP17,
    /// c++2020
    CPP20,
    /// gnu++1998
    GNU98,
    /// gnu++2003
    GNU03,
    /// gnu++2014
    GNU11,
    /// gnu++2014
    GNU14,
    /// gnu++2017
    GNU17,
    /// gnu++2020
    GNU20,
}
impl CPU {
    pub fn defs(&self) -> Vec<String> {
        match *self {
            CPU::CortexM0 => "CORTEX_USE_FPU=FALSE",
            CPU::CortexM0Plus => "CORTEX_USE_FPU=FALSE",
            CPU::CortexM3 => "CORTEX_USE_FPU=FALSE",
            CPU::CortexM4 => "CORTEX_USE_FPU=FALSE",
            CPU::CortexM4F => "CORTEX_USE_FPU=TRUE",
            CPU::CortexM7 => "CORTEX_USE_FPU=FALSE",
            CPU::CortexM7F => "CORTEX_USE_FPU=TRUE",
            CPU::CortexM7FDP => "CORTEX_USE_FPU=TRUE",
            CPU::CortexM23 => "CORTEX_USE_FPU=FALSE",
            CPU::CortexM33 => "CORTEX_USE_FPU=FALSE",
            CPU::CortexM33F => "CORTEX_USE_FPU=TRUE",
            CPU::CortexA7 => "CORTEX_USE_FPU=TRUE",
        }
        .split(" ")
        .map(|s| s.to_owned())
        .collect()
    }
    pub fn mcpu_flags(&self) -> String {
        let mut a = String::new();
        a.push_str(match *self {
            CPU::CortexM0 => "-mcpu=cortex-m0",
            CPU::CortexM0Plus => "-mcpu=cortex-m0plus",
            CPU::CortexM3 => "-mcpu=cortex-m3",
            CPU::CortexM4 => "-mcpu=cortex-m4",
            CPU::CortexM4F => {
                "-mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16 \
                 -fsingle-precision-constant"
            }
            CPU::CortexM7 => "-mcpu=cortex-m7",
            CPU::CortexM7F => "-mcpu=cortex-m7 -mfloat-abi=hard -mfpu=fpv5-sp-d16",
            CPU::CortexM7FDP => "-mcpu=cortex-m7 -mfloat-abi=hard -mfpu=fpv5-d16",
            CPU::CortexM23 => "-mcpu=cortex-m23",
            CPU::CortexM33 => "-mcpu=cortex-m33",
            CPU::CortexM33F => {
                "-mcpu=cortex-m33 -mfloat-abi=hard -mfpu=fpv5-sp-d16 -fsingle-precision-constant"
            }
            CPU::CortexA7 => {
                "-mcpu=cortex-a7 -march=armv7ve -mfpu=neon-vfpv4 -mlittle-endian -mfloat-abi=hard"
            }
        });
        a.push_str(match *self {
            CPU::CortexM0
            | CPU::CortexM0Plus
            | CPU::CortexM3
            | CPU::CortexM4
            | CPU::CortexM4F
            | CPU::CortexM7
            | CPU::CortexM7F
            | CPU::CortexM7FDP
            | CPU::CortexM23
            | CPU::CortexM33
            | CPU::CortexM33F => {
                " -mno-thumb-interwork -DTHUMB_NO_INTERWORKING -mthumb -DTHUMB -DTHUMB_PRESENT"
            }
            CPU::CortexA7 => "",
        });
        a
    }
    pub fn link_arg(&self) -> String {
        self.mcpu_flags()
    }
}
impl Optimize {
    pub fn arg(&self) -> String {
        let mut a = String::new();
        a.push_str(match *self {
            Optimize::O0 => "-O0",
            Optimize::O1 => "-O1",
            Optimize::O2 => "-O2",
            Optimize::O3 => "-O3",
            Optimize::Og => "-Og",
            Optimize::Os => "-Os",
        });
        a
    }
}
impl CStandard {
    pub fn arg(&self) -> String {
        let mut a = String::new();
        a.push_str(match *self {
            CStandard::C90 => "-std=c90",
            CStandard::C99 => "-std=c99",
            CStandard::C11 => "-std=c11",
            CStandard::C17 => "-std=c17",
            CStandard::GNU90 => "-std=gnu90",
            CStandard::GNU99 => "-std=gnu99",
            CStandard::GNU11 => "-std=gnu11",
            CStandard::GNU17 => "-std=gnu17",
        });
        a
    }
    pub fn to_str(&self) -> String {
        let mut a = String::new();
        a.push_str(match *self {
            CStandard::C90 => "c90",
            CStandard::C99 => "c99",
            CStandard::C11 => "c11",
            CStandard::C17 => "c17",
            CStandard::GNU90 => "gnu90",
            CStandard::GNU99 => "gnu99",
            CStandard::GNU11 => "gnu11",
            CStandard::GNU17 => "gnu17",
        });
        a
    }
}
impl CPPStandard {
    pub fn arg(&self) -> String {
        let mut a = String::new();
        a.push_str(match *self {
            CPPStandard::CPP98 => "-std=c++98",
            CPPStandard::CPP03 => "-std=c++03",
            CPPStandard::CPP11 => "-std=c++11",
            CPPStandard::CPP14 => "-std=c++14",
            CPPStandard::CPP17 => "-std=c++17",
            CPPStandard::CPP20 => "-std=c++20",
            CPPStandard::GNU98 => "-std=gnu++98",
            CPPStandard::GNU03 => "-std=gnu++03",
            CPPStandard::GNU11 => "-std=gnu++11",
            CPPStandard::GNU14 => "-std=gnu++14",
            CPPStandard::GNU17 => "-std=gnu++17",
            CPPStandard::GNU20 => "-std=gnu++20",
        });
        a
    }
    pub fn to_str(&self) -> String {
        let mut a = String::new();
        a.push_str(match *self {
            CPPStandard::CPP98 => "c++98",
            CPPStandard::CPP03 => "c++03",
            CPPStandard::CPP11 => "c++11",
            CPPStandard::CPP14 => "c++14",
            CPPStandard::CPP17 => "c++17",
            CPPStandard::CPP20 => "c++20",
            CPPStandard::GNU98 => "gnu++98",
            CPPStandard::GNU03 => "gnu++03",
            CPPStandard::GNU11 => "gnu++11",
            CPPStandard::GNU14 => "gnu++14",
            CPPStandard::GNU17 => "gnu++17",
            CPPStandard::GNU20 => "gnu++20",
        });
        a
    }
}

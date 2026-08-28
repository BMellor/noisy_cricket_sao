use bitflags::bitflags;
use build_system::*;

bitflags! {
#[derive(Default)]
pub struct Features: u64 {
    const DEFAULT = 0;

    const DSP_COMMON = 1<<0;
    const DSP_BASIC_MATH = 1<<1;
    const DSP_BAYES = 1<<2;
    const DSP_COMPLEX_MATH = 1<<3;
    const DSP_CONTROLLER = 1<<4;
    const DSP_DISTANCE = 1<<5;
    const DSP_FAST_MATH = 1<<6;
    const DSP_FILTERING = 1<<7;
    const DSP_MATRIX = 1<<8;
    const DSP_STATISTICS = 1<<9;
    const DSP_SUPPORT = 1<<10;
    const DSP_SVM = 1<<11;
    const DSP_TRANSFORM = 1<<12;
    const DSP_ALL =
      Self::DSP_COMMON.bits |
      Self::DSP_BASIC_MATH.bits |
      Self::DSP_BAYES.bits |
      Self::DSP_COMPLEX_MATH.bits |
      Self::DSP_CONTROLLER.bits |
      Self::DSP_DISTANCE.bits |
      Self::DSP_FAST_MATH.bits |
      Self::DSP_FILTERING.bits |
      Self::DSP_MATRIX.bits |
      Self::DSP_STATISTICS.bits |
      Self::DSP_SUPPORT.bits |
      Self::DSP_SVM.bits |
      Self::DSP_TRANSFORM.bits;
// API Options
    const DSP_API_MATRIX_CHECK = 1<<32;
    const DSP_API_ROUNDING = 1<<33;

// For CPU Support Libraries
    const CPU_SUPPORT_DSP = (1<<48) | Self::OPTIMIZE_LOOPUNROLL.bits;
    const CPU_SUPPORT_BIG_ENDIAN = 1<<49;
    const CPU_SUPPORT_NEON = 1<<50;
    const CPU_SUPPORT_HELIUM = 1<<51;
// Optimize Options
    const OPTIMIZE_LOOPUNROLL = 1<<60; // Auto Selected by SUPPORT_DSP
    const OPTIMIZE_SIZE = 1<<61; // Default if not provided is Opt::O2
    const OPTIMIZE_DEBUG = 1<<62; // takes precedence over OPTIMIZE_SPEED3
    const OPTIMIZE_SPEED3 = 1<<63; // takes precedence over OPTIMIZE_SIZE
}
}

pub fn add(state: &mut State, features: Features) {
    add_system_include_dir!(state, "Core/Include");

    let opt = if features.contains(Features::OPTIMIZE_DEBUG) {
        Optimize::O2
    } else if features.contains(Features::OPTIMIZE_SPEED3) {
        Optimize::O3
    } else if features.contains(Features::OPTIMIZE_SIZE) {
        Optimize::Os
    } else {
        Optimize::O2
    };

    if features.intersects(Features::DSP_ALL) {
        add_system_include_dir!(state, "DSP/Include");
        add_system_include_dir!(state, "DSP/PrivateInclude");

        if features.contains(Features::CPU_SUPPORT_BIG_ENDIAN) {
            state.add_define("ARM_MATH_BIG_ENDIAN");
        }
        if features.contains(Features::DSP_API_MATRIX_CHECK) {
            state.add_define("ARM_MATH_MATRIX_CHECK");
        }
        if features.contains(Features::DSP_API_ROUNDING) {
            state.add_define("ARM_MATH_ROUNDING");
        }
        if features.contains(Features::OPTIMIZE_LOOPUNROLL) {
            state.add_define("ARM_MATH_LOOPUNROLL");
        }
        if features.contains(Features::CPU_SUPPORT_NEON) {
            state.add_define("ARM_MATH_NEON");
        }
        if features.contains(Features::CPU_SUPPORT_HELIUM) {
            state.add_define("ARM_MATH_HELIUM");
        }
    }

    if features.contains(Features::DSP_BASIC_MATH) {
        add_c_file_sys!(
            state,
            "DSP/Source/BasicMathFunctions/BasicMathFunctions.c",
            opt
        );
    }
    if features.contains(Features::DSP_BAYES) {
        add_c_file_sys!(state, "DSP/Source/BayesFunctions/BayesFunctions.c", opt);
    }
    if features.contains(Features::DSP_COMMON) {
        add_c_file_sys!(state, "DSP/Source/CommonTables/CommonTables.c", opt);
    }
    if features.contains(Features::DSP_COMPLEX_MATH) {
        add_c_file_sys!(
            state,
            "DSP/Source/ComplexMathFunctions/ComplexMathFunctions.c",
            opt
        );
    }
    if features.contains(Features::DSP_CONTROLLER) {
        add_c_file_sys!(
            state,
            "DSP/Source/ControllerFunctions/ControllerFunctions.c",
            opt
        );
    }
    if features.contains(Features::DSP_DISTANCE) {
        add_c_file_sys!(
            state,
            "DSP/Source/DistanceFunctions/DistanceFunctions.c",
            opt
        );
    }
    if features.contains(Features::DSP_FAST_MATH) {
        add_c_file_sys!(
            state,
            "DSP/Source/FastMathFunctions/FastMathFunctions.c",
            opt
        );
    }
    if features.contains(Features::DSP_FILTERING) {
        add_c_file_sys!(
            state,
            "DSP/Source/FilteringFunctions/FilteringFunctions.c",
            opt
        );
    }
    if features.contains(Features::DSP_MATRIX) {
        add_c_file_sys!(state, "DSP/Source/MatrixFunctions/MatrixFunctions.c", opt);
    }
    if features.contains(Features::DSP_STATISTICS) {
        add_c_file_sys!(
            state,
            "DSP/Source/StatisticsFunctions/StatisticsFunctions.c",
            opt
        );
    }
    if features.contains(Features::DSP_SUPPORT) {
        add_c_file_sys!(state, "DSP/Source/SupportFunctions/SupportFunctions.c", opt);
    }
    if features.contains(Features::DSP_SVM) {
        add_c_file_sys!(state, "DSP/Source/SVMFunctions/SVMFunctions.c", opt);
    }
    if features.contains(Features::DSP_TRANSFORM) {
        add_c_file_sys!(
            state,
            "DSP/Source/TransformFunctions/TransformFunctions.c",
            opt
        );
    }
}

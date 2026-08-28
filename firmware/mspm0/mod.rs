use build_system::*;

#[allow(dead_code)]
pub enum Device {
    MSPM0C1103,
    MSPM0C1104,
    MSPM0C1105,
    MSPM0C1106,
    MSPM0G1105,
    MSPM0G1106,
    MSPM0G1107,
    MSPM0G1505,
    MSPM0G1506,
    MSPM0G1507,
    MSPM0G1518,
    MSPM0G1519,
    MSPM0G3105,
    MSPM0G3106,
    MSPM0G3107,
    MSPM0G3505,
    MSPM0G3506,
    MSPM0G3507,
    MSPM0G3518,
    MSPM0G3519,
    MSPM0G3529,
    MSPM0G5115,
    MSPM0G5116,
    MSPM0G5117,
    MSPM0G5187,
    MSPM0H3215,
    MSPM0H3216,
    MSPM0L1105,
    MSPM0L1106,
    MSPM0L1116,
    MSPM0L1117,
    MSPM0L1126,
    MSPM0L1127,
    MSPM0L1227,
    MSPM0L1228,
    MSPM0L1303,
    MSPM0L1304,
    MSPM0L1305,
    MSPM0L1306,
    MSPM0L1343,
    MSPM0L1344,
    MSPM0L1345,
    MSPM0L1346,
    MSPM0L2116,
    MSPM0L2117,
    MSPM0L2227,
    MSPM0L2228,
}

pub fn add(state: &mut State, device: Device) {
    add_include_dir!(state, ".");
    state.set_cpu(CPU::CortexM0Plus);
    cmsis::add(state, cmsis::Features::DEFAULT);

    state.add_define(match device {
        Device::MSPM0C1103 | Device::MSPM0C1104 => "DeviceFamily_MSPM0C110X",
        Device::MSPM0C1105 | Device::MSPM0C1106 => "DeviceFamily_MSPM0C1105_C1106",
        Device::MSPM0G1105 | Device::MSPM0G1106 | Device::MSPM0G1107 => "DeviceFamily_MSPM0G110X",
        Device::MSPM0G1505 | Device::MSPM0G1506 | Device::MSPM0G1507 => "DeviceFamily_MSPM0G150X",
        Device::MSPM0G1518 | Device::MSPM0G1519 => "DeviceFamily_MSPM0G151X",
        Device::MSPM0G3105 | Device::MSPM0G3106 | Device::MSPM0G3107 => "DeviceFamily_MSPM0G310X",
        Device::MSPM0G3505 | Device::MSPM0G3506 | Device::MSPM0G3507 => "DeviceFamily_MSPM0G350X",
        Device::MSPM0G3518 | Device::MSPM0G3519 => "DeviceFamily_MSPM0G351X",
        Device::MSPM0G3529 => "DeviceFamily_MSPM0G352X",
        Device::MSPM0G5115 | Device::MSPM0G5116 | Device::MSPM0G5117 => "DeviceFamily_MSPM0G511X",
        Device::MSPM0G5187 => "DeviceFamily_MSPM0G518X",
        Device::MSPM0H3215 | Device::MSPM0H3216 => "DeviceFamily_MSPM0H321X",
        Device::MSPM0L1105 | Device::MSPM0L1106 => "DeviceFamily_MSPM0L110X",
        Device::MSPM0L1116 | Device::MSPM0L1117 => "DeviceFamily_MSPM0L111X",
        Device::MSPM0L1126 | Device::MSPM0L1127 => "DeviceFamily_MSPM0L112X",
        Device::MSPM0L1227 | Device::MSPM0L1228 => "DeviceFamily_MSPM0L122X",
        Device::MSPM0L1303 | Device::MSPM0L1304 | Device::MSPM0L1305 | Device::MSPM0L1306 => {
            "DeviceFamily_MSPM0L130X"
        }
        Device::MSPM0L1343 | Device::MSPM0L1344 | Device::MSPM0L1345 | Device::MSPM0L1346 => {
            "DeviceFamily_MSPM0L134X"
        }
        Device::MSPM0L2116 | Device::MSPM0L2117 => "DeviceFamily_MSPM0L211X",
        Device::MSPM0L2227 | Device::MSPM0L2228 => "DeviceFamily_MSPM0L222X",
    });

    linker_script!(
        state,
        &format!(
            "ti/devices/msp/m0p/linker_files/gcc/{}.lds",
            match device {
                Device::MSPM0C1103 => "mspm0c1103",
                Device::MSPM0C1104 => "mspm0c1104",
                Device::MSPM0C1105 => "mspm0c1105",
                Device::MSPM0C1106 => "mspm0c1106",
                Device::MSPM0G1105 => "mspm0g1105",
                Device::MSPM0G1106 => "mspm0g1106",
                Device::MSPM0G1107 => "mspm0g1107",
                Device::MSPM0G1505 => "mspm0g1505",
                Device::MSPM0G1506 => "mspm0g1506",
                Device::MSPM0G1507 => "mspm0g1507",
                Device::MSPM0G1518 => "mspm0g1518",
                Device::MSPM0G1519 => "mspm0g1519",
                Device::MSPM0G3105 => "mspm0g3105",
                Device::MSPM0G3106 => "mspm0g3106",
                Device::MSPM0G3107 => "mspm0g3107",
                Device::MSPM0G3505 => "mspm0g3505",
                Device::MSPM0G3506 => "mspm0g3506",
                Device::MSPM0G3507 => "mspm0g3507",
                Device::MSPM0G3518 => "mspm0g3518",
                Device::MSPM0G3519 => "mspm0g3519",
                Device::MSPM0G3529 => "mspm0g3529",
                Device::MSPM0G5115 => "mspm0g5115",
                Device::MSPM0G5116 => "mspm0g5116",
                Device::MSPM0G5117 => "mspm0g5117",
                Device::MSPM0G5187 => "mspm0g5187",
                Device::MSPM0H3215 => "mspm0h3215",
                Device::MSPM0H3216 => "mspm0h3216",
                Device::MSPM0L1105 => "mspm0l1105",
                Device::MSPM0L1106 => "mspm0l1106",
                Device::MSPM0L1116 => "mspm0l1116",
                Device::MSPM0L1117 => "mspm0l1117",
                Device::MSPM0L1126 => "mspm0l1126",
                Device::MSPM0L1127 => "mspm0l1127",
                Device::MSPM0L1227 => "mspm0l1227",
                Device::MSPM0L1228 => "mspm0l1228",
                Device::MSPM0L1303 => "mspm0l1303",
                Device::MSPM0L1304 => "mspm0l1304",
                Device::MSPM0L1305 => "mspm0l1305",
                Device::MSPM0L1306 => "mspm0l1306",
                Device::MSPM0L1343 => "mspm0l1343",
                Device::MSPM0L1344 => "mspm0l1344",
                Device::MSPM0L1345 => "mspm0l1345",
                Device::MSPM0L1346 => "mspm0l1346",
                Device::MSPM0L2116 => "mspm0l2116",
                Device::MSPM0L2117 => "mspm0l2117",
                Device::MSPM0L2227 => "mspm0l2227",
                Device::MSPM0L2228 => "mspm0l2228",
            }
        )
    );
    add_c_file!(
        state,
        &format!(
            "ti/devices/msp/m0p/startup_system_files/gcc/startup_{}_gcc.c",
            match device {
                Device::MSPM0C1103 | Device::MSPM0C1104 => "mspm0c110x",
                Device::MSPM0C1105 | Device::MSPM0C1106 => "mspm0c1105_c1106",
                Device::MSPM0G1105 | Device::MSPM0G1106 | Device::MSPM0G1107 => "mspm0g110x",
                Device::MSPM0G1505 | Device::MSPM0G1506 | Device::MSPM0G1507 => "mspm0g150x",
                Device::MSPM0G1518 | Device::MSPM0G1519 => "mspm0g151x",
                Device::MSPM0G3105 | Device::MSPM0G3106 | Device::MSPM0G3107 => "mspm0g310x",
                Device::MSPM0G3505 | Device::MSPM0G3506 | Device::MSPM0G3507 => "mspm0g350x",
                Device::MSPM0G3518 | Device::MSPM0G3519 => "mspm0g351x",
                Device::MSPM0G3529 => "mspm0g352x",
                Device::MSPM0G5115 | Device::MSPM0G5116 | Device::MSPM0G5117 => "mspm0g511x",
                Device::MSPM0G5187 => "mspm0g518x",
                Device::MSPM0H3215 | Device::MSPM0H3216 => "mspm0h321x",
                Device::MSPM0L1105 | Device::MSPM0L1106 => "mspm0l110x",
                Device::MSPM0L1116 | Device::MSPM0L1117 => "mspm0l111x",
                Device::MSPM0L1126 | Device::MSPM0L1127 => "mspm0l112x",
                Device::MSPM0L1227 | Device::MSPM0L1228 => "mspm0l122x",
                Device::MSPM0L1303 | Device::MSPM0L1304 | Device::MSPM0L1305 |
                Device::MSPM0L1306 => "mspm0l130x",
                Device::MSPM0L1343 | Device::MSPM0L1344 | Device::MSPM0L1345 |
                Device::MSPM0L1346 => "mspm0l134x",
                Device::MSPM0L2116 | Device::MSPM0L2117 => "mspm0l211x",
                Device::MSPM0L2227 | Device::MSPM0L2228 => "mspm0l222x",
            }
        )
    );

    // driverlib/m0p/sysctl
    add_c_file!(
        state,
        &format!(
            "ti/driverlib/m0p/sysctl/dl_sysctl_{}.c",
            match device {
                Device::MSPM0C1103 | Device::MSPM0C1104 => "mspm0c110x",
                Device::MSPM0C1105 | Device::MSPM0C1106 => "mspm0c1105_c1106",
                Device::MSPM0G1105 | Device::MSPM0G1106 | Device::MSPM0G1107 |
                Device::MSPM0G1505 | Device::MSPM0G1506 | Device::MSPM0G1507 |
                Device::MSPM0G3105 | Device::MSPM0G3106 | Device::MSPM0G3107 |
                Device::MSPM0G3505 | Device::MSPM0G3506 | Device::MSPM0G3507 => "mspm0g1x0x_g3x0x",
                Device::MSPM0G1518 | Device::MSPM0G1519 | Device::MSPM0G3518 |
                Device::MSPM0G3519 => "mspm0gx51x",
                Device::MSPM0G3529 => "mspm0g352x",
                Device::MSPM0G5115 | Device::MSPM0G5116 | Device::MSPM0G5117 => "mspm0g511x",
                Device::MSPM0G5187 => "mspm0g518x",
                Device::MSPM0H3215 | Device::MSPM0H3216 => "mspm0h321x",
                Device::MSPM0L1105 | Device::MSPM0L1106 | Device::MSPM0L1126 |
                Device::MSPM0L1127 | Device::MSPM0L1303 | Device::MSPM0L1304 |
                Device::MSPM0L1305 | Device::MSPM0L1306 | Device::MSPM0L1343 |
                Device::MSPM0L1344 | Device::MSPM0L1345 | Device::MSPM0L1346 => "mspm0l11xx_l13xx",
                Device::MSPM0L1116 | Device::MSPM0L1117 => "mspm0l111x",
                Device::MSPM0L1227 | Device::MSPM0L1228 | Device::MSPM0L2227 |
                Device::MSPM0L2228 => "mspm0l122x_l222x",
                Device::MSPM0L2116 | Device::MSPM0L2117 => "mspm0l211x_l112x",
            }
        )
    );
    // driverlib/m0p
    add_c_file!(state, "ti/driverlib/m0p/dl_factoryregion.c");
    add_c_file!(state, "ti/driverlib/m0p/dl_interrupt.c");
    // driverlib
    add_c_file!(state, "ti/driverlib/dl_adc12.c");
    add_c_file!(state, "ti/driverlib/dl_gpio.c");
    add_c_file!(state, "ti/driverlib/dl_rtc_a.c");
    add_c_file!(state, "ti/driverlib/dl_timerg.c");
    add_c_file!(state, "ti/driverlib/dl_aesadv.c");
    add_c_file!(state, "ti/driverlib/dl_i2c.c");
    add_c_file!(state, "ti/driverlib/dl_rtc_b.c");
    add_c_file!(state, "ti/driverlib/dl_trng.c");
    add_c_file!(state, "ti/driverlib/dl_aes.c");
    add_c_file!(state, "ti/driverlib/dl_i2s.c");
    add_c_file!(state, "ti/driverlib/dl_rtc.c");
    add_c_file!(state, "ti/driverlib/dl_uart.c");
    add_c_file!(state, "ti/driverlib/dl_common.c");
    add_c_file!(state, "ti/driverlib/dl_iwdt.c");
    add_c_file!(state, "ti/driverlib/dl_rtc_common.c");
    add_c_file!(state, "ti/driverlib/dl_unicomm.c");
    add_c_file!(state, "ti/driverlib/dl_comp.c");
    add_c_file!(state, "ti/driverlib/dl_keystorectl.c");
    add_c_file!(state, "ti/driverlib/dl_scratchpad.c");
    add_c_file!(state, "ti/driverlib/dl_unicommi2cc.c");
    add_c_file!(state, "ti/driverlib/dl_crc.c");
    add_c_file!(state, "ti/driverlib/dl_lcd.c");
    add_c_file!(state, "ti/driverlib/dl_spgss.c");
    add_c_file!(state, "ti/driverlib/dl_unicommi2ct.c");
    add_c_file!(state, "ti/driverlib/dl_crcp.c");
    add_c_file!(state, "ti/driverlib/dl_lfss.c");
    add_c_file!(state, "ti/driverlib/dl_spi.c");
    add_c_file!(state, "ti/driverlib/dl_unicommspi.c");
    add_c_file!(state, "ti/driverlib/dl_dac12.c");
    add_c_file!(state, "ti/driverlib/dl_mathacl.c");
    add_c_file!(state, "ti/driverlib/dl_tamperio.c");
    add_c_file!(state, "ti/driverlib/dl_unicommuart.c");
    add_c_file!(state, "ti/driverlib/dl_dma.c");
    add_c_file!(state, "ti/driverlib/dl_mcan.c");
    add_c_file!(state, "ti/driverlib/dl_timera.c");
    add_c_file!(state, "ti/driverlib/dl_vref.c");
    add_c_file!(state, "ti/driverlib/dl_flashctl.c");
    add_c_file!(state, "ti/driverlib/dl_npu.c");
    add_c_file!(state, "ti/driverlib/dl_timerb.c");
    add_c_file!(state, "ti/driverlib/dl_wwdt.c");
    add_c_file!(state, "ti/driverlib/dl_gpamp.c");
    add_c_file!(state, "ti/driverlib/dl_opa.c");
    add_c_file!(state, "ti/driverlib/dl_timer.c");
}

#!/bin/sh
cargo run
probe-rs download --reset --chip MSPM0C1104 ./target/build/mspm0.elf

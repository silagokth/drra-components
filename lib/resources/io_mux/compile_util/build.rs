use std::fs;
use std::env;
use std::path::Path;

fn main() {
    println!("cargo:rerun-if-changed=build.rs");
    println!("cargo:rerun-if-changed=data.txt");

    let local_isa_config_path = Path::new("./isa.json");
    let parent_isa_config_path = Path::new("../isa.json");
    let isa_config_path = if local_isa_config_path.exists() {
        local_isa_config_path
    } else {
        parent_isa_config_path
    };
    let config = fs::read_to_string(isa_config_path)
        .map_err(|e| {
            eprintln!(
                "Failed to read ISA config file ({}): {}",
                isa_config_path.to_str().unwrap_or("unknown path"),
                e
            );
            std::process::exit(1);
        })
        .unwrap();

    let isa_config: serde_json::Value = serde_json::from_str(&config)
        .map_err(|e| {
            eprintln!("Failed to parse ISA config JSON: {}", e);
            std::process::exit(1);
        })
        .unwrap();

    let instructions = isa_config["instructions"].as_array().unwrap();
    let rep_instr = instructions
        .iter()
        .find(|instr| instr["name"] == "rep")
        .unwrap_or_else(|| {
            eprintln!("Failed to find 'rep' instruction in ISA config");
            std::process::exit(1);
        });
    let rep_segments = rep_instr["segments"].as_array().unwrap();

    let iter_bw = rep_segments
        .iter()
        .find(|seg| seg["name"] == "iter")
        .unwrap()["bitwidth"]
        .as_u64()
        .unwrap();
    let step_bw = rep_segments
        .iter()
        .find(|seg| seg["name"] == "step")
        .unwrap()["bitwidth"]
        .as_u64()
        .unwrap();
    let delay_bw = rep_segments
        .iter()
        .find(|seg| seg["name"] == "delay")
        .unwrap()["bitwidth"]
        .as_u64()
        .unwrap();

    let out = format!(
        "pub const ITER_BITWIDTH: u32 = {};\npub const STEP_BITWIDTH: u32 = {};\n\
     pub const DELAY_BITWIDTH: u32 = {};",
        iter_bw, step_bw, delay_bw
    );

    let out_dir = env::var("OUT_DIR").unwrap_or_else(|e| {
        eprintln!("Failed to read OUT_DIR: {}", e);
        std::process::exit(1);
    });
    let out_path = Path::new(&out_dir).join("isa_config.rs");
    fs::write(&out_path, out).unwrap_or_else(|e| {
        eprintln!("Failed to write isa_config.rs: {}", e);
        std::process::exit(1);
    });
}

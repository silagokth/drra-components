use std::env;
use std::fs;
use std::io::Write;
use std::process;

use serde_json::Value;

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() != 5 {
        eprintln!(
            "Usage: {} <isa_path> <arch_path> <tpl_path> <out_path>",
            args[0]
        );
        process::exit(1);
    }
    let isa_path = &args[1];
    let arch_path = &args[2];
    let tpl_path = &args[3];
    let out_path = &args[4];

    // Read and parse context JSON
    let isa_data = fs::read_to_string(isa_path).expect("Failed to read context file");
    let arch_data = fs::read_to_string(arch_path).expect("Failed to read context file");

    // Combine isa and arch into a single context
    let ctx_json = combine_json(&isa_data, &arch_data).expect("Failed to combine JSON");

    // Read template
    let tpl_str = fs::read_to_string(tpl_path).expect("Failed to read template file");

    // Jinja env setup
    let mut env = minijinja::Environment::new();
    env.set_trim_blocks(true);
    env.set_lstrip_blocks(true);
    env.add_template("template", &tpl_str)
        .expect("Failed to add template");
    let jinja_template = env
        .get_template("template")
        .expect("Failed to get template");

    // Render template with context
    println!("Rendering template ({})...", tpl_path);
    let result = jinja_template.render(ctx_json).map_err(|e| {
        eprintln!("Template rendering error: {}", e);
        process::exit(1);
    });

    let output = result.unwrap_or_else(|_| {
        eprintln!("Failed to render template");
        process::exit(1);
    });
    let mut file = fs::File::create(out_path).expect("Failed to create output file");
    Write::write_all(&mut file, output.as_bytes()).expect("Failed to write to output file");
}

fn combine_json(isa_json: &str, arch_json: &str) -> serde_json::Result<Value> {
    let isa_content: Value = serde_json::from_str(isa_json)?;
    let arch_content: Value = serde_json::from_str(arch_json)?;

    let mut combined = arch_content.as_object().unwrap().clone();
    combined.insert("isa".to_string(), isa_content);

    Ok(Value::Object(combined))
}

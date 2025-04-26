use std::collections::HashMap;
use std::env;
use std::fs::File;
use std::io::{BufRead, BufReader, Write};

/// A structure to hold all lines belonging to a particular
/// template block (including the start/end marker lines).
struct TemplateBlock {
    lines: Vec<String>,
}

/// Parse the original template file into a map: block_id -> block content (including markers).
fn parse_template_file(path: &str) -> std::io::Result<HashMap<String, TemplateBlock>> {
    let file = File::open(path)?;
    let reader = BufReader::new(file);

    let mut blocks = HashMap::new();
    let mut current_block_id: Option<String> = None;
    let mut current_block_lines = Vec::new();

    // Whether we are currently *inside* a block
    let mut in_block = false;

    // Regex-like patterns for start/end markers.
    // For simplicity, we do naive string checks; you could use real regex if you prefer.
    let start_prefix = "// vesyla_template_start ";
    let end_prefix = "// vesyla_template_end ";

    for line_result in reader.lines() {
        let line = line_result?;
        if line.trim_start().starts_with(start_prefix) {
            // We are entering a new block
            in_block = true;
            // Save the block identifier
            let block_id = line.trim_start()
                               .trim_start_matches(start_prefix)
                               .to_string();
            current_block_id = Some(block_id.clone());
            // Start collecting lines, including this marker
            current_block_lines.clear();
            current_block_lines.push(line.clone());
        } else if line.trim_start().starts_with(end_prefix) {
            // We are ending the current block
            if let Some(ref block_id) = current_block_id {
                // Include this end-marker line
                current_block_lines.push(line.clone());

                // Store the lines for this block in our map
                blocks.insert(
                    block_id.clone(),
                    TemplateBlock { lines: current_block_lines.clone() }
                );
            }
            // Reset
            in_block = false;
            current_block_id = None;
            current_block_lines.clear();
        } else if in_block {
            // We are inside a block, just collect lines
            current_block_lines.push(line.clone());
        }
        // If we are outside a block, we do nothing with lines here;
        // they are simply "outside" lines not stored in blocks.
    }

    Ok(blocks)
}

fn main() -> std::io::Result<()> {
    // Expect: cargo run -- <original_template> <expanded> <output>
    let args: Vec<String> = env::args().collect();
    if args.len() != 4 {
        eprintln!("Usage: {} <template_file> <expanded_file> <output_file>", args[0]);
        std::process::exit(1);
    }

    let template_file = &args[1];
    let expanded_file = &args[2];
    let output_file = &args[3];

    // 1. Parse the original template blocks
    let blocks_map = parse_template_file(template_file)?;

    // 2. Open the expanded file (which may be changed outside blocks)
    let exp_file = File::open(expanded_file)?;
    let exp_reader = BufReader::new(exp_file);

    // 3. Open output for writing
    let mut output = File::create(output_file)?;

    let start_prefix = "// vesyla_template_start ";
    let end_prefix = "// vesyla_template_end ";

    // Whether we are *inside* a block in the expanded file.  If we are,
    // we are currently skipping lines until we see the end marker for that block.
    let mut skipping_block = false;
    let mut current_block_id: Option<String> = None;

    for line_result in exp_reader.lines() {
        let line = line_result?;

        // Check for block start
        if !skipping_block && line.trim_start().starts_with(start_prefix) {
            // We have encountered a block start in the expanded code
            let block_id = line.trim_start()
                               .trim_start_matches(start_prefix)
                               .to_string();

            // If the original template has that block, write *the original block* out:
            if let Some(original_block) = blocks_map.get(&block_id) {
                for block_line in &original_block.lines {
                    writeln!(output, "{}", block_line)?;
                }
            } else {
                // If there's no matching block in original, we could copy or skip.
                // Let's just copy it as-is, to avoid losing data:
                writeln!(output, "{}", line)?;
            }

            // Now we start skipping lines until we find the matching end
            skipping_block = true;
            current_block_id = Some(block_id);
            continue;
        }

        // Check for block end
        if skipping_block && line.trim_start().starts_with(end_prefix) {
            let block_id = line.trim_start()
                               .trim_start_matches(end_prefix)
                               .to_string();

            // If it matches the block we're skipping
            if Some(block_id) == current_block_id {
                // We found the end, so end skipping
                skipping_block = false;
                current_block_id = None;
            }
            // We don't write this line here, because the original block's end line was
            // already written from the template. So we skip it entirely.
            continue;
        }

        // If we're outside a block, just copy the line directly.
        if !skipping_block {
            writeln!(output, "{}", line)?;
        } else {
            // We are inside a block we’re skipping, so ignore these lines.
            continue;
        }
    }

    Ok(())
}


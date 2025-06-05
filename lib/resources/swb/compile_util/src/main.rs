/**
 * This is the main file for the Rust implementation of the compile_util program.
 *
 * It requires three arguments: the subcommand, the input file, and the output file.
 *
 * The subcommand specifies the operation to be performed. It can be one of the
 * following:
 * - get_timing_model: to extract operation expressions timing model
 * - reshape_instr: to reshape the instruction fields and instrucion list
 *
 * The input file is a JSON file that contains the following fields:
 * - row: the number of rows in the timing model
 * - col: the number of columns in the timing model
 * - slot: the number of slots in the timing model
 * - port: the number of ports in the timing model
 * - op_name: the name of the operation
 * - instr_list: a list of instructions, each instruction contains the following fields:
 *  - name: the name of the instruction
 *  - fields: a list of fields, each field contains the following fields:
 *   - name: the name of the field
 *   - value: the value of the field
 *
 * The output file is a text file. Its content depends on the subcommand:
 * - get_timing_model: the output file will contain the timing model in a plain text format
 * - reshape_instr: the output file will contain the reshaped instruction list in json format
 *
 * The function get_timing_model(op_name, instr_list) and reshape_instr(op_name, instr_list)
 * are the main functions that you need to implement. Don't change the function
 * interface and any other functions. You can add helper functions if needed.
 *
 */
use serde_json::Value;
use serde_json::json;
use std::collections::HashMap;
use std::env;

/*******************************************************************************
 * Modify here to implement the function. Don't change the function interface.
 ******************************************************************************/
fn get_timing_model(op: Op) -> String {
    let row = op.row;
    let col = op.col;
    let slot = op.slot;
    let port = op.port;
    let instr_list = op.instr_list;

    if row < 0 || col < 0 || slot < 0 || port < 0 {
        panic!("Invalid input");
    }

    let mut t: HashMap<i64, String> = HashMap::new();
    let mut r: HashMap<i64, (String, String)> = HashMap::new();
    let mut expr = format!("e0");

    for instr in instr_list {
        if instr.0 == "rep" {
            let instr_fields = instr.1;
            let mut iter = "0".to_string();
            let mut delay = "0".to_string();
            let mut level = 0;
            for field in instr_fields {
                match field.0.as_str() {
                    "iter" => iter = field.1,
                    "delay" => delay = field.1,
                    "level" => level = field.1.parse::<i64>().unwrap(),
                    _ => {}
                }
            }
            iter = (iter.parse::<i64>().unwrap() + 1).to_string();
            r.insert(level, (iter, delay.to_string()));
        } else if instr.0 == "fsm" {
            let instr_fields = instr.1;
            t.insert(0, "0".to_string());
            t.insert(1, "0".to_string());
            t.insert(2, "0".to_string());
            for field in instr_fields {
                match field.0.as_str() {
                    "delay_0" => {
                        t.insert(0, field.1);
                    }
                    "delay_1" => {
                        t.insert(1, field.1);
                    }
                    "delay_2" => {
                        t.insert(2, field.1);
                    }
                    _ => {}
                }
            }
        }
    }

    let mut event_counter = 1;
    for i in 0..3 {
        if let Some(delay) = t.get(&i) {
            if delay != "0" {
                expr = format!("T<{}>({}, e{})", delay, expr, event_counter);
                event_counter += 1;
            } else {
                break;
            }
        }
    }

    for i in 0..8 {
        if let Some(values) = r.get(&i) {
            expr = format!("R<{},{}>({})", values.0, values.1, expr);
        } else {
            break;
        }
    }
    expr
}

fn reshape_instr(op: Op) -> Op {
    let mut reshaped_instr_list = Vec::new();
    for instr in op.instr_list {
        if instr.0 == "rep" {
            let mut iter = 0;
            let mut delay = 0;
            let mut step = 0;

            for field in instr.1.iter() {
                if field.0 == "iter" {
                    iter = field.1.parse::<i64>().unwrap();
                } else if field.0 == "delay" {
                    delay = field.1.parse::<i64>().unwrap();
                } else if field.0 == "step" {
                    step = field.1.parse::<i64>().unwrap();
                }
            }

            let mut repx_flag = false;
            let mut iterx = 0;
            let mut delayx = 0;
            let mut stepx = 0;
            if iter > 2i64.pow(8) - 1 {
                repx_flag = true;
                iterx = iter / 2i64.pow(8);
                iter = iter % 2i64.pow(8);
            }
            if delay > 2i64.pow(8) - 1 {
                repx_flag = true;
                delayx = delay / 2i64.pow(8);
                delay = delay % 2i64.pow(8);
            }
            if step > 2i64.pow(8) - 1 {
                repx_flag = true;
                stepx = step / 2i64.pow(8);
                step = step % 2i64.pow(8);
            }

            if repx_flag {
                let mut rep_instr = instr.clone();
                let mut repx_instr = instr.clone();
                for field in rep_instr.1.iter_mut() {
                    if field.0 == "iter" {
                        field.1 = iter.to_string();
                    } else if field.0 == "delay" {
                        field.1 = delay.to_string();
                    } else if field.0 == "step" {
                        field.1 = step.to_string();
                    }
                }
                for field in repx_instr.1.iter_mut() {
                    if field.0 == "iter" {
                        field.1 = iterx.to_string();
                    } else if field.0 == "delay" {
                        field.1 = delayx.to_string();
                    } else if field.0 == "step" {
                        field.1 = stepx.to_string();
                    }
                }
                reshaped_instr_list.push(rep_instr);
                reshaped_instr_list.push(repx_instr);
            } else {
                reshaped_instr_list.push(instr.clone());
            }
        } else {
            reshaped_instr_list.push(instr.clone());
        }
    }
    Op {
        op_name: op.op_name,
        row: op.row,
        col: op.col,
        slot: op.slot,
        port: op.port,
        instr_list: reshaped_instr_list,
    }
}

/*******************************************************************************
 * Modification ends here
 ******************************************************************************/

#[derive(Debug, Clone)]
struct Op {
    op_name: String,
    row: i64,
    col: i64,
    slot: i64,
    port: i64,
    instr_list: Vec<(String, Vec<(String, String)>)>,
}

impl Op {
    fn from_json(j: Value) -> Op {
        let op_name = j["name"].as_str().unwrap().to_string();
        let row = j["row"].as_i64().unwrap();
        let col = j["col"].as_i64().unwrap();
        let slot = j["slot"].as_i64().unwrap();
        let port = j["port"].as_i64().unwrap();
        let mut instr_list = Vec::new();
        for instr_json in j["instr_list"].as_array().unwrap() {
            let instr_name = instr_json["name"].as_str().unwrap().to_string();
            let mut instr_fields = Vec::new();
            for field_json in instr_json["fields"].as_array().unwrap() {
                let field_name = field_json["name"].as_str().unwrap().to_string();
                let field_value = field_json["value"].as_str().unwrap().to_string();
                instr_fields.push((field_name, field_value));
            }
            instr_list.push((instr_name, instr_fields));
        }
        Op {
            op_name,
            row,
            col,
            slot,
            port,
            instr_list,
        }
    }

    fn to_json(&self) -> Value {
        let mut instr_list_json = Vec::new();
        for (instr_name, fields) in &self.instr_list {
            let mut fields_json = Vec::new();
            for (field_name, field_value) in fields {
                fields_json.push(json!({
                    "name": field_name,
                    "value": field_value
                }));
            }
            instr_list_json.push(json!({
                "name": instr_name,
                "fields": fields_json
            }));
        }
        json!({
            "op_name": self.op_name,
            "row": self.row,
            "col": self.col,
            "slot": self.slot,
            "port": self.port,
            "instr_list": instr_list_json
        })
    }
}

fn main() {
    let args = env::args().collect::<Vec<String>>();
    if args.len() < 4 {
        println!("Usage: {} <subcommand> <input_file> <output_file>", args[0]);
        panic!("No subcommand/input file/output file specified!");
    }
    let subcommand = &args[1]; // subcommand
    let input_file = &args[2]; // input_file
    let output_file = &args[3]; // output_file
    // read the input file as json
    let input = std::fs::read_to_string(input_file).unwrap();
    let j: Value = serde_json::from_str(&input).unwrap();
    let op = Op::from_json(j);

    // Call the appropriate function based on the subcommand
    match subcommand.as_str() {
        "get_timing_model" => {
            let expr = get_timing_model(op);
            std::fs::write(output_file, expr).unwrap();
        }
        "reshape_instr" => {
            let new_op = reshape_instr(op);
            let reshaped_json = new_op.to_json();
            std::fs::write(output_file, serde_json::to_string(&reshaped_json).unwrap()).unwrap();
        }
        _ => panic!("Unknown subcommand: {}", subcommand),
    }
}

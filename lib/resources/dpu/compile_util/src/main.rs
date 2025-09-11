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
 * - kind: the type of the operation, it can be "rop" or "cop"
 * - id: the id of the operation
 * - row: the row location of the operation
 * - col: the column location of the operation
 * - slot: the slot location of the operation, only exists for "rop"
 * - port: the port location of the operation, only exists for "rop"
 * - body: a list of instructions, each instruction contains the following fields:
 *  - id: the name of the instruction
 *  - kind: the type of the instruction
 *  - params: a list of parameter fields, each field contains:
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
use std::{
    collections::HashMap,
    env,
    ops::{Deref, DerefMut},
};

use serde_json::{json, Value};

/*******************************************************************************
 * Modify here to implement the function. Don't change the function interface.
 ******************************************************************************/
fn get_timing_model(op: Op) -> String {
    let mut t: HashMap<i64, String> = HashMap::new();
    let mut r: HashMap<i64, (String, String)> = HashMap::new();
    let mut expr = "e0".to_string();

    for instr in op.body {
        if instr.kind == "rep" {
            let instr_fields = instr.params;
            let mut iter = instr_fields.get_value("iter");
            let delay = instr_fields.get_value("delay");
            let level = instr_fields.get_value("level");
            iter = (iter.parse::<i64>().unwrap() + 1).to_string();
            r.insert(
                level.parse::<i64>().unwrap(),
                (iter.to_string(), delay.to_string()),
            );
        } else if instr.kind == "fsm" {
            let instr_fields = instr.params;
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
    let mut t_keys: Vec<_> = t.keys().cloned().collect();
    t_keys.sort();
    for i in t_keys {
        if let Some(delay) = t.get(&i) {
            if delay != "0" {
                expr = format!("T<{}>({}, e{})", delay, expr, event_counter);
                event_counter += 1;
            } else {
                break;
            }
        }
    }

    let mut r_keys: Vec<_> = r.keys().cloned().collect();
    r_keys.sort();
    for i in r_keys {
        if let Some(values) = r.get(&i) {
            expr = format!("R<{},{}>({})", values.0, values.1, expr);
        } else {
            break;
        }
    }

    expr
}

fn reshape_instr(op: Op) -> Op {
    let mut new_op = op.clone();
    let mut new_body = Vec::new();
    for instr in op.body {
        if instr.kind == "rep" {
            let mut iter = instr
                .params
                .get_value("iter")
                .parse::<i64>()
                .expect("Failed to parse iter as i64");
            let mut delay = instr
                .params
                .get_value("delay")
                .parse::<i64>()
                .expect("Failed to parse iter as i64");

            let mut step = instr
                .params
                .get_value("step")
                .parse::<i64>()
                .expect("Failed to parse iter as i64");

            let mut repx_flag = false;
            let mut iterx = 0;
            let mut delayx = 0;
            let mut stepx = 0;
            if iter > 2i64.pow(6) - 1 {
                repx_flag = true;
                iterx = iter / 2i64.pow(6);
                iter %= 2i64.pow(6);
            }
            if delay > 2i64.pow(6) - 1 {
                repx_flag = true;
                delayx = delay / 2i64.pow(6);
                delay %= 2i64.pow(6);
            }
            if step > 2i64.pow(6) - 1 {
                repx_flag = true;
                stepx = step / 2i64.pow(6);
                step %= 2i64.pow(6);
            }

            if repx_flag {
                let mut rep_instr = instr.clone();
                let mut repx_instr = instr.clone();
                rep_instr.kind = "rep".to_string();
                repx_instr.kind = "repx".to_string();
                for field in rep_instr.params.iter_mut() {
                    if field.0 == "iter" {
                        field.1 = iter.to_string();
                    } else if field.0 == "delay" {
                        field.1 = delay.to_string();
                    } else if field.0 == "step" {
                        field.1 = step.to_string();
                    }
                }
                for field in repx_instr.params.iter_mut() {
                    if field.0 == "iter" {
                        field.1 = iterx.to_string();
                    } else if field.0 == "delay" {
                        field.1 = delayx.to_string();
                    } else if field.0 == "step" {
                        field.1 = stepx.to_string();
                    }
                }
                new_body.push(rep_instr);
                new_body.push(repx_instr);
            } else {
                new_body.push(instr.clone());
            }
        } else {
            new_body.push(instr.clone());
        }
    }
    new_op.body = new_body;
    new_op
}

/*******************************************************************************
 * Modification ends here
 ******************************************************************************/

#[derive(Debug, Clone)]
struct InstrSegments(Vec<(String, String)>);
impl InstrSegments {
    pub fn new(fields: Vec<(String, String)>) -> Self {
        InstrSegments(fields)
    }
    pub fn get_value(&self, key: &str) -> String {
        let instr_fields = &self.0;
        instr_fields
            .iter()
            .find_map(|(k, v)| (k == key).then_some(v).cloned())
            .unwrap_or("0".to_string())
    }
}

impl Deref for InstrSegments {
    type Target = Vec<(String, String)>;

    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

impl DerefMut for InstrSegments {
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.0
    }
}

impl IntoIterator for InstrSegments {
    type Item = (String, String);
    type IntoIter = std::vec::IntoIter<(String, String)>;

    fn into_iter(self) -> Self::IntoIter {
        self.0.into_iter()
    }
}

impl<'a> IntoIterator for &'a InstrSegments {
    type Item = &'a (String, String);
    type IntoIter = std::slice::Iter<'a, (String, String)>;

    fn into_iter(self) -> Self::IntoIter {
        self.0.iter()
    }
}

#[derive(Debug, Clone)]
struct Instr {
    kind: String,
    id: String,
    params: InstrSegments,
}

impl Instr {
    fn from_json(j: Value) -> Instr {
        let id = j["id"].as_str().unwrap().to_string();
        let kind = j["kind"].as_str().unwrap().to_string();
        let mut params = Vec::new();
        for param_json in j["params"].as_array().unwrap() {
            let name = param_json["name"].as_str().unwrap().to_string();
            let value = param_json["value"].as_str().unwrap().to_string();
            params.push((name, value));
        }
        Instr {
            id,
            kind,
            params: InstrSegments::new(params),
        }
    }
    fn to_json(&self) -> Value {
        let mut params_json = Vec::new();
        for (name, value) in &self.params {
            params_json.push(json!({
                "name": name,
                "value": value
            }));
        }
        json!({
            "id": self.id,
            "kind": self.kind,
            "params": params_json
        })
    }
}

#[derive(Debug, Clone)]
struct Op {
    kind: String,
    id: String,
    row: i64,
    col: i64,
    slot: i64,
    port: i64,
    body: Vec<Instr>,
}

impl Op {
    fn from_json(j: Value) -> Op {
        if j["kind"] != "rop" && j["kind"] != "cop" {
            panic!("Unknown operation kind: {}", j["kind"]);
        }

        let id = j["id"].as_str().unwrap().to_string();
        let kind = j["kind"].as_str().unwrap().to_string();
        let row = j["row"].as_i64().unwrap();
        let col = j["col"].as_i64().unwrap();
        let mut slot = -1;
        let mut port = -1;
        if kind == "rop" {
            slot = j["slot"].as_i64().unwrap();
            port = j["port"].as_i64().unwrap();
        }
        let mut body = Vec::new();
        for instr_json in j["body"].as_array().unwrap() {
            let instr = Instr::from_json(instr_json.clone());
            body.push(instr);
        }
        Op {
            kind,
            id,
            row,
            col,
            slot,
            port,
            body,
        }
    }

    fn to_json(&self) -> Value {
        let mut body_json = Vec::new();
        for instr in &self.body {
            body_json.push(instr.to_json());
        }
        if self.kind == "rop" {
            json!({
                "kind": self.kind,
                "id": self.id,
                "row": self.row,
                "col": self.col,
                "slot": self.slot,
                "port": self.port,
                "body": body_json
            })
        } else if self.kind == "cop" {
            json!({
                "kind": self.kind,
                "id": self.id,
                "row": self.row,
                "col": self.col,
                "body": body_json
            })
        } else {
            panic!("Unknown operation kind: {}", self.kind);
        }
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
    let input = std::fs::read_to_string(input_file).expect("Failed to read input file");
    let j: Value = serde_json::from_str(&input).expect("Failed to parse input file as JSON");
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

// agu_array_static — the AGU subsystem of a resource in a fabric without
// reconfiguration.
//
// agu_array's counterpart for resources whose AGU configurations are fixed
// when the fabric is generated rather than loaded by EVT / REP / TRANS
// instructions. Every AGU runs a sequence of configurations, one per
// activation, held in a ROM the resource generates from its static_config
// (lib/common/jinja/static_agu.j2):
//   * `config_index` tells the ROM which configuration each AGU runs next. It
//     starts at 0 and steps to the next one when a run finishes, the way
//     agu_controller clears an AGU's configuration on `done` for the next
//     one to be loaded,
//   * the ROM answers with that configuration on `agu_configs` and the
//     per-lane initial addresses on `init_address`.
//
// Everything after the configuration is agu_array's: one agu_rtr per used
// AGU, the init-address adder and the AGU_LATENCY output stages, with the same
// timing. The init-address path is always present, since a resource without
// reconfiguration also uses it to start an option-selecting AGU at the option
// the compiler allocated its run.
module agu_array_static #(
    parameter int NUM_AGUS          = 2,
    parameter int ADDRESS_WIDTH     = 16,
    parameter int NUMBER_IR         = 4,
    parameter int NUMBER_MT         = 3,
    parameter int NUMBER_OR         = 4,
    parameter int REP_DELAY_WIDTH   = 6,
    parameter int REP_ITER_WIDTH    = 6,
    parameter int REP_STEP_WIDTH    = 6,
    parameter int TRANS_DELAY_WIDTH = 12,

    // Per-AGU address width, as in agu_array.
    parameter int AGU_ADDR_WIDTH [NUM_AGUS] = '{default: ADDRESS_WIDTH},

    // Which AGUs are built, as in agu_array.
    parameter logic [NUM_AGUS-1:0] AGU_USED = '1,

    // Width of `config_index`: enough to count past the longest sequence, so
    // an AGU activated more often than it has configurations runs the cleared
    // one the ROM returns for an index it does not hold.
    parameter int CONFIG_INDEX_WIDTH = 1,

    // Cycles from an activation to the first address, as in agu_array.
    parameter int AGU_LATENCY = 1
) (
    input logic clk,
    input logic rst_n,

    // The configuration each AGU runs next, and the ROM's answer.
    output logic [NUM_AGUS-1:0][CONFIG_INDEX_WIDTH-1:0]   config_index,
    agu_cfg_if.consumer                                   agu_configs [NUM_AGUS],
    input  logic [NUM_AGUS-1:0][NUMBER_MT:0][ADDRESS_WIDTH-1:0] init_address,

    input  logic [NUM_AGUS-1:0]                    activation,
    output logic [NUM_AGUS-1:0]                    addr_valid,
    output logic [NUM_AGUS-1:0][ADDRESS_WIDTH-1:0] addr,
    output logic [NUM_AGUS-1:0]                    done
);

  initial begin
    if (AGU_LATENCY < 1) $error("agu_array_static: AGU_LATENCY must be at least 1");
  end

  localparam int LANE_W = (NUMBER_MT > 0) ? $clog2(NUMBER_MT + 1) : 1;
  localparam int OUTPUT_STAGES = AGU_LATENCY - 1;

  logic [NUM_AGUS-1:0][            LANE_W-1:0] active_lane;
  logic [NUM_AGUS-1:0][ADDRESS_WIDTH-1:0] raw_addr;
  logic [NUM_AGUS-1:0][ADDRESS_WIDTH-1:0] gen_addr;
  logic [NUM_AGUS-1:0]                    gen_valid;

  // Step to the next configuration when a run finishes. `done` may stay high
  // for more than one cycle at the end of a run, so its rising edge counts.
  logic [NUM_AGUS-1:0] done_q;
  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      done_q       <= '0;
      config_index <= '0;
    end else begin
      done_q <= done;
      for (int i = 0; i < NUM_AGUS; i++) begin
        if (done[i] && !done_q[i] && config_index[i] != '1) begin
          config_index[i] <= config_index[i] + 1'b1;
        end
      end
    end
  end

  genvar i;
  generate
    for (i = 0; i < NUM_AGUS; i++) begin : gen_agu
      localparam int LOCAL_ADDR_WIDTH = AGU_ADDR_WIDTH[i];
      logic [LOCAL_ADDR_WIDTH-1:0] local_addr;

      if (AGU_USED[i]) begin : gen_used
        agu_rtr #(
            .ADDRESS_WIDTH    (LOCAL_ADDR_WIDTH),
            .NUMBER_IR        (NUMBER_IR),
            .NUMBER_MT        (NUMBER_MT),
            .NUMBER_OR        (NUMBER_OR),
            .REP_DELAY_WIDTH  (REP_DELAY_WIDTH),
            .REP_ITER_WIDTH   (REP_ITER_WIDTH),
            .REP_STEP_WIDTH   (REP_STEP_WIDTH),
            .TRANS_DELAY_WIDTH(TRANS_DELAY_WIDTH)
        ) agu_inst (
            .clk        (clk),
            .rst_n      (rst_n),
            .enable     (1'b1),
            .activation (activation[i]),
            .cfg        (agu_configs[i]),
            .addr       (local_addr),
            .addr_valid (gen_valid[i]),
            .done       (done[i]),
            .active_lane(active_lane[i])
        );
      end else begin : gen_unused
        assign local_addr     = '0;
        assign gen_valid[i]   = 1'b0;
        assign done[i]        = 1'b0;
        assign active_lane[i] = '0;
      end

      assign raw_addr[i] = ADDRESS_WIDTH'(local_addr);
      // The base belongs to the lane the address was generated by. The ROM
      // holds it for the whole run: config_index only moves after `done`.
      assign gen_addr[i] = raw_addr[i] + init_address[i][active_lane[i]];
    end
  endgenerate

  // Output stages, after the init-address adder as in agu_array.
  generate
    if (OUTPUT_STAGES == 0) begin : gen_no_output_stages
      assign addr       = gen_addr;
      assign addr_valid = gen_valid;
    end else begin : gen_output_stages
      logic [OUTPUT_STAGES-1:0][NUM_AGUS-1:0][ADDRESS_WIDTH-1:0] addr_q;
      logic [OUTPUT_STAGES-1:0][NUM_AGUS-1:0]                    valid_q;

      always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
          addr_q  <= '0;
          valid_q <= '0;
        end else begin
          addr_q[0]  <= gen_addr;
          valid_q[0] <= gen_valid;
          for (int s = 1; s < OUTPUT_STAGES; s++) begin
            addr_q[s]  <= addr_q[s-1];
            valid_q[s] <= valid_q[s-1];
          end
        end
      end

      assign addr       = addr_q[OUTPUT_STAGES-1];
      assign addr_valid = valid_q[OUTPUT_STAGES-1];
    end
  endgenerate

endmodule

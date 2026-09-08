// agu_array — the complete AGU subsystem of a DRRA resource.
//
// Owns everything between a resource's instruction decoder and its datapath:
//   * one agu_controller, which picks the target AGU from the EVT port index
//     and folds the EVT / REP / TRANS fields into that AGU's configuration,
//   * the agu_cfg_if instances carrying those configurations,
//   * one agu_rtr per AGU, generating addresses once activated.
//
// A resource top supplies the three decoded instructions and the per-AGU
// activation strobes, and consumes `addr` / `addr_valid`. Everything else —
// the configuration interfaces, the done handshake back to the controller,
// the per-AGU address widths and the initial-address offset — stays in here.
//
// Instruction fields arrive as plain buses rather than the ISA-generated
// evt_t / rep_t / trans_t structs, so this module stays ISA-agnostic and is
// compiled once and shared. The resource's generated decoder unpacks the
// structs; the top passes the fields straight through.
module agu_array #(
    parameter int NUM_AGUS          = 2,
    parameter int ADDRESS_WIDTH     = 16,
    parameter int NUMBER_IR         = 4,
    parameter int NUMBER_MT         = 3,
    parameter int NUMBER_OR         = 4,
    parameter int REP_DELAY_WIDTH   = 6,
    parameter int REP_ITER_WIDTH    = 6,
    parameter int REP_STEP_WIDTH    = 6,
    parameter int TRANS_DELAY_WIDTH = 12,

    // Per-AGU address width. Resources whose AGUs address different things
    // (dpu: mode table vs accumulator reset) size the
    // individual counters here. `addr` is always ADDRESS_WIDTH wide and each
    // AGU's narrower output is zero-extended into it.
    parameter int AGU_ADDR_WIDTH [NUM_AGUS] = '{default: ADDRESS_WIDTH},

    // Add the EVT initial address to every generated address. Resources with
    // no init-address path (rf, dpu, swb) leave this 0 and tie evt_init_addr
    // off, which drops the adders.
    parameter bit USE_INIT_ADDR = 1'b0
) (
    input logic clk,
    input logic rst_n,

    // EVT — `evt_port` selects the AGU that this instruction, and the REP /
    // TRANS instructions following it, configure.
    input logic                        evt_valid,
    input logic [$clog2(NUM_AGUS)-1:0] evt_port,
    input logic [ADDRESS_WIDTH-1:0]    evt_init_addr,

    // REP — half-width fields; a base REP (ext=0) carries the lower half and a
    // REPX (ext=1) the upper half of the same entry.
    input logic                         rep_valid,
    input logic                         rep_ext,
    input logic [REP_DELAY_WIDTH/2-1:0] rep_delay,
    input logic [REP_ITER_WIDTH/2-1:0]  rep_iter,
    input logic [REP_STEP_WIDTH/2-1:0]  rep_step,

    // TRANS
    input logic                         trans_valid,
    input logic [TRANS_DELAY_WIDTH-1:0] trans_delay,

    input  logic [NUM_AGUS-1:0]                    activation,
    output logic [NUM_AGUS-1:0]                    addr_valid,
    output logic [NUM_AGUS-1:0][ADDRESS_WIDTH-1:0] addr,
    output logic [NUM_AGUS-1:0]                    done,

    // High per AGU once any of its mt/ir configs has been written. Optional —
    // resources that don't need it leave it unconnected.
    output logic [NUM_AGUS-1:0]                    is_configured
);

  agu_cfg_if #(
      .NUMBER_IR        (NUMBER_IR),
      .NUMBER_MT        (NUMBER_MT),
      .NUMBER_OR        (NUMBER_OR),
      .REP_DELAY_WIDTH  (REP_DELAY_WIDTH),
      .REP_ITER_WIDTH   (REP_ITER_WIDTH),
      .REP_STEP_WIDTH   (REP_STEP_WIDTH),
      .TRANS_DELAY_WIDTH(TRANS_DELAY_WIDTH)
  ) agu_configs [NUM_AGUS] ();

  localparam int LANE_W = (NUMBER_MT > 0) ? $clog2(NUMBER_MT + 1) : 1;

  logic [NUM_AGUS-1:0][NUMBER_MT:0][ADDRESS_WIDTH-1:0] init_address;
  logic [NUM_AGUS-1:0][            LANE_W-1:0]         active_lane;
  logic [NUM_AGUS-1:0][ADDRESS_WIDTH-1:0] raw_addr;

  agu_controller #(
      .ADDRESS_WIDTH    (ADDRESS_WIDTH),
      .NUM_AGUS         (NUM_AGUS),
      .NUMBER_IR        (NUMBER_IR),
      .NUMBER_MT        (NUMBER_MT),
      .NUMBER_OR        (NUMBER_OR),
      .REP_DELAY_WIDTH  (REP_DELAY_WIDTH),
      .REP_ITER_WIDTH   (REP_ITER_WIDTH),
      .REP_STEP_WIDTH   (REP_STEP_WIDTH),
      .TRANS_DELAY_WIDTH(TRANS_DELAY_WIDTH)
  ) controller_inst (
      .clk  (clk),
      .rst_n(rst_n),

      .evt_valid    (evt_valid),
      .evt_port     (evt_port),
      .evt_init_addr(evt_init_addr),

      .rep_valid(rep_valid),
      .rep_ext  (rep_ext),
      .rep_delay(rep_delay),
      .rep_iter (rep_iter),
      .rep_step (rep_step),

      .trans_valid(trans_valid),
      .trans_delay(trans_delay),

      .agu_done         (done),
      .agu_configs      (agu_configs),
      .init_address     (init_address),
      .agu_is_configured(is_configured)
  );

  genvar i;
  generate
    for (i = 0; i < NUM_AGUS; i++) begin : gen_agu
      localparam int LOCAL_ADDR_WIDTH = AGU_ADDR_WIDTH[i];
      logic [LOCAL_ADDR_WIDTH-1:0] local_addr;

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
          .clk       (clk),
          .rst_n     (rst_n),
          .enable    (1'b1),
          .activation(activation[i]),
          .cfg       (agu_configs[i]),
          .addr      (local_addr),
          .addr_valid(addr_valid[i]),
          .done      (done[i]),
          .active_lane(active_lane[i])
      );

      assign raw_addr[i] = ADDRESS_WIDTH'(local_addr);
      // The base belongs to the lane the address was generated by: each EVT
      // opens a lane and carries its own initial address.
      assign addr[i] = USE_INIT_ADDR
                         ? raw_addr[i] + init_address[i][active_lane[i]]
                         : raw_addr[i];
    end
  endgenerate

endmodule

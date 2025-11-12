module ir_generator #(
    parameter int ADDRESS_WIDTH,
    parameter int ITER_WIDTH,
    parameter int DELAY_WIDTH,
    parameter int STEP_WIDTH
) (
    input logic clk,
    input logic rst_n,

    input logic enable,

    input [ ITER_WIDTH-1:0] iter,
    input [DELAY_WIDTH-1:0] delay,
    input [ STEP_WIDTH-1:0] step,

    output logic [ADDRESS_WIDTH-1:0] ir_addr,
    output logic                     ir_valid,
    output logic                     ir_done
);

  // Delay counter
  logic [DELAY_WIDTH-1:0] delay_count;
  logic delay_done;
  assign delay_done = (delay_count >= delay);
  logic delay_configured;
  assign delay_configured = (delay > 0);
  up_counter #(
      .WIDTH(DELAY_WIDTH)
  ) delay_counter_inst (
      .clk   (clk),
      .rst_n (rst_n),
      .enable(enable && delay_configured && !delay_done),
      .init0 (delay_done),
      .count (delay_count)
  );

  // Iteration counter
  logic [ITER_WIDTH-1:0] iter_count;
  logic                  iter_done;
  assign iter_done = (iter_count >= iter - 1);
  up_counter #(
      .WIDTH(ITER_WIDTH)
  ) iter_counter_inst (
      .clk   (clk),
      .rst_n (rst_n),
      .enable(enable && delay_done && !iter_done),
      .init0 (enable && iter_done && delay_done),
      .count (iter_count)
  );
  assign ir_valid = enable && delay_done;
  assign ir_done  = enable && delay_done && iter_done;

  // Address generator
  logic [ADDRESS_WIDTH-1:0] addr_reg;
  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) addr_reg <= 0;
    else if (ir_done) addr_reg <= 0;  // reset address at end
    else if (!enable) addr_reg <= addr_reg;  // hold address when not enabled
    else if (delay_done) if (!iter_done) addr_reg <= addr_reg + step;
  end
  assign ir_addr = addr_reg;
endmodule

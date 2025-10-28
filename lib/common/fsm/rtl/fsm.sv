module fsm #(
    parameter FSM_MAX_STATES = 4,
    parameter FSM_DELAY_WIDTH = 4
) (
    input logic clk,
    input logic rst_n,
    input logic activate,
    input logic [FSM_MAX_STATES-2:0][FSM_DELAY_WIDTH-1:0] fsm_delays,
    input logic [$clog2(FSM_MAX_STATES)-1:0] max_init_state,
    input logic reset_fsm,
    output logic [$clog2(FSM_MAX_STATES)-1:0] state
);

  logic [$clog2(FSM_MAX_STATES)-1:0] next_state;
  logic [FSM_DELAY_WIDTH-1:0] delay_counter;
  logic [FSM_DELAY_WIDTH-1:0] delay_counter_next;
  logic activate_reg;

  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      state <= 0;
      activate_reg <= 0;
      delay_counter <= fsm_delays[0];
    end else if (activate && !activate_reg) begin
      activate_reg <= 1;
      delay_counter <= fsm_delays[0];
    end else begin
      state <= next_state;
      delay_counter <= delay_counter_next;
    end
  end

  always_comb begin
    // default
    next_state = state;
    delay_counter_next = delay_counter;

    if (reset_fsm)
      next_state = 0;

    if (activate_reg && (max_init_state > 0)) begin
      if (delay_counter == 0) begin
        next_state = state + 1;
        delay_counter_next = fsm_delays[state];
      end else if (state != max_init_state)
        delay_counter_next = delay_counter - 1;
    end
  end
endmodule


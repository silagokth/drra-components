module fsm #(
    parameter FSM_MAX_STATES = 8,
    parameter FSM_DELAY_WIDTH = 8
) (
    input logic clk,
    input logic rst_n,
    input logic activate,
    input logic [FSM_MAX_STATES-2:0][FSM_DELAY_WIDTH-1:0] fsm_delays,
    output logic [$clog2(FSM_MAX_STATES)-1:0] state
);

  logic [$clog2(FSM_MAX_STATES)-1:0] next_state;
  logic [FSM_DELAY_WIDTH-1:0] delay_counter;
  logic [FSM_DELAY_WIDTH-1:0] delay_counter_next;


  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      instruction_reg <= '0;
    end else begin
      if (instruction_valid) begin
        instruction_reg <= instruction;
      end
    end
  end

  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      delay_counter <= fsm_delays[0];
    end else begin
      delay_counter <= delay_counter_next;
    end
  end

  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      state <= 0;
    end else begin
      state <= next_state;
    end
  end

  always_comb begin
    next_state = 0;
    delay_counter_next = 0;
    if (delay_counter == 0) begin
      if (state == FSM_MAX_STATES - 1) begin
        next_state = 0;
      end else begin
        next_state = state + 1;
        delay_counter_next = fsm_delays[state];
      end
    end else begin
      delay_counter_next = delay_counter - 1;
    end
  end

endmodule


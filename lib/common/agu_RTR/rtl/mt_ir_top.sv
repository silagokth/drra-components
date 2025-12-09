module mt_ir_top
  import agu_RTR_pkg::*;
#(
    parameter int ADDRESS_WIDTH,
    parameter int NUMBER_IR,
    parameter int NUMBER_MT
) (
    input logic clk,
    input logic rst_n,
    input logic enable,

    // Configuration
    input trans_config_t [NUMBER_MT-1:0] mt_configs,
    input rep_config_t   [NUMBER_IR-1:0] rep_configs[NUMBER_MT+1],

    // Outputs
    output logic [ADDRESS_WIDTH-1:0] ir_addr,
    output logic                     ir_valid,
    output logic                     ir_done
);

  // State machine states
  typedef enum logic [1:0] {
    IDLE,
    RUN_LANE,
    TRANSITION_DELAY,
    DONE
  } mt_state_t;
  mt_state_t state, state_next;

  logic                           ir_valid_array             [NUMBER_MT+1];
  logic                           ir_done_array              [NUMBER_MT+1];
  logic [      ADDRESS_WIDTH-1:0] ir_addr_array              [NUMBER_MT+1];
  logic                           ir_enable_array            [NUMBER_MT+1];

  logic [$clog2(NUMBER_MT+1)-1:0] active_lane_ptr;
  logic [$clog2(NUMBER_MT+1)-1:0] active_lane_ptr_next;
  logic [                   31:0] transition_cnt;
  logic [                   31:0] transition_cnt_next;
  logic [$clog2(NUMBER_MT+1)-1:0] current_mux_ptr;
  logic [$clog2(NUMBER_MT+1)-1:0] max_lane_index;

  // Calculate number of transitions configured
  logic [$clog2(NUMBER_MT+1)-1:0] num_transitions_configured;
  always_comb begin
    num_transitions_configured = '0;
    for (int k = 0; k <= NUMBER_MT; k++) begin
      if (mt_configs[k].is_configured) begin
        num_transitions_configured = k + 1;
      end else begin
        break;
      end
    end
  end

  // Max Lane Index Calculation
  logic found_config;
  always_comb begin
    max_lane_index = '0;
    found_config   = 1'b0;
    for (int k = 0; k <= NUMBER_MT; k++) begin
      if (rep_configs[k][0].is_configured) begin
        max_lane_index = k;
        found_config   = 1'b1;
      end
    end
    if (!found_config) begin
      max_lane_index = num_transitions_configured;
    end
  end

  // Check if current step is done
  logic step_done;
  logic step_configured;
  always_comb begin
    if (active_lane_ptr > max_lane_index) begin
      step_configured = 1'b0;
    end else begin
      step_configured = rep_configs[active_lane_ptr][0].is_configured;
    end
  end
  assign step_done = step_configured ? ir_done_array[active_lane_ptr] : 1'b1;

  // Mux pointer logic
  always_comb begin
    if (state == IDLE && enable) begin
      current_mux_ptr = '0;
    end else begin
      current_mux_ptr = active_lane_ptr;
    end
  end

  // IR lane instantiation
  genvar i;
  generate
    for (i = 0; i <= NUMBER_MT; i++) begin : gen_ir_lanes
      ir_top #(
          .ADDRESS_WIDTH(ADDRESS_WIDTH),
          .NUMBER_IR(NUMBER_IR),
          .DELAY_WIDTH(REP_DELAY_WIDTH),
          .ITER_WIDTH(REP_ITER_WIDTH),
          .STEP_WIDTH(REP_STEP_WIDTH)
      ) ir_top_inst (
          .clk        (clk),
          .rst_n      (rst_n),
          .enable     (ir_enable_array[i]),
          .rep_configs(rep_configs[i]),
          .ir_addr    (ir_addr_array[i]),
          .ir_valid   (ir_valid_array[i]),
          .ir_done    (ir_done_array[i])
      );
    end
  endgenerate

  // Output muxing
  always_comb begin
    ir_valid = 1'b0;
    ir_addr  = '0;

    if (current_mux_ptr <= max_lane_index && rep_configs[current_mux_ptr][0].is_configured) begin
      ir_addr  = ir_addr_array[current_mux_ptr];
      ir_valid = ir_valid_array[current_mux_ptr];
    end else begin
      // Counter Mode
      ir_addr  = current_mux_ptr;
      ir_valid = 1'b1;
    end

    if (!((state == RUN_LANE) || (state == IDLE && enable))) begin
      ir_valid = 1'b0;
    end
  end

  assign ir_done = (state_next == DONE);

  // Lane enable logic
  always_comb begin
    for (int k = 0; k <= NUMBER_MT; k++) begin
      ir_enable_array[k] = 1'b0;
    end
    if ((state == RUN_LANE) || (state == IDLE && enable)) begin
      ir_enable_array[current_mux_ptr] = 1'b1;
    end
  end

  // State machine logic
  always_comb begin
    state_next           = state;
    active_lane_ptr_next = active_lane_ptr;
    transition_cnt_next  = transition_cnt;

    case (state)
      IDLE: begin
        active_lane_ptr_next = '0;
        transition_cnt_next  = '0;

        if (enable) begin
          // Check Lane 0 safely
          if (rep_configs[0][0].is_configured) begin
            state_next = RUN_LANE;
          end else begin
            // Counter Mode logic for Cycle 0
            if (max_lane_index == 0) begin
              state_next = DONE;
            end else if (mt_configs[0].delay > 0) begin
              state_next = TRANSITION_DELAY;
              transition_cnt_next = mt_configs[0].delay;
            end else begin
              active_lane_ptr_next = 1;
              state_next = RUN_LANE;
            end
          end
        end
      end

      RUN_LANE: begin
        if (step_done) begin
          if (active_lane_ptr >= max_lane_index) begin
            state_next = DONE;
          end else begin
            if (mt_configs[active_lane_ptr].delay > 0) begin
              state_next = TRANSITION_DELAY;
              transition_cnt_next = mt_configs[active_lane_ptr].delay;
            end else begin
              active_lane_ptr_next = active_lane_ptr + 1;
              state_next = RUN_LANE;
            end
          end
        end
      end

      TRANSITION_DELAY: begin
        if (transition_cnt <= 1) begin
          state_next           = RUN_LANE;
          active_lane_ptr_next = active_lane_ptr + 1;
          transition_cnt_next  = '0;
        end else begin
          transition_cnt_next = transition_cnt - 1'b1;
        end
      end

      DONE: begin
        if (!enable) begin
          state_next = IDLE;
        end
      end

      default: state_next = IDLE;
    endcase
  end

  // Sequential logic
  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      state           <= IDLE;
      active_lane_ptr <= '0;
      transition_cnt  <= '0;
    end else begin
      state           <= state_next;
      active_lane_ptr <= active_lane_ptr_next;
      transition_cnt  <= transition_cnt_next;
    end
  end

endmodule


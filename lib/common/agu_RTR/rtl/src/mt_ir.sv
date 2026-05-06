module mt_ir
  import agu_rtr_pkg::*;
#(
    parameter int ADDRESS_WIDTH,
    parameter int NUMBER_IR,
    parameter int NUMBER_MT,
    parameter int REP_DELAY_WIDTH,
    parameter int REP_ITER_WIDTH,
    parameter int REP_STEP_WIDTH,
    parameter int TRANS_DELAY_WIDTH
) (
    input  logic                                   clk,
    input  logic                                   rst_n,
    input  logic                                   enable,
           agu_cfg_if.consumer                     cfg,
    output logic               [ADDRESS_WIDTH-1:0] ir_addr,
    output logic                                   ir_valid,
    output logic                                   ir_done
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
  logic [  TRANS_DELAY_WIDTH-1:0] transition_cnt;
  logic [  TRANS_DELAY_WIDTH-1:0] transition_cnt_next;
  logic [$clog2(NUMBER_MT+1)-1:0] current_mux_ptr;
  logic [$clog2(NUMBER_MT+1)-1:0] max_lane_index;

  // Calculate number of transitions configured
  logic [$clog2(NUMBER_MT+1)-1:0] num_transitions_configured;
  always_comb begin
    num_transitions_configured = '0;
    for (int k = 0; k < NUMBER_MT; k++) begin
      if (cfg.mt_configs[k].is_configured) begin
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
      if (cfg.ir_configs[k][0].is_configured) begin
        max_lane_index = k;
        found_config   = 1'b1;
      end
    end
    if (!found_config) begin
      max_lane_index = num_transitions_configured;
    end
  end

  // Since empty lanes is always done, step is done when lane is done
  logic step_done;
  assign step_done = ir_done_array[active_lane_ptr];

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
      ir #(
          .ADDRESS_WIDTH(ADDRESS_WIDTH),
          .NUMBER_IR(NUMBER_IR),
          .DELAY_WIDTH(REP_DELAY_WIDTH),
          .ITER_WIDTH(REP_ITER_WIDTH),
          .STEP_WIDTH(REP_STEP_WIDTH),
          .LANE(i)
      ) ir_inst (
          .clk     (clk),
          .rst_n   (rst_n),
          .enable  (ir_enable_array[i]),
          .cfg     (cfg),
          .ir_addr (ir_addr_array[i]),
          .ir_valid(ir_valid_array[i]),
          .ir_done (ir_done_array[i])
      );
    end
  endgenerate

  // Output muxing
  always_comb begin
    ir_addr  = ir_addr_array[current_mux_ptr];
    ir_valid = ir_valid_array[current_mux_ptr];

    if (!((state == RUN_LANE) || (state == IDLE && enable))) begin
      ir_valid = 1'b0;
    end
  end

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
    ir_done              = 1'b0;

    case (state)
      IDLE: begin
        active_lane_ptr_next = '0;
        transition_cnt_next  = '0;

        if (enable) begin
          // If no transitions and lane 0 done, skip to done state
          if (max_lane_index == '0 && ir_done_array[0]) begin
            ir_done = 1'b1;
            state_next = DONE;
            // If lane 0 not done, run lane 0
          end else if (!ir_done_array[0]) begin
            state_next = RUN_LANE;
            // If lane 0 done, forward to next lane or transition delay
          end else if (cfg.mt_configs[0].delay > 0) begin
            state_next = TRANSITION_DELAY;
            transition_cnt_next = cfg.mt_configs[0].delay;
          end else begin
            active_lane_ptr_next = 1;
            state_next = RUN_LANE;
          end
        end
      end

      RUN_LANE: begin
        if (step_done) begin
          // If done with all lanes, go to done state
          if (active_lane_ptr >= max_lane_index) begin
            ir_done = 1'b1;
            state_next = IDLE;
            // If not done with all lanes, forward to next lane or transition delay
          end else if (cfg.mt_configs[active_lane_ptr].delay > 0) begin
            state_next = TRANSITION_DELAY;
            transition_cnt_next = cfg.mt_configs[active_lane_ptr].delay;
          end else begin
            active_lane_ptr_next = active_lane_ptr + 1;
            state_next = RUN_LANE;
          end
        end
      end

      TRANSITION_DELAY: begin
        if (transition_cnt <= 1) begin
          state_next           = RUN_LANE;
          active_lane_ptr_next = active_lane_ptr + 1;
          transition_cnt_next  = '0;

          // Check if all done. Widen the LHS by 1 bit before the +1 so
          // an `active_lane_ptr` already at its max value does not wrap
          // back to 0 and miss the comparison.
          if ({1'b0, active_lane_ptr} + 1'b1 >= max_lane_index) begin
            if (ir_done_array[max_lane_index]) begin
              state_next = IDLE;
              ir_done    = 1'b1;
            end
          end
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

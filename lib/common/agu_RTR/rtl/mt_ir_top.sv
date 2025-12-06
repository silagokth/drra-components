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
    // Note: We need NUMBER_MT+1 configs for the lanes
    input rep_config_t   [NUMBER_IR-1:0] rep_configs[NUMBER_MT+1],

    // Outputs
    output logic [ADDRESS_WIDTH-1:0] ir_addr,
    output logic                     ir_valid,
    output logic                     ir_done
);

  //-------------------------------------------------------------------------
  // Internal Signals & Types
  //-------------------------------------------------------------------------
  typedef enum logic [1:0] {
    IDLE,
    RUN_LANE,
    TRANSITION_DELAY,
    DONE
  } mt_state_t;

  mt_state_t state, state_next;

  // Arrays for IR Lane connectivity
  logic                           ir_valid_array       [NUMBER_MT+1];
  logic                           ir_done_array        [NUMBER_MT+1];

  logic [      ADDRESS_WIDTH-1:0] ir_addr_array        [NUMBER_MT+1];
  logic                           ir_enable_array      [NUMBER_MT+1];

  // Control Signals
  logic [$clog2(NUMBER_MT+1)-1:0] active_lane_ptr;
  logic [$clog2(NUMBER_MT+1)-1:0] active_lane_ptr_next;

  // Delay Counter for MT transitions
  logic [                   31:0] transition_cnt;
  logic [                   31:0] transition_cnt_next;

  // Signal to select which pointer to use for data path/enable in the current cycle
  logic [$clog2(NUMBER_MT+1)-1:0] current_mux_ptr;

  // Calculate the maximum configured lane index to know when to stop
  logic [$clog2(NUMBER_MT+1)-1:0] max_lane_index;

  always_comb begin
    // Default to 0 if nothing configured
    max_lane_index = '0;
    // Check which lanes are configured based on the rep_configs of that lane
    for (int k = 0; k <= NUMBER_MT; k++) begin
      if (rep_configs[k][0].is_configured) begin
        max_lane_index = k;
      end
    end
  end

  // Logic to determine if a zero-delay transition is occurring in this cycle
  logic immediate_advance;
  assign immediate_advance = (state == RUN_LANE) &&
                             ir_done_array[active_lane_ptr] &&
                             (mt_configs[active_lane_ptr].delay == 0) &&
                             (active_lane_ptr != max_lane_index); // Must not be the last lane

  // Determine the pointer to use for muxing and enabling in the CURRENT cycle.
  always_comb begin
    if (state == IDLE && enable) begin
      // Immediate start: Use Lane 0
      current_mux_ptr = '0;
      //end else if (immediate_advance) begin
      //  // Immediate transition: Use the next pointer combinatorially
      //  current_mux_ptr = active_lane_ptr + 1;
    end else begin
      // Default: Use the registered pointer (for steady RUN_LANE, DELAY, DONE)
      current_mux_ptr = active_lane_ptr;
    end
  end

  //-------------------------------------------------------------------------
  // 1. IR Lane Instantiation
  //-------------------------------------------------------------------------
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
          .enable     (ir_enable_array[i]),  // Controlled by FSM/Muxing
          .rep_configs(rep_configs[i]),
          .ir_addr    (ir_addr_array[i]),
          .ir_valid   (ir_valid_array[i]),
          .ir_done    (ir_done_array[i])
      );
    end
  endgenerate

  //-------------------------------------------------------------------------
  // 2. Output Muxing (Data Path)
  //-------------------------------------------------------------------------
  // Mux the address and valid signal based on the combinatorially determined active lane
  always_comb begin
    ir_addr  = ir_addr_array[current_mux_ptr];
    ir_valid = ir_valid_array[current_mux_ptr];

    // Output is valid if we are in RUN_LANE, OR if we are in IDLE 
    // and 'enable' is asserted (for the zero-latency initial output).
    if (!((state == RUN_LANE) || (state == IDLE && enable))) begin
      ir_valid = 1'b0;  // Mask output if we are not strictly in a running/starting state
    end
  end

  assign ir_done = (state_next == DONE);

  //-------------------------------------------------------------------------
  // 3. Lane Enable Logic (De-Mux)
  //-------------------------------------------------------------------------
  always_comb begin
    for (int k = 0; k <= NUMBER_MT; k++) begin
      ir_enable_array[k] = 1'b0;
    end

    // Only enable the lane determined by current_mux_ptr if we are in an active state
    if ((state == RUN_LANE) || (state == IDLE && enable)) begin
      ir_enable_array[current_mux_ptr] = 1'b1;
    end
  end

  //-------------------------------------------------------------------------
  // 4. State Machine & Control Logic
  //-------------------------------------------------------------------------
  always_comb begin
    // Defaults
    state_next           = state;
    active_lane_ptr_next = active_lane_ptr;
    transition_cnt_next  = transition_cnt;

    case (state)
      // ----------------------------------------------------------------
      IDLE: begin
        active_lane_ptr_next = '0;
        transition_cnt_next  = '0;
        if (enable) begin
          // Output is generated combinatorially via current_mux_ptr in this cycle.
          // State moves to RUN_LANE in the next cycle.
          state_next = RUN_LANE;
        end
      end

      // ----------------------------------------------------------------
      RUN_LANE: begin
        // Monitor the DONE signal of the current lane
        if (ir_done_array[active_lane_ptr]) begin

          // Check if this was the last configured lane
          if (active_lane_ptr == max_lane_index) begin
            state_next = DONE;
          end else begin
            // Prepare for transition to next lane
            if (mt_configs[active_lane_ptr].delay > 0) begin
              state_next = TRANSITION_DELAY;
              transition_cnt_next = mt_configs[active_lane_ptr].delay;
            end else begin
              // No delay: The current cycle's logic handles the immediate switch,
              // but the FSM still needs to register the new pointer for the next cycle.
              active_lane_ptr_next = active_lane_ptr + 1;
              state_next = RUN_LANE;
            end
          end
        end
      end

      // ----------------------------------------------------------------
      TRANSITION_DELAY: begin
        // Count down the delay
        if (transition_cnt <= 1) begin
          state_next           = RUN_LANE;
          active_lane_ptr_next = active_lane_ptr + 1;
          transition_cnt_next  = '0;
        end else begin
          transition_cnt_next = transition_cnt - 1'b1;
        end
      end

      // ----------------------------------------------------------------
      DONE: begin
        if (!enable) begin
          state_next = IDLE;
        end
      end

      default: state_next = IDLE;
    endcase
  end

  //-------------------------------------------------------------------------
  // 5. Sequential Logic
  //-------------------------------------------------------------------------
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

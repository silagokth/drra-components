// Combinational NCC cross-product comparator.
//
// Given two already-shifted candidate triples (s_a, s_a2, s_ab) — current and
// current-best — plus the per-template constant s_b and the constant K =
// 2^K_LOG2, the unit computes the discriminant
//
//     score = num^2 * denom_other,   num = K*S_AB - S_A*S_B,
//                                    denom = K*S_A2 - S_A*S_A
//
// for both sides and outputs `update_best` when the current candidate scores
// strictly higher than the current best, or when there is no valid best yet
// (and the current denominator is non-degenerate).
//
// All arithmetic is signed. The cross product is computed in full precision
// then saturated to PRODUCT_BITWIDTH before the final compare.

module compare_unit #(
    // Input widths are post-load, post-shift widths.
    parameter int LINEAR_BITWIDTH  = 24,
    parameter int QUAD_BITWIDTH    = 32,
    parameter int K_LOG2           = 14,
    parameter int PRODUCT_BITWIDTH = 144,

    // Numerator / denominator width: dominated by max(K*quad, linear*linear),
    // plus 2 bits of headroom for sign and subtraction.
    localparam int K_TERM_W = K_LOG2 + QUAD_BITWIDTH,
    localparam int SQ_TERM_W = 2 * LINEAR_BITWIDTH,
    localparam int ND_W     = ((K_TERM_W > SQ_TERM_W) ? K_TERM_W : SQ_TERM_W) + 2,
    localparam int SQ_W     = 2 * ND_W,
    localparam int FULL_W   = SQ_W + ND_W
) (
    input  logic signed [LINEAR_BITWIDTH-1:0] s_a_cur,
    input  logic signed [QUAD_BITWIDTH-1:0]   s_a2_cur,
    input  logic signed [QUAD_BITWIDTH-1:0]   s_ab_cur,
    input  logic signed [LINEAR_BITWIDTH-1:0] s_b,
    input  logic signed [LINEAR_BITWIDTH-1:0] s_a_best,
    input  logic signed [QUAD_BITWIDTH-1:0]   s_a2_best,
    input  logic signed [QUAD_BITWIDTH-1:0]   s_ab_best,
    input  logic                              best_valid,

    output logic update_best
);

  // ────── Numerator / Denominator ──────
  logic signed [ND_W-1:0] num_cur, num_best, denom_cur, denom_best;
  logic signed [ND_W-1:0] s_ab_cur_kshift, s_ab_best_kshift;
  logic signed [ND_W-1:0] s_a2_cur_kshift, s_a2_best_kshift;
  logic signed [ND_W-1:0] s_a_s_b_cur, s_a_s_b_best;
  logic signed [ND_W-1:0] s_a_sq_cur,  s_a_sq_best;

  assign s_ab_cur_kshift  = ND_W'($signed(s_ab_cur))   <<< K_LOG2;
  assign s_ab_best_kshift = ND_W'($signed(s_ab_best))  <<< K_LOG2;
  assign s_a2_cur_kshift  = ND_W'($signed(s_a2_cur))   <<< K_LOG2;
  assign s_a2_best_kshift = ND_W'($signed(s_a2_best))  <<< K_LOG2;

  assign s_a_s_b_cur  = ND_W'($signed(s_a_cur)  * $signed(s_b));
  assign s_a_s_b_best = ND_W'($signed(s_a_best) * $signed(s_b));
  assign s_a_sq_cur   = ND_W'($signed(s_a_cur)  * $signed(s_a_cur));
  assign s_a_sq_best  = ND_W'($signed(s_a_best) * $signed(s_a_best));

  assign num_cur    = s_ab_cur_kshift  - s_a_s_b_cur;
  assign num_best   = s_ab_best_kshift - s_a_s_b_best;
  assign denom_cur  = s_a2_cur_kshift  - s_a_sq_cur;
  assign denom_best = s_a2_best_kshift - s_a_sq_best;

  // ────── Squared numerator and full cross product ──────
  logic signed [SQ_W-1:0]   num_cur_sq, num_best_sq;
  logic signed [FULL_W-1:0] lhs_full, rhs_full;

  assign num_cur_sq  = $signed(num_cur)  * $signed(num_cur);
  assign num_best_sq = $signed(num_best) * $signed(num_best);
  assign lhs_full    = $signed(num_cur_sq)  * $signed(denom_best);
  assign rhs_full    = $signed(num_best_sq) * $signed(denom_cur);

  // ────── Saturate to PRODUCT_BITWIDTH ──────
  logic signed [PRODUCT_BITWIDTH-1:0] lhs_sat, rhs_sat;

  generate
    if (FULL_W > PRODUCT_BITWIDTH) begin : gen_saturate_products
      // The discarded high bits must all match the retained sign bit for the
      // value to fit without saturation.
      function automatic logic signed [PRODUCT_BITWIDTH-1:0] saturate(
          input logic signed [FULL_W-1:0] x);
        logic signed [PRODUCT_BITWIDTH-1:0] sat_min;
        logic signed [PRODUCT_BITWIDTH-1:0] sat_max;
        logic [FULL_W-PRODUCT_BITWIDTH-1:0] discarded;
        logic retained_sign;
        begin
          sat_min    = {1'b1, {(PRODUCT_BITWIDTH-1){1'b0}}};
          sat_max    = {1'b0, {(PRODUCT_BITWIDTH-1){1'b1}}};
          discarded  = x[FULL_W-1:PRODUCT_BITWIDTH];
          retained_sign = x[PRODUCT_BITWIDTH-1];

          if (discarded == {FULL_W-PRODUCT_BITWIDTH{retained_sign}})
            saturate = x[PRODUCT_BITWIDTH-1:0];
          else if (x[FULL_W-1])
            saturate = sat_min;
          else
            saturate = sat_max;
        end
      endfunction

      assign lhs_sat = saturate(lhs_full);
      assign rhs_sat = saturate(rhs_full);
    end else begin : gen_extend_products
      assign lhs_sat = PRODUCT_BITWIDTH'($signed(lhs_full));
      assign rhs_sat = PRODUCT_BITWIDTH'($signed(rhs_full));
    end
  endgenerate

  // Skip degenerate denominator.
  logic degenerate_cur;
  assign degenerate_cur = (denom_cur <= 0);

  assign update_best = !degenerate_cur && (!best_valid || (lhs_sat > rhs_sat));

endmodule

// kogge_stone_adder — a WIDTH-bit parallel-prefix adder.
//
// sum = a + b + cin, with the carry out of the top bit on cout. The carries are
// computed by a Kogge-Stone prefix tree: $clog2(WIDTH) levels of (G, P)
// combines, each level doubling the span, every bit at every level. That gives
// the minimum logic depth and fan-out of 2 at the cost of the most wiring and
// combine cells of the prefix adders.
//
// Plain binary addition, so it serves signed and unsigned operands alike. The
// caller does the rest:
//   - subtraction: a - b = a + ~b + 1, so pass ~b and cin = 1;
//   - signed overflow: the operands agree in sign and the sum does not;
//   - a wider result: sign- or zero-extend the operands by one bit first.
//
// Written for drra-components after P. M. Kogge and H. S. Stone, "A Parallel
// Algorithm for the Efficient Solution of a General Class of Recurrence
// Equations", IEEE Trans. Computers, C-22(8), 1973.
module kogge_stone_adder #(
    parameter int WIDTH = 16,

    localparam int STAGES = (WIDTH > 1) ? $clog2(WIDTH) : 0
) (
    input  logic [WIDTH-1:0] a,
    input  logic [WIDTH-1:0] b,
    input  logic             cin,
    output logic [WIDTH-1:0] sum,
    output logic             cout
);

  // g[s][i] / p[s][i]: generate and propagate of the group of bits ending at
  // bit i after level s, spanning 2**s bits (clipped at bit 0).
  logic [WIDTH-1:0]             p_bit;
  logic [STAGES:0][WIDTH-1:0]   g, p;

  // Level 0: per-bit generate and propagate. cin is folded into bit 0's
  // generate, so every group that reaches bit 0 already includes it and the
  // tree needs no separate carry-in path.
  always_comb begin
    p_bit   = a ^ b;
    g[0]    = a & b;
    g[0][0] = (a[0] & b[0]) | (p_bit[0] & cin);
    p[0]    = p_bit;
  end

  // Prefix levels. A bit whose span already reaches bit 0 is complete and
  // passes through unchanged.
  for (genvar s = 0; s < STAGES; s++) begin : gen_level
    localparam int D = 1 << s;
    for (genvar i = 0; i < WIDTH; i++) begin : gen_bit
      if (i >= D) begin : gen_combine
        assign g[s+1][i] = g[s][i] | (p[s][i] & g[s][i-D]);
        assign p[s+1][i] = p[s][i] & p[s][i-D];
      end else begin : gen_pass
        assign g[s+1][i] = g[s][i];
        assign p[s+1][i] = p[s][i];
      end
    end
  end

  // g[STAGES][i] is the carry out of bit i, i.e. the carry into bit i+1.
  always_comb begin
    sum[0] = p_bit[0] ^ cin;
    for (int i = 1; i < WIDTH; i++) begin
      sum[i] = p_bit[i] ^ g[STAGES][i-1];
    end
  end

  assign cout = g[STAGES][WIDTH-1];

endmodule

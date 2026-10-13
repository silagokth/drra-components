// booth_multiplier — a WIDTH x WIDTH signed multiplier, full 2*WIDTH-bit product.
//
// Three stages, all combinational:
//   1. Radix-4 Booth encoding. B is scanned in overlapping 3-bit groups
//      {b[2i+1], b[2i], b[2i-1]} (b[-1] = 0), each selecting one of
//      0, +-A, +-2A, so a WIDTH-bit B gives ceil(WIDTH/2) partial products
//      instead of WIDTH.
//   2. Carry-save reduction. The partial products are summed by layers of 3:2
//      compressors (a Wallace tree at row granularity): every layer turns each
//      group of three rows into a sum row and a carry row, with no carry
//      propagation, until two rows are left.
//   3. One carry-propagate add of those two rows, by kogge_stone_adder.
//
// Rows are kept at the full product width, each partial product sign-extended
// to it. That is simpler than the usual sign-extension-prevention constants and
// costs only cells the synthesis tool can trim; the arithmetic is exact
// modulo 2**(2*WIDTH), and a WIDTH x WIDTH signed product always fits.
//
// A negative partial product is formed as ~(k*A) shifted into place, and the
// +1 that completes the two's complement is collected, one bit per partial
// product, into one extra row. Partial product i sits at bit 2i, so those bits
// never collide.
//
// Written for drra-components. The structure follows the standard
// formulation; for background see:
//   - A. D. Booth, "A Signed Binary Multiplication Technique", Quarterly
//     Journal of Mechanics and Applied Mathematics, 4(2), 1951 (radix-2; the
//     radix-4 form is O. L. MacSorley, "High-Speed Arithmetic in Binary
//     Computers", Proc. IRE, 49(1), 1961);
//   - C. S. Wallace, "A Suggestion for a Fast Multiplier", IEEE Trans.
//     Electronic Computers, EC-13(1), 1964;
//   - github.com/bloominho/multiplier_booth (Andrew Park), a radix-4 Booth
//     multiplier with a Kogge-Stone final adder, used as a reference for the
//     overall approach. No code is taken from it.
module booth_multiplier #(
    parameter int WIDTH = 16,

    localparam int PRODUCT_WIDTH = 2 * WIDTH,
    localparam int NUM_PP        = (WIDTH + 1) / 2,
    // The Booth partial products plus the row of negation bits.
    localparam int NUM_ROWS      = NUM_PP + 1
) (
    input  logic signed [WIDTH-1:0]         a,
    input  logic signed [WIDTH-1:0]         b,
    output logic signed [PRODUCT_WIDTH-1:0] product
);

  // ─────────────────────────────────────────────────────────────────────────
  // Reduction tree geometry, evaluated at elaboration
  // ─────────────────────────────────────────────────────────────────────────
  // A layer compresses each full group of three rows into two and passes the
  // one or two rows left over straight through.
  function automatic int rows_after_layer(int rows);
    return 2 * (rows / 3) + rows % 3;
  endfunction

  function automatic int rows_at_layer(int layer);
    int rows = NUM_ROWS;
    for (int l = 0; l < layer; l++) rows = rows_after_layer(rows);
    return rows;
  endfunction

  function automatic int count_layers();
    int rows   = NUM_ROWS;
    int layers = 0;
    while (rows > 2) begin
      rows = rows_after_layer(rows);
      layers++;
    end
    return layers;
  endfunction

  localparam int NUM_LAYERS = count_layers();

  // ─────────────────────────────────────────────────────────────────────────
  // Booth encoding and partial products
  // ─────────────────────────────────────────────────────────────────────────
  // B sign-extended to an even 2*NUM_PP bits (only odd WIDTH needs it), with
  // the implicit b[-1] = 0 appended below.
  logic [2*NUM_PP:0]        b_ext;
  logic [PRODUCT_WIDTH-1:0] a_ext;
  assign b_ext = {(2 * NUM_PP)'(b), 1'b0};  // b is signed: sign-extends
  assign a_ext = PRODUCT_WIDTH'(a);  // a is signed, so this sign-extends

  // tree[l][r]: row r entering layer l. Layer 0 is the partial products.
  logic [NUM_LAYERS:0][NUM_ROWS-1:0][PRODUCT_WIDTH-1:0] tree;

  logic [PRODUCT_WIDTH-1:0] negate_row;

  for (genvar i = 0; i < NUM_PP; i++) begin : gen_pp
    logic [2:0]               group;
    logic                     one, two, negative;
    logic [PRODUCT_WIDTH-1:0] magnitude;

    assign group = b_ext[2*i +: 3];

    // 000, 111 -> 0      001, 010 -> +A     011 -> +2A
    // 100 -> -2A         101, 110 -> -A
    // 111 also sets `negative`: ~0 + 1 is 0 modulo the product width, so the
    // result is still zero.
    assign one      = group[1] ^ group[0];
    assign two      = (group == 3'b011) || (group == 3'b100);
    assign negative = group[2];

    assign magnitude = one ? a_ext : (two ? a_ext << 1 : '0);

    // -(k*A) * 4**i = (~(k*A) << 2i) + (1 << 2i); the shifted-in zeros below
    // bit 2i are correct because the +1 lands at bit 2i.
    assign tree[0][i]       = (negative ? ~magnitude : magnitude) << (2 * i);
    assign negate_row[2*i]  = negative;
    if (2 * i + 1 < PRODUCT_WIDTH) begin : gen_negate_gap
      assign negate_row[2*i+1] = 1'b0;
    end
  end

  if (2 * NUM_PP < PRODUCT_WIDTH) begin : gen_negate_top
    assign negate_row[PRODUCT_WIDTH-1:2*NUM_PP] = '0;
  end

  assign tree[0][NUM_PP] = negate_row;

  // ─────────────────────────────────────────────────────────────────────────
  // Carry-save reduction
  // ─────────────────────────────────────────────────────────────────────────
  for (genvar l = 0; l < NUM_LAYERS; l++) begin : gen_layer
    localparam int ROWS_IN  = rows_at_layer(l);
    localparam int ROWS_OUT = rows_after_layer(ROWS_IN);
    localparam int GROUPS   = ROWS_IN / 3;

    for (genvar g = 0; g < GROUPS; g++) begin : gen_csa
      logic [PRODUCT_WIDTH-1:0] x, y, z;
      assign x = tree[l][3*g];
      assign y = tree[l][3*g+1];
      assign z = tree[l][3*g+2];

      // 3:2 compressor on every bit: the sum stays in place, the carry moves
      // up one bit. The carry out of the top bit is dropped (modulo product).
      assign tree[l+1][2*g]   = x ^ y ^ z;
      assign tree[l+1][2*g+1] = ((x & y) | (x & z) | (y & z)) << 1;
    end

    for (genvar r = 0; r < ROWS_IN % 3; r++) begin : gen_pass
      assign tree[l+1][2*GROUPS+r] = tree[l][3*GROUPS+r];
    end

    for (genvar r = ROWS_OUT; r < NUM_ROWS; r++) begin : gen_unused
      assign tree[l+1][r] = '0;
    end
  end

  // ─────────────────────────────────────────────────────────────────────────
  // Final carry-propagate add
  // ─────────────────────────────────────────────────────────────────────────
  // The tree always ends on exactly two rows: even a single partial product
  // comes with the negation row, so NUM_ROWS is never below 2.
  kogge_stone_adder #(
      .WIDTH(PRODUCT_WIDTH)
  ) final_adder (
      .a   (tree[NUM_LAYERS][0]),
      .b   (tree[NUM_LAYERS][1]),
      .cin (1'b0),
      .sum (product),
      .cout()
  );

endmodule

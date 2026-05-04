module tree_adder #(
    parameter int N        = 8,
    parameter int IN_WIDTH = 16,
    parameter bit SIGNED   = 0,

    localparam int OUT_WIDTH = IN_WIDTH + $clog2(N),
    localparam int LEVELS    = $clog2(N)
) (
    input  logic [IN_WIDTH-1:0]  din [N],
    output logic [OUT_WIDTH-1:0] sum
);

    // Each level keeps up to N nodes; unused nodes are tied to zero.
    logic [OUT_WIDTH-1:0] stage [LEVELS:0][N-1:0];

    genvar i;
    genvar l;

    // Level 0: extend inputs to the output width.
    generate
        for (i = 0; i < N; i++) begin : gen_input_extend
            if (SIGNED) begin : gen_signed_extend
                assign stage[0][i] =
                    {{(OUT_WIDTH-IN_WIDTH){din[i][IN_WIDTH-1]}}, din[i]};
            end else begin : gen_unsigned_extend
                assign stage[0][i] =
                    {{(OUT_WIDTH-IN_WIDTH){1'b0}}, din[i]};
            end
        end
    endgenerate

    // Following levels: add nodes pairwise.
    generate
        for (l = 0; l < LEVELS; l++) begin : gen_levels
            localparam int CUR_NUM  = (N + (1 << l) - 1) >> l;
            localparam int NEXT_NUM = (CUR_NUM + 1) >> 1;

            for (i = 0; i < NEXT_NUM; i++) begin : gen_nodes
                if ((2*i + 1) < CUR_NUM) begin : gen_pair
                    assign stage[l+1][i] =
                        stage[l][2*i] + stage[l][2*i+1];
                end else begin : gen_single
                    assign stage[l+1][i] =
                        stage[l][2*i];
                end
            end

            for (i = NEXT_NUM; i < N; i++) begin : gen_unused
                assign stage[l+1][i] = '0;
            end
        end
    endgenerate

    assign sum = stage[LEVELS][0];

endmodule

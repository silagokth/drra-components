module signal_selector #(
    parameter NUM_SLOTS=16,
    parameter RESOURCE_SLOTS = 1,
    parameter FSM_PER_SLOT = 4
) (
    input logic [$clog2(NUM_SLOTS)-1:0] resource_address,
    input logic [NUM_SLOTS-1:0][FSM_PER_SLOT-1:0] activate_resource,
    input logic [NUM_SLOTS-1:0] instr_valid_resource,

    output logic instr_valid [RESOURCE_SLOTS-1:0],
    output logic [FSM_PER_SLOT-1:0] activate [RESOURCE_SLOTS-1:0],
    output logic [$clog2(NUM_SLOTS)-1:0] resource_address_out
);

    assign resource_address_out = resource_address + RESOURCE_SLOTS;

    genvar i;
    generate
        for(i=0;i<RESOURCE_SLOTS;i=i+1) begin
            assign instr_valid[i] = instr_valid_resource[resource_address + i];
            assign activate[i] = activate_resource[resource_address + i];
        end
    endgenerate

endmodule
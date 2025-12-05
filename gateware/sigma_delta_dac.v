// sigma_delta_dac.v
// Simple Sigma-Delta DAC for audio output
// Converts N-bit digital audio to 1-bit PWM output
// Connect output through low-pass RC filter (10k + 100nF) for analog audio

module sigma_delta_dac #(
    parameter BITS = 18
) (
    input wire clk,
    input wire rst_n,
    input wire [BITS-1:0] data_in,
    output reg audio_out
);

    // Accumulator needs extra bits for overflow handling
    reg [BITS+1:0] sigma_latch;
    wire [BITS+1:0] delta_b;
    wire [BITS+1:0] delta_adder;
    wire [BITS+1:0] sigma_adder;
    wire [BITS+1:0] dat_q;
    
    // Input with sign extension
    assign dat_q = {2'b00, data_in};
    
    // Delta feedback
    assign delta_b = {sigma_latch[BITS+1], sigma_latch[BITS+1], {BITS{1'b0}}};
    
    // Sigma-delta computation
    assign delta_adder = dat_q + delta_b;
    assign sigma_adder = delta_adder + sigma_latch;
    
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            sigma_latch <= {1'b1, {(BITS+1){1'b0}}};
            audio_out <= 1'b0;
        end else begin
            sigma_latch <= sigma_adder;
            audio_out <= sigma_latch[BITS+1];
        end
    end

endmodule

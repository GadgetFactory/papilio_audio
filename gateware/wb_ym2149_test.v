// wb_ym2149_test.v
// Simple Verilog test tone generator at YM2149 address
// Outputs tone when ANY write is received

module wb_ym2149_simple #(
    parameter CLK_FREQ_MHZ = 27
) (
    input  wire        wb_clk_i,
    input  wire        wb_rst_i,
    
    input  wire [4:0]  wb_adr_i,
    input  wire [7:0]  wb_dat_i,
    output wire [7:0]  wb_dat_o,
    input  wire        wb_cyc_i,
    input  wire        wb_stb_i,
    input  wire        wb_we_i,
    output wire        wb_ack_o,
    
    output wire [17:0] audio_data
);

    // Simple counter for ~1kHz tone at 27MHz (27000000/2/1000 = 13500)
    reg [13:0] tone_cnt;
    reg tone_out;
    
    // Flag: set on any write, never cleared
    reg got_write;
    
    // Wishbone acknowledge
    assign wb_ack_o = wb_cyc_i & wb_stb_i;
    
    // DEBUG: Return status in wb_dat_o
    // Bit 0: got_write flag
    // Bit 1: wb_cyc_i
    // Bit 2: wb_stb_i  
    // Bit 3: wb_we_i
    assign wb_dat_o = {4'b0000, wb_we_i, wb_stb_i, wb_cyc_i, got_write};
    
    // Set flag when wb_we_i goes high (write enable signal)
    always @(posedge wb_clk_i) begin
        if (wb_we_i) begin
            got_write <= 1'b1;
        end
    end
    
    // Initialize to 0 at power-up only
    initial begin
        got_write = 1'b0;
    end
    
    // Generate 1kHz square wave
    always @(posedge wb_clk_i) begin
        if (wb_rst_i) begin
            tone_cnt <= 14'd0;
            tone_out <= 1'b0;
        end else begin
            if (tone_cnt == 14'd0) begin
                tone_cnt <= 14'd13500;
                tone_out <= ~tone_out;
            end else begin
                tone_cnt <= tone_cnt - 1'b1;
            end
        end
    end
    
    // Output: tone UNCONDITIONALLY for testing
    assign audio_data = tone_out ? 18'h3FC00 : 18'h00000;

endmodule

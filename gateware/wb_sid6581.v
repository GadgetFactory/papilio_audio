// wb_sid6581.v
// Wishbone wrapper for SID 6581 sound chip
// Simple 8-bit Wishbone interface compatible with Papilio Arcade SPI bridge
//
// Register map (directly maps to SID registers):
//   0x00-0x06: Voice 1 (Freq Lo/Hi, PW Lo/Hi, Control, AD, SR)
//   0x07-0x0D: Voice 2 (Freq Lo/Hi, PW Lo/Hi, Control, AD, SR)
//   0x0E-0x14: Voice 3 (Freq Lo/Hi, PW Lo/Hi, Control, AD, SR)
//   0x15-0x18: Filter (FC Lo/Hi, Res/Filt, Mode/Vol)
//   0x19-0x1C: Misc (Paddle X/Y, Osc3, Env3)

module wb_sid6581 (
    input wire clk,
    input wire rst_n,
    
    // 1MHz clock for SID core
    input wire clk_1mhz,
    
    // Wishbone interface
    input wire [7:0] wb_adr_i,
    input wire [7:0] wb_dat_i,
    output wire [7:0] wb_dat_o,
    input wire wb_cyc_i,
    input wire wb_stb_i,
    input wire wb_we_i,
    output reg wb_ack_o,
    
    // Audio output
    output wire audio_out,          // PWM audio output
    output wire [17:0] audio_data   // Raw audio data for mixer
);

    // Wishbone bus handling
    wire wb_valid = wb_cyc_i & wb_stb_i;
    wire cs = wb_valid & ~wb_ack_o;
    
    // Active high reset for SID core
    wire rst = ~rst_n;
    
    // Generate ack
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            wb_ack_o <= 1'b0;
        end else begin
            wb_ack_o <= 1'b0;
            if (wb_valid && !wb_ack_o) begin
                wb_ack_o <= 1'b1;
            end
        end
    end

    // Instantiate SID6581 core
    // Note: 'do' is a reserved word in Verilog, use escaped identifier
    sid6581 u_sid (
        .clk_1MHz   (clk_1mhz),
        .clk32      (clk),
        .clk_DAC    (clk),          // Use main clock for DAC
        .reset      (rst),
        .cs         (cs),
        .we         (wb_we_i),
        .addr       (wb_adr_i[4:0]),
        .di         (wb_dat_i),
        .\do        (wb_dat_o),     // Escaped identifier for VHDL port 'do'
        .pot_x      (1'b0),         // No paddle input
        .pot_y      (1'b0),         // No paddle input
        .audio_out  (audio_out),
        .audio_data (audio_data)
    );

endmodule

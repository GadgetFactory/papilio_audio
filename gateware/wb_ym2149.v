// wb_ym2149.v
// Wishbone wrapper for YM2149 (AY-3-8910) sound chip
// Simple 8-bit Wishbone interface compatible with Papilio Arcade SPI bridge
//
// Register map (directly maps to YM2149 registers):
//   0x00: Channel A Fine Tune (8-bit)
//   0x01: Channel A Coarse Tune (4-bit)
//   0x02: Channel B Fine Tune (8-bit)
//   0x03: Channel B Coarse Tune (4-bit)
//   0x04: Channel C Fine Tune (8-bit)
//   0x05: Channel C Coarse Tune (4-bit)
//   0x06: Noise Period (5-bit)
//   0x07: Mixer Control - Enable (active low for tone/noise per channel)
//   0x08: Channel A Amplitude (5-bit, bit 4 = envelope mode)
//   0x09: Channel B Amplitude (5-bit, bit 4 = envelope mode)
//   0x0A: Channel C Amplitude (5-bit, bit 4 = envelope mode)
//   0x0B: Envelope Fine Tune (8-bit)
//   0x0C: Envelope Coarse Tune (8-bit)
//   0x0D: Envelope Shape/Cycle (4-bit)
//
// Based on YM2149 core by MikeJ (fpgaarcade.com)
// Adapted for simple Wishbone interface by GadgetFactory

module wb_ym2149 #(
    parameter CLK_FREQ_MHZ = 74  // Input clock frequency in MHz
) (
    input wire clk,
    input wire rst_n,
    
    // Wishbone interface
    input wire [7:0] wb_adr_i,
    input wire [7:0] wb_dat_i,
    output reg [7:0] wb_dat_o,
    input wire wb_cyc_i,
    input wire wb_stb_i,
    input wire wb_we_i,
    output reg wb_ack_o,
    
    // Audio output
    output wire [17:0] audio_data   // 18-bit audio data for mixer/DAC
);

    // Internal registers (14 registers, 8-bit each)
    reg [7:0] regs [0:15];
    
    // Wishbone bus handling
    wire wb_valid = wb_cyc_i & wb_stb_i;
    wire [3:0] reg_addr = wb_adr_i[3:0];  // Register address (0x0-0xF)
    
    // Clock divider for ~2MHz YM2149 clock from input clock
    // YM2149 needs ~2MHz, we divide by CLK_FREQ_MHZ/2
    localparam DIVIDER = (CLK_FREQ_MHZ / 2) - 1;
    reg [$clog2(DIVIDER+1)-1:0] clk_div_cnt;
    reg clk_2mhz_en;
    
    // Tone generator counters (12-bit each)
    reg [11:0] tone_cnt_a, tone_cnt_b, tone_cnt_c;
    reg tone_out_a, tone_out_b, tone_out_c;
    
    // Noise generator
    reg [4:0] noise_cnt;
    reg [16:0] noise_lfsr;
    reg noise_out;
    reg noise_toggle;
    
    // Envelope generator
    reg [15:0] env_cnt;
    reg [4:0] env_vol;
    reg env_hold, env_alt, env_attack, env_continue;
    reg env_holding;
    reg env_step_up;
    
    // Mixer outputs
    wire [7:0] mix_a, mix_b, mix_c;
    reg [9:0] audio_sum;
    
    // Volume table (attempt at log curve)
    function [7:0] vol_table;
        input [4:0] vol;
        begin
            case (vol)
                5'd0:  vol_table = 8'd0;
                5'd1:  vol_table = 8'd1;
                5'd2:  vol_table = 8'd2;
                5'd3:  vol_table = 8'd3;
                5'd4:  vol_table = 8'd4;
                5'd5:  vol_table = 8'd5;
                5'd6:  vol_table = 8'd7;
                5'd7:  vol_table = 8'd9;
                5'd8:  vol_table = 8'd11;
                5'd9:  vol_table = 8'd14;
                5'd10: vol_table = 8'd17;
                5'd11: vol_table = 8'd22;
                5'd12: vol_table = 8'd28;
                5'd13: vol_table = 8'd35;
                5'd14: vol_table = 8'd44;
                5'd15: vol_table = 8'd56;
                5'd16: vol_table = 8'd70;
                5'd17: vol_table = 8'd88;
                5'd18: vol_table = 8'd110;
                5'd19: vol_table = 8'd139;
                5'd20: vol_table = 8'd174;
                5'd21: vol_table = 8'd174;
                5'd22: vol_table = 8'd174;
                5'd23: vol_table = 8'd174;
                5'd24: vol_table = 8'd219;
                5'd25: vol_table = 8'd219;
                5'd26: vol_table = 8'd219;
                5'd27: vol_table = 8'd219;
                5'd28: vol_table = 8'd255;
                5'd29: vol_table = 8'd255;
                5'd30: vol_table = 8'd255;
                5'd31: vol_table = 8'd255;
            endcase
        end
    endfunction
    
    // Register addresses
    wire [11:0] tone_period_a = {regs[1][3:0], regs[0]};
    wire [11:0] tone_period_b = {regs[3][3:0], regs[2]};
    wire [11:0] tone_period_c = {regs[5][3:0], regs[4]};
    wire [4:0]  noise_period  = regs[6][4:0];
    wire [7:0]  mixer         = regs[7];
    wire [4:0]  amp_a         = regs[8][4:0];
    wire [4:0]  amp_b         = regs[9][4:0];
    wire [4:0]  amp_c         = regs[10][4:0];
    wire [15:0] env_period    = {regs[12], regs[11]};
    wire [3:0]  env_shape     = regs[13][3:0];
    
    // Mixer control bits (active LOW enables)
    wire tone_en_a  = ~mixer[0];
    wire tone_en_b  = ~mixer[1];
    wire tone_en_c  = ~mixer[2];
    wire noise_en_a = ~mixer[3];
    wire noise_en_b = ~mixer[4];
    wire noise_en_c = ~mixer[5];
    
    // Wishbone ACK generation
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
    
    // Register write
    integer i;
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            for (i = 0; i < 16; i = i + 1)
                regs[i] <= 8'h00;
            // Default mixer: all disabled (bits are active low)
            regs[7] <= 8'h3F;
        end else if (wb_valid && wb_we_i && !wb_ack_o) begin
            regs[reg_addr] <= wb_dat_i;
        end
    end
    
    // Register read
    always @(*) begin
        case (reg_addr)
            4'h0: wb_dat_o = regs[0];
            4'h1: wb_dat_o = {4'b0, regs[1][3:0]};
            4'h2: wb_dat_o = regs[2];
            4'h3: wb_dat_o = {4'b0, regs[3][3:0]};
            4'h4: wb_dat_o = regs[4];
            4'h5: wb_dat_o = {4'b0, regs[5][3:0]};
            4'h6: wb_dat_o = {3'b0, regs[6][4:0]};
            4'h7: wb_dat_o = regs[7];
            4'h8: wb_dat_o = {3'b0, regs[8][4:0]};
            4'h9: wb_dat_o = {3'b0, regs[9][4:0]};
            4'hA: wb_dat_o = {3'b0, regs[10][4:0]};
            4'hB: wb_dat_o = regs[11];
            4'hC: wb_dat_o = regs[12];
            4'hD: wb_dat_o = {4'b0, regs[13][3:0]};
            default: wb_dat_o = 8'h00;
        endcase
    end
    
    // Clock divider for 2MHz enable
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            clk_div_cnt <= 0;
            clk_2mhz_en <= 1'b0;
        end else begin
            clk_2mhz_en <= 1'b0;
            if (clk_div_cnt == 0) begin
                clk_div_cnt <= DIVIDER;
                clk_2mhz_en <= 1'b1;
            end else begin
                clk_div_cnt <= clk_div_cnt - 1;
            end
        end
    end
    
    // Tone generator A
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            tone_cnt_a <= 12'd0;
            tone_out_a <= 1'b0;
        end else if (clk_2mhz_en) begin
            if (tone_cnt_a == 12'd0) begin
                tone_cnt_a <= (tone_period_a == 12'd0) ? 12'd0 : tone_period_a - 1;
                tone_out_a <= ~tone_out_a;
            end else begin
                tone_cnt_a <= tone_cnt_a - 1;
            end
        end
    end
    
    // Tone generator B
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            tone_cnt_b <= 12'd0;
            tone_out_b <= 1'b0;
        end else if (clk_2mhz_en) begin
            if (tone_cnt_b == 12'd0) begin
                tone_cnt_b <= (tone_period_b == 12'd0) ? 12'd0 : tone_period_b - 1;
                tone_out_b <= ~tone_out_b;
            end else begin
                tone_cnt_b <= tone_cnt_b - 1;
            end
        end
    end
    
    // Tone generator C
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            tone_cnt_c <= 12'd0;
            tone_out_c <= 1'b0;
        end else if (clk_2mhz_en) begin
            if (tone_cnt_c == 12'd0) begin
                tone_cnt_c <= (tone_period_c == 12'd0) ? 12'd0 : tone_period_c - 1;
                tone_out_c <= ~tone_out_c;
            end else begin
                tone_cnt_c <= tone_cnt_c - 1;
            end
        end
    end
    
    // Noise generator (17-bit LFSR)
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            noise_cnt <= 5'd0;
            noise_lfsr <= 17'h1FFFF;
            noise_out <= 1'b0;
            noise_toggle <= 1'b0;
        end else if (clk_2mhz_en) begin
            noise_toggle <= ~noise_toggle;
            if (noise_toggle) begin
                if (noise_cnt == 5'd0) begin
                    noise_cnt <= (noise_period == 5'd0) ? 5'd0 : noise_period - 1;
                    // LFSR feedback: bits 0 and 3
                    noise_lfsr <= {noise_lfsr[0] ^ noise_lfsr[3], noise_lfsr[16:1]};
                    noise_out <= noise_lfsr[0];
                end else begin
                    noise_cnt <= noise_cnt - 1;
                end
            end
        end
    end
    
    // Envelope generator
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            env_cnt <= 16'd0;
            env_vol <= 5'd0;
            env_holding <= 1'b0;
            env_step_up <= 1'b0;
            env_hold <= 1'b0;
            env_alt <= 1'b0;
            env_attack <= 1'b0;
            env_continue <= 1'b0;
        end else begin
            // Detect envelope shape register write to reset envelope
            if (wb_valid && wb_we_i && !wb_ack_o && reg_addr == 4'hD) begin
                env_cnt <= 16'd0;
                env_holding <= 1'b0;
                // Decode shape: CONT ATT ALT HOLD
                env_continue <= wb_dat_i[3];
                env_attack   <= wb_dat_i[2];
                env_alt      <= wb_dat_i[1];
                env_hold     <= wb_dat_i[0];
                env_step_up  <= wb_dat_i[2];  // Start direction = attack bit
                if (wb_dat_i[2])
                    env_vol <= 5'd0;
                else
                    env_vol <= 5'd31;
            end else if (clk_2mhz_en && !env_holding) begin
                if (env_cnt == 16'd0) begin
                    env_cnt <= env_period;
                    // Step envelope
                    if (env_step_up) begin
                        if (env_vol == 5'd31) begin
                            if (env_continue) begin
                                if (env_alt)
                                    env_step_up <= 1'b0;
                                else
                                    env_vol <= 5'd0;
                                if (env_hold)
                                    env_holding <= 1'b1;
                            end else begin
                                env_vol <= 5'd0;
                                env_holding <= 1'b1;
                            end
                        end else begin
                            env_vol <= env_vol + 1;
                        end
                    end else begin
                        if (env_vol == 5'd0) begin
                            if (env_continue) begin
                                if (env_alt)
                                    env_step_up <= 1'b1;
                                else
                                    env_vol <= 5'd31;
                                if (env_hold)
                                    env_holding <= 1'b1;
                            end else begin
                                env_holding <= 1'b1;
                            end
                        end else begin
                            env_vol <= env_vol - 1;
                        end
                    end
                end else begin
                    env_cnt <= env_cnt - 1;
                end
            end
        end
    end
    
    // Channel mixing
    // Mixer bits are active LOW: 0=enabled, 1=disabled (output forced to 1)
    // When disabled, output is forced high (which means channel is "always on")
    // chan_X_on = (tone signal OR tone_disabled) AND (noise signal OR noise_disabled)
    wire chan_a_on = (tone_out_a | ~tone_en_a) & (noise_out | ~noise_en_a);
    wire chan_b_on = (tone_out_b | ~tone_en_b) & (noise_out | ~noise_en_b);
    wire chan_c_on = (tone_out_c | ~tone_en_c) & (noise_out | ~noise_en_c);
    
    // Volume selection (envelope or fixed)
    wire [4:0] vol_a = amp_a[4] ? env_vol : {1'b0, amp_a[3:0]};
    wire [4:0] vol_b = amp_b[4] ? env_vol : {1'b0, amp_b[3:0]};
    wire [4:0] vol_c = amp_c[4] ? env_vol : {1'b0, amp_c[3:0]};
    
    // DAC output per channel
    assign mix_a = chan_a_on ? vol_table(vol_a) : 8'd0;
    assign mix_b = chan_b_on ? vol_table(vol_b) : 8'd0;
    assign mix_c = chan_c_on ? vol_table(vol_c) : 8'd0;
    
    // Mix all channels (10-bit sum of three 8-bit values)
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            audio_sum <= 10'd0;
        end else begin
            audio_sum <= {2'b0, mix_a} + {2'b0, mix_b} + {2'b0, mix_c};
        end
    end
    
    // Output: scale 10-bit sum to 18-bit audio data (shift left 8 bits)
    assign audio_data = {audio_sum, 8'b0};

endmodule

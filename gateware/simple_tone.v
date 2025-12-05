// simple_tone.v
// Simple tone generator for audio output testing
// Generates a ~440Hz square wave (A4 note) from 27MHz clock

module simple_tone (
    input wire clk,           // 27MHz clock
    input wire rst_n,
    input wire enable,        // Enable tone output
    output reg audio_out      // PWM/square wave output
);

    // 27MHz / 440Hz / 2 = 30682 (divide by this for 440Hz square wave)
    // For a more audible 1kHz tone: 27MHz / 1000Hz / 2 = 13500
    localparam DIVISOR = 13500;  // ~1kHz tone
    
    reg [15:0] counter;
    
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            counter <= 16'd0;
            audio_out <= 1'b0;
        end else if (enable) begin
            if (counter >= DIVISOR - 1) begin
                counter <= 16'd0;
                audio_out <= ~audio_out;  // Toggle output
            end else begin
                counter <= counter + 1'b1;
            end
        end else begin
            audio_out <= 1'b0;
            counter <= 16'd0;
        end
    end

endmodule

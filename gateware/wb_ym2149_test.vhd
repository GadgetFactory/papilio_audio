-- wb_ym2149_test.vhd
-- Simple test tone generator at YM2149 address
-- Outputs tone when ANY write is received

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;

entity wb_ym2149_simple is
    generic (
        CLK_FREQ_MHZ : integer := 27
    );
    port (
        wb_clk_i    : in  std_logic;
        wb_rst_i    : in  std_logic;
        
        wb_adr_i    : in  std_logic_vector(4 downto 0);
        wb_dat_i    : in  std_logic_vector(7 downto 0);
        wb_dat_o    : out std_logic_vector(7 downto 0);
        wb_cyc_i    : in  std_logic;
        wb_stb_i    : in  std_logic;
        wb_we_i     : in  std_logic;
        wb_ack_o    : out std_logic;
        
        audio_data  : out std_logic_vector(17 downto 0)
    );
end entity wb_ym2149_simple;

architecture rtl of wb_ym2149_simple is
    -- Simple counter for ~1kHz tone at 27MHz
    signal tone_cnt : integer range 0 to 13500 := 0;
    signal tone_out : std_logic := '0';
    
    -- Flag: set on any write, never cleared
    signal got_write : std_logic := '0';
    
begin
    -- Wishbone acknowledge
    wb_ack_o <= wb_cyc_i and wb_stb_i;
    wb_dat_o <= x"00";
    
    -- Set flag on ANY write
    process(wb_clk_i)
    begin
        if rising_edge(wb_clk_i) then
            if wb_rst_i = '1' then
                got_write <= '0';
            elsif wb_cyc_i = '1' and wb_we_i = '1' and wb_stb_i = '1' then
                got_write <= '1';
            end if;
        end if;
    end process;
    
    -- Generate 1kHz square wave
    process(wb_clk_i)
    begin
        if rising_edge(wb_clk_i) then
            if wb_rst_i = '1' then
                tone_cnt <= 0;
                tone_out <= '0';
            else
                if tone_cnt = 0 then
                    tone_cnt <= 13500;
                    tone_out <= not tone_out;
                else
                    tone_cnt <= tone_cnt - 1;
                end if;
            end if;
        end if;
    end process;
    
    -- Output: tone only if we received a write
    audio_data <= x"FF" & "0000000000" when (tone_out = '1' and got_write = '1') else
                  (others => '0');

end architecture rtl;

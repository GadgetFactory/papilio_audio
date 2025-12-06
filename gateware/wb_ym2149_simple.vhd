-- wb_ym2149_simple.vhd
-- Simple 8-bit Wishbone wrapper for YM2149
-- Adapted from AUDIO_zpuino_wb_YM2149 for Papilio Arcade simple Wishbone interface
--
-- This wraps the original ZPUino YM2149 implementation but provides
-- a simple 8-bit Wishbone interface instead of the packed vector interface.

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;

entity wb_ym2149_simple is
    generic (
        CLK_FREQ_MHZ : integer := 27  -- Input clock frequency in MHz
    );
    port (
        wb_clk_i    : in  std_logic;
        wb_rst_i    : in  std_logic;
        
        -- Simple 8-bit Wishbone interface
        wb_adr_i    : in  std_logic_vector(4 downto 0);  -- 5-bit address (0x00-0x1F)
        wb_dat_i    : in  std_logic_vector(7 downto 0);
        wb_dat_o    : out std_logic_vector(7 downto 0);
        wb_cyc_i    : in  std_logic;
        wb_stb_i    : in  std_logic;
        wb_we_i     : in  std_logic;
        wb_ack_o    : out std_logic;
        
        -- Audio output
        audio_data  : out std_logic_vector(17 downto 0)
    );
end entity wb_ym2149_simple;

architecture rtl of wb_ym2149_simple is

    -- YM2149 registers
    type reg_array is array (0 to 15) of std_logic_vector(7 downto 0);
    signal regs : reg_array := (others => (others => '0'));
    
    -- Clock divider for ~2MHz from input clock
    constant PRE_CLOCK_DIVIDER : integer := (CLK_FREQ_MHZ / 2) - 1;
    signal predivcnt : integer range 0 to PRE_CLOCK_DIVIDER := PRE_CLOCK_DIVIDER;
    signal divclken : std_logic := '0';
    
    -- Tone/noise enable divider
    signal cnt_div : std_logic_vector(3 downto 0) := (others => '0');
    signal noise_div : std_logic := '0';
    signal ena_div : std_logic := '0';
    signal ena_div_noise : std_logic := '0';
    
    -- Noise generator
    signal poly17 : std_logic_vector(16 downto 0) := (others => '1');
    signal noise_gen_cnt : std_logic_vector(4 downto 0) := (others => '0');
    signal noise_gen_op : std_logic := '0';
    
    -- Tone generators (12-bit counters)
    type tone_cnt_array is array (1 to 3) of std_logic_vector(11 downto 0);
    signal tone_gen_cnt : tone_cnt_array := (others => (others => '0'));
    signal tone_gen_op : std_logic_vector(3 downto 1) := "000";
    
    -- Envelope generator
    signal env_gen_cnt : std_logic_vector(15 downto 0) := (others => '0');
    signal env_reset : std_logic := '0';
    signal env_ena : std_logic := '0';
    signal env_hold : std_logic := '0';
    signal env_inc : std_logic := '0';
    signal env_vol : std_logic_vector(4 downto 0) := (others => '0');
    
    -- Mixer signals
    signal tone_ena_l : std_logic := '1';
    signal tone_src : std_logic := '0';
    signal noise_ena_l : std_logic := '1';
    signal chan_vol : std_logic_vector(4 downto 0) := (others => '0');
    
    -- Output mixing
    signal dac_amp : std_logic_vector(7 downto 0) := (others => '0');
    signal audio_mix : std_logic_vector(9 downto 0) := (others => '0');
    signal audio_final : std_logic_vector(9 downto 0) := (others => '0');
    signal O_AUDIO : std_logic_vector(7 downto 0) := (others => '0');

begin

    -- Wishbone acknowledge
    wb_ack_o <= wb_cyc_i and wb_stb_i;
    
    -- Audio output: 8-bit shifted to 18-bit
    audio_data <= O_AUDIO & "0000000000";
    
    -- Register write process
    p_wdata: process(wb_clk_i)
    begin
        if rising_edge(wb_clk_i) then
            env_reset <= '0';
            if wb_rst_i = '1' then
                regs <= (others => (others => '0'));
                regs(7) <= x"3F";  -- All disabled by default
            elsif wb_we_i = '1' then
                regs(conv_integer(wb_adr_i(3 downto 0))) <= wb_dat_i;
                -- Envelope reset on register 0xD write
                if wb_adr_i(3 downto 0) = x"D" then
                    env_reset <= '1';
                end if;
            end if;
        end if;
    end process;
    
    -- Register read process
    p_rdata: process(wb_adr_i, regs)
    begin
        case wb_adr_i(3 downto 0) is
            when x"0" => wb_dat_o <= regs(0);
            when x"1" => wb_dat_o <= "0000" & regs(1)(3 downto 0);
            when x"2" => wb_dat_o <= regs(2);
            when x"3" => wb_dat_o <= "0000" & regs(3)(3 downto 0);
            when x"4" => wb_dat_o <= regs(4);
            when x"5" => wb_dat_o <= "0000" & regs(5)(3 downto 0);
            when x"6" => wb_dat_o <= "000" & regs(6)(4 downto 0);
            when x"7" => wb_dat_o <= regs(7);
            when x"8" => wb_dat_o <= "000" & regs(8)(4 downto 0);
            when x"9" => wb_dat_o <= "000" & regs(9)(4 downto 0);
            when x"A" => wb_dat_o <= "000" & regs(10)(4 downto 0);
            when x"B" => wb_dat_o <= regs(11);
            when x"C" => wb_dat_o <= regs(12);
            when x"D" => wb_dat_o <= "0000" & regs(13)(3 downto 0);
            when others => wb_dat_o <= x"00";
        end case;
    end process;
    
    -- Clock pre-divider for ~2MHz
    predivider: process(wb_clk_i)
    begin
        if rising_edge(wb_clk_i) then
            if wb_rst_i = '1' then
                divclken <= '0';
                predivcnt <= PRE_CLOCK_DIVIDER;
            else
                divclken <= '0';
                if predivcnt = 0 then
                    divclken <= '1';
                    predivcnt <= PRE_CLOCK_DIVIDER;
                else
                    predivcnt <= predivcnt - 1;
                end if;
            end if;
        end if;
    end process;
    
    -- Main divider (divide by 8 or 16)
    p_divider: process(wb_clk_i)
    begin
        if rising_edge(wb_clk_i) then
            ena_div <= '0';
            ena_div_noise <= '0';
            if divclken = '1' then
                if cnt_div = "0000" then
                    cnt_div <= "0111";  -- Divide by 8
                    ena_div <= '1';
                    noise_div <= not noise_div;
                    if noise_div = '1' then
                        ena_div_noise <= '1';
                    end if;
                else
                    cnt_div <= cnt_div - "1";
                end if;
            end if;
        end if;
    end process;
    
    -- Noise generator
    p_noise_gen: process(wb_clk_i)
        variable noise_gen_comp : std_logic_vector(4 downto 0);
        variable poly17_zero : std_logic;
    begin
        if rising_edge(wb_clk_i) then
            if regs(6)(4 downto 0) = "00000" then
                noise_gen_comp := "00000";
            else
                noise_gen_comp := regs(6)(4 downto 0) - "1";
            end if;
            
            poly17_zero := '0';
            if poly17 = "00000000000000000" then
                poly17_zero := '1';
            end if;
            
            if ena_div_noise = '1' then
                if noise_gen_cnt >= noise_gen_comp then
                    noise_gen_cnt <= "00000";
                    poly17 <= (poly17(0) xor poly17(2) xor poly17_zero) & poly17(16 downto 1);
                else
                    noise_gen_cnt <= noise_gen_cnt + "1";
                end if;
            end if;
        end if;
    end process;
    
    noise_gen_op <= poly17(0);
    
    -- Tone generators
    p_tone_gens: process(wb_clk_i)
        variable tone_gen_freq : tone_cnt_array;
        variable tone_gen_comp : tone_cnt_array;
    begin
        if rising_edge(wb_clk_i) then
            tone_gen_freq(1) := regs(1)(3 downto 0) & regs(0);
            tone_gen_freq(2) := regs(3)(3 downto 0) & regs(2);
            tone_gen_freq(3) := regs(5)(3 downto 0) & regs(4);
            
            for i in 1 to 3 loop
                if tone_gen_freq(i) = x"000" then
                    tone_gen_comp(i) := x"000";
                else
                    tone_gen_comp(i) := tone_gen_freq(i) - "1";
                end if;
            end loop;
            
            if ena_div = '1' then
                for i in 1 to 3 loop
                    if tone_gen_cnt(i) >= tone_gen_comp(i) then
                        tone_gen_cnt(i) <= x"000";
                        tone_gen_op(i) <= not tone_gen_op(i);
                    else
                        tone_gen_cnt(i) <= tone_gen_cnt(i) + "1";
                    end if;
                end loop;
            end if;
        end if;
    end process;
    
    -- Envelope frequency
    p_envelope_freq: process(wb_clk_i)
        variable env_gen_freq : std_logic_vector(15 downto 0);
        variable env_gen_comp : std_logic_vector(15 downto 0);
    begin
        if rising_edge(wb_clk_i) then
            env_gen_freq := regs(12) & regs(11);
            if env_gen_freq = x"0000" then
                env_gen_comp := x"0000";
            else
                env_gen_comp := env_gen_freq - "1";
            end if;
            
            env_ena <= '0';
            if ena_div = '1' then
                if env_gen_cnt >= env_gen_comp then
                    env_gen_cnt <= x"0000";
                    env_ena <= '1';
                else
                    env_gen_cnt <= env_gen_cnt + "1";
                end if;
            end if;
        end if;
    end process;
    
    -- Envelope shape
    p_envelope_shape: process(wb_clk_i)
        variable is_bot : boolean;
        variable is_bot_p1 : boolean;
        variable is_top_m1 : boolean;
        variable is_top : boolean;
    begin
        if rising_edge(wb_clk_i) then
            if env_reset = '1' then
                if regs(13)(2) = '0' then
                    env_vol <= "11111";
                    env_inc <= '0';
                else
                    env_vol <= "00000";
                    env_inc <= '1';
                end if;
                env_hold <= '0';
            else
                is_bot := (env_vol = "00000");
                is_bot_p1 := (env_vol = "00001");
                is_top_m1 := (env_vol = "11110");
                is_top := (env_vol = "11111");
                
                if env_ena = '1' then
                    if env_hold = '0' then
                        if env_inc = '1' then
                            env_vol <= env_vol + "00001";
                        else
                            env_vol <= env_vol + "11111";
                        end if;
                    end if;
                    
                    if regs(13)(3) = '0' then
                        if env_inc = '0' then
                            if is_bot_p1 then env_hold <= '1'; end if;
                        else
                            if is_top then env_hold <= '1'; end if;
                        end if;
                    else
                        if regs(13)(0) = '1' then
                            if env_inc = '0' then
                                if regs(13)(1) = '1' then
                                    if is_bot then env_hold <= '1'; end if;
                                else
                                    if is_bot_p1 then env_hold <= '1'; end if;
                                end if;
                            else
                                if regs(13)(1) = '1' then
                                    if is_top then env_hold <= '1'; end if;
                                else
                                    if is_top_m1 then env_hold <= '1'; end if;
                                end if;
                            end if;
                        elsif regs(13)(1) = '1' then
                            if env_inc = '0' then
                                if is_bot_p1 then env_hold <= '1'; end if;
                                if is_bot then env_hold <= '0'; env_inc <= '1'; end if;
                            else
                                if is_top_m1 then env_hold <= '1'; end if;
                                if is_top then env_hold <= '0'; env_inc <= '0'; end if;
                            end if;
                        end if;
                    end if;
                end if;
            end if;
        end if;
    end process;
    
    -- Channel mixer selection
    p_chan_mixer: process(cnt_div, regs, tone_gen_op)
    begin
        tone_ena_l <= '1';
        tone_src <= '1';
        noise_ena_l <= '1';
        chan_vol <= "00000";
        
        case cnt_div(1 downto 0) is
            when "00" =>
                tone_ena_l <= regs(7)(0);
                tone_src <= tone_gen_op(1);
                chan_vol <= regs(8)(4 downto 0);
                noise_ena_l <= regs(7)(3);
            when "01" =>
                tone_ena_l <= regs(7)(1);
                tone_src <= tone_gen_op(2);
                chan_vol <= regs(9)(4 downto 0);
                noise_ena_l <= regs(7)(4);
            when "10" =>
                tone_ena_l <= regs(7)(2);
                tone_src <= tone_gen_op(3);
                chan_vol <= regs(10)(4 downto 0);
                noise_ena_l <= regs(7)(5);
            when "11" =>
                null;
            when others =>
                null;
        end case;
    end process;
    
    -- Output mixer
    p_op_mixer: process(wb_clk_i)
        variable chan_mixed : std_logic;
        variable chan_amp : std_logic_vector(4 downto 0);
    begin
        if rising_edge(wb_clk_i) then
            if divclken = '1' then
                chan_mixed := (tone_ena_l or tone_src) and (noise_ena_l or noise_gen_op);
                chan_amp := (others => '0');
                
                if chan_mixed = '1' then
                    if chan_vol(4) = '0' then
                        if chan_vol(3 downto 0) = "0000" then
                            chan_amp := "00000";
                        else
                            chan_amp := chan_vol(3 downto 0) & '1';
                        end if;
                    else
                        chan_amp := env_vol(4 downto 0);
                    end if;
                end if;
                
                -- DAC amplitude lookup
                dac_amp <= x"00";
                case chan_amp is
                    when "11111" => dac_amp <= x"FF";
                    when "11110" => dac_amp <= x"D9";
                    when "11101" => dac_amp <= x"BA";
                    when "11100" => dac_amp <= x"9F";
                    when "11011" => dac_amp <= x"88";
                    when "11010" => dac_amp <= x"74";
                    when "11001" => dac_amp <= x"63";
                    when "11000" => dac_amp <= x"54";
                    when "10111" => dac_amp <= x"48";
                    when "10110" => dac_amp <= x"3D";
                    when "10101" => dac_amp <= x"34";
                    when "10100" => dac_amp <= x"2C";
                    when "10011" => dac_amp <= x"25";
                    when "10010" => dac_amp <= x"1F";
                    when "10001" => dac_amp <= x"1A";
                    when "10000" => dac_amp <= x"16";
                    when "01111" => dac_amp <= x"13";
                    when "01110" => dac_amp <= x"10";
                    when "01101" => dac_amp <= x"0D";
                    when "01100" => dac_amp <= x"0B";
                    when "01011" => dac_amp <= x"09";
                    when "01010" => dac_amp <= x"08";
                    when "01001" => dac_amp <= x"07";
                    when "01000" => dac_amp <= x"06";
                    when "00111" => dac_amp <= x"05";
                    when "00110" => dac_amp <= x"04";
                    when "00101" => dac_amp <= x"03";
                    when "00100" => dac_amp <= x"03";
                    when "00011" => dac_amp <= x"02";
                    when "00010" => dac_amp <= x"02";
                    when "00001" => dac_amp <= x"01";
                    when "00000" => dac_amp <= x"00";
                    when others => null;
                end case;
                
                -- Mix channels (accumulate over 3 clock cycles)
                if cnt_div(1 downto 0) = "10" then
                    audio_mix <= (others => '0');
                    audio_final <= audio_mix;
                else
                    audio_mix <= audio_mix + ("00" & dac_amp);
                end if;
            end if;
            
            -- Output with clipping
            if wb_rst_i = '1' then
                O_AUDIO <= (others => '0');
            elsif divclken = '1' then
                if audio_final(9) = '0' then
                    O_AUDIO <= audio_final(8 downto 1);
                else
                    O_AUDIO <= x"FF";
                end if;
            end if;
        end if;
    end process;

end architecture rtl;

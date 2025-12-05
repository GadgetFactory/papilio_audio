--
-- Simple Wishbone wrapper around SID6581
-- Based on ZPUino wrapper but with standard (non-packed) Wishbone interface
-- 
-- Address mapping: 5-bit SID address from wb_adr_i(4 downto 0)
-- Data: 8-bit, directly mapped
--

library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.std_logic_unsigned.all;
use IEEE.numeric_std.all;

entity wb_sid6581_simple is
  port (
    -- Wishbone interface (active high reset)
    wb_clk_i    : in  std_logic;
    wb_rst_i    : in  std_logic;
    wb_adr_i    : in  std_logic_vector(4 downto 0);
    wb_dat_i    : in  std_logic_vector(7 downto 0);
    wb_dat_o    : out std_logic_vector(7 downto 0);
    wb_we_i     : in  std_logic;
    wb_cyc_i    : in  std_logic;
    wb_stb_i    : in  std_logic;
    wb_ack_o    : out std_logic;

    -- 1MHz clock for SID core
    clk_1mhz    : in  std_logic;
    
    -- Audio output (directly from SID)
    audio_data  : out std_logic_vector(17 downto 0)
  );
end entity wb_sid6581_simple;

architecture rtl of wb_sid6581_simple is

  component sid6581 is
  port (
    clk_1MHz    : in std_logic;
    clk32       : in std_logic;
    clk_DAC     : in std_logic;
    reset       : in std_logic;
    cs          : in std_logic;
    we          : in std_logic;
    addr        : in std_logic_vector(4 downto 0);
    di          : in std_logic_vector(7 downto 0);
    do          : out std_logic_vector(7 downto 0);
    pot_x       : in std_logic;
    pot_y       : in std_logic;
    audio_out   : out std_logic;
    audio_data  : out std_logic_vector(17 downto 0)
  );
  end component;

  signal cs     : std_logic;
  signal ack_i  : std_logic;
  signal do_sig : std_logic_vector(7 downto 0);

begin

  -- Generate chip select - active when strobe and cycle, but not during ack
  cs <= (wb_stb_i and wb_cyc_i) and not ack_i;
  
  -- Data out
  wb_dat_o <= do_sig;
  wb_ack_o <= ack_i;

  -- Ack generation process
  process(wb_clk_i)
  begin
    if rising_edge(wb_clk_i) then
      if wb_rst_i = '1' then
        ack_i <= '0';
      else
        ack_i <= '0';
        if ack_i = '0' then
          if wb_stb_i = '1' and wb_cyc_i = '1' then
            ack_i <= '1';
          end if;
        end if;
      end if;
    end if;
  end process;

  -- SID6581 instance
  sid: sid6581
  port map (
    clk_1MHz    => clk_1mhz,
    clk32       => wb_clk_i,
    clk_DAC     => '0',           -- Disable internal DAC
    reset       => wb_rst_i,
    cs          => cs,
    we          => wb_we_i,
    addr        => wb_adr_i,
    di          => wb_dat_i,
    do          => do_sig,
    pot_x       => '0',
    pot_y       => '0',
    audio_out   => open,          -- Don't use internal DAC output
    audio_data  => audio_data     -- Use raw audio data
  );

end rtl;

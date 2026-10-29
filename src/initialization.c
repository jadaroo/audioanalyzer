#include "initialization.h"

#include <stdint.h>
#include <hardware/dma.h>
#include <hardware/adc.h>

void init_dma_with_chaining(uint *ch1, uint *ch2, uint16_t *first_array, uint16_t *second_array, uint16_t buffer_size){
  dma_channel_claim(*ch1);
  dma_channel_claim(*ch2);
  
  dma_channel_config first_chan_config = dma_channel_get_default_config(*ch1);
  channel_config_set_transfer_data_size(&first_chan_config, DMA_SIZE_16);
  channel_config_set_read_increment(&first_chan_config, false);
  channel_config_set_write_increment(&first_chan_config, true);
  channel_config_set_chain_to(&first_chan_config, *ch2);
  
  dma_channel_config second_chan_config = dma_channel_get_default_config(*ch2);
  channel_config_set_transfer_data_size(&second_chan_config, DMA_SIZE_16);
  channel_config_set_read_increment(&second_chan_config, false);
  channel_config_set_write_increment(&second_chan_config, true);
  channel_config_set_chain_to(&second_chan_config, *ch1);

  channel_config_set_dreq(&first_chan_config, DREQ_ADC);
  channel_config_set_dreq(&second_chan_config, DREQ_ADC);
  
  dma_channel_set_irq0_enabled(*ch1, true);
  dma_channel_set_irq0_enabled(*ch2, true);

  dma_channel_configure(
    *ch1,
    &first_chan_config,
    first_array,
    &adc_hw->fifo,
    buffer_size,
    false
  );

  dma_channel_configure(
    *ch2,
    &second_chan_config,
    second_array,
    &adc_hw->fifo,
    buffer_size,
    false
  );
}

void init_adc_with_fifo(uint8_t input, uint16_t clkdiv){
  adc_init();
  adc_set_temp_sensor_enabled(true);
  adc_select_input(input);

  adc_fifo_setup(
    true,
    true,
    1,
    false,
    false
  );

  adc_set_clkdiv(clkdiv);
}
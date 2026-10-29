#include <hardware/adc.h>
#include <hardware/dma.h>
#include <hardware/irq.h>
#include <hardware/regs/dreq.h>
#include <hardware/regs/intctrl.h>
#include <hardware/structs/dma.h>
#include <math.h>
#include <pico/stdio.h>
#include <pico/stdlib.h>
#include <pico/time.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define BUFFER_SIZE 256
#define FIRST_CHANNEL 0
#define SECOND_CHANNEL 1
#define CLKDIV 999
#define TEMP_SENSOR_ADC 4
#define SLEEP_BEFORE_START_MS 3000
#define DISPLAY_LENGTH 128

float test_sin[256];
float test_cos[256];

uint16_t first_array[BUFFER_SIZE];
uint16_t second_array[BUFFER_SIZE];

float mag[DISPLAY_LENGTH];
int32_t waves_buffer[BUFFER_SIZE];

volatile bool first_array_ready = false;
volatile bool second_array_ready = false;

void dma_handler(){
  if(dma_hw->ints0 & (1u << FIRST_CHANNEL)){
    dma_channel_set_write_addr(FIRST_CHANNEL, &first_array[0], false);
    first_array_ready = true;
    dma_hw->ints0 = 1u << FIRST_CHANNEL;
  } 
   
  if(dma_hw->ints0 & (1u << SECOND_CHANNEL)){
    dma_channel_set_write_addr(SECOND_CHANNEL, &second_array[0], false);
    second_array_ready = true;
    dma_hw->ints0 = 1u << SECOND_CHANNEL;
  }
}

void init_adc(uint8_t input){
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

  adc_set_clkdiv(CLKDIV);
}


void init_dma(){
  dma_channel_claim(FIRST_CHANNEL);
  dma_channel_claim(SECOND_CHANNEL);
  
  dma_channel_config first_chan_config = dma_channel_get_default_config(FIRST_CHANNEL);
  channel_config_set_transfer_data_size(&first_chan_config, DMA_SIZE_16);
  channel_config_set_read_increment(&first_chan_config, false);
  channel_config_set_write_increment(&first_chan_config, true);
  channel_config_set_chain_to(&first_chan_config, SECOND_CHANNEL);
  
  dma_channel_config second_chan_config = dma_channel_get_default_config(SECOND_CHANNEL);
  channel_config_set_transfer_data_size(&second_chan_config, DMA_SIZE_16);
  channel_config_set_read_increment(&second_chan_config, false);
  channel_config_set_write_increment(&second_chan_config, true);
  channel_config_set_chain_to(&second_chan_config, FIRST_CHANNEL);

  channel_config_set_dreq(&first_chan_config, DREQ_ADC);
  channel_config_set_dreq(&second_chan_config, DREQ_ADC);
  
  dma_channel_set_irq0_enabled(FIRST_CHANNEL, true);
  dma_channel_set_irq0_enabled(SECOND_CHANNEL, true);
  
  irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
  irq_set_enabled(DMA_IRQ_0, true);

  dma_channel_configure(
    FIRST_CHANNEL,
    &first_chan_config,
    first_array,
    &adc_hw->fifo,
    BUFFER_SIZE,
    false
  );

  dma_channel_configure(
    SECOND_CHANNEL,
    &second_chan_config,
    second_array,
    &adc_hw->fifo,
    BUFFER_SIZE,
    false
  );
}

void set_sin_tests(){
  for(uint16_t i = 0; i < BUFFER_SIZE; i++){
    test_sin[i] = sinf(2 * M_PI * i / BUFFER_SIZE);
  }
}

void set_cos_tests(){
  for(uint16_t i = 0; i < BUFFER_SIZE; i++){
    test_cos[i] = cosf(2 * M_PI * i / BUFFER_SIZE);
  }
}

uint16_t calculate_mid(uint16_t *sample_buffer){
  uint32_t mid = 0;
  for(uint16_t i = 0; i < BUFFER_SIZE; i++){
    mid += sample_buffer[i];
  }

  return mid / BUFFER_SIZE;
}


void process_buffer(uint16_t *array){
  uint16_t mid = calculate_mid(array);
  for(uint16_t i = 0; i < BUFFER_SIZE; i++){
    waves_buffer[i] = (int32_t)array[i] - mid;
  }

  for(uint8_t j = 0; j < DISPLAY_LENGTH; j++){
    uint16_t test_pos = 0;
    float co = 0;
    float si = 0;
    for(uint16_t f = 0; f < BUFFER_SIZE; f++){
      test_pos %= BUFFER_SIZE;
      co += waves_buffer[f] * test_cos[test_pos];
      si += waves_buffer[f] * test_sin[test_pos];
      test_pos += j;
    }

    mag[j] = sqrtf(co * co + si * si);
  }
}

int main(){  
  stdio_init_all();
  
  sleep_ms(SLEEP_BEFORE_START_MS);

  set_sin_tests();
  set_cos_tests();
  
  init_adc(TEMP_SENSOR_ADC);
  init_dma();

  dma_start_channel_mask(1u << FIRST_CHANNEL);
  
  printf("Starting...\n");
  adc_run(true);
  
  while(true){    
    if(first_array_ready){
      process_buffer(first_array);
      first_array_ready = false;
    } 

    if(second_array_ready){
      process_buffer(second_array);
      second_array_ready = false;
    }

    for(uint8_t i = 0; i < DISPLAY_LENGTH; i++){
      printf("%f, ", mag[i]);
    }
    printf("\n");
  }
}
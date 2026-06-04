#include "main.h"
#include "adc.h"
#include "dac.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include <stdio.h>
#include <math.h>

#define SAMPLE_POINT 256
#define ADC_CH_NUM   2
#define UART_RX_LEN  16

typedef struct{
  uint8_t wave_type;
  uint16_t freq;
  uint16_t amp;
}WaveParam_t;

WaveParam_t g_wave_cfg={0,100,1024};
uint16_t dac_wave_buf[SAMPLE_POINT];
uint16_t adc_buf[ADC_CH_NUM*SAMPLE_POINT];
uint8_t adc_sample_en=0;
uint8_t uart_rx_buf[UART_RX_LEN];
uint8_t uart_rx_cnt=0;

void Gen_Sine(WaveParam_t *cfg,uint16_t *buf)
{
  float step = 2*M_PI/SAMPLE_POINT;
  for(uint16_t i=0;i<SAMPLE_POINT;i++){
    float val = cfg->amp*sin(i*step)+2048;
    buf[i] = (uint16_t)val;
  }
}

void Gen_Square(WaveParam_t *cfg,uint16_t *buf)
{
  uint16_t half=SAMPLE_POINT/2;
  for(uint16_t i=0;i<SAMPLE_POINT;i++){
    if(i<half) buf[i]=2048+cfg->amp;
    else buf[i]=2048-cfg->amp;
  }
}

void Gen_Triangle(WaveParam_t *cfg,uint16_t *buf)
{
  uint16_t half=SAMPLE_POINT/2;
  for(uint16_t i=0;i<SAMPLE_POINT;i++){
    if(i<half) buf[i]=2048-cfg->amp + (2*cfg->amp*i)/half;
    else buf[i]=2048+cfg->amp - (2*cfg->amp*(i-half))/half;
  }
}

void Refresh_DAC_Wave(WaveParam_t *cfg)
{
  switch(cfg->wave_type){
    case 0:Gen_Sine(cfg,dac_wave_buf);break;
    case 1:Gen_Square(cfg,dac_wave_buf);break;
    case 2:Gen_Triangle(cfg,dac_wave_buf);break;
  }
  uint32_t arr=170000000/(cfg->freq*SAMPLE_POINT)-1;
  __HAL_TIM_SET_AUTORELOAD(&htim6,arr);
  HAL_DAC_Stop_DMA(&hdac1,DAC_CHANNEL_1);
  HAL_DAC_Start_DMA(&hdac1,DAC_CHANNEL_1,(uint32_t*)dac_wave_buf,SAMPLE_POINT,DAC_ALIGN_12B_R);
  HAL_TIM_Base_Start(&htim6);
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
  if(hadc->Instance==ADC1 && adc_sample_en)
  {
    char tx_buf[1024];
    uint16_t len=sprintf(tx_buf,"#");
    for(uint16_t i=0;i<SAMPLE_POINT;i++){
      len+=sprintf(tx_buf+len,"%d,%d;",adc_buf[2*i],adc_buf[2*i+1]);
    }
    HAL_UART_Transmit(&huart1,(uint8_t*)tx_buf,len,100);
  }
}

void Set_ADC_Sample(uint8_t en)
{
  adc_sample_en=en;
  if(en){
    HAL_ADC_Start_DMA(&hadc1,(uint32_t*)adc_buf,ADC_CH_NUM*SAMPLE_POINT);
    HAL_TIM_Base_Start(&htim2);
  }else{
    HAL_TIM_Base_Stop(&htim2);
    HAL_ADC_Stop_DMA(&hadc1);
  }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if(huart->Instance==USART1)
  {
    uint8_t dat=uart_rx_buf[uart_rx_cnt++];
    if(dat==';' || uart_rx_cnt>=UART_RX_LEN){
      uint8_t w;uint16_t f,a;
      sscanf((char*)uart_rx_buf,"$%d,%d,%d;",&w,&f,&a);
      g_wave_cfg.wave_type=w;
      g_wave_cfg.freq=(f<1000)?f:1000;
      g_wave_cfg.amp=(a<2047)?a:2047;
      Refresh_DAC_Wave(&g_wave_cfg);
      uart_rx_cnt=0;memset(uart_rx_buf,0,UART_RX_LEN);
    }
    HAL_UART_Receive_IT(&huart1,uart_rx_buf+uart_rx_cnt,1);
  }
}

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_DAC1_Init();
  MX_TIM2_Init();
  MX_TIM6_Init();
  MX_USART1_UART_Init();

  Refresh_DAC_Wave(&g_wave_cfg);
  HAL_UART_Receive_IT(&huart1,uart_rx_buf,1);

  while (1)
  {
    if(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_0)==GPIO_PIN_RESET)
    {
      HAL_Delay(20);
      while(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_0)==GPIO_PIN_RESET);
      adc_sample_en=!adc_sample_en;
      Set_ADC_Sample(adc_sample_en);
    }
    HAL_Delay(10);
  }
}

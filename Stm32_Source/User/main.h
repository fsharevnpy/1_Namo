#include "stm32f10x.h"
#include "stm32f10x_conf.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "list.h"
#include "display.h"

#define READ_BUTTON			(GPIO_ReadInputData(GPIOB) & GPIO_Pin_1)
#define READ_AI 				(GPIO_ReadInputData(GPIOB) & GPIO_Pin_4)
#define READ_DT					(GPIO_ReadInputData(GPIOB) & GPIO_Pin_10)
#define READ_CLK				(GPIO_ReadInputData(GPIOB) & GPIO_Pin_11)

#define LINE_LENGTH			17
#define STEP 4

void RCC_Configuration(void);
void ADC_Configuration(void);
void GPIO_Configuration(void);
void convert_int_2_str(uint8_t num, char* dst, uint8_t num_len);
uint8_t copy_str(char* src, char* dst);
uint8_t limit(uint8_t num, uint8_t lower, uint8_t upper);
uint8_t inc_range(uint8_t count, uint8_t lower, uint8_t upper, uint8_t step);

uint16_t ADCConvertedValue;
float battery;

uint8_t count_enc, range_namo[]={0,1}, count_namo[]={0,0};

void RCC_Configuration(void)
{
	/* ADC1 clock config */
	RCC_ADCCLKConfig(RCC_PCLK2_Div4);
	
  /* DMA clock enable */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

  /* Enable GPIO, AFIO, USART1 and ADC1 clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO | RCC_APB2Periph_ADC1, ENABLE);
}

void ADC_Configuration(void)
{
	ADC_InitTypeDef ADC_InitStructure;

  /* ADC1 configuration ------------------------------------------------------*/
  ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
  ADC_InitStructure.ADC_ScanConvMode = ENABLE;
  ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
  ADC_InitStructure.ADC_NbrOfChannel = 4;
  ADC_Init(ADC1, &ADC_InitStructure);

  /* ADC1 regular channel14 configuration */ 
  ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_239Cycles5);
  ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 2, ADC_SampleTime_239Cycles5);
  ADC_RegularChannelConfig(ADC1, ADC_Channel_2, 3, ADC_SampleTime_239Cycles5);
  ADC_RegularChannelConfig(ADC1, ADC_Channel_3, 4, ADC_SampleTime_239Cycles5);

  /* Enable ADC1 DMA */
  ADC_DMACmd(ADC1, ENABLE);
  
  /* Enable ADC1 */
  ADC_Cmd(ADC1, ENABLE);

  /* Enable ADC1 reset calibration register */   
  ADC_ResetCalibration(ADC1);
  /* Check the end of ADC1 reset calibration register */
  while(ADC_GetResetCalibrationStatus(ADC1));

  /* Start ADC1 calibration */
  ADC_StartCalibration(ADC1);
  /* Check the end of ADC1 calibration */
  while(ADC_GetCalibrationStatus(ADC1));
     
  /* Start ADC1 Software Conversion */ 
  ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

void DMA_Configuration(void)
{
	DMA_InitTypeDef DMA_InitStructure;
	
  /* DMA1 channel1 configuration ----------------------------------------------*/
  DMA_DeInit(DMA1_Channel1);
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)0x4001244C;
  DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)&ADCConvertedValue;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
  DMA_InitStructure.DMA_BufferSize = 1;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
  DMA_Init(DMA1_Channel1, &DMA_InitStructure);
  
  /* Enable DMA1 channel1 */
  DMA_Cmd(DMA1_Channel1, ENABLE);
}

void GPIO_Configuration(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
	
  // Cofigure PA1 as input with internal pull-up resistor
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_4 | GPIO_Pin_1;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

	/* Configure ADC pins as alternate function push-pull */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void convert_int_2_str(uint8_t num, char* dst, uint8_t num_len)
{
  uint8_t i = 0;
  uint8_t tmp = num;
  uint8_t digits = 0;

  if (num == 0)
  {
    digits = 1;
  }
  else
  {
    while (tmp != 0)
    {
      digits++;
      tmp /= 10;
    }
  }

  i = (digits > num_len) ? digits : num_len;
  dst[i] = '\0';

  while (i > 0)
  {
    dst[--i] = (num % 10) + '0';
    num /= 10;
  }
}


uint8_t copy_str(char* src, char* dst)
{
	uint8_t ret;
	for (uint8_t i = 0; i < LINE_LENGTH; i++)
	{
		if (dst[i] != src[i])
		{
			ret = 1;
			break;
		}
		else
		{
			ret = 0;
		}
	}
	if (ret == 1)
	{
		for (uint8_t i = 0; i < LINE_LENGTH; i++)
		{
			dst[i] = src[i];
		}
	}
	return ret;
}

uint8_t limit(uint8_t num, uint8_t lower, uint8_t upper)
{
	if (num > upper)
	{
		num = lower;
	}
	if (num < lower)
	{
		num = upper;
	}
	return num;
}

uint8_t inc_range(uint8_t count, uint8_t lower, uint8_t upper, uint8_t step)
{
  uint8_t range_size = (upper - lower + 1);
  uint8_t step_size = range_size / step;
  uint8_t remainder = range_size % step;

  uint8_t offset = count - lower;

  for (uint8_t i = 0, acc = 0; i < step; ++i) {
    uint8_t size = step_size + (i < remainder ? 1 : 0);
    if (offset < acc + size)
      return i + 1;
    acc += size;
  }

  return step;
}

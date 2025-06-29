/**
  ******************************************************************************
  * @file    Project/STM32F10x_StdPeriph_Template/main.c 
  * @author  MCD Application Team
  * @version V3.6.0
  * @date    20-September-2021
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2011 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <main.h>

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

static void Display_Task(void *pvParameters);
static void Encoder_Task(void *pvParameters);
static void AI_Task(void *pvParameters);
static void ReadADC_Task(void *pvParameters);

int main()
{
	count_enc = 0;
	count_namo[0] = 0;
	range_namo[0]=0;
	range_namo[1]=1;

	RCC_Configuration();
	GPIO_Configuration();
	DMA_Configuration();
	ADC_Configuration();
	
	xTaskCreate(Display_Task, (const char *) "DisplayTask", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
	xTaskCreate(Encoder_Task, (const char *) "EncoderTask", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
	xTaskCreate(AI_Task, (const char *) "AITask", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
	xTaskCreate(ReadADC_Task, (const char *) "ReadADCTask", configMINIMAL_STACK_SIZE, NULL, 2, NULL);

	vTaskStartScheduler();
	for(;;){;}
}

static void Display_Task(void *pvParameters)
{
	char line_count[17], line_bat[17];
	int8_t delta;
	InitDisplay();
	WriteDisplayNoClear("000" , &Font_16x26, DISPLAY_BLUE_COLOR , 5 , 25);
	WriteDisplayNoClear("000" , &Font_11x18, DISPLAY_BLUE_COLOR , 85 , 5);
	for(;;)
	{
		range_namo[1] = inc_range(count_enc, 1, 60, 15);
		if (range_namo[0] != range_namo[1])
		{
			delta = range_namo[1] - range_namo[0];
			if ((delta > 0 && delta < STEP / 2) || (delta < 0 && -delta > STEP / 2)) {
        count_namo[0]++;
			} else {
        count_namo[0]--;
			}

			count_namo[0] = limit(count_namo[0], 0, 108);
		}
		range_namo[0] = range_namo[1];
		
		if (count_namo[0] != count_namo[1])
		{
			convert_int_2_str(count_namo[0], line_count, 3);
			WriteDisplayNoClear(line_count , &Font_16x26 , DISPLAY_BLUE_COLOR , 5 , 25);
		}
		
		count_namo[1] = count_namo[0];

		convert_int_2_str((uint8_t)battery, line_bat, 3);
		line_bat[3]= '%';
		WriteDisplayNoClear(line_bat , &Font_11x18, DISPLAY_BLUE_COLOR , 85 , 5);
		vTaskDelay(50 / portTICK_RATE_MS);
	}
}

static void Encoder_Task(void *pvParameters)
{
	uint8_t state,last_state=0;
	for(;;)
	{
		state = !READ_DT << 1 | !READ_CLK;
		if (((state == 1) && (last_state == 0)) ||
				((state == 3) && (last_state == 1)) || 
				((state == 2) && (last_state == 3)) || 
				((state == 0) && (last_state == 2)))
		{ // 0 - 1 - 3 - 2 - 0
			count_enc++;
		}
		else if (((state == 0) && (last_state == 2)) ||
						 ((state == 1) && (last_state == 3)) || 
						 ((state == 3) && (last_state == 2)) || 
						 ((state == 2) && (last_state == 0)))
		{ // 0 - 2 - 3 - 1 - 0
			count_enc--;
		}
		
		count_enc = limit(count_enc, 1, 60);
		last_state = state;

		if (READ_BUTTON)
		{
			GPIO_SetBits(GPIOC, GPIO_Pin_13);
		}
		else
		{
			count_enc = 0;
			count_namo[0] = 0;
			range_namo[0]=0;
			range_namo[1]=1;
			GPIO_ResetBits(GPIOC, GPIO_Pin_13);
		}

		vTaskDelay(2 / portTICK_RATE_MS);
	}
}

static void AI_Task(void *pvParameters)
{
	uint8_t state,last_state=0;
	for(;;)
	{
		state = READ_AI;
		if ((state != 0) && (state != last_state))
		{
			count_namo[0]+=1;
		}
		count_namo[0] = limit(count_namo[0], 0, 108);
		last_state = state;

		vTaskDelay(2 / portTICK_RATE_MS);
	}
}

void ReadADC_Task(void *pvParameters)
{
	for (;;)
	{
		battery = 100.0f * (ADCConvertedValue - 2486.0f)/1609.0f;
		if (battery > 100.0f) battery = 100.0f;
		if (battery < 0.0f) battery = 0.0f;
		vTaskDelay(125 / portTICK_RATE_MS);
	}
}

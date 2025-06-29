/*
		iic.c file is a I2C Driver file.
    Copyright (C) 2018 Nima Mohammadi

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "iic.h"
#include "stm32f10x_i2c.h"

void i2cm_init(I2C_TypeDef* I2Cx, uint32_t i2c_clock)
{
  if (I2Cx == I2C1)
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
  else
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

  I2C_Cmd(I2Cx, DISABLE);
  I2C_DeInit(I2Cx);
  I2C_InitTypeDef i2c_InitStruct;
  i2c_InitStruct.I2C_Mode = I2C_Mode_I2C;
  i2c_InitStruct.I2C_ClockSpeed = i2c_clock;
  i2c_InitStruct.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
  i2c_InitStruct.I2C_DutyCycle = I2C_DutyCycle_2;
  i2c_InitStruct.I2C_Ack = I2C_Ack_Enable;
  i2c_InitStruct.I2C_OwnAddress1 = 0x3C;
  I2C_Cmd(I2Cx, ENABLE);
  I2C_Init(I2Cx, &i2c_InitStruct);

  GPIO_InitTypeDef InitStruct;
  InitStruct.GPIO_Mode = GPIO_Mode_AF_OD;
  InitStruct.GPIO_Speed = GPIO_Speed_50MHz;

  if (I2Cx == I2C1)
    InitStruct.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
  else
    InitStruct.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;

  GPIO_Init(GPIOB, &InitStruct);
}

void i2cm_Start(I2C_TypeDef* I2Cx, uint8_t slave_addr, uint8_t IsRead)
{
	while (I2Cx->SR2 & I2C_SR2_BUSY);
	I2C_GenerateSTART(I2Cx, ENABLE);
	while (!(I2Cx->SR1 & I2C_SR1_SB)){;}
	I2C_Send7bitAddress(I2Cx, slave_addr, IsRead);
	while (!(I2Cx->SR1 & I2C_SR1_ADDR)){;}
	(void) I2Cx->SR2; 						    
	while (!(I2Cx->SR1 & I2C_SR1_TXE));
}

int8_t i2cm_Stop(I2C_TypeDef* I2Cx)
{
  I2C_GenerateSTOP(I2Cx, ENABLE);
  while (I2C_GetFlagStatus(I2Cx, I2C_FLAG_STOPF));
  return I2C_ERR_Ok;
}

int8_t i2cm_WriteBuff(I2C_TypeDef* I2Cx, uint8_t *pbuf, uint16_t len)
{
  while (len--)
  {
    I2C_SendData(I2Cx, *(pbuf++));
    while (!(I2Cx->SR1 & I2C_SR1_TXE));
  }

  return I2C_ERR_Ok;
}

int8_t i2cm_ReadBuffAndStop(I2C_TypeDef* I2Cx, uint8_t *pbuf, uint16_t len)
{
  I2C_AcknowledgeConfig(I2Cx, ENABLE);

  while (len-- != 1)
  {
    while(!(I2Cx->SR1&I2C_SR1_RXNE)){;}
    *pbuf++ = I2C_ReceiveData(I2Cx);
  }

  I2C_AcknowledgeConfig(I2Cx, DISABLE);
  I2C_GenerateSTOP(I2Cx,ENABLE);

  while(!(I2Cx->SR1&I2C_SR1_RXNE)){;}
  *pbuf++ = I2C_ReceiveData(I2Cx);

  i2cm_Stop(I2Cx);

  return I2C_ERR_Ok;
}

/*
	*
  ******************************************************************************
  * @file    myiic.c
  * @author  Sercan ERAT
  * @version V1.0.0
  ******************************************************************************
	*
*/

/*

SCL -> PB8
SDA -> PB9

*/

#include "IMU.h"
#include "stm32f10x_conf.h"

void IIC_Init(void)
{
	/*Enable clock access to GPIOB*/
	RCC->APB2ENR|=RCC_APB2ENR_IOPBEN;

	/*Set PB6 to output 50MHz*/
	GPIOB->CRL|=GPIO_CRL_MODE6;
	/*Set PB6 to ALternate Open drain*/
	GPIOB->CRL|=GPIO_CRL_CNF6;

	/*Set PB7 to output 50MHz*/
	GPIOB->CRL|=GPIO_CRL_MODE7;
	/*Set PB7 to ALternate Open drain*/
	GPIOB->CRL|=GPIO_CRL_CNF7;

	/*Enable clock access to alternate function of the pins*/
	RCC->APB2ENR|=RCC_APB2ENR_AFIOEN;

	/*Enable clock access to I2C1*/
	RCC->APB1ENR|=RCC_APB1ENR_I2C1EN;


	/*Tell the peripheral that the clock is 8MHz*/
	I2C1->CR2&=~(I2C_CR2_FREQ);
	I2C1->CR2|=(8<<0U);
	/*Set the rise time*/
	I2C1->TRISE=9;

	I2C1->CCR|=0x28;

	I2C1->CR1|=I2C_CR1_PE;
}

void I2C_start(I2C_TypeDef* I2Cx, uint8_t address, uint8_t direction){
	
	while (I2C1->SR2 & I2C_SR2_BUSY);
	I2C_GenerateSTART(I2Cx, ENABLE);
	while (!(I2C1->SR1 & I2C_SR1_SB)){;}
	I2C_Send7bitAddress(I2Cx, address, direction);
	while (!(I2C1->SR1 & I2C_SR1_ADDR)){;}
	(void) I2C1->SR2; 						    
	while (!(I2C1->SR1 & I2C_SR1_TXE)); 
}

void I2C_write(I2C_TypeDef* I2Cx, uint8_t data) {
	I2C_SendData(I2Cx, data);
	while (!(I2C1->SR1 & I2C_SR1_TXE)); 
}

uint8_t I2C_read_ack(I2C_TypeDef* I2Cx){
	
	uint8_t data;
	I2C_AcknowledgeConfig(I2Cx, ENABLE);
	while(!(I2C1->SR1&I2C_SR1_RXNE)){;}
	data = I2C_ReceiveData(I2Cx);
	return data;
}

uint8_t I2C_read_nack(I2C_TypeDef* I2Cx){
	
	uint8_t data; 
	I2C_AcknowledgeConfig(I2Cx, DISABLE);
	I2C_GenerateSTOP(I2Cx, ENABLE);
	while(!(I2C1->SR1&I2C_SR1_RXNE)){;}
	data = I2C_ReceiveData(I2Cx);
	return data;
}

void I2C_stop(I2C_TypeDef* I2Cx){
	
	I2C_GenerateSTOP(I2Cx, ENABLE);
}

void I2C_readByte(uint8_t slave_address, uint8_t readAddr, uint8_t *data){
	
	I2C_start(I2C1, slave_address, I2C_Direction_Transmitter);
	I2C_write(I2C1, readAddr); 
	I2C_stop(I2C1);
	I2C_start(I2C1, slave_address, I2C_Direction_Receiver); 
	*data = I2C_read_nack(I2C1); 
}

void I2C_writeByte(uint8_t slave_address, uint8_t writeAddr, uint8_t data){
	
	I2C_start(I2C1, slave_address, I2C_Direction_Transmitter); 
	I2C_write(I2C1, writeAddr);
  I2C_write(I2C1, data);
  I2C_stop(I2C1);
}

void I2C_readBytes(uint8_t slave_address, uint8_t readAddr, uint8_t length, uint8_t *data){
	
	I2C_start(I2C1, slave_address, I2C_Direction_Transmitter); 	
	I2C_write(I2C1, readAddr); 																	
	I2C_stop(I2C1); 																						
	I2C_start(I2C1, slave_address, I2C_Direction_Receiver); 		
	while(length){
		if(length==1)
			*data = I2C_read_nack(I2C1);
		else
			*data = I2C_read_ack(I2C1);																
		
		data++;
		length--;
	}
}

void I2C_writeBytes(uint8_t slave_address, uint8_t writeAddr, uint8_t length, uint8_t *data){
	
	int i=0;
	I2C_start(I2C1, slave_address, I2C_Direction_Transmitter);
	I2C_write(I2C1, writeAddr);
	for(i=0; i<length; i++){	
		I2C_write(I2C1, data[i]);
  }
  I2C_stop(I2C1);
}

void I2C_readBit(uint8_t slave_address, uint8_t regAddr, uint8_t bitNum, uint8_t *data){
	
  uint8_t tmp;
  I2C_readByte(slave_address, regAddr, &tmp);  
  *data = tmp & (1 << bitNum);
}

void I2C_readBits(uint8_t slave_address, uint8_t regAddr, uint8_t bitStart, uint8_t length, uint8_t *data){
	
  // 01101001 read byte
  // 76543210 bit numbers
  //    xxx   args: bitStart=4, length=3
  //    010   masked
  //   -> 010 shifted
  uint8_t mask,tmp;
  I2C_readByte(slave_address, regAddr, &tmp); 
  mask = ((1 << length) - 1) << (bitStart - length + 1);
  tmp &= mask;
  tmp >>= (bitStart - length + 1);
	*data = tmp;
}

void I2C_writeBit(uint8_t slave_address, uint8_t regAddr, uint8_t bitNum, uint8_t data){
  
	uint8_t tmp;
  I2C_readByte(slave_address, regAddr, &tmp);  
  tmp = (data != 0) ? (tmp | (1 << bitNum)) : (tmp & ~(1 << bitNum));
  I2C_writeByte(slave_address,regAddr,tmp); 
}

void I2C_writeBits(uint8_t slave_address, uint8_t regAddr, uint8_t bitStart, uint8_t length, uint8_t data){
	
  //      010 value to write
  // 76543210 bit numbers
  //    xxx   args: bitStart=4, length=3
  // 00011100 mask byte
  // 10101111 original value (sample)
  // 10100011 original & ~mask
  // 10101011 masked | value
  uint8_t tmp,mask;
  I2C_readByte(slave_address, regAddr, &tmp);
  mask = ((1 << length) - 1) << (bitStart - length + 1);
  data <<= (bitStart - length + 1); 
  data &= mask; 
  tmp &= ~(mask); 
  tmp |= data; 
  I2C_writeByte(slave_address, regAddr, tmp);	
}

void I2C_writeWord(uint8_t slave_address, uint8_t writeAddr, uint16_t data){
	
	I2C_start(I2C1, slave_address, I2C_Direction_Transmitter); 
	I2C_write(I2C1, writeAddr);         
  I2C_write(I2C1, (data >> 8));      // send MSB
	I2C_write(I2C1, (data << 8));			 // send LSB
  I2C_stop(I2C1);
}

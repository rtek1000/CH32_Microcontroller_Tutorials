/*
 *CH32V006K8U6 - TSL2591 Light Sensor
 *https://curiousscientist.tech/blog/ch32v006k8u6-with-tsl2591-light-sensor
 */

#include "i2c.h"        
#include <ch32v00X_conf.h>  

void IIC_Init()
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    I2C_InitTypeDef I2C_InitStructure = {0};

    RCC_PB2PeriphClockCmd(RCC_PB2Periph_GPIOC | RCC_PB2Periph_AFIO, ENABLE);
    RCC_PB1PeriphClockCmd(RCC_PB1Periph_I2C1, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1; //PC1 - SDA
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD; //OD because of the pullup resistor
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2; //PC2 - SCL
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD; //OD because of the pullup resistor
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    I2C_InitStructure.I2C_ClockSpeed = 400000; //typical speed
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_16_9; 
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit; 
    I2C_Init(I2C1, &I2C_InitStructure);

    I2C_Cmd(I2C1, ENABLE);
    I2C_AcknowledgeConfig(I2C1, ENABLE ); //Host mode
}

void I2C_SendByte(uint8_t deviceAddress, uint8_t *Buffer, uint8_t numberOfBytes)
{
    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) != RESET); //Wait for the flag to change

    I2C_GenerateSTART(I2C1, ENABLE); //Generate start

    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)); //Wait for confirmation

    I2C_Send7bitAddress(I2C1, deviceAddress << 1, I2C_Direction_Transmitter); //Pay attention to the shifting due to the 8-bit variable but 7-bit address

    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)); //Set MCU as transmitter

    for(uint8_t n=0; n < numberOfBytes; n++) //Send n bytes to the slave
    {
       if( I2C_GetFlagStatus(I2C1, I2C_FLAG_TXE) !=  RESET) //Wait for the I2C hardware to accept the next byte
        {
            I2C_SendData( I2C1, Buffer[n]); //Write the n-th byte from the buffer
            while( !I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)); //Wait until the byte is fully transmitted
        }
    }

    I2C_GenerateSTOP(I2C1, ENABLE); //Generate stop
}

void I2C_ReceiveByte(uint8_t address, uint8_t *Buffer, uint8_t numberOfBytes)
{
    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) != RESET);

    I2C_GenerateSTART(I2C1, ENABLE);

    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(I2C1, address << 1, I2C_Direction_Receiver);

    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));

    for(uint8_t n=0; n<numberOfBytes; n++) //Read n bytes from the slave
    {
        while(!I2C_GetFlagStatus(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED | I2C_FLAG_BTF)); //Wait until the next byte is fully received

        Delay_Us(1); //"safety delay" to let the flags settle

        if(I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE) != RESET) //Check if the RX buffer is not empty to avoid reading garbage
        {
            Buffer[n] = I2C_ReceiveData(I2C1); //Pass the received byte to the n-th item in the buffer
        }
    }

    I2C_GenerateSTOP(I2C1, ENABLE);
}
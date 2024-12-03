/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2023/12/25
 * Description        : Main program body.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/*
 *@Note
 *CH32V003F4P6 - I2C communication basics
 * https://curiousscientist.tech/blog/ch32v003f4p6-i2c-basics
 *
 */

#include "debug.h"

/* Global define */


/* Global Variable */
uint8_t IOConfig = 0b11111111; //default value
/*********************************************************************
 * @fn      USARTx_CFG
 *
 * @brief   Initializes the USART2 & USART3 peripheral.
 *
 * @return  none
 */
void USARTx_CFG(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure = {0};
    USART_InitTypeDef USART_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_USART1, ENABLE);

    /* USART1 TX-->D.5   RX-->D.6 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;

    USART_Init(USART1, &USART_InitStructure);
    USART_Cmd(USART1, ENABLE);
}

void IIC_Init()
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    I2C_InitTypeDef I2C_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1; //PC1 - SDA
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2; //PC2 - SCL
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    I2C_InitStructure.I2C_ClockSpeed = 400000; //typical speed, also used for Arduinos
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_16_9;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit; 
    I2C_Init(I2C1, &I2C_InitStructure);

    I2C_Cmd(I2C1, ENABLE);
}


void I2C_SendByte(uint8_t address, uint8_t data)
{
    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) != RESET); //Wait for the flag to change

    I2C_GenerateSTART(I2C1, ENABLE); //Generate start

    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)); //Wait for confirmation

    I2C_Send7bitAddress(I2C1, address << 1, I2C_Direction_Transmitter); //Pay attention to the shifting due to the 8-bit variable but 7-bit address

    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)); //Set MCU as transmitter

    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_TXE) == RESET); //Wait for the flag to change

    I2C_SendData(I2C1, data); //send a byte of data

    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)); //Wait for confirmation

    I2C_GenerateSTOP(I2C1, ENABLE); //Generate stop
}

uint8_t I2C_ReceiveByte(uint8_t address)
{
    uint8_t receivedByte;

    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) != RESET);

    I2C_GenerateSTART(I2C1, ENABLE);

    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(I2C1, address << 1, I2C_Direction_Receiver);

    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));

    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE) == RESET);

    receivedByte = I2C_ReceiveData(I2C1);

    I2C_GenerateSTOP(I2C1, ENABLE);

    return receivedByte;
}

void readIOExpander()
{
    uint8_t IOpins = I2C_ReceiveByte(0x20); //Receive a byte of data (8 pins; how convenient!) from the 0x20 address

    for(int i = 0; i <8; i++)
    {
        printf("%d ", (IOpins >> i) & 1); //Shift and print each individual bits to see the pins' status
    }
    printf("\n");
}

void toggleLEDUSART()
{
    if(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) != RESET) //poll the USART
    {
        printf("Before: ");
        readIOExpander(); //Read and print the "before" state of the GPIO expander pins
		
        uint8_t receivedData = USART_ReceiveData(USART1); //Parse the new status from the USART
        IOConfig ^= (1 << (receivedData - '0')); //Modify (flip) the i-th bit
        I2C_SendByte(0x20, IOConfig); //Send the new config to the GPIO expander
		
        printf("After: ");
        readIOExpander(); //Read the expander again to see if the new status is correct
    }
}

uint16_t ReadRawAngleAS5600()
{
    uint8_t address = 0x36;
    int16_t lowByte, highByte;
    uint16_t rawAngle;

    I2C_SendByte(address, 0x0D); //Request low byte
    lowByte = I2C_ReceiveByte(address); //Receive the low byte
    //printf("lowByte: %d\n", lowByte); //Print the low byte

    I2C_SendByte(address, 0x0C); //Request high byte
    highByte = I2C_ReceiveByte(address); //Receive high byte
    //printf("highByte: %d\n", highByte); //Print high byte

    highByte = highByte << 8;
    rawAngle = highByte | lowByte;

    return rawAngle;
}

float calculateDegreeAngle(uint16_t rawAngle)
{
    float degAngle = ((float)rawAngle * 360.0f) / 4096.0f; //12-bit readings converted into degrees angle
    return degAngle;
}

void printDegAngle(float degToPrint)
{
    int16_t wholePart = (int)(degToPrint);
    int16_t decimalPart = (int)((degToPrint - wholePart)*10000);
    printf("%d.%04d\n", wholePart, decimalPart);
	//The usual way of decomposing a float into two decimal numbers so we can print them using the printf()
}

/*********************************************************************
 * @fn      main
 *
 * @brief   Main program.
 *
 * @return  none
 */
int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();
    USART_Printf_Init(115200);
    USARTx_CFG();
	
    printf("CH32V003F4P6 - DEMO 6 - i2c communication\n");

    IIC_Init();

    while(1)
    {
        //toggleLEDUSART();
        uint16_t angle = ReadRawAngleAS5600();
        Delay_Ms(20);
        printf("Angle: %d\n", angle);
        printDegAngle(calculateDegreeAngle(angle));
    }
}

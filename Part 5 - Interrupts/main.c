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
 *CH32V003F4P6 - Interrupts
 *https://curiousscientist.tech/blog/ch32v003f4p6-interrupts
 */

#include "debug.h"


/* Global define */


/* Global Variable */
//Button
volatile uint8_t buttonPressed = 0; //0 or 1 - "int flag" instead of bool because there's no bool here

//Rotary encoder
volatile uint8_t encoderClicked = 0; //flag
volatile uint16_t encoderClicks = 0; //Actual number of encoder clicks

//USART
volatile char receivedBuffer[20]; //Buffer char array with a size of 20
volatile uint8_t buffer_index = 0; //Index of an item in the buffer
volatile char receivedCharacter = 0; //Character received from USART
volatile uint8_t usart_completed = 0; //flag
volatile uint16_t txHead = 0,  txTail = 0; //Indices of the head and tail of the transmitted USART data
volatile char txBuffer[64]; //Transmission buffer for USART

//ADC
volatile uint16_t adcValue = 0; //Conversion value
volatile uint8_t adcAvailable = 0; //flag

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

    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE); //Receive interrupt
    USART_Cmd(USART1, ENABLE);

    NVIC_InitTypeDef NVIC_InitStructure = {0};
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

void EXTI0_INT_INIT()
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    EXTI_InitTypeDef EXTI_InitStructure = {0};
    NVIC_InitTypeDef NVIC_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOC, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_2; //PD0+PD2
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1; //PC1
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    GPIO_EXTILineConfig(GPIO_PortSourceGPIOD, GPIO_PinSource0); //PD0+PD2
    EXTI_InitStructure.EXTI_Line = EXTI_Line0;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = EXTI7_0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

void initializeTimer(uint16_t prsc, uint16_t arr)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure = {0};
    NVIC_InitTypeDef NVIC_InitStructure = {0};
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC , ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1; //PC1
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    TIM_TimeBaseStructure.TIM_Period = arr;
    TIM_TimeBaseStructure.TIM_Prescaler = prsc;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    TIM_SelectOutputTrigger(TIM2, TIM_TRGOSource_Update),
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    TIM_Cmd(TIM2, ENABLE);
}

void ADC1_Init(void)
{
    ADC_InitTypeDef ADC_InitStructure = {0};
    NVIC_InitTypeDef NVIC_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T2_TRGO;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &ADC_InitStructure);

    ADC_RegularChannelConfig(ADC1, ADC_Channel_2, 1, ADC_SampleTime_57Cycles);

    NVIC_InitStructure.NVIC_IRQChannel = ADC_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    ADC_ITConfig(ADC1, ADC_IT_EOC, ENABLE);

    ADC_Cmd(ADC1, ENABLE);

    ADC_ResetCalibration(ADC1);
    while(ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1));
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
    printf("CH32V003F4P6 - DEMO - Part 5 - Interrupts\n");

    //EXTI0_INT_INIT(); //Enable interrupts for PD0

    initializeTimer(47999, 999); // 1kHz, 1s period
    ADC1_Init();
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);


    while(1)
    {

        /*
        if(buttonPressed == 1)
        {
            buttonPressed = 0;
            printf("The button was pressed! \n");
            GPIO_WriteBit(GPIOC, GPIO_Pin_1, (GPIO_ReadOutputDataBit(GPIOC, GPIO_Pin_1) == Bit_SET) ? Bit_RESET : Bit_SET);
        }
        */

        /*
        if(encoderClicked == 1)
        {
            encoderClicked = 0;
            printf("Click: %d \n", encoderClicks);
        }
        */
        /*
        if( usart_completed == 1)
        {
            usart_completed = 0;
            printf(receivedBuffer); //print buffer
            memset(receivedBuffer, 0, 20); //erase buffer
        }
        */
        /*
        if(adcAvailable == 1)
        {
            adcAvailable = 0;
            printf("ADC: %d\n", adcValue);
        }
        */

    }
}

/*
void EXTI7_0_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

void EXTI7_0_IRQHandler(void)
{
    if(EXTI_GetITStatus(EXTI_Line0) != RESET)
    {
        buttonPressed = 1; //set the flag to 1

        EXTI_ClearITPendingBit(EXTI_Line0); //Clear ISR flag
    }
}
*/

/*
void EXTI7_0_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

void EXTI7_0_IRQHandler(void)
{
    if(EXTI_GetITStatus(EXTI_Line0) != RESET)
    {
        encoderClicked = 1;

        if(GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_2) == 1)
        {
            encoderClicks++;
        }
        else
        {
            encoderClicks--;
        }

        GPIO_WriteBit(GPIOC, GPIO_Pin_1, (GPIO_ReadOutputDataBit(GPIOC, GPIO_Pin_1) == Bit_SET) ? Bit_RESET : Bit_SET);
        EXTI_ClearITPendingBit(EXTI_Line0); //Clear ISR flag
    }
}
*/


void USART1_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

void USART1_IRQHandler(void)
{
    if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        receivedCharacter = USART_ReceiveData(USART1);

        if (receivedCharacter == '\n') //Check for an endline character
        {
            receivedBuffer[buffer_index] = '\0'; //Add a null-terminate character to the buffer
            buffer_index = 0; //Reset the buffer index
            usart_completed = 1; //Set the flag for the main while()
        }
        else
        {
            if (buffer_index < 20 - 1) //Check if we're within the bounds of the buffer
            {
                receivedBuffer[buffer_index++] = receivedCharacter; //Place the new character in the buffer
            }
        }
    }

    if(USART_GetITStatus(USART1, USART_IT_TXE) != RESET)
    {
        if(txHead != txTail)
        {
            USART_SendData(USART1, txBuffer[txTail]);
            txTail = (txTail + 1) % 64;
        }
        else
        {
            USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
        }

    }
}


void TIM2_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

void TIM2_IRQHandler(void)
{
    if(TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    {
        GPIO_WriteBit(GPIOC, GPIO_Pin_1, (GPIO_ReadOutputDataBit(GPIOC, GPIO_Pin_1) == Bit_SET) ? Bit_RESET : Bit_SET);
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    }
}

/*
void ADC1_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

void ADC1_IRQHandler(void)
{
    if(ADC_GetITStatus(ADC1, ADC_IT_EOC) != RESET)
    {
        adcValue = ADC_GetConversionValue(ADC1);
        adcAvailable = 1;

        ADC_ClearITPendingBit(ADC1, ADC_IT_EOC);

    }
}
*/

void ADC1_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

void ADC1_IRQHandler(void)
{
    if(ADC_GetITStatus(ADC1, ADC_IT_EOC) != RESET)
    {
        adcValue = ADC_GetConversionValue(ADC1);

        txBuffer[txHead] = (adcValue >> 8) & 0xFF;
        txHead = (txHead + 1 ) % 64;

        txBuffer[txHead] = adcValue & 0xFF;
        txHead = (txHead + 1) % 64;

        USART_ITConfig(USART1, USART_IT_TXE, ENABLE);

        ADC_ClearITPendingBit(ADC1, ADC_IT_EOC);
    }
}





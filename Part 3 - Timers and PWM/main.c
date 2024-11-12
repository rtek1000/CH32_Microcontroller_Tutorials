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
 *CH32V003F4P6 - Basic timers and PWM
 *https://curiousscientist.tech/blog/ch32v003f4p6-timers-and-pwm
 */

#include "debug.h"


/* Global define */


/* Global Variable */
char usart_buffer[100]; //Buffer to receive characters from the USART
int buffer_index = 0; //Counter for the buffer index to keep track of the position in the buffer
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

void initializeTimerPWM(uint16_t PRSC, uint16_t ARR, uint16_t CCR)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_TIM1, ENABLE);

    //Timer pin config
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; //alternate function, PP: push-pull
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_Init(GPIOD, &GPIO_InitStructure);    

    TIM_Cmd(TIM1, DISABLE); //Disable timer before (re)configuring it
    TIM_OCInitTypeDef TIM_OCInitStructure = {0};
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure = {0};

    TIM_TimeBaseInitStructure.TIM_Period = ARR; //Auto-Reload Register (counter's max value before overflowing)
    TIM_TimeBaseInitStructure.TIM_Prescaler = PRSC; //Prescaler - Main clock's divider
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1; //No clock division
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up; //Counting upwards
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseInitStructure);

    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1; //PWM
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; //PWM is also directed to a physical pin
    TIM_OCInitStructure.TIM_Pulse = CCR; //Capture-compare register (PWM changes the OCPolarity when counter reaches this value)
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; //PWM is high when the counter starts at 0
    TIM_OC1Init(TIM1, &TIM_OCInitStructure);

    TIM_CtrlPWMOutputs(TIM1, ENABLE);
    TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM1, ENABLE);
    TIM_Cmd(TIM1, ENABLE);
}

void pollUSART()
{
    if(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) != RESET) //Check for a new character
    {
        char receivedCharacter = USART_ReceiveData(USART1); //Read the new character and pass it to a variable

        if(receivedCharacter == '\n') //Check if it is an endline character
        {
            usart_buffer[buffer_index] = '\0'; //Put a null-terminate character at the end of the buffer
            parse_USART(); //Parse the buffer because after a null-terminate character the message is complete
            buffer_index = 0; //Reset the counter so a new message can be parsed in the future
        }
        else
        {
            if(buffer_index < 100-1)
            {
                usart_buffer[buffer_index++] = receivedCharacter; //Keep adding the character to the buffer
            }
        }
    }
}

void parse_USART()
{
    int _prsc = 0, _arr = 0, _ccr = 0; //Local variables for the timer's parameters

    sscanf(usart_buffer, "%d %d %d", &_prsc, &_arr, &_ccr); //Expected format: 47999 199 100

    printf("The parsed values are: %d, %d, %d\n", _prsc, _arr, _ccr); //Print the parsed values as a check

    initializeTimerPWM(_prsc, _arr, _ccr); //(Re)initialize the timer with the new parameters
}

void initializeTimerDelay()
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_TIM1, ENABLE);

        //Timer pin config
        GPIO_InitTypeDef GPIO_InitStructure = {0};
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
        GPIO_Init(GPIOD, &GPIO_InitStructure);
        //

        TIM_Cmd(TIM1, DISABLE);
        TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure = {0};

        TIM_TimeBaseInitStructure.TIM_Period = 19999; //Max counter value before it resets (Won't be reached)
        TIM_TimeBaseInitStructure.TIM_Prescaler = 9599; //48 MHz/9600 = 5 kHz -> 200 us
        TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
        TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
        TIM_TimeBaseInit(TIM1, &TIM_TimeBaseInitStructure);

        TIM_Cmd(TIM1, ENABLE);
}

void pollTimer(uint16_t elapsedTicks) //1s - 5000 ticks
{
    if(TIM_GetCounter(TIM1) >= elapsedTicks) //When the timer reaches the user-defined amount of counts
    {
       GPIO_WriteBit(GPIOD, GPIO_Pin_2, (GPIO_ReadOutputDataBit(GPIOD, GPIO_Pin_2) == Bit_SET) ? Bit_RESET : Bit_SET); //Toggle GPIO
       TIM_SetCounter(TIM1, 0); //Reset counter to restart the counting of time
       printf("Timer tick occured\n"); //Show it on the serial terminal that the time has passed
    }

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
    Delay_Ms(5000);
    printf("CH32V003F4P6 - Demo - Part 3 - Timers and PWM\n");

    //initializeTimerPWM(47999, 199, 100);
    initializeTimerDelay();
    while(1)
    {
        //pollUSART();
        pollTimer(500); //5000 * 200 us = 1s
    }
}

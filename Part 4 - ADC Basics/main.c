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
 *CH32V003F4P6 - Basic USART communication
 *https://curiousscientist.tech/blog/ch32v003f4p6-adc-basics
 */

#include "debug.h"


/* Global define */


/* Global Variable */
uint16_t ADCBuffer[3]; //ADC buffer holding 3 16-bit integers for the multichannel acquisition
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

void initializeADC()
{
    ADC_InitTypeDef ADC_InitStructure = {0};
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_ADC1, ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div8);

    //GPIO Config
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN; //Analogue input
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    ADC_DeInit(ADC1); //Turn OFF ADC

    //Initialize ADC1
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE; //single channel acquisition
    ADC_InitStructure.ADC_ContinuousConvMode = ENABLE; //Continuous sampling
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None; //No external trigger
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1; //Number of channels
    ADC_Init(ADC1, &ADC_InitStructure);

    ADC_RegularChannelConfig(ADC1, ADC_Channel_2, 1, ADC_SampleTime_241Cycles);

    ADC_Cmd(ADC1, ENABLE); //Enable ADC peripheral
    ADC_ResetCalibration(ADC1); //Perform calibration
    while(ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1));

    ADC_SoftwareStartConvCmd(ADC1, ENABLE); //Start conversion
}

float calculateVoltage(uint16_t ADCbits)
{
    float vref = 3.3f; //Internal VREF of the board (3.3V rail)
    float adcVoltage = (vref * ADCbits)/1024.0f; 
    return adcVoltage;
}

void printADCVoltage()
{
    uint16_t adcValue = ADC_GetConversionValue(ADC1); //Fetch a conversion from ADC1 and pass it to a variable

    if(adcValue == 0)
    {
        printf("ADC value is zero!\n");
    }
    else
    {
		//Create the whole and decimal parts from a float and print them on the serial 
		float adcVoltage = calculateVoltage(adcValue); //3.1415
		int wholePart = (int) adcVoltage; //3.1415 - > 3
		int decimalPart = (int)((adcVoltage-wholePart)*10000); //3.1415-3 = 0.1415, 1415
		printf("%d.%04d\n",wholePart, decimalPart );
    }
}

uint16_t ADC_OversampleAndAverage(ADC_TypeDef* ADCx)
{
    uint32_t sum_adc_readings = 0; //(We need 32 bits because the sum can overflow the range of 16 bits)

    for (int i = 0; i < 16; i++) //Fetch 16 samples from ADCx (ADC peripheral is passed as a parameter)
    {
        sum_adc_readings += ADC_GetConversionValue(ADCx); //Sum up each conversions
    }

    uint16_t average_adc_value = sum_adc_readings >> 4; //division by 16 in a neat way 

    return average_adc_value;
}

void printADCVoltage_Oversampled() //The printing is exactly the same as previously, I just made a separate function
{
    uint16_t averaged_value = ADC_OversampleAndAverage(ADC1);

    if(averaged_value == 0)
    {
        printf("ADC value is zero!\n");
    }
    else
    {
     float adcVoltage = calculateVoltage(averaged_value); //3.1415
     int wholePart = (int) adcVoltage; //3.1415 - > 3
     int decimalPart = (int)((adcVoltage-wholePart)*10000); //3.1415-3 = 0.1415, 1415
     printf("%d.%04d\n",wholePart, decimalPart );
    }
}

void ADC_Multichannel_Init()
{
    ADC_InitTypeDef ADC_InitStructure = {0};
       GPIO_InitTypeDef GPIO_InitStructure = {0};

       RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD | RCC_APB2Periph_ADC1, ENABLE);
       RCC_ADCCLKConfig(RCC_PCLK2_Div8);

       //GPIO Config
       GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4; //PC4
       GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN; //Analogue input
       GPIO_Init(GPIOC, &GPIO_InitStructure);

       GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2; //PD2
         GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN; //Analogue input
         GPIO_Init(GPIOD, &GPIO_InitStructure);

       ADC_DeInit(ADC1);

       //Initialize ADC1
       ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
       ADC_InitStructure.ADC_ScanConvMode = ENABLE; //multi-channel acquisition
       ADC_InitStructure.ADC_ContinuousConvMode = ENABLE; //Continuous sampling
       ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None; //No external trigger
       ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
       ADC_InitStructure.ADC_NbrOfChannel = 3; //Number of channels
       ADC_Init(ADC1, &ADC_InitStructure);

       ADC_RegularChannelConfig(ADC1, ADC_Channel_2, 1, ADC_SampleTime_241Cycles);
       ADC_RegularChannelConfig(ADC1, ADC_Channel_3, 2, ADC_SampleTime_241Cycles);
       ADC_RegularChannelConfig(ADC1, ADC_Channel_Vrefint, 3, ADC_SampleTime_241Cycles);

       ADC_DMACmd(ADC1, ENABLE);

       ADC_Cmd(ADC1, ENABLE);
       ADC_ResetCalibration(ADC1);
       while(ADC_GetResetCalibrationStatus(ADC1));
       ADC_StartCalibration(ADC1);
       while(ADC_GetCalibrationStatus(ADC1));

}

void DMA_Tx_Init(DMA_Channel_TypeDef *DMA_CHx, uint32_t peripheralAddress, uint32_t memoryAddress, uint16_t bufferSize)
{
    DMA_InitTypeDef DMA_InitStructure = {0};

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    DMA_DeInit(DMA_CHx);
    DMA_InitStructure.DMA_PeripheralBaseAddr = peripheralAddress;
    DMA_InitStructure.DMA_MemoryBaseAddr = memoryAddress;
    DMA_InitStructure.DMA_BufferSize = bufferSize;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord; //16 bits
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord; //16 bits
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
    DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA_CHx, &DMA_InitStructure);
}

void printADCVoltage_Multi()
{
    for (int i = 0; i<3; i++) //Iterate over the 3 channels and print them one by one
    {
        float adcVoltage = calculateVoltage(ADCBuffer[i]);
        int wholePart = (int) adcVoltage;
        int decimalPart = (int)((adcVoltage-wholePart)*10000);
        printf("Channel - %d : %d.%04d\t",i+1,wholePart, decimalPart );
    }
    printf("\n");
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

    printf("CH32V003F4P6 - DEMO - Part 4 - ADC Basics\n");
    //initializeADC();

    ADC_Multichannel_Init();
    DMA_Tx_Init(DMA1_Channel1, (u32)&ADC1->RDATAR, (u32)ADCBuffer, 3);
    DMA_Cmd(DMA1_Channel1, ENABLE);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);

    while(1)
    {
        //printADCVoltage();
        //printADCVoltage_Oversampled();
        printADCVoltage_Multi();
        Delay_Ms(500);
    }
}

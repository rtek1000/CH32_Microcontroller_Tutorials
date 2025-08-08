/*
 *CH32V006K8U6 - TSL2591 Light Sensor
 *https://curiousscientist.tech/blog/ch32v006k8u6-with-tsl2591-light-sensor
 */

#include "debug.h"
#include "i2c.h"
#include "TSL2591.h"


/* Global define */

/* Global Variable */

//Interrupt-based variables (TSL2591 interrupt)
volatile uint8_t LuxInterrupt = 0;

/*********************************************************************
 * @fn      USARTx_CFG
 *
 * @brief   Initializes the USART1 peripheral.
 *
 * @return  none
 */
void USARTx_CFG(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure = {0};
    USART_InitTypeDef USART_InitStructure = {0};

    RCC_PB2PeriphClockCmd(RCC_PB2Periph_GPIOD | RCC_PB2Periph_USART1, ENABLE);

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

void configGPIO() //Interrupt pin configuration
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    RCC_PB2PeriphClockCmd(RCC_PB2Periph_GPIOB, ENABLE);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6; //PB6 - TSL2591 INT
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //TSL2591 has no resistor on the interrupt line
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource6); //INT 

    EXTI_InitTypeDef EXTI_InitStructure;
    EXTI_InitStructure.EXTI_Line    = EXTI_Line6 ;
    EXTI_InitStructure.EXTI_Mode    = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling; 
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel                   = EXTI7_0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    Delay_Ms(100);
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
    IIC_Init();
    
    Delay_Ms(5000);

    printf("CH32V006K8U6 - TSL2591 LUX sensor demo\n"); //Print some text on the serial port

    Delay_Ms(5000);

    uint8_t tx[1]; //Local buffer for transmitted data
    uint8_t rx[1]; //Local buffer for received data
    tx[0] = 0xA0 | 0x12; //Command plus ID register (answer should be 0x50)

    I2C_SendByte(0x29, tx ,1); //Send the contents of the tx buffer (1 byte) to the 0x29 address
    I2C_ReceiveByte(0x29, rx,1); //Receive one byte from the 0x29 address and place it in the rx buffer
    printf("ID: 0x%02X\n" , rx[0]);

    TSL2591_Disable(); //Disable the sensor (clear registers)
    TSL2591_Enable(); //Enable the sensor

    TSL2591_setGainAndIntegrationTime(TSL2591_GAIN_MED, TSL2591_INTEGRATION_300MS); //Set gain and integration time

    Delay_Ms(2000);

    uint8_t readBackGain = TSL2591_GetGain(); //Read and print gain value
    printf("Set gain: 0x%02X\n", readBackGain);

    uint8_t readBackTime = TSL2591_GetIntegrationTime(); //Read and print integration time value
    printf("Set Integration Time: 0x%02X\n", readBackTime);

    TSL2591_EnableALSInterrupt(2000, 10000, 0x05); //Low: 2000, High: 10000, 10 events.
    //TSL2591_EnableNoPersistInterrupt(2000, 10000); //Low: 2000, High: 10000, 1 event.

    uint16_t NoPersist[2];
    TSL2591_GetNoPersistThresholds(&NoPersist[0], &NoPersist[1]);    
    printf("No-persist 1: %u, No-persist 2: %u \n", NoPersist[0], NoPersist[1]); //Print the values from the recent register update

    TSL2591_ClearInterrupt(); 
    configGPIO(); //Must come after enabling the interrupts
    EXTI_ClearITPendingBit(EXTI_Line6);

    while(1)
    {
      uint32_t fulllum =  TSL2591_GetFullLuminosity(); //Get the full luminosity
      uint16_t full = (uint16_t)(fulllum & 0xFFFF); //Mask the values so we keep the lower 16 bits
      printf("%u \n", full); //Print
    
      if(LuxInterrupt == 1) //Check for the interrupt flag change
      {
        LuxInterrupt = 0; //Reset interrupt flag
        TSL2591_ClearInterrupt(); //Clear the chip's interrupt, too

        printf("Interrupt occurred!\n");
      }      
    }
}

void EXTI7_0_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void EXTI7_0_IRQHandler(void)
{  
    if(EXTI_GetITStatus(EXTI_Line6) != RESET)
    {
        LuxInterrupt = 1; //set the flag to 1

        EXTI_ClearITPendingBit(EXTI_Line6); //Clear ISR flag
    }
}

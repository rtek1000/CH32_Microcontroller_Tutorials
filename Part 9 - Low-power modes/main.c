
/*
 *CH32V003J4M6 - Low-power modes
 *https://curiousscientist.tech/blog/ch32v003j4m6-low-power-modes
 */

#include "debug.h"


/* Global define */


/* Global Variable */

/*********************************************************************
*/

void GPIOConfig()
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All; //Apply the settings on all pins under the selected port
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //Internal pull-up -> Contributes to power saving
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1; //PC1 - LED pin
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; //Push-pull ("toggleable") configuration
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; //Lowest speed
    GPIO_Init(GPIOC, &GPIO_InitStructure);
}

void EXTI2_INT_INIT(void)
{
    EXTI_InitTypeDef EXTI_InitStructure = {0};
    NVIC_InitTypeDef NVIC_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource2); //PA2
    EXTI_InitStructure.EXTI_Line = EXTI_Line2; //Pin 2
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling; //Since the pin is configured as IPU, it should be pulled down by the button
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = EXTI7_0_IRQn; //Combined interrupt handler for GPIO pins (interrupt lines) 0-7
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; //Highest interrupt priority (among competing interrupts)
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1; //Second highest sub-priority (among competing interrupts)
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

void EXTI9_INT_INIT(void)
{
    EXTI_InitTypeDef EXTI_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    EXTI_InitStructure.EXTI_Line = EXTI_Line9; //Internal line, see reference manual 2.3.4 (AWU)
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Event;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
}

void standbyConfig()
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
    RCC_LSICmd(ENABLE); //128 kHz internal oscillator
    while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET); //Wait until the oscillator becomes ready
    PWR_AWU_SetPrescaler(PWR_AWU_Prescaler_61440); //128 kHz -> 1/128kHz/61400 = 480 ms
    PWR_AWU_SetWindowValue(24); //+1 is added by the code later. 25 * 480 = 12 s -> expected sleep time
    PWR_AutoWakeUpCmd(ENABLE); //Enable the auto-wakeup feature
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

    Delay_Ms(5000); //Wait for 5 seconds so you have time to reprogram the microcontroller before it goes asleep

    GPIOConfig(); //Configure the GPIOs -> All IPUs + 1 GPIO for the LED

    EXTI2_INT_INIT(); //Button interrupt for wake up
    EXTI9_INT_INIT(); //Internal wake-up triggered by AWU

    //standbyConfig(); //Auto wakeup (LSI), if disabled, EXTI9_INT_INIT() can be disabled, too.

    while(1)
    {
        GPIO_WriteBit(GPIOC, GPIO_Pin_1, Bit_SET); //Turn the LED on
        Delay_Ms(2000); //Wait 2 seconds
        GPIO_WriteBit(GPIOC, GPIO_Pin_1, Bit_RESET); //Turn the LED off
        //__WFI(); //Go to sleep (600 uA)
        PWR_EnterSTANDBYMode(PWR_STANDBYEntry_WFE); //Go to standby (9-10 uA)
    }
}

void EXTI7_0_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast"))); //Interrupt handler for the button

void EXTI7_0_IRQHandler(void)
{
    if(EXTI_GetITStatus(EXTI_Line2) != RESET) //Check the interrupt bit
    {
        EXTI_ClearITPendingBit(EXTI_Line2); //Reset, so the MCU can receive another interrupt
    }
}


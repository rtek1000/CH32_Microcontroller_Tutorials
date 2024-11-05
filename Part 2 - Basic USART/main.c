 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/*
 *@Note
 *CH32V003F4P6 - Basic USART communication
 *https://curiousscientist.tech/blog/ch32v003f4p6-usart-basics
 */

#include "debug.h"
#include "string.h"


/* Global define */


/* Global Variable */
char receivedData; //The data which we exchange via the USART, it is a single character
char commandBuffer[10]; //Buffer that contains the incoming command (e.g. "L1" toggles LED 1)


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

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOC | RCC_APB2Periph_USART1, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5; //PD5 - TX
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; //Alternate (USART) push-pull function
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6; //PD6 - RX
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; //Floating input (controlled by the other USART device's TX pin)
    GPIO_Init(GPIOD, &GPIO_InitStructure);
    //---

    //LED GPIO
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_2 | GPIO_Pin_3; //Set the GPIO pin 0, 2 and 3
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; //Push-pull output
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; //Sets the toggling speed
    GPIO_Init(GPIOD, &GPIO_InitStructure); //Initialize GPIO port D with the above specified configuration

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1; //Set the GPIO pin 1
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; //Push-pull output
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; //Sets the toggling speed
    GPIO_Init(GPIOC, &GPIO_InitStructure); //Initialize GPIO port D with the above specified configuration
    //---

    //Button GPIO
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4; //Set the GPIO pin 0, 2, 3 and 4 as the GPIO pin in use
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; //Floating due to external pull-up
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; //Sets the toggling speed
    GPIO_Init(GPIOC, &GPIO_InitStructure); //Initialize GPIO port C with the above specified configuration
    //---

    USART_InitStructure.USART_BaudRate = 115200; //Baud rate must be the same on the serial terminal!
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;

    USART_Init(USART1, &USART_InitStructure);
    USART_Cmd(USART1, ENABLE);
}

void echoCharacterUSART()
{
    if(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) != RESET) //Wait for the RX flag to go SET
    {
        receivedData = (USART_ReceiveData(USART1)); //Fetch a byte from USART1 and pass it to the variable
        USART_SendData(USART1, receivedData); //Send back the recently populated variable to USART1
    }
}

void toggleLEDUSART()
{
	if(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) != RESET) //Get the data from the serial port after the flag changes
    {
        receivedData = (USART_ReceiveData(USART1));
    }
	
    switch(receivedData) //Do something based on thee value of the receivedData variable (char!)
    {
        case '1': //If the value is '1' (char)
            GPIO_WriteBit(GPIOD, GPIO_Pin_0, (GPIO_ReadOutputDataBit(GPIOD, GPIO_Pin_0) == Bit_SET) ? Bit_RESET : Bit_SET); //Toggle the pin
            Delay_Ms(10); //Wait a bit
            break;
        case '2':
            GPIO_WriteBit(GPIOC, GPIO_Pin_1, (GPIO_ReadOutputDataBit(GPIOC, GPIO_Pin_1) == Bit_SET) ? Bit_RESET : Bit_SET);
            Delay_Ms(10);
            break;
        case '3':
            GPIO_WriteBit(GPIOD, GPIO_Pin_2, (GPIO_ReadOutputDataBit(GPIOD, GPIO_Pin_2) == Bit_SET) ? Bit_RESET : Bit_SET);
            Delay_Ms(10);
            break;
        case '4':
            GPIO_WriteBit(GPIOD, GPIO_Pin_3, (GPIO_ReadOutputDataBit(GPIOD, GPIO_Pin_3) == Bit_SET) ? Bit_RESET : Bit_SET);
            Delay_Ms(10);
        break;
    }
}

void sendButtonNumberUSART()
{
    if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_0) == Bit_RESET) //If PC0 goes LOW, we enter this part of the code
    {
        printf("Button 1 was pressed\n");
    }

   if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_2) == Bit_RESET) //If PC2 goes LOW, we enter this part of the code
   {
       printf("Button 2 was pressed\n");
   }

   if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_3) == Bit_RESET) //If PC3 goes LOW, we enter this part of the code
   {
       printf("Button 3 was pressed\n");
   }

   if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_4) == Bit_RESET) //If PC4 goes LOW, we enter this part of the code
   {
       printf("Button 4 was pressed\n");
   }
   Delay_Ms(200); //Cheap debounce, must be adjusted based on the button's behaviour. Blocks the whole code!
}



void receiveCommandUSART(char *commandBuffer)
{
    uint8_t index = 0; //Index for keeping track of the position inside the buffer
    char receivedCharacter; //Received character from USART

    memset(commandBuffer, 0, 10); //Clear the buffer. Copy zeroes (0) to the first 10 items (length)

    while (index < (10 - 1)) //10-1 because the last character must be the null-terminator we add manually
    {
        while (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET); //Wait for the flag

        receivedCharacter = USART_ReceiveData(USART1); //Store the received character

        if (receivedCharacter == '\n' || receivedCharacter == '\r') //Abort if the character is an endline character
        {
            break;
        }

        commandBuffer[index++] = receivedCharacter; //Otherwise add the character to the buffer
    }

    commandBuffer[index] = '\0'; //Null-terminate the string after all the characters are received (when endline breaks the while)
}

void processCommandUSART(const char *command)
{
    if (command[0] == 'L' && strlen(command) >= 2) //Check if the command character is L and it is at least 2 long
    {
        char commandNumber = command[1];  //Fetch the LED number

        int commandIndex = commandNumber - '0';  //"char to int"

        if (commandIndex >= 1 && commandIndex <= 4) //Check if we received the correct number
        {
            switch (commandIndex) 
            {
            case 1:
                GPIO_WriteBit(GPIOD, GPIO_Pin_0, (GPIO_ReadOutputDataBit(GPIOD, GPIO_Pin_0) == Bit_SET) ? Bit_RESET : Bit_SET);
                break;
            case 2:
                GPIO_WriteBit(GPIOC, GPIO_Pin_1, (GPIO_ReadOutputDataBit(GPIOC, GPIO_Pin_1) == Bit_SET) ? Bit_RESET : Bit_SET);
                break;
            case 3:
                GPIO_WriteBit(GPIOD, GPIO_Pin_2, (GPIO_ReadOutputDataBit(GPIOD, GPIO_Pin_2) == Bit_SET) ? Bit_RESET : Bit_SET);
                break;
            case 4:
                GPIO_WriteBit(GPIOD, GPIO_Pin_3, (GPIO_ReadOutputDataBit(GPIOD, GPIO_Pin_3) == Bit_SET) ? Bit_RESET : Bit_SET);
                break;
            }
        }
    }
    if (command[0] == 'B' && strlen(command) >= 2) //Check if the command character is B and it is at least 2 long
        {
            char commandNumber = command[1];  //Fetch the button number
            int commandIndex = commandNumber - '0';  //"char to int"
            uint8_t pinStatus = 0; //RESET or SET enumeration

            if (commandIndex >= 1 && commandIndex <= 4) //Check if we received the correct number
            {
                switch (commandIndex)
                {
                case 1:
                    pinStatus = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_0);
                    break;
                case 2:
                    pinStatus = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_2);
                    break;
                case 3:
                    pinStatus = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_3);
                    break;
                case 4:
                    pinStatus = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_4);
                    break;
                }

                if (pinStatus == Bit_SET)
                {
                       printf("Pin %d is HIGH\n", commandIndex);
                }
                else
                {
                       printf("Pin %d is LOW\n", commandIndex);
                }

            }

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
    Delay_Ms(3000);
    printf("CH32V003F4P6 Demo part 2 - USART\n");
    USARTx_CFG();

    while(1)
    {
        //echoCharacterUSART();
        //toggleLEDUSART();
        //sendButtonNumberUSART();
        receiveCommandUSART(commandBuffer);
        processCommandUSART(commandBuffer);
    }
}

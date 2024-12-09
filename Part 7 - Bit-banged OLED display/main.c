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
 *CH32V003F4P6 - I2C bit-banged OLED driver
 https://curiousscientist.tech/blog/ch32v003f4p6-i2c-bit-banging-oled-display
 */

#include "debug.h"


/* Global define */


/* Global Variable */
uint8_t i2caddress = 0x78; //OLED Display address
volatile char receivedBuffer[200]; //Large buffer to store the characters we print on the display
volatile uint8_t buffer_index = 0; //Index of the character in the buffer
volatile uint8_t usart_completed = 0; //USART completed flag
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

    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE); //Add interrupt to USART
    USART_Cmd(USART1, ENABLE);

    NVIC_InitTypeDef NVIC_InitStructure = {0};
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

void bitbangI2C_Init()
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1; //PC1 - SDA
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2; //PC2 - SCL
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
}

void I2C_BitBang(uint8_t data)
{
    uint8_t counter = 8;

    while(counter--)
    {
        if(data & 0x80) //0x80 = 0b10000000
        {
            GPIO_WriteBit(GPIOC, GPIO_Pin_1, SET); //SDA = 1
        }
        else
        {
            GPIO_WriteBit(GPIOC, GPIO_Pin_1, RESET); //SDA = 0
        }

        Delay_Us(3);
        GPIO_WriteBit(GPIOC, GPIO_Pin_2, SET); //SCL = 1
        Delay_Us(3);
        GPIO_WriteBit(GPIOC, GPIO_Pin_2, RESET); //SCL = 0
        Delay_Us(3);
        data = data << 1;
    }
}

void I2C_ACK()
{
    GPIO_WriteBit(GPIOC, GPIO_Pin_1, SET); //SDA = 1
    Delay_Us(3);
    GPIO_WriteBit(GPIOC, GPIO_Pin_2, SET); //SCL = 1
    Delay_Us(3);
    GPIO_WriteBit(GPIOC, GPIO_Pin_2, RESET); //SCL = 0
    Delay_Us(3);
}

void I2C_Start(uint8_t address)
{
    GPIO_WriteBit(GPIOC, GPIO_Pin_1, RESET); //SDA = 0
    Delay_Us(3);
    GPIO_WriteBit(GPIOC, GPIO_Pin_2, SET); //SCL = 1
    Delay_Us(3);
    GPIO_WriteBit(GPIOC, GPIO_Pin_2, RESET); //SCL = 0
    Delay_Us(3);
    I2C_BitBang(address);
    I2C_ACK();
}

void I2C_Stop()
{
    GPIO_WriteBit(GPIOC, GPIO_Pin_2, SET); //SCL = 1
    Delay_Us(3);
    GPIO_WriteBit(GPIOC, GPIO_Pin_1, RESET); //SDA = 0
    Delay_Us(3);
    GPIO_WriteBit(GPIOC, GPIO_Pin_1, SET); //SDA = 1
    Delay_Us(3);
}

void I2C_Write_Command(uint8_t command)
{
    I2C_Start(i2caddress);
    I2C_BitBang(0x00);
    I2C_ACK();
    I2C_BitBang(command);
    I2C_ACK();
    I2C_Stop();
}

void I2C_Write_Data(uint8_t data)
{
    I2C_Start(i2caddress);
    I2C_BitBang(0x40);
    I2C_ACK();
    I2C_BitBang(data);
    I2C_ACK();
    I2C_Stop();
}

void initializeOLED()
{
    I2C_Write_Command(0xAE); //Display OFF
    I2C_Write_Command(0xA8);
    I2C_Write_Command(0x27); //MUX ratio - Not set
    I2C_Write_Command(0xD3);
    I2C_Write_Command(0x00); //Vertical shift - Reset
    I2C_Write_Command(0x40); //Display from RAM
    I2C_Write_Command(0xA1); //Segment remap - A1
    I2C_Write_Command(0xC8); //Port scan direction - Remapped
    I2C_Write_Command(0xDA);
    I2C_Write_Command(0x12); //Set COM pins 
    I2C_Write_Command(0x81);
    I2C_Write_Command(0x80); //Contrast = 128
    I2C_Write_Command(0xA4); //Entire display ON according to the GDDRAM content
    I2C_Write_Command(0xA6); //Normal display
    I2C_Write_Command(0xD5);
    I2C_Write_Command(0x80); //Display divide ratio and oscillator frequency
    I2C_Write_Command(0x8D);
    I2C_Write_Command(0x14); //Enable charge pump
    I2C_Write_Command(0xAD);
    I2C_Write_Command(0x30); //IREF
    I2C_Write_Command(0xAF); //Display ON
}

void eraseDisplay()
{
    for(uint8_t page = 0; page < 5; page++) //5 lines, 8 pixels (1 byte) per line
    {
        I2C_Write_Command(0x22); //Set page address command
        I2C_Write_Command(0xb0+page); //Actual page address
        I2C_Write_Command(0x04); //Set border
        I2C_Write_Command(0x0C); //Lower 4-bit column address
        I2C_Write_Command(0x11); //Higher 4-bit column address

        for(uint8_t segment = 0; segment < 72; segment++) //72 (pixel) columns
        {
            I2C_Write_Data(0x00); //Set all 8 pixels to 0 (turn them off)
        }
    }
}

const uint8_t font[97][5] =
{
    { 0x00, 0x00, 0x00, 0x00, 0x00 }, // " " 0x20
    { 0x00, 0x00, 0x4f, 0x00, 0x00 }, // !   0x21
    { 0x00, 0x07, 0x00, 0x07, 0x00 }, // "   0x22
    { 0x14, 0x7f, 0x14, 0x7f, 0x14 }, // #   0x23
    { 0x24, 0x2a, 0x7f, 0x2a, 0x12 }, // $   0x24
    { 0x23, 0x13, 0x08, 0x64, 0x62 }, // %   0x25
    { 0x36, 0x49, 0x55, 0x22, 0x50 }, // &   0x26
    { 0x00, 0x05, 0x03, 0x00, 0x00 }, // '   0x27
    { 0x00, 0x1c, 0x22, 0x41, 0x00 }, // (   0x28
    { 0x00, 0x41, 0x22, 0x1c, 0x00 }, // )   0x29
    { 0x14, 0x08, 0x3e, 0x08, 0x14 }, // *   0x2A
    { 0x08, 0x08, 0x3e, 0x08, 0x08 }, // +   0x2B
    { 0x00, 0x50, 0x30, 0x00, 0x00 }, // ,   0x2C
    { 0x08, 0x08, 0x08, 0x08, 0x08 }, // -   0x2D
    { 0x00, 0x60, 0x60, 0x00, 0x00 }, // .   0x2E
    { 0x20, 0x10, 0x08, 0x04, 0x02 }, // /   0x2F
    { 0x3e, 0x51, 0x49, 0x45, 0x3e }, // 0   0x30
    { 0x00, 0x42, 0x7f, 0x40, 0x00 }, // 1   0x31
    { 0x42, 0x61, 0x51, 0x49, 0x46 }, // 2   0x32
    { 0x21, 0x41, 0x45, 0x4b, 0x31 }, // 3   0x33
    { 0x18, 0x14, 0x12, 0x7f, 0x10 }, // 4   0x34
    { 0x27, 0x45, 0x45, 0x45, 0x39 }, // 5   0x35
    { 0x3c, 0x4a, 0x49, 0x49, 0x30 }, // 6   0x36
    { 0x01, 0x71, 0x09, 0x05, 0x03 }, // 7   0x37
    { 0x36, 0x49, 0x49, 0x49, 0x36 }, // 8   0x38
    { 0x06, 0x49, 0x49, 0x29, 0x1e }, // 9   0x39
    { 0x00, 0x36, 0x36, 0x00, 0x00 }, // :   0x3A
    { 0x00, 0x56, 0x36, 0x00, 0x00 }, // ;   0x3B
    { 0x08, 0x14, 0x22, 0x41, 0x00 }, // <   0x3C
    { 0x14, 0x14, 0x14, 0x14, 0x14 }, // =   0x3D
    { 0x00, 0x41, 0x22, 0x14, 0x08 }, // >   0x3E
    { 0x02, 0x01, 0x51, 0x09, 0x06 }, // ?   0x3F
    { 0x32, 0x49, 0x79, 0x41, 0x3e }, // @   0x40
    { 0x7e, 0x11, 0x11, 0x11, 0x7e }, // A   0x41
    { 0x7f, 0x49, 0x49, 0x49, 0x36 }, // B   0x42
    { 0x3e, 0x41, 0x41, 0x41, 0x22 }, // C   0x43
    { 0x7f, 0x41, 0x41, 0x22, 0x1c }, // D   0x44
    { 0x7f, 0x49, 0x49, 0x49, 0x41 }, // E   0x45
    { 0x7f, 0x09, 0x09, 0x09, 0x01 }, // F   0x46
    { 0x3e, 0x41, 0x49, 0x49, 0x7a }, // G   0x47
    { 0x7f, 0x08, 0x08, 0x08, 0x7f }, // H   0x48
    { 0x00, 0x41, 0x7f, 0x41, 0x00 }, // I   0x49
    { 0x20, 0x40, 0x41, 0x3f, 0x01 }, // J   0x4A
    { 0x7f, 0x08, 0x14, 0x22, 0x41 }, // K   0x4B
    { 0x7f, 0x40, 0x40, 0x40, 0x40 }, // L   0x4C
    { 0x7f, 0x02, 0x0c, 0x02, 0x7f }, // M   0x4D
    { 0x7f, 0x04, 0x08, 0x10, 0x7f }, // N   0x4E
    { 0x3e, 0x41, 0x41, 0x41, 0x3e }, // O   0x4F
    { 0x7f, 0x09, 0x09, 0x09, 0x06 }, // P   0X50
    { 0x3e, 0x41, 0x51, 0x21, 0x5e }, // Q   0X51
    { 0x7f, 0x09, 0x19, 0x29, 0x46 }, // R   0X52
    { 0x46, 0x49, 0x49, 0x49, 0x31 }, // S   0X53
    { 0x01, 0x01, 0x7f, 0x01, 0x01 }, // T   0X54
    { 0x3f, 0x40, 0x40, 0x40, 0x3f }, // U   0X55
    { 0x1f, 0x20, 0x40, 0x20, 0x1f }, // V   0X56
    { 0x3f, 0x40, 0x38, 0x40, 0x3f }, // W   0X57
    { 0x63, 0x14, 0x08, 0x14, 0x63 }, // X   0X58
    { 0x07, 0x08, 0x70, 0x08, 0x07 }, // Y   0X59
    { 0x61, 0x51, 0x49, 0x45, 0x43 }, // Z   0X5A
    { 0x00, 0x7f, 0x41, 0x41, 0x00 }, // [   0X5B
    { 0x02, 0x04, 0x08, 0x10, 0x20 }, // "\" 0X5C
    { 0x00, 0x41, 0x41, 0x7f, 0x00 }, // ]   0X5D
    { 0x04, 0x02, 0x01, 0x02, 0x04 }, // ^   0X5E
    { 0x40, 0x40, 0x40, 0x40, 0x40 }, // _   0X5F
    { 0x00, 0x01, 0x02, 0x04, 0x00 }, // `   0X60
    { 0x20, 0x54, 0x54, 0x54, 0x78 }, // a   0X61
    { 0x7f, 0x48, 0x44, 0x44, 0x38 }, // b   0X62
    { 0x38, 0x44, 0x44, 0x44, 0x20 }, // c   0X63
    { 0x38, 0x44, 0x44, 0x48, 0x7f }, // d   0X64
    { 0x38, 0x54, 0x54, 0x54, 0x18 }, // e   0X65
    { 0x08, 0x7e, 0x09, 0x01, 0x02 }, // f   0X66
    { 0x0c, 0x52, 0x52, 0x52, 0x3e }, // g   0X67
    { 0x7f, 0x08, 0x04, 0x04, 0x78 }, // h   0X68
    { 0x00, 0x44, 0x7d, 0x40, 0x00 }, // i   0X69
    { 0x20, 0x40, 0x44, 0x3d, 0x00 }, // j   0X6A
    { 0x7f, 0x10, 0x28, 0x44, 0x00 }, // k   0X6B
    { 0x00, 0x41, 0x7f, 0x40, 0x00 }, // l   0X6C
    { 0x7c, 0x04, 0x18, 0x04, 0x78 }, // m   0X6D
    { 0x7c, 0x08, 0x04, 0x04, 0x78 }, // n   0X6E
    { 0x38, 0x44, 0x44, 0x44, 0x38 }, // o   0X6F
    { 0x7c, 0x14, 0x14, 0x14, 0x08 }, // p   0X70
    { 0x08, 0x14, 0x14, 0x18, 0x7c }, // q   0X71
    { 0x7c, 0x08, 0x04, 0x04, 0x08 }, // r   0X72
    { 0x48, 0x54, 0x54, 0x54, 0x20 }, // s   0X73
    { 0x04, 0x3f, 0x44, 0x40, 0x20 }, // t   0X74
    { 0x3c, 0x40, 0x40, 0x20, 0x7c }, // u   0X75
    { 0x1c, 0x20, 0x40, 0x20, 0x1c }, // v   0X76
    { 0x3c, 0x40, 0x30, 0x40, 0x3c }, // w   0X77
    { 0x44, 0x28, 0x10, 0x28, 0x44 }, // x   0X78
    { 0x0c, 0x50, 0x50, 0x50, 0x3c }, // y   0X79
    { 0x44, 0x64, 0x54, 0x4c, 0x44 }, // z   0X7A
    { 0x00, 0x08, 0x36, 0x41, 0x00 }, // {   0X7B
    { 0x00, 0x00, 0x7f, 0x00, 0x00 }, // |   0X7C
    { 0x00, 0x41, 0x36, 0x08, 0x00 }, // }   0X7D
    { 0x08, 0x08, 0x2a, 0x1c, 0x08 }, // ->  0X7E
    { 0x08, 0x1c, 0x2a, 0x08, 0x08 }, // <-  0X7F
};


void printCharacter(uint8_t page, uint8_t segment, uint8_t letter)
{
    I2C_Write_Command(0x22);
    I2C_Write_Command(0xb0+page);

    I2C_Write_Command(0x04);

    uint16_t address = (0x11 << 4) | 0x0C; //Combine the lower and higher bits into one address
    address += segment; //Increment the address by the segment value
    uint8_t newLower = address & 0x0F; //Disassemble the new address and create the new higher and lower bits
    uint8_t newHigher = (address >> 4) & 0x0F;

    I2C_Write_Command(newLower);
    I2C_Write_Command(0x10 | newHigher); //Pass the address to the OLED

    for(uint8_t i = 0; i < 5 ; i ++) //Read the 5 bytes from the font[][] array and send it to the display byte-by-byte
    {
        I2C_Write_Data(font[letter][i]);
        //{ 0x3c, 0x40, 0x30, 0x40, 0x3c }, // w
    }
}

void printText(const uint8_t *receivedBuffer, uint8_t pageStart, uint8_t columnStart)
{
    uint8_t column = columnStart;

    for(uint8_t i = 0; receivedBuffer[i] != '\0'; i++) //Read until the null-terminate character
    {
        char currentCharacter = receivedBuffer[i]; //Pass the i-th character to the variable
		
        printf("Current charater: %d\n", currentCharacter); //Print the character on the serial port (debug)

        if(currentCharacter < 0x20 || currentCharacter > 0x7E) //Handling invalid characters. See, ASCII table
        {
            printf("Invalid character was received\n");
            continue; //jump to the next iteration of the for() loop
        }

        uint8_t fontPosition = (int)currentCharacter - 0x20; //Shift the font position because our ASCII starts with the character at 0x20
        printf("Font position: %d\n", fontPosition); //Print the value on the terminal as a validation

        printCharacter(pageStart, column, fontPosition); //Print the character

        column += 6; //Move the cursor to the next position

        if(column > 72-6) //Handle wrapping the text
        {
            column = columnStart; //If the character would not fit in the current line...
            pageStart += 1; //... start a new line
        }

        if(pageStart > 4)
        {
            break; //Neglect everything which exceeds the total printable area on the display (jump out from the whole code)
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

    USARTx_CFG();

    bitbangI2C_Init(); //initialize bit-banged GPIO-I2C pins
    initializeOLED(); //Initialize the display
    Delay_Ms(2000); 
    eraseDisplay(); //Erase the display
    Delay_Ms(2000);
    printCharacter(0, 0, 33); //Print a random character at the corner

    printf("CH32V003F4P6 - DEMO 7 - I2C BitBanged OLED\n");

    while(1)
    {
        if(usart_completed == 1) //If the a new text was received from the serial port
        {
            usart_completed = 0; //Reset the flag so we can receive new data
            eraseDisplay(); //Remove the previous content from the display
            printText(receivedBuffer, 0, 0); //Update the display, starting at the (0,0) (top left corner) position
            memset(receivedBuffer, 0, 200); //Erase the text buffer array.
        }
    }
}

void USART1_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void USART1_IRQHandler(void)
{
    if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) //If the interrupt flag changed
    {
        uint8_t receivedCharacter = USART_ReceiveData(USART1); //Read in the actual character

        if(receivedCharacter == '\n') //If the character is a linebreak character
        {
            receivedBuffer[buffer_index] = '\0'; //Put a null-terminate in the buffer
            buffer_index = 0; //Reset the buffer index for the next reading
            usart_completed = 1; //Tell the main code that the USART transaction is done (data is received)
        }
        else //Normal characters were received
        {
            if(buffer_index < 200-1)
            {
                receivedBuffer[buffer_index++] = receivedCharacter; //Keep adding the characters to the array
            }
        }
    }
}








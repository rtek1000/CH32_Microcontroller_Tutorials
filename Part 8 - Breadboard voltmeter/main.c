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
 *CH32V003J4M6 - Breadboard Voltmeter
 https://curiousscientist.tech/blog/ch32v003j4m6-breadboard-voltmeter-coding
 https://curiousscientist.tech/blog/breadboard-voltmeter
 */

#include "debug.h"

/* Global define */


/* Global Variable */
uint8_t i2caddress = 0x78; //OLED Display address
uint16_t OLEDBuffer[2]; //OLED buffer for the 2 numbers printed on the OLED display
float vref = 3.3f; //voltage reference value for the ADC

void ADC_Multichannel_Init()
{
   ADC_DeInit(ADC1); //Turn off ADC before configuring it

   ADC_InitTypeDef ADC_InitStructure = {0};
   GPIO_InitTypeDef GPIO_InitStructure = {0};

   RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE); //PC4 - A2
   RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE); //PA2 - A0
   RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
   RCC_ADCCLKConfig(RCC_PCLK2_Div8);

   //GPIO Config
   GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2; //PA2 - A0 - ATTENTION! this is also the OSCO pin!
   //Check system_ch32v00x.c file and only enable this line: #define SYSCLK_FREQ_48MHZ_HSI   48000000 (INTERNAL OSC!!!)
   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN; //Analogue input
   GPIO_Init(GPIOA, &GPIO_InitStructure);

   GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4; //PC4 - A2
   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN; //Analogue input
   GPIO_Init(GPIOC, &GPIO_InitStructure);

   //Initialize ADC1
   ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
   ADC_InitStructure.ADC_ScanConvMode = DISABLE; //Single-channel acquisition
   ADC_InitStructure.ADC_ContinuousConvMode = DISABLE; //Single sampling
   ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None; //No external trigger
   ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
   ADC_InitStructure.ADC_NbrOfChannel = 1; //Number of channels, 1 because we sample 1 at a time, then we switch
   ADC_Init(ADC1, &ADC_InitStructure);

   ADC_Cmd(ADC1, ENABLE); //Enable ADC

   ADC_ResetCalibration(ADC1); //Perform calibration
   while(ADC_GetResetCalibrationStatus(ADC1));
   ADC_StartCalibration(ADC1);
   while(ADC_GetCalibrationStatus(ADC1));
}

void bitbangI2C_Init()
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1; //PC1 - SDA
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD; //Open-drain configuration (bidirectional communication)
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2; //PC2 - SCL
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; //Push-pull configuration (unidirectional communication)
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
    for(uint8_t page = 0; page < 5; page++) //5 lines, 8 pixels (1 byte) per line - 40 pixels total (height)
    {
        I2C_Write_Command(0x22); //Set page address command
        I2C_Write_Command(0xb0+page); //Actual page address
        I2C_Write_Command(0x04); //Set border
        I2C_Write_Command(0x0C); //Lower 4-bit column address
        I2C_Write_Command(0x11); //Higher 4-bit column address

        for(uint8_t segment = 0; segment < 72; segment++) //72 (pixel) columns (width)
        {
            I2C_Write_Data(0x00); //Set all 8 pixels to 0 (turn them off)
        }
    }
}

const uint8_t font_large[13][24] __attribute__((aligned(4))) =
{
    { // " " (space), character 0x20 in the ASCII table
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Top 12 bytes
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   // Bottom 12 bytes
    },

    { // 0
            0x00, 0b11111000, 0b11111100, 0b00000110, 0b00000110, 0b00000110, 0b00000110, 0b00000110, 0b00000110, 0b11111100, 0b11111000, 0x00,
            0x00, 0b00011111, 0b00111111, 0b01100000, 0b01100000, 0b01100000, 0b01100000, 0b01100000, 0b01100000, 0b00111111, 0b00011111, 0x00
    },

    { // 1
            0x00, 0x00, 0x00, 0b00011000, 0b00011100, 0b11111110, 0b11111110, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0b01100000, 0b01100000, 0b01100000, 0b01111111, 0b01111111, 0b01100000, 0b01100000, 0b01100000, 0x00, 0x00
    },

    { // 2
            0x00, 0x00, 0b00011100, 0b00001110, 0b00000110, 0b00000110, 0b00000110, 0b10001110, 0b11111110, 0b11111000, 0x00, 0x00,
            0x00, 0x00, 0b01100000, 0b01110000, 0b01111000, 0b01111100, 0b01101110, 0b01100111, 0b01100011, 0b01100000, 0x00, 0x00
    },

    { // 3
            0x00, 0b00001100, 0b00001100, 0b10000110, 0b10000110, 0b10000110, 0b10000110, 0b10000110, 0b11111110, 0b01111100, 0x00, 0x00,
            0x00, 0b00110000, 0b00110000, 0b01100001, 0b01100001, 0b01100001, 0b01100001, 0b01100001, 0b01111111, 0b00111110, 0x00, 0x00
    },

    { // 4
            0x00, 0x00, 0b10000000, 0b11100000, 0b01111000, 0b00011110, 0b00000110, 0b11111110, 0b11111110, 0x00, 0x00, 0x00,
            0x00, 0x00, 0b00000111, 0b00000111, 0b00000110, 0b00000110, 0b00000110, 0b01111111, 0b01111111, 0b00000110, 0x00, 0x00
    },

    { // 5
            0x00, 0x00, 0b11111110, 0b11111110, 0b11000110, 0b11000110, 0b11000110, 0b11000110, 0b11000110, 0b10000110, 0x00, 0x00,
            0x00, 0x00, 0b01100000, 0b01100000, 0b01100000, 0b01100000, 0b01100000, 0b01100000, 0b01111111, 0b00111111, 0x00, 0x00
    },

    { // 6
            0x00, 0x00, 0b11111100, 0b11111110, 0b10000110, 0b10000110, 0b10000110, 0b10000110, 0b10000110, 0x00, 0x00, 0x00,
            0x00, 0x00, 0b00111111, 0b01111111, 0b01100001, 0b01100001, 0b01100001, 0b01100001, 0b01111111, 0b00111111, 0x00, 0x00
    },

    { // 7
            0x00, 0x00, 0b00000110, 0b00000110, 0b10000110, 0b11000110, 0b11100110, 0b01110110, 0b00111110, 0b00011110, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0b01111111, 0b01111111, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    },

    { // 8
            0x00, 0x00, 0b01111100, 0b11111110, 0b10000110, 0b10000110, 0b10000110, 0b10000110, 0b11111110, 0b01111100, 0x00, 0x00,
            0x00, 0x00, 0b00111110, 0b01111111, 0b01100001, 0b01100001, 0b01100001, 0b01100001, 0b01111111, 0b00111110, 0x00, 0x00
    },

    { // 9
            0x00, 0x00, 0b01111100, 0b11111110, 0b10000110, 0b10000110, 0b10000110, 0b10000110, 0b11111110, 0b11111100, 0x00, 0x00,
            0x00, 0x00, 0b01100000, 0b01100001, 0b01100001, 0b01100001, 0b01100001, 0b01100001, 0b01111111, 0b00111111, 0x00, 0x00
    },

    { // - (minus)
            0x00, 0x00, 0x00, 0b10000000, 0b10000000, 0b10000000, 0b10000000, 0b10000000, 0b10000000, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000001, 0x00, 0x00, 0x00
    },

    { // . (decimal dot)
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0b01110000, 0b01110000, 0b01110000, 0b01110000, 0x00, 0x00, 0x00, 0x00
    },

    //0b11000111: read from 0b, but on the bitmap it is pixel 7, 6, 5...0.
    //So, read left to right but fill pixels in bottom up!
};


void printCharacter_Large(uint8_t page, uint8_t segment, uint8_t letter)
{
    I2C_Write_Command(0x22); //Set page address command
    I2C_Write_Command(0xb0+page); //Page address
    I2C_Write_Command(0x04); //Set border

    uint16_t address = (0x11 << 4) | 0x0C; //Combine the lower and higher bits into one address
    address += segment; //Increment the address by the segment value
    uint8_t newLower = address & 0x0F; //Disassemble the new address and create the new higher and lower bits
    uint8_t newHigher = (address >> 4) & 0x0F;

    I2C_Write_Command(newLower);
    I2C_Write_Command(0x10 | newHigher); //Pass the address to the OLED

    for(uint8_t i = 0; i < 12 ; i ++) //read the font's bitmap from 0-11 bytes and send it to the display
    {
        I2C_Write_Data(font_large[letter][i]);
    }

    I2C_Write_Command(0x22);
    I2C_Write_Command(0xb0+page+1);
    I2C_Write_Command(0x04); //Set the new page

    I2C_Write_Command(newLower);
    I2C_Write_Command(0x10 | newHigher); //Pass the address to the OLED
    //Note: same as the previous, because this is the segment (column) address and we need to start printing from the same segment as previously!

    for(uint8_t i = 12; i < 24 ; i ++) //read the font's bitmap from 12-23 bytes and send it to the display
    {
        I2C_Write_Data(font_large[letter][i]);
    }
}

void printNumber(const uint8_t *receivedBuffer, uint8_t pageStart, uint8_t columnStart)
{
    uint8_t column = columnStart; //Column start index if the printed number should be indented

    for(uint8_t i = 0; receivedBuffer[i] != '\0'; i++) //Read until the null-terminate character
    {
        char currentCharacter = receivedBuffer[i]; //Pass the i-th character to the variable

        if(currentCharacter < 0x20 || currentCharacter > 0x7E) //Handling invalid characters. See, ASCII table
        {
            continue; //jump to the next iteration of the for() loop
        }

        uint8_t fontPosition = 0; //Position of the font in the font array

       if((int)currentCharacter >= 0x30 && (int)currentCharacter<=0x39) //If it is ASCII 0-9
       {
           fontPosition = (int)currentCharacter - 0x2f;
           //Starts at 0 and shifted by 0x2f because the font array has 1 character before 0 (space)
       }
       else if ((int)currentCharacter == 0x20) //space
       {
           fontPosition = 0;
       }
       else if ((int)currentCharacter == 0x2d) //hyphen (minus sign)
       {
           fontPosition = 11;
       }
       else if ((int)currentCharacter == 0x2e) //dot
       {
           fontPosition = 12;
       }

        printCharacter_Large(pageStart, column, fontPosition); //Print the character

        column += 12; //Move the cursor to the next position (1 character is 12 px wide)

        if(column > 72-12) //Handle wrapping the text
        {
            column = columnStart; //If the character would not fit in the current line...
            pageStart += 3; //... start a new line
        }

        if(pageStart > 4)
        {
            break; //Neglect everything which exceeds the total printable area on the display (jump out from the whole code)
        }
        //Note: The last 2 if() can be skipped because we intend to print fixed width series of characters

    }

}

void printADCChannel1()
{
    char buffer[16]; //Character that holds the numbers that are printed on the display.

    float adcVoltage = 3.636 * (vref * OLEDBuffer[0])/1024.0f; //Conversion from ADC bits to voltage
    //3.636 comes from the voltage divider (R1 = 10k, R2 = 3.9k).

    int wholePart = (int) adcVoltage; //3.1415 -> 3
    int decimalPart = (int)((adcVoltage-wholePart)*100); //(3.1415-3)*100 = (0.1415)*100 = 14.15 -> (int)14.15 = 14

    sprintf(buffer, "%d.%02d", wholePart, decimalPart); //Format the float into the buffer "3.14"

    printNumber((uint8_t *)buffer, 0, 10); //print the contents of the buffer in the 1st line (0) and indent it by 10 pixels (10)
}

void printADCChannel2()
{
    char buffer[16];

    float adcVoltage = 3.636 * (vref * OLEDBuffer[1])/1024.0f;

    int wholePart = (int) adcVoltage;
    int decimalPart = (int)((adcVoltage-wholePart)*100);

    sprintf(buffer, "%d.%02d", wholePart, decimalPart); // Format the float into the buffer

    printNumber((uint8_t *)buffer, 3, 10); //print the contents of the buffer in the 4th line (3) and indent it by 10 pixels (10)
}

uint16_t ADC_ReadChannel(uint8_t channel)
{
    ADC_RegularChannelConfig(ADC1, channel, 1, ADC_SampleTime_3Cycles); //Set up the ADC to sample a certain channel

    ADC_SoftwareStartConvCmd(ADC1, ENABLE); //Start the conversion

    while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC)); //Wait for the result

    uint16_t result = ADC_GetConversionValue(ADC1); //Pass the result to a local variable

    return result; //Return the result
}

void ReadMultipleChannels()
{
    OLEDBuffer[0] = ADC_ReadChannel(ADC_Channel_0); //Read A0 (PA2)
    Delay_Ms(20);

    OLEDBuffer[1] = ADC_ReadChannel(ADC_Channel_2); //Read A2 (PC4)
    Delay_Ms(20);
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

    bitbangI2C_Init(); //initialize bit-banged GPIO-I2C pins
    initializeOLED(); //initialize the display
    ADC_Multichannel_Init(); //initialize the ADC
    Delay_Ms(2000);
    eraseDisplay(); //Erase the display
    Delay_Ms(2000);

    while(1)
    {
        ReadMultipleChannels();
        printADCChannel1();
        printADCChannel2();
    }
}

/*
 *CH32V003F4P6 - DS18B20 digital thermometer
 *https://curiousscientist.tech/blog/ch32v003f4p6-ds18b20-thermometer
 */
 
#include "debug.h"
#include "stdlib.h"

/* Global define */

/* Global Variable */


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

void ds18b20_set_pin_mode(uint8_t output)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2; //PD2
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; //50 MHz

    if (output == 1) //If output == 1 (output)
    {
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD; //Output output-drain (writing)
    } 
    else if(output == 0) //If output == 0 (input)
    {
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; //Input floating (release + reading)
    }

    GPIO_Init(GPIOD, &GPIO_InitStructure);
    //Note: Add a pull-up resistor to the line!
}

uint8_t ds18b20_init(void) //See Figure 15
{
    uint8_t presence; //presence byte to see if chip is detected

    //Reset pulse
    ds18b20_set_pin_mode(1); //Set output
    GPIO_WriteBit(GPIOD, GPIO_Pin_2, Bit_RESET); //Pull bus LOW
    Delay_Us(480); //Wait at least 480 us
    //------------------------------------------
    //Presence pulse
    ds18b20_set_pin_mode(0); //Set input (release bus)
    Delay_Us(30); //Wait 30 us (DS: 15-60 us)
    presence = !GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_2); // 0 = presence of chip is confirmed 
    
    Delay_Us(450); //480-30 = 450
    return presence;
}

void ds18b20_write_bit(uint8_t bit) //Figure 16
{
    ds18b20_set_pin_mode(1); //Set output

    GPIO_WriteBit(GPIOD, GPIO_Pin_2, Bit_RESET); //Pull bus LOW

    if (bit == 1) //Master write "1" slot 
    {
        Delay_Us(5); //Wait 5 us (DS: 15 us)
        ds18b20_set_pin_mode(0);  //Release early
        Delay_Us(55); //Wait 55 us (DS: 15+15+30-5)
    } 
    else //Master write "0" slot
    {
        Delay_Us(60); //Wait 60 us (DS: 15+15+30)
        ds18b20_set_pin_mode(0);  //Release late
        Delay_Us(1); //Wait 1 us
    }
}

uint8_t ds18b20_read_bit(void)
{
    uint8_t bit = 0;

    ds18b20_set_pin_mode(1); //Set output
    GPIO_WriteBit(GPIOD, GPIO_Pin_2, Bit_RESET); //Pull bus LOW

    Delay_Us(6); //Wait a bit for the bit to settle

    ds18b20_set_pin_mode(0); //Set input - Release
    Delay_Us(6); //Wait, but not more than 15 us!
    bit = GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_2); //Read the bus
    Delay_Us(48); //Wait 48 us (60-12 = 48)

    return bit;
}

void ds18b20_write_byte(uint8_t byte)
{
    for(int i = 0; i < 8; i++) 
    {
        ds18b20_write_bit(byte & 0x01); //Extract the LSB of the byte and write it on the bus
        byte >>= 1; // Shift the byte so in the next iteration, there will be a new LSB
    }
}

uint8_t ds18b20_read_byte(void)
{
    uint8_t byte = 0; //Byte read from the chip

    for(int i = 0; i < 8; i++) 
    {
        byte >>= 1; //shift the current bit in by one (no effect on the first bit, obviously)
        if(ds18b20_read_bit() == 1) //Read the bit and if it is 1
        {
            byte |= 0x80; //Set MSB to 1
        }             
        //In each iteration, the leftmost number gets shifted to the right by 1. After the for, the first incoming number becomes the lowest bit
        //1000 0000 -> 0000 0001
    }
    return byte;
}

int16_t ds18b20_get_temperature_raw(void)
{
    if (ds18b20_init() == 0) //If the initialization was unsuccesful
    {
        printf("Error");
        return -10000; //error
    } 
        
    ds18b20_write_byte(0xCC); //Skip ROM
    ds18b20_write_byte(0x44); //Convert T
    ds18b20_set_pin_mode(0);  //Release bus so line goes HIGH

    // Wait for conversion (750 ms max for 12-bit)
    while(!ds18b20_read_bit());

    ds18b20_init(); //Initialize again
    ds18b20_write_byte(0xCC); //Skip ROM
    ds18b20_write_byte(0xBE); //Read Scratchpad

    uint8_t temp_lsb = ds18b20_read_byte();
    uint8_t temp_msb = ds18b20_read_byte();

    return (int16_t)((temp_msb << 8) | temp_lsb); //raw 16-bit value reconstructed from LSB and MSB
}

void printFloatTemp(int16_t raw)
{
    int32_t scaled = (int32_t) raw * 10 / 16; //Division by 16 is due to the selected resolution (12-bit)
    int whole = (int)scaled / 10; //Extract the whole part
    int frac = (int)abs(scaled % 10); //Extract the fractional part
    printf("%d.%01d°C\n", whole, frac); //just printing 1 digit, we aren't that accurate anyway... (+/-0.5°C typically)
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

    Delay_Ms(3000);

    printf("CH32V003F4P6 - DS1820B - 1-Wire\n");

    if(ds18b20_init() == 1)
    {
        //Proceed
    }
    else 
    {
        printf("Chip presence was not detected! Check the wiring!\n");
        while(1){};
    }

    while(1)
    {
        int16_t raw = ds18b20_get_temperature_raw();
        printf("raw: %d\n", raw);
        printFloatTemp(raw);

        Delay_Ms(1000);
    }
}

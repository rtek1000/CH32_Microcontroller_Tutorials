/*
    CH32V003F4P6 - SPI SD card using FatFs
    https://www.curiousscientist.tech/blog/ch32v003f4p6-spi-sd-card-fatfs
*/

#include "debug.h"
#include "../User/SDCard/spi.h"
#include "../User/SDCard/sd.h"
#include "../User/SDCard/ff.h"
#include "stdlib.h"
#include "string.h"

/* Global define */
#define DEBUG_ 1 //0 for NOT printing debug printf(), 1 for printing all printf()

#if DEBUG_
  #include <stdio.h>
  #define DBG_PRINTF(...)   printf(__VA_ARGS__)
#else
  #define DBG_PRINTF(...)   do { } while (0)
#endif

//SD card detect port and pin - Some card modules have their CD port available
#define SD_DETECT_PORT  GPIOC
#define SD_DETECT_PIN   GPIO_Pin_4

/* Global Variable */
FATFS fs;                     //FatFs File system object
FIL filewrite;                //File object 
FIL fileread;                 //File object 
FRESULT result;               //File operation results 
UINT fnum;                    //Number of successful file write/read 
const char msg[] = "CH32V003F4P6 - SD Card with FatFS0.15a + DS18B20 temperature logger!\n"; //Welcome message
uint16_t buflen;        //Buffer length used for the thermometer
char buf[16];           //Thermometer data buffer
char line[64];          //SD read/write line buffer
uint8_t counter = 0;    //Counter for writing only a limited amount of data in the while(1)


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

    if (output == 1) //If output 1 (output)
    {
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD; //Output output-drain (writing)
    } 
    else if(output == 0) //If output 0 (input)
    {
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; //Input floating (reading)
    }

    GPIO_Init(GPIOD, &GPIO_InitStructure);
    //Note: Add a pull-up resistor to the line
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
    ds18b20_set_pin_mode(0); //Set input
    Delay_Us(30); //Wait 30 us (DS: 15-60 us)
    presence = !GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_2); // 0 = presence 
    
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
        DBG_PRINTF("Error");
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

    return (int16_t)((temp_msb << 8) | temp_lsb); // raw 16-bit value
}

int printFloatTemp(int16_t raw, char *buf, size_t buflen)
{
    int32_t scaled = (int32_t) raw * 10 / 16;   //calculate the scaled Celsius value at max resolution (12-bit)
    int whole = (int)scaled / 10;               //Get the whole part of the float
    int frac = (int)abs(scaled % 10);           //Get the fractional part of the float

    return snprintf(buf, buflen, "%d.%01d\n", whole, frac); //Return with the length of the float "string" + pass the values to the buf buffer
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

    if(DEBUG_ == 1) //Don't use the USART when debug is not enabled
    {
        USART_Printf_Init(115200); //When used as a standalone board, remove USART, it needs like 2 kBytes...
        USARTx_CFG();
    }

    SD_SetChipDetect(SD_DETECT_PORT, SD_DETECT_PIN); //Define chip detect
    //Here I cheat because I don't have CD pin, so I read the CS which should have 1 in the beginning

    DBG_PRINTF("CH32V003F4P6 - Part 12 - SPI SD Card\n"); //Print some welcome message
    DBG_PRINTF("Insert SD Card in the next 10 seconds!\n");

    for(int i = 0; i < 10; i++) //"Visual countdown"
    {
        Delay_Ms(1000);
        DBG_PRINTF("."); //Print a new point on the serial terminal every second
    }

    uint8_t cardDetect = !SD_Detect(SD_DETECT_PORT, SD_DETECT_PIN); //present == 1, not present == 0
    DBG_PRINTF("Card: %d\n", cardDetect); 

    if(!cardDetect) //Check if the card is detected
    {
        DBG_PRINTF("\nSD card not detected!\n");     
        while(1){}; //let the code stall here   
    }
    else
    {
        DBG_PRINTF("\nSD card detected!\n");

        if(SD_Initialize()) //After successful insertion detection, check for successful init()
        {
            DBG_PRINTF("SD card error!\n");
            Delay_Ms(500);
        }
        else
        {
            DBG_PRINTF("SD card initialized!\n");
            uint32_t sd_size = SD_GetSectorCount();//get the number of sectors
            DBG_PRINTF("SD Card size: %d MB.\n", sd_size >> 11);

            SD_HighSpeed(); //Set the SD card to a higher speed after initializing the card
        }
    }   

    result = f_mount(&fs,"0:",1); //Mount the file system
    //--
    if(result == FR_NO_FILESYSTEM)
    {
        DBG_PRINTF("The SD card has no file system...\n");
    }
    else if(result!=FR_OK)
    {
        DBG_PRINTF("SD card file system mounting failed.(%d)\n",result); //Print the error message together with the result to see the possible error
        while(1); //Get the code stuck here to prevent even more failures and error messages
    }
    else
    {
        DBG_PRINTF("Successful mounting!\n");
    }

    Delay_Ms(5000);
    DBG_PRINTF("READ TEST: \n");

    result = f_open(&fileread, "0:READTEST.TXT", FA_READ);  //Open the file we prepared on the SD card for reading. Note: file name length must be <8 characters
    if (result != FR_OK) 
    { 
        DBG_PRINTF("f_open failed: %u\n", result);
    }

    while (f_gets(line, sizeof(line), &fileread)) 
    {              
        DBG_PRINTF("%s", line); //Print each line with 1 second delay between
        Delay_Ms(1000);
    }

    f_close(&fileread); //Close the file

    Delay_Ms(5000);
    DBG_PRINTF("\nWRITE TEST: \n");

    result = f_open(&filewrite, "0:Writetes.txt", FA_CREATE_ALWAYS | FA_WRITE ); //Create (overwrite) a new file with the name

    if ( result == FR_OK )
    {
        DBG_PRINTF("Succesful write operation!\n");
        result=f_write(&filewrite, msg, strlen(msg), &fnum); //Print our message in the file

        if(result==FR_OK)
        {
            DBG_PRINTF("File written successfully, byte data written: %d\n",fnum);
        }
        else
        {
            DBG_PRINTF("Writing in the file has failed: (%d)\n",result); //Print the error message with the potential reason for the error
        }

        f_close(&filewrite); //Close the file
    }
    else
    {
        DBG_PRINTF("Failed to open/create file\n");
    }    

    while(1)
    {
        if(counter < 10) //Only print 10 times
        {
            DBG_PRINTF("Update.\n");

            result = f_open(&filewrite, "0:Writetes.txt", FA_OPEN_ALWAYS | FA_WRITE); //Open the previously created file for writing
            if (result != FR_OK) 
            { 
                DBG_PRINTF("Error writing..."); 
                continue; 
            }

            result = f_lseek(&filewrite, f_size(&filewrite)); //Find the end of the file ("append")
            if (result != FR_OK) 
            { 
                f_close(&filewrite);
                continue;
            }          

            int16_t raw = ds18b20_get_temperature_raw(); //Fetch the raw temperature reading
            int length = printFloatTemp(raw, line, sizeof(line)); //Calculate the floating point temperature and return with the length of the number from the conversion            
            result = f_write(&filewrite, line, length, &fnum);    //Write the conversion on the SD card
            if (result != FR_OK || fnum < (UINT)length) 
            { 
                DBG_PRINTF("Error writing..."); 
                continue;             
            }
            
            f_close(&filewrite); //Close the file
            counter++; //Increase the counter
            Delay_Ms(1000); //Wait 1 second
        }
        else
        {
        
        }
    }
}

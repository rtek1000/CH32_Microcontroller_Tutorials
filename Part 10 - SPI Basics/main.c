/*
 *CH32V003F4P6 - SPI Basics
 *https://curiousscientist.tech/blog/ch32v003f4p6-spi-communication-with-adxl345
 */

#include "debug.h"
#include <stdlib.h> //Include this for more math stuff

/* Global define */
//SPI PINS
//MISO: PC7
//MOSI: PC6
//SCK: PC5
//CS*: PC4 (CS can be arbitrary)

/* Global Variable */
uint8_t buffer[10]; //Buffer for the bytes received from the accelerometer

void USARTx_CFG(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure = {0};
    USART_InitTypeDef USART_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_USART1, ENABLE);

    /* USART1 TX-->D.5   RX-->D.6 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5; //PD5
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6; //PD6
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

void SPI_Initialize(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    SPI_InitTypeDef  SPI_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_SPI1, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4; //PC4 - CS
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; //Master-driven (master sets CS), so it is push-pull
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    GPIO_WriteBit(GPIOC, GPIO_Pin_4, Bit_SET);  //PC4 - CS -> HIGH

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6; //PC5 - SCK //PC6 - MOSI (SDA)
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; //Master-driven (master drives SCK and SDA), so it is push-pull
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7; //PC7 - MISO (SDO)
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; //Slave-driven ("master-IN"), so it must float
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex; //Bidirectional communication, slave sends back data
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master; //The microcontroller is the master
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b; //8-bit data size because the ADXL345 has 8 bit registers
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_High; //Clock polarity (idle level of SCK)
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge; //Second clock edge is used to sample and shift data 
    //CPOL is high, so 1st transition is H->L, and the 2nd is L->H. This is to fulfil the IMU's requirements
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft; //Bit-banged chip select (CS)
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_32; //1.5 MHz bus
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB; //MSB-first protocol
    SPI_Init(SPI1, &SPI_InitStructure);

    SPI_Cmd(SPI1, ENABLE);
}

uint8_t SPI_Transfer(uint8_t data) //Send a byte to the slave and return a byte
{    
    SPI_I2S_SendData(SPI1, data); //Send the byte 

    while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET); //Wait until the buffer is transferred   
    
    while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET); //Wait until the buffer is received

    return SPI_I2S_ReceiveData(SPI1); //Return the received byte
}

void ADXL345_WriteRegister(uint8_t registerAddress, uint8_t registerValue) //Write a selected register
{
    //Set CS LOW
    GPIO_WriteBit(GPIOC, GPIO_Pin_4, Bit_RESET);  //PC4 - CS -> LOW

    SPI_Transfer(registerAddress);
    SPI_Transfer(registerValue);

    GPIO_WriteBit(GPIOC, GPIO_Pin_4, Bit_SET);  //PC4 - CS -> HIGH
}

void ADXL345_ReadRegister(uint8_t registerAddress, int numberOfBytes, uint8_t *buffer) //Read a selected register
{
    uint8_t commandByte = 0x80 | registerAddress; //Set D7 to 1 which means we want to READ (0x80 = 1000 0000) -> Datasheet R/W bit

    if(numberOfBytes > 1) //In case there are multiple bytes to read
    {
        commandByte |= 0x40; //Set D6 to 1 (0x40 = 0100 0000) -> Datasheet MB bit
    }
    //---------------------

    //Set CS LOW
    GPIO_WriteBit(GPIOC, GPIO_Pin_4, Bit_RESET); //PC4 - CS -> LOW

    SPI_Transfer(commandByte); //send the reconstructed command byte to the accelerometer

    for(int i = 0; i < numberOfBytes; i++)
    {
        buffer[i] = SPI_Transfer(0x00); //Shift out bytes, pass them to the i-th buffer item
    }

    GPIO_WriteBit(GPIOC, GPIO_Pin_4, Bit_SET); //PC4 - CS -> HIGH    
}

void printAccelG(int16_t raw)
{
    //raw/256.0 gives g; scaled = raw * 10000/256 gives fixed-point g*10000. We use 10000 because we want 4 digits
    //256 is the LSB/g unit, see Table 1 in the datasheet.
    int32_t scaled = (int32_t)raw * 10000 / 256; //Let's say raw = -300 LSB, Then this value becomes -11718
    int whole     =  (int)scaled / 10000; //-11718 / 10000 = -1 -> this is the whole part
    int frac      =  (int)abs(scaled % 10000); //abs(-11718 % 10000) = 1718
    printf("%d.%04d", whole, frac); //-1,1718 will be printed -> first %d is -1, second %04d is 1718 (04 -> 4 digits)
}

void ADXL345_ReadAcceleration()
{
    ADXL345_ReadRegister(0x32, 6, buffer); //Start reading from 0x32, read 6 bytes (3 axes, 2 bytes/axis), put it in the buffer

    int16_t x = (int16_t)(((int16_t)buffer[1]<<8) | buffer[0]); //Combine the bytes together into a single value
    int16_t y = (int16_t)(((int16_t)buffer[3]<<8) | buffer[2]);
    int16_t z = (int16_t)(((int16_t)buffer[5]<<8) | buffer[4]);
    
    //Print for Arduino IDE's serial plotter
    printf("X:");
    printAccelG(x);
    printf(",Y:");
    printAccelG(y);
    printf(",Z:");
    printAccelG(z);
    printf("\r\n");
}

void ADXL345_Init()
{
    ADXL345_WriteRegister(0x2D, 0x08); //Power 
    // D7 D6 D5 D4 D3 D2 D1 D0
    // 0  0  0  0  1  0  0  0
    Delay_Ms(100);

    ADXL345_WriteRegister(0x31, 0x08); //Data format
    // D7 D6 D5 D4 D3 D2 D1 D0
    // 0  0  0  0  1  0  0  0    
    Delay_Ms(100);

    ADXL345_WriteRegister(0x2C, 0x0F); //BW_Rate
    // D7 D6 D5 D4 D3 D2 D1 D0
    // 0  0  0  0  1  1  1  1    
    Delay_Ms(100);

    //Discover chip: read 0x00 DEVID register and it should respond 0xE5 (datasheet page 24)   
    uint8_t who; 
    ADXL345_ReadRegister(0x00, 1, &who);

    if(who == 0xE5) //Check if the chip responds with the value from the datasheet
    {
        printf("Chip recognized - %d.\n", who);
    }
    else 
    {
        printf("Chip not recognized - %d.\n", who);    
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
    SPI_Initialize(); //Initialize SPI communication
    IntPinsInit(); //Initialize the 2 INT pins of the ADXL345 accelerometer
    ADXL345_Init(); //Set up the accelerometer
  
    printf("CH32V003F4P6 - DEMO 10 - SPI Communication using ADXL345 accelerometer\n");    

    Delay_Ms(3000);
    
    while(1)
    {
         ADXL345_ReadAcceleration();
         Delay_Ms(10);
    }
}

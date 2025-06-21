#include "sd.h"
#include "spi.h"
#include "ch32v00x_gpio.h"

uint8_t  SD_Type = 0; //SD card type
GPIO_TypeDef* SD_CD_PORT;
uint16_t      SD_CD_PIN;

//Low speed to initialize SD card
void SD_LowSpeed(void)
{
    SPI1_SetSpeed(SPI_BaudRatePrescaler_256);
}

//High speed to run SD card
void SD_HighSpeed(void)
{
    SPI1_SetSpeed(SPI_BaudRatePrescaler_4);    
}

//SD card insertion detection - User can select the CD pin - Not be mistaken with chip select!
uint8_t SD_Detect(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin_x)
{
    return (GPIO_ReadInputDataBit(GPIOx, GPIO_Pin_x) == Bit_RESET); //0 - no card present, 1 - card present
}

void SD_SetChipDetect(GPIO_TypeDef* port, uint16_t pin) //Call before SD_Detect()!
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};    

    if(port == GPIOA)
    {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    }
    else if(port == GPIOC)
    {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    }
    else if(port == GPIOD)
    {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
    }
    //Note: These are relevant for CH32V003xxxx which does not have GPIOB!

    GPIO_InitStructure.GPIO_Pin = pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; //Remember checking the datasheet of the module and add optional pull-up resistor
    GPIO_Init(port, &GPIO_InitStructure);

    SD_CD_PORT = port;
    SD_CD_PIN  = pin;
}

//SPI Initialization
void SD_SPI_Init(void)
{
    SPI1_Init();
    SD_ChipSelect_High;
}

//Unselect SD and release SPI bus
void SD_Deselect(void)
{
    SD_ChipSelect_High;
    SPI_TransferByte(0xff);
}

//Select SD card
uint8_t SD_Select(void)
{
    SD_ChipSelect_Low;
    
    if(SD_WaitReady() == 0)
    {
         return 0;//Wait for successful connection
    }

    SD_Deselect();
    return 1; //Failure
}

//Wait for SD card to be ready
uint8_t SD_WaitReady(void)
{
    uint32_t t=0;
    do
    {
        //While the card does not return 0xFF, we keep running the loop. Once the card is not busy, it returns 0xFF and the if() is performed
        if(SPI_TransferByte(0xff) == 0xff)
        {
            return 0; //jump out when the 0xff is received
        }
        t++;
    }while(t<0XFFFFFF); 

    //Return 1 when timeout
    return 1;
}

//Get response from SD card
uint8_t SD_GetResponse(uint8_t response)
{
    uint16_t Count=0xFFFF; //Waiting time

    while ((SPI_TransferByte(0XFF)!=response)&&Count)
    {
        Count--; //Count down (wait)
    }

    if (Count==0)
    {
        return MSD_RESPONSE_FAILURE; //Failed to get response
    }
    else
    {
         return MSD_RESPONSE_NO_ERROR; //OK response
    }
}

//Receive data
uint8_t SD_ReceiveData(uint8_t *buffer, uint16_t length)
{
    if (SD_GetResponse(0xFE))
    {
        return 1;    //wait for data token 0xFE
    } 

    //Polling version йдйд
    for (u16 i = 0; i < length; i++) 
    {
        buffer[i] = SPI_TransferByte(0xFF);
    }

    //discard CRC
    SPI_TransferByte(0xFF);
    SPI_TransferByte(0xFF);
    return 0;   //success
}

//Send block
uint8_t SD_SendBlock(uint8_t *buffer, uint8_t command)
{
    uint16_t response;
    if (SD_WaitReady()) 
    {
        return 1;          //wait until card is ready
    }

    SPI_TransferByte(command);           //send data token

    //Polling version
    for (u16 i = 0; i < 512; i++) 
    {
        SPI_TransferByte(buffer[i]);
    }

    //dummy CRC
    SPI_TransferByte(0xFF);
    SPI_TransferByte(0xFF);

    //data response
    response = SPI_TransferByte(0xFF);
    if ((response & 0x1F) != 0x05)
    {
        return 2;   //data accepted token is 0x05
    }

    //wait until write complete (bus idle = 0xFF)
    while (SPI_TransferByte(0xFF) == 0);
    return 0;   //success
}

uint8_t SD_SendCommand(uint8_t command, uint32_t argument, uint8_t crc)
{
    uint8_t r1;
    uint8_t retry=0;
    SD_Deselect();

    if(SD_Select())
    {
        return 0XFF;
    }

    SPI_TransferByte(command | 0x40);
    SPI_TransferByte(argument >> 24);
    SPI_TransferByte(argument >> 16);
    SPI_TransferByte(argument >> 8);
    SPI_TransferByte(argument);
    SPI_TransferByte(crc);
    if(command==CMD12)
    {
        SPI_TransferByte(0xff);//Skip a stuff byte when stop reading
    }
    
    retry=0X1F;
    do
    {
        r1=SPI_TransferByte(0xFF);
    }while((r1&0X80) && retry--);
    
    return r1;
}

//Get SD card's CID
uint8_t SD_GetCID(uint8_t *cid_data)
{
    uint8_t r1;
    r1=SD_SendCommand(CMD10,0,0x01);
    if(r1==0x00)
    {
        r1=SD_ReceiveData(cid_data,16); //Receive 16 bytes
    }
    SD_Deselect();

    if(r1)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

//Get SD card's CSD
uint8_t SD_GetCSD(uint8_t *csd_data)
{
    uint8_t r1;
    r1=SD_SendCommand(CMD9,0,0x01);
    if(r1==0)
    {
        r1=SD_ReceiveData(csd_data, 16);
    }
    SD_Deselect();
    if(r1)return 1;
    else return 0;
}


uint32_t SD_GetSectorCount(void)
{
    uint8_t csd[16];
    uint64_t Capacity;
    uint8_t n;
    uint16_t csize;

    if(SD_GetCSD(csd)!=0) 
    {
        return 0;
    }
    if((csd[0]&0xC0)==0x40)  //SDHC card
    {
        csize = csd[9] + ((uint16_t)csd[8] << 8) + 1;
        Capacity = (uint32_t)csize << 10;
    }
    else
    {
        n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
        csize = (csd[8] >> 6) + ((uint16_t)csd[7] << 2) + ((uint16_t)(csd[6] & 3) << 10) + 1;
        Capacity= (uint32_t)csize << (n - 9);
    }
    return Capacity;
}


uint8_t SD_Initialize(void)
{
    uint8_t r1;     
    uint16_t retry = 20;  
    uint8_t buf[4];
    uint16_t i;

    SD_SPI_Init();    

    for(i = 0; i < 10; i++)
    {
        SPI_TransferByte(0XFF);
    }

    do
    {
        r1=SD_SendCommand(CMD0,0,0x95);
    }while((r1!=0X01) && retry--);

    SD_Type=0;
    
    if(r1==0X01)
    {
        if(SD_SendCommand(CMD8,0x1AA,0x87)==1)//SD V2.0
        {
            for(i=0;i<4;i++)
            {
                buf[i]=SPI_TransferByte(0XFF);  
            }
            if(buf[2]==0X01&&buf[3]==0XAA)
            {
                retry=0XFFFE;
                do
                {
                    SD_SendCommand(CMD55,0,0X01);   
                    r1=SD_SendCommand(CMD41,0x40000000,0X01);
                }while(r1&&retry--);

                if(retry&&SD_SendCommand(CMD58,0,0X01) == 0)
                {
                    for(i=0;i<4;i++)
                    {
                        buf[i]=SPI_TransferByte(0XFF);
                    }

                    if(buf[0]&0x40)
                    {
                        SD_Type=SD_TYPE_V2HC;
                    }   
                    else
                    {
                        SD_Type=SD_TYPE_V2;
                    }
                }
            }
        }else//SD V1.x / MMC V3
        {
            SD_SendCommand(CMD55,0,0X01);       
            r1=SD_SendCommand(CMD41,0,0X01);    
            if(r1<=1)
            {
                SD_Type=SD_TYPE_V1;
                retry=0XFFFE;
                do 
                {
                    SD_SendCommand(CMD55,0,0X01);  
                    r1=SD_SendCommand(CMD41,0,0X01);
                }while(r1&&retry--);
            }else
            {
                SD_Type=SD_TYPE_MMC;
                retry=0XFFFE;
                do 
                {
                    r1=SD_SendCommand(CMD1,0,0X01);
                }while(r1&&retry--);
            }
            if(retry==0||SD_SendCommand(CMD16,512,0X01)!=0)
            {
                SD_Type=SD_TYPE_ERR;
            }
        }
    }
    SD_Deselect();

    if(SD_Type)
    {        
        return 0;
    }
    else if(r1)
    {
        return r1;
    }
    else
    {
        return 0xAA;
    }
}

uint8_t SD_ReadDisk(uint8_t*buffer,uint32_t sector,uint8_t count)
{
    uint8_t r1;
    if(SD_Type!=SD_TYPE_V2HC)sector <<= 9;

    if(count==1)
    {
        r1=SD_SendCommand(CMD17,sector,0X01);
        if(r1==0)
        {
            r1=SD_ReceiveData(buffer,512);
        }
    }else
    {
        r1=SD_SendCommand(CMD18,sector,0X01);
        do
        {
            r1=SD_ReceiveData(buffer,512);
            buffer+=512;
        }while(--count && r1==0);
        SD_SendCommand(CMD12,0,0X01); 
    }
    SD_Deselect();
    return r1;
}

uint8_t SD_WriteDisk(uint8_t*buffer,uint32_t sector,uint8_t cnt)
{
    uint8_t r1;
    if(SD_Type!=SD_TYPE_V2HC)sector *= 512;
    if(cnt==1)
    {
        r1=SD_SendCommand(CMD24,sector,0X01);
        if(r1==0)
        {
            r1=SD_SendBlock(buffer,0xFE);
        }
    }else
    {
        if(SD_Type!=SD_TYPE_MMC)
        {
            SD_SendCommand(CMD55,0,0X01);
            SD_SendCommand(CMD23,cnt,0X01);
        }
        r1=SD_SendCommand(CMD25,sector,0X01);
        if(r1==0)
        {
            do
            {
                r1=SD_SendBlock(buffer,0xFC);
                buffer+=512;
            }while(--cnt && r1==0);

            r1=SD_SendBlock(0,0xFD);
        }
    }
    SD_Deselect();
    return r1;
}

/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */
#include "sd.h"			/* SD card communication */
#include "spi.h"		/* SPI configuration */

/* Definitions of physical drive number for each drive */
#define DEV_MMC		0 // Example: Map MMC/SD card to physical drive 0

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{	
	//Not implemented
	return 0;
}

/*-----------------------------------------------------------------------*/
/* Initialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{	
	uint8_t result = 1;
	
	result = SD_Initialize(); //SD_Initialize()

	if(result)
	{
		SPI_TransferByte(0xff);
	}

	if(result)
    {
    	return STA_NOINIT; //Initialization failed
    }
    else
    {
    	return 0; //Successful initialization
    } 
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{	
	uint8_t result = 1;

	result = SD_ReadDisk(buff,sector,count);

	if(result)
	{
		SPI_TransferByte(0xff);
	}

	if(!result)
    {
        return RES_OK;
    }
    else
    {
        return RES_ERROR;
    }
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0
DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	uint8_t result = 0;

	result = SD_WriteDisk((u8*)buff,sector,count);

	if(!result)
    {
        return RES_OK;
    }
    else
    { 
        return RES_ERROR;
    }
}
#endif

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res = RES_ERROR;

    // Process of the command for the MMC/SD card
    switch(cmd)
    {
        case CTRL_SYNC:
            SD_ChipSelect_Low;
            if(SD_WaitReady()==0)res = RES_OK;
            else res = RES_ERROR;
            SD_ChipSelect_High;
            break;
        case GET_SECTOR_SIZE:
            *(WORD*)buff = 512;
            res = RES_OK;
            break;
        case GET_BLOCK_SIZE:
            *(WORD*)buff = 8;
            res = RES_OK;
            break;
        case GET_SECTOR_COUNT:
            *(DWORD*)buff = SD_GetSectorCount();
            res = RES_OK;
            break;
        default:
            res = RES_PARERR;
            break;
    }
	return res;
}

DWORD get_fattime (void)
{
    return 0;
}
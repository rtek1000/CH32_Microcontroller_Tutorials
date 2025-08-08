/*
 *CH32V006K8U6 - TSL2591 Light Sensor
 *https://curiousscientist.tech/blog/ch32v006k8u6-with-tsl2591-light-sensor
 */

#ifndef TSL2591_H
#define TSL2591_H

#include <stdint.h>

#define TSL2591_COMMAND_BIT 0xA0    //Command byte for the sensor (0b10100000) - CMD = 1, Transaction = 01 (normal operation)
#define TSL2591_ENABLE_REG 0x00     //Enable register
#define TSL2591_CONTROL_REG 0x01    //ALS gain and integration time configuration (AGAIN and ATIME)
#define TSL2591_AILTL 0x04          //ALS interrupt low threshold low byte
#define TSL2591_AILTH 0x05          //ALS interrupt low threshold high byte
#define TSL2591_AIHTL 0x06          //ALS interrupt high threshold low byte
#define TSL2591_AIHTH 0x07          //ALS interrupt high threshold high byte
#define TSL2591_NPAILTL 0x08        //No Persist ALS interrupt low threshold low byte
#define TSL2591_NPAILTH 0x09        //No Persist ALS interrupt low threshold high byte
#define TSL2591_NPAIHTL 0x0A        //No Persist ALS interrupt high threshold low byte
#define TSL2591_NPAIHTH 0x0B        //No Persist ALS interrupt high threshold high byte
#define TSL2591_PERSIST 0x0C        //Interrupt persistence filter
#define TSL2591_PackageID 0x11      //Package ID
#define TSL2591_DeviceID 0x12       //Device ID (0x50)
#define TSL2591_STATUS 0x13         //status
#define TSL2591_C0DATAL  0x14       //CH0 ADC low data byte 
#define TSL2591_C0DATAH  0x15       //CH0 ADC high data byte 
#define TSL2591_C1DATAL  0x16       //CH1 ADC low data byte 
#define TSL2591_C1DATAH  0x17      //CH1 ADC high data byte

//Enable register defines
#define TSL2591_POFF    0x00  //Power OFF
#define TSL2591_PON     0x01  //Bit 0: Power ON
#define TSL2591_AEN     0x02  //Bit 1: ALS Enable
#define TSL2591_AIEN    0x10  //Bit 4: ALS Interrupt Enable
#define TSL2591_SAI     0x40  //Bit 6: Sleep After Interrupt
#define TSL2591_NPIEN   0x80  //Bit 7: No Persist Interrupt Enable

//Integration time options (100 ms * (2^it)), codes for CONTROL register bits 2:0
typedef enum {
    TSL2591_INTEGRATION_100MS = 0x00,
    TSL2591_INTEGRATION_200MS = 0x01,
    TSL2591_INTEGRATION_300MS = 0x02,
    TSL2591_INTEGRATION_400MS = 0x03,
    TSL2591_INTEGRATION_500MS = 0x04,
    TSL2591_INTEGRATION_600MS = 0x05
} tsl2591_integration_time_t;

//Gain options, codes for CONTROL register bits 5:4
typedef enum {
    TSL2591_GAIN_LOW  = 0x00,  //1
    TSL2591_GAIN_MED  = 0x10,  //25
    TSL2591_GAIN_HIGH = 0x20,  //428
    TSL2591_GAIN_MAX  = 0x30   //9876
} tsl2591_gain_t;

extern tsl2591_integration_time_t _integration;
extern tsl2591_gain_t _gain;

//Initialize the TSL2591 (power up, default gain/time)
void TSL2591_Init(void);

//Power control and interrupt control
void TSL2591_Enable(void);
void TSL2591_Disable(void);
void TSL2591_EnableALSInterrupt(uint16_t lowThreshold, uint16_t highThreshold, uint8_t persistence);
void TSL2591_EnableNoPersistInterrupt(uint16_t lowThreshold, uint16_t highThreshold);
void TSL2591_ClearInterrupt(void);

//Configure integration time (in register units) and gain, status, thresholds
void TSL2591_setGainAndIntegrationTime(tsl2591_gain_t gain, tsl2591_integration_time_t integrationTime);
uint8_t TSL2591_GetIntegrationTime(void);
uint8_t TSL2591_GetGain(void);
uint8_t TSL2591_GetStatus(void);
uint8_t TSL2591_GetPersist(void);
void TSL2591_GetNoPersistThresholds(uint16_t *low, uint16_t *high);
void TSL2591_GetThresholds(uint16_t *low, uint16_t *high);
void TSL2591_SetPersist(uint8_t persist);
void TSL2591_SetNoPersistThresholds(uint16_t low, uint16_t high);
void TSL2591_SetThresholds(uint16_t low, uint16_t high);


uint32_t TSL2591_GetFullLuminosity(void); //Returns combined 32-bit: [HIGH16 bits = full-spectrum (CH0), LOW16 bits = IR (CH1)]
uint16_t TSL2591_GetVisibleLuminosity(void); //Visible light only (full - IR)

float TSL2591_CalculateLux(void); //Convert to lux using current gain and integration settings

#ifdef __cplusplus
}
#endif

#endif //TSL2591_H


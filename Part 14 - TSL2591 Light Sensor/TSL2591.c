/*
 *CH32V006K8U6 - TSL2591 Light Sensor
 *https://curiousscientist.tech/blog/ch32v006k8u6-with-tsl2591-light-sensor
 */

#include "i2c.h" //Home-made I2C library
#include "debug.h" //Delay
#include "TSL2591.h" //Header

tsl2591_integration_time_t _integration; //Locally stored integration time
tsl2591_gain_t _gain; //Locally stored gain

void TSL2591_Enable(void)
{   
    uint8_t tx[2]; //Local buffer for transmitted data
    tx[0] = TSL2591_COMMAND_BIT | TSL2591_ENABLE_REG; //Write the enable register
    tx[1] = TSL2591_PON | TSL2591_AEN; //Power ON and AEN ON

    I2C_SendByte(0x29, tx, 2); //Send the 2 bytes to the sensor

    Delay_Ms(100);
}

void TSL2591_Disable(void) //Sleep
{   
    uint8_t tx[2]; //Local buffer for transmitted data
    tx[0] = TSL2591_COMMAND_BIT | TSL2591_ENABLE_REG; //Write the enable register
    tx[1] = TSL2591_POFF; //Set PON to 0 to turn off the sensor

    I2C_SendByte(0x29, tx, 2); //Send the 2 bytes to the sensor

    Delay_Ms(100);
}

void TSL2591_EnableALSInterrupt(uint16_t lowThreshold, uint16_t highThreshold, uint8_t persistence)
{
    uint8_t tx[5];  
  
    tx[0] = TSL2591_COMMAND_BIT | TSL2591_AILTL; //Start at AILTL
    tx[1] = lowThreshold  & 0xFF;       //Low threshold LSB
    tx[2] = lowThreshold  >> 8;         //Low threshold MSB
    tx[3] = highThreshold & 0xFF;       //High threshold LSB
    tx[4] = highThreshold >> 8;         //High threshold MSB
    I2C_SendByte(0x29, tx, 5);    
    
    tx[0] = TSL2591_COMMAND_BIT | TSL2591_PERSIST; 
    tx[1] = persistence & 0x0F;  //Only lower 4 bits are valid
    I2C_SendByte(0x29, tx, 2);   

    tx[0] = TSL2591_COMMAND_BIT | TSL2591_ENABLE_REG;  //ENABLE register
    tx[1] = TSL2591_PON | TSL2591_AEN | TSL2591_AIEN; 
    I2C_SendByte(0x29, tx, 2);
}

void TSL2591_EnableNoPersistInterrupt(uint16_t lowThreshold, uint16_t highThreshold)
{
    uint8_t tx[5];

    tx[0] = TSL2591_COMMAND_BIT | TSL2591_NPAILTL;
    tx[1] = lowThreshold  & 0xFF;
    tx[2] = lowThreshold  >> 8;
    tx[3] = highThreshold & 0xFF;
    tx[4] = highThreshold >> 8;
    I2C_SendByte(0x29, tx, 5);
    
    tx[0] = TSL2591_COMMAND_BIT | TSL2591_ENABLE_REG;
    tx[1] = TSL2591_PON | TSL2591_AEN | TSL2591_NPIEN;
    I2C_SendByte(0x29, tx, 2);
}

void TSL2591_ClearInterrupt(void)
{
    uint8_t cmd = 0xE7;  //(0b11100111) CMD=1, Transaction=11, SF=00111 (clear all ALS INT)
    I2C_SendByte(0x29, &cmd, 1);
}


uint8_t TSL2591_GetGain(void)
{
    uint8_t configValue = 0; //Byte read from the config register

    uint8_t tx[1]; //Local buffer for transmitted data
    uint8_t rx[1]; //Local buffer for received data

    tx[0] = TSL2591_COMMAND_BIT | TSL2591_CONTROL_REG; //Write the control register

    I2C_SendByte(0x29, tx, 1); //Send a byte to the sensor
    I2C_ReceiveByte(0x29, rx, 1); //Receive the value of the control register

    configValue = rx[0]; //Pass the value to the local variable   
    configValue &= 0x30;  //Keep only the bits for gain (0x30 = 0b00110000)
    
    return configValue >> 4; //Return the shifted current gain value
}

uint8_t TSL2591_GetIntegrationTime(void)
{
    uint8_t configValue = 0; //Byte read from the config register

    uint8_t tx[1]; //Local buffer for transmitted data
    uint8_t rx[1]; //Local buffer for received data
    tx[0] = TSL2591_COMMAND_BIT | TSL2591_CONTROL_REG; //Write the control register
    I2C_SendByte(0x29, tx, 1); //Send a byte to the sensor
    I2C_ReceiveByte(0x29, rx, 1); //Receive the value of the control register
    configValue = rx[0]; //Pass the value to the local variable   

    configValue &= 0b00000111; //Keep the 2:0 bits only

    return configValue; //Return the kept bits (ATIME)
}

void TSL2591_setGainAndIntegrationTime(tsl2591_gain_t gain, tsl2591_integration_time_t integrationTime)
{
    uint8_t tx[2]; //Local buffer for transmitted data

    tx[0] = TSL2591_COMMAND_BIT | TSL2591_CONTROL_REG; //Write the control register
    tx[1] = integrationTime | gain; //Combine the values of the two values
    I2C_SendByte(0x29, tx, 2); //Send the 2 bytes to the sensor

    _integration = integrationTime; //Store both ATIME and AGAIN locally
    _gain = gain;
}

uint32_t TSL2591_GetFullLuminosity(void)
{
    Delay_Ms( (uint16_t)(_integration + 1) * 100 ); //Wait integration time amount of time. "+1" is because 100 ms = 0x00, 200 ms = 0x01...etc.
    
    uint8_t rx[2]; //Local buffer for received data
    uint8_t tx[1]; //Local buffer for transmitted data

    tx[0] = TSL2591_COMMAND_BIT | TSL2591_C0DATAL; //Read the C0DATAL (0x14) and eventually C0DATAH (auto-increment)
    I2C_SendByte(0x29, tx, 1); //Send the command
    I2C_ReceiveByte(0x29, rx, 2); //Receive the 2 bytes
    uint16_t full = ((uint16_t)rx[1] << 8) | rx[0];  //Compile the received bytes into a 16-bit value; FULL spectrum 

    
    tx[0] = TSL2591_COMMAND_BIT | TSL2591_C1DATAL; //Read C1DATAL (0x16) and eventually C1DATAH (auto-increment)
    I2C_SendByte(0x29, tx, 1); //Send the command
    I2C_ReceiveByte(0x29, rx, 2); //Receive the 2 bytes
    uint16_t ir = ((uint16_t)rx[1] << 8) | rx[0];  //Compile the received bytes into a 16-bit value; IR spectrum
    
    uint32_t luminosity = ((uint32_t)ir << 16) | full; //Pack the data [31:16]=IR, [15:0]=full
    
    return luminosity;
}


uint16_t TSL2591_GetVisibleLuminosity(void) 
{
    uint32_t luminosity  = TSL2591_GetFullLuminosity(); //Get the full luminosity that contains both channel's information

    uint16_t full = (uint16_t)(luminosity & 0xFFFF); //Extract the lower 16 bits of full luminosity which is CH0 full spectrum
    uint16_t ir   = (uint16_t)(luminosity >> 16); //Extract the upper 16 bits of full luminosity which is CH1 IR spectrum
   
    return (full > ir) ? (full - ir) : 0; 
    //If full > IR, return full-IR (visible spectrum), otherwise return 0. This avoids negative values. Light intensity cannot be negative!
}

uint16_t TSL2591_GetIRLuminosity(void)
{
    uint32_t luminosity = TSL2591_GetFullLuminosity();
    return (uint16_t)(luminosity >> 16); //Upper 16 bits = CH1 (IR)
}

float TSL2591_CalculateLux() //Inspired by the Adafruit library for Arduino
{
  uint32_t lum = TSL2591_GetFullLuminosity(); //Get the full luminosity
  uint16_t ir, full; //Separate the values into IR and visible
  ir = lum >> 16;
  full = lum & 0xFFFF;  

  uint16_t atime = 0; //Define ATIME and AGAIN locally
  uint16_t again = 0;

  float cpl, lux; //Define CPL and LUX

    switch (_integration) //Assign ATIME based on the selected integration time
    {
    case TSL2591_INTEGRATION_100MS:
        atime = 100;
        break;
    case TSL2591_INTEGRATION_200MS:
        atime = 200;
        break;
    case TSL2591_INTEGRATION_300MS:
        atime = 300;
        break;
    case TSL2591_INTEGRATION_400MS:
        atime = 400;
        break;
    case TSL2591_INTEGRATION_500MS:
        atime = 500;
        break;
    case TSL2591_INTEGRATION_600MS:
        atime = 600;
        break;
    default: //100ms
        atime = 100;
        break;
    }

    switch (_gain) //Assign AGAIN based on the selected gain value
    {
    case TSL2591_GAIN_LOW:
        again = 1;
        break;
    case TSL2591_GAIN_MED:
        again = 25;
        break;
    case TSL2591_GAIN_HIGH:
        again = 428;
        break;
    case TSL2591_GAIN_MAX:
        again = 9876;
        break;
    default:
        again = 1;
        break;
    }

   cpl = ((float)atime * (float)again) / 408.0F; //calculate CPL   
   lux = (((float)full - (float)ir)) * (1.0F - ((float)ir / (float)full)) / cpl; //calculate LUX
  
   return lux; //return LUX
}

void TSL2591_SetThresholds(uint16_t low, uint16_t high)
{
    uint8_t tx[3];

    //Write low threshold (0x04, 0x05)
    tx[0] = TSL2591_COMMAND_BIT | TSL2591_AILTL;
    tx[1] = (uint8_t)(low & 0xFF);      //Low byte
    tx[2] = (uint8_t)(low >> 8);        //High byte
    I2C_SendByte(0x29, tx, 3);

    //Write high threshold (0x06, 0x07)
    tx[0] = TSL2591_COMMAND_BIT | TSL2591_AIHTL;
    tx[1] = (uint8_t)(high & 0xFF);
    tx[2] = (uint8_t)(high >> 8);
    I2C_SendByte(0x29, tx, 3);
}

void TSL2591_SetNoPersistThresholds(uint16_t low, uint16_t high)
{
    uint8_t tx[3];

    //Write no-persist low threshold (0x08, 0x09)
    tx[0] = TSL2591_COMMAND_BIT | TSL2591_NPAILTL;
    tx[1] = (uint8_t)(low & 0xFF);
    tx[2] = (uint8_t)(low >> 8);
    I2C_SendByte(0x29, tx, 3);

    //Write no-persist high threshold (0x0A, 0x0B)
    tx[0] = TSL2591_COMMAND_BIT | TSL2591_NPAIHTL;
    tx[1] = (uint8_t)(high & 0xFF);
    tx[2] = (uint8_t)(high >> 8);
    I2C_SendByte(0x29, tx, 3);
}

void TSL2591_SetPersist(uint8_t persist)
{
    uint8_t tx[2];
    tx[0] = TSL2591_COMMAND_BIT | TSL2591_PERSIST;
    tx[1] = persist & 0x0F; //APERS is only lower 4 bits
    I2C_SendByte(0x29, tx, 2);
}

void TSL2591_GetThresholds(uint16_t *low, uint16_t *high)
{
    uint8_t rx[4]; 
    uint8_t tx[1];
    tx[0] = TSL2591_COMMAND_BIT | TSL2591_AILTL;
    I2C_SendByte(0x29, tx, 1);
    I2C_ReceiveByte(0x29, rx, 4); 

    //Order: AILTL, AILTH, AIHTL, AIHTH
    *low  = ((uint16_t)rx[1] << 8) | rx[0];
    *high = ((uint16_t)rx[3] << 8) | rx[2];
}

void TSL2591_GetNoPersistThresholds(uint16_t *low, uint16_t *high)
{
    uint8_t rx[4]; 
    uint8_t tx[1];
    tx[0] = TSL2591_COMMAND_BIT | TSL2591_NPAILTL;
    I2C_SendByte(0x29, tx, 1);
    I2C_ReceiveByte(0x29, rx, 4);  

    //Order: NPAILTL, NPAILTH, NPAIHTL, NPAIHTH
    *low  = ((uint16_t)rx[1] << 8) | rx[0];
    *high = ((uint16_t)rx[3] << 8) | rx[2];
}

uint8_t TSL2591_GetPersist(void)
{
    uint8_t rx[1]; 
    uint8_t tx[1];
    tx[0] = TSL2591_COMMAND_BIT | TSL2591_PERSIST;
    I2C_SendByte(0x29, tx, 1);
    I2C_ReceiveByte(0x29, rx, 1);

    return rx[0] & 0x0F;
}


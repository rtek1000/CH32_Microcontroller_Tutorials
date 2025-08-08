/*
 *CH32V006K8U6 - TSL2591 Light Sensor
 *https://curiousscientist.tech/blog/ch32v006k8u6-with-tsl2591-light-sensor
 */

#ifndef I2C_H
#define I2C_H

#include <stdint.h>

//Native IIC
void IIC_Init();
void I2C_ReceiveByte(uint8_t address, uint8_t *Buffer, uint8_t numberOfBytes);
void I2C_SendByte(uint8_t deviceAddress, uint8_t *Buffer, uint8_t numberOfBytes);
void I2C_WriteRegister(uint8_t devAddr7, uint8_t reg, uint8_t value);

#endif //I2C_H
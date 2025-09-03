main(){
        uint16_t rawAngle = 0;
        uint8_t error = 0;

        error = ReadRawAngleAS5600(&rawAngle);

        if (error != 0) {
          lcd_print("  IIC error");
        } else {
          lcd_print("           ");
        }
}

void IIC_Init()
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    I2C_InitTypeDef I2C_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_10 | GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    I2C_InitStructure.I2C_ClockSpeed = 400000; //typical speed, also used for Arduinos
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_16_9;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit; 
    I2C_Init(I2C1, &I2C_InitStructure);

    I2C_Init(I2C2, &I2C_InitStructure);

    I2C_Cmd(I2C1, ENABLE);
    I2C_Cmd(I2C2, ENABLE);
}

uint8_t I2C_SendByte(uint8_t address, uint8_t data)
{
  uint32_t timeout = 0xffff;
  uint8_t error = 0;

    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) != RESET){  // Wait for the flag to change
      if (timeout > 0) {
        timeout--;
      } else {
        error = 1;
        break;
      }
    };

    if (error == 1) {
      return error;
    }

    I2C_GenerateSTART(I2C1, ENABLE); //Generate start

    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)){ // Wait for confirmation
      if (timeout > 0) {
        timeout--;
      } else {
        error = 1;
        break;
      }
    }; 

    if (error == 1) {
      return error;
    }

    I2C_Send7bitAddress(I2C1, address << 1, I2C_Direction_Transmitter); // Pay attention to the shifting due to the 8-bit variable but 7-bit address

    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)){ // Set MCU as transmitter
      if (timeout > 0) {
        timeout--;
      } else {
        error = 1;
        break;
      }
    }; 

    if (error == 1) {
      return error;
    }

    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_TXE) == RESET){ // Wait for the flag to change
      if (timeout > 0) {
        timeout--;
      } else {
        error = 1;
        break;
      }
    }; 

    if (error == 1) {
      return error;
    }

    I2C_SendData(I2C1, data); // Send a byte of data

    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)){ // Wait for confirmation
      if (timeout > 0) {
        timeout--;
      } else {
        error = 1;
        break;
      }
    }; 

    if (error == 1) {
      return error;
    }
    
    I2C_GenerateSTOP(I2C1, ENABLE); // Generate stop

    return error;
}

uint8_t I2C_ReceiveByte(uint8_t address, uint8_t* data)
{
  uint32_t timeout = 0xffff;
  uint8_t error = 0;
    uint8_t receivedByte;

    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) != RESET){
      if (timeout > 0) {
        timeout--;
      } else {
        error = 1;
        break;
      }
    }; 

    if (error == 1) {
      return error;
    }

    I2C_GenerateSTART(I2C1, ENABLE);

    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)){
      if (timeout > 0) {
        timeout--;
      } else {
        error = 1;
        break;
      }
    }; 

    if (error == 1) {
      return error;
    }

    I2C_Send7bitAddress(I2C1, address << 1, I2C_Direction_Receiver);

    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)){
      if (timeout > 0) {
        timeout--;
      } else {
        error = 1;
        break;
      }
    }; 

    if (error == 1) {
      return error;
    }

    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE) == RESET){
      if (timeout > 0) {
        timeout--;
      } else {
        error = 1;
        break;
      }
    }; 

    if (error == 1) {
      return error;
    }

    receivedByte = I2C_ReceiveData(I2C1);

    I2C_GenerateSTOP(I2C1, ENABLE);

    *data = receivedByte;

    return error;
}

uint8_t ReadRawAngleAS5600(uint16_t* data)
{
    uint8_t address = 0x36;
    uint8_t lowByte, highByte;
    uint16_t rawAngle;

    I2C_SendByte(address, 0x0D); //Request low byte
    if (I2C_ReceiveByte(address, &lowByte) != 0) { //Receive the low byte
      //printf("IIC error\n");
      return 1;
    }

    //printf("lowByte: %d\n", lowByte); //Print the low byte

    I2C_SendByte(address, 0x0C); //Request high byte
    if (I2C_ReceiveByte(address, &highByte) != 0) { //Receive high byte
      //printf("IIC error\n");
      return 1;
    }

    //printf("highByte: %d\n", highByte); //Print high byte

    highByte = highByte << 8;
    rawAngle = highByte | lowByte;

    *data = rawAngle;

    return 0;
}
/*
 * driver_MCP23S17.h
 *
 *  Created on: Dec 3, 2024
 *      Author: oliver
 */

#ifndef DRIVERS_DRIVER_MCP23S17_H_
#define DRIVERS_DRIVER_MCP23S17_H_

#define MCP23S17_CONTROL_ADDR 0b000
#define MCP23S17_IODIRA  0x00
#define MCP23S17_IODIRB  0x01
#define MCP23S17_OLATA   0x14
#define MCP23S17_OLATB 	 0x15

#define MCP23S17_ALL_ON	 0x00
#define MCP23S17_ALL_OFF 0xFF

// Builds the VU-Metre control byte
#define MCP23S17_CONTROL_BYTE(adress, RW)\
		((0b0100 << 4) | (adress & 0b111 << 1) | RW)

/**
  * @brief  MCP23S17 READ and WRITE enumeration
  */
typedef enum
{
	VU_WRITE = 0U,
	VU_READ
} MCP23S17_Mode;


void MCP23S17_WriteRegister(uint8_t, uint8_t);
void MCP23S17_Init();
void MCP23S17_Set_LED_id(uint8_t);
void MCP23S17_Toggle_LED_id(uint8_t);
void MCP23S17_Set_LEDs(uint16_t);
void MCP23S17_VUMetre_R(int);
void MCP23S17_VUMetre_L(int);

#endif /* DRIVERS_DRIVER_MCP23S17_H_ */

/*
 * driver_MCP23S17.h
 *
 *  Created on: Dec 3, 2024
 *      Author: oliver
 */

#ifndef DRIVERS_MCP23S17_H_
#define DRIVERS_MCP23S17_H_

#include <stdint.h>

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


void MCP23S17_WriteRegister(uint8_t reg, uint8_t data);
void MCP23S17_Init(void);
void MCP23S17_Set_LED_id(uint8_t led);
void MCP23S17_Toggle_LED_id(uint8_t led);
void MCP23S17_Set_LEDs(uint16_t leds);
void MCP23S17_Update_LEDs(void);
void MCP23S17_level_R(int level);
void MCP23S17_level_L(int level);

#endif /* DRIVERS_MCP23S17_H_ */

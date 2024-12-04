/*
 * SGTL5000.h
 *
 *  Created on: Dec 4, 2024
 *      Author: oliver
 */

#ifndef DRIVERS_SGTL5000_H_
#define DRIVERS_SGTL5000_H_

#include <stdint.h>

#define SGTL5000_MEM_SIZE 2	// Size in bytes of addresses

#define SGTL5000_CODEC 0x14	// SGTL5000 I2C Address

// SGTL5000 Register Definitions
#define SGTL5000_CHIP_ID	  0x0000
#define CHIP_ANA_POWER        0x0030 // Analog Power Control Register
#define CHIP_LINREG_CTRL      0x0022 // Linear Regulator Control Register
#define CHIP_REF_CTRL         0x0028 // Reference Voltage Control Register
#define CHIP_LINE_OUT_CTRL    0x002C // Line Output Control Register
#define CHIP_SHORT_CTRL       0x002E // Short Circuit Detect Control Register
#define CHIP_ANA_CTRL         0x0034 // Analog Audio Control Register
#define CHIP_DIG_POWER        0x0002 // Digital Power Control Register
#define CHIP_LINE_OUT_VOL     0x002A // Line Out Volume Control Register
#define CHIP_CLK_CTRL         0x0004 // Clock Control Register
#define CHIP_I2S_CTRL         0x0006 // I2S Control Register
#define CHIP_ADCDAC_CTRL      0x000E // ADC and DAC Control Register
#define CHIP_DAC_VOL          0x0010 // DAC Volume Control Register

// Function prototypes
void SGTL5000_Init(void);
void SGTL5000_ReadRegister(uint16_t address, uint8_t* pData, uint16_t length);
void SGTL5000_WriteRegister(uint16_t address, uint16_t value);
void SGTL5000_ErrorHandler(const char* message);

#endif /* DRIVERS_SGTL5000_H_ */

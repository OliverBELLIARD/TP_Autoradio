/*
 * SGTL5000.c
 *
 *  Created on: Dec 4, 2024
 *      Author: oliver
 */
#include "SGTL5000.h"
#include "i2c.h"
#include <stdio.h>
#include <stdlib.h>

#define LOGS 0

typedef struct {
	I2C_HandleTypeDef * hi2c;
	uint16_t chip_id;
} h_SGTL5000_t;

h_SGTL5000_t hSGTL5000;


/**
 * @brief Error handler for SGTL5000 operations.
 * @param message: Error message to display.
 */
void SGTL5000_ErrorHandler(const char* message)
{
    printf("SGTL5000 Error: %s\r\n", message);
    Error_Handler(); // Use HAL's Error_Handler or customize as needed
}

/**
 * @brief Reads data from a register of SGTL5000 with error management.
 * @param address: Register address to read from.
 * @param pData: Pointer to data buffer for storing the read data.
 * @param length: Number of bytes to read.
 */
void SGTL5000_ReadRegister(uint16_t address, uint8_t* pData, uint16_t length)
{
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(hSGTL5000.hi2c, SGTL5000_CODEC,
    		address, SGTL5000_MEM_SIZE, pData, length, HAL_MAX_DELAY);

    if (status != HAL_OK) {
        printf("Error: Failed to read from address 0x%04X\r\n", address);
        SGTL5000_ErrorHandler("ReadRegister failed");
    }
}

/**
 * @brief Writes data to a register of SGTL5000 with error management.
 * @param address: Register address to write to.
 * @param value: Data value to write to the register.
 */
void SGTL5000_WriteRegister(uint16_t address, uint16_t value)
{
    uint8_t data[2] = { (uint8_t)(value >> 8), (uint8_t)(value & 0xFF) };
    HAL_StatusTypeDef status = HAL_I2C_Mem_Write(hSGTL5000.hi2c, SGTL5000_CODEC,
    		address, SGTL5000_MEM_SIZE, data, 2, HAL_MAX_DELAY);

    if (status != HAL_OK) {
        printf("Error: Failed to write 0x%04X to address 0x%04X\r\n", value, address);
        SGTL5000_ErrorHandler("WriteRegister failed");
    }
}

/**
 * @brief Initializes the SGTL5000 codec.
 */
void SGTL5000_Init(void)
{
    hSGTL5000.hi2c = &hi2c2;

    uint8_t chip_id_data[2];
    SGTL5000_ReadRegister(SGTL5000_CHIP_ID, chip_id_data, SGTL5000_MEM_SIZE);
    hSGTL5000.chip_id = (chip_id_data[0] << 8) | chip_id_data[1];

    if (hSGTL5000.chip_id != 0xA011) { // Example CHIP_ID, replace with actual expected ID
        SGTL5000_ErrorHandler("Invalid CHIP_ID detected");
    }

    // Initialize registers
    SGTL5000_WriteRegister(CHIP_LINREG_CTRL, 0x0008);     // Configure VDDD level to 1.2V
    SGTL5000_WriteRegister(CHIP_ANA_POWER, 0x7260);       // Power up internal linear regulator
    SGTL5000_WriteRegister(CHIP_REF_CTRL, 0x004E);        // Set reference voltage
    SGTL5000_WriteRegister(CHIP_LINE_OUT_CTRL, 0x0322);   // Configure line-out reference and bias
    SGTL5000_WriteRegister(CHIP_SHORT_CTRL, 0x1106);      // Enable short circuit detect
    SGTL5000_WriteRegister(CHIP_ANA_CTRL, 0x0133);        // Enable analog controls
    SGTL5000_WriteRegister(CHIP_DIG_POWER, 0x0073);       // Power up digital blocks
    SGTL5000_WriteRegister(CHIP_LINE_OUT_VOL, 0x0505);    // Set line-out volume

#if (LOGS)
    printf("SGTL5000 initialized successfully, CHIP_ID: 0x%04X\r\n", hSGTL5000.chip_id);
#endif
}

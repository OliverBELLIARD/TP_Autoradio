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
#define DEBUG 0

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
	Error_Handler();
	NVIC_SystemReset();
}

/**
 * @brief Reads data from a register of SGTL5000 with error management.
 * @param address: Register address to read from.
 * @param pData: Pointer to data buffer for storing the read data.
 * @param length: Number of bytes to read.
 */
void SGTL5000_i2c_ReadRegister(uint16_t address, uint8_t* pData, uint16_t length)
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
void SGTL5000_i2c_WriteRegister(uint16_t address, uint16_t value)
{
	uint8_t data[2] = { (uint8_t)(value >> 8), (uint8_t)(value & 0xFF) };
	HAL_StatusTypeDef status = HAL_I2C_Mem_Write(hSGTL5000.hi2c, SGTL5000_CODEC,
			address, SGTL5000_MEM_SIZE, data, 2, HAL_MAX_DELAY);

	// Handle all possible I2C errors
	switch (status) {
	case HAL_OK:
		// Write successful
#if LOGS
		printf("Successfully wrote 0x%04X to address 0x%04X\r\n", value, address);
#endif
		break;

	case HAL_ERROR:
		// General HAL error
		printf("Error: HAL_ERROR while writing 0x%04X to address 0x%04X\r\n", value, address);
		SGTL5000_ErrorHandler("General HAL_ERROR during WriteRegister");
		break;

	case HAL_BUSY:
		// HAL busy error
		printf("Error: HAL_BUSY, I2C bus is busy while writing 0x%04X to 0x%04X\r\n", value, address);
		SGTL5000_ErrorHandler("I2C bus busy during WriteRegister");
		break;

	case HAL_TIMEOUT:
		// Timeout error
		printf("Error: HAL_TIMEOUT while writing 0x%04X to address 0x%04X\r\n", value, address);
		SGTL5000_ErrorHandler("Timeout during WriteRegister");
		break;

	default:
		// Unexpected error code
		printf("Error: Unknown error (status code: %d) while writing 0x%04X to address 0x%04X\r\n", status, value, address);
		SGTL5000_ErrorHandler("Unknown error during WriteRegister");
		break;
	}
}

/**
 * @brief Initializes the SGTL5000 codec.
 */
void SGTL5000_Init(void)
{
	hSGTL5000.hi2c = &hi2c2;

	uint8_t chip_id_data[2];
	SGTL5000_i2c_ReadRegister(SGTL5000_CHIP_ID, chip_id_data, SGTL5000_MEM_SIZE);
	hSGTL5000.chip_id = (chip_id_data[0] << 8) | chip_id_data[1];

	if (hSGTL5000.chip_id != 0xA011) { // Example CHIP_ID, replace with actual expected ID
		SGTL5000_ErrorHandler("Invalid CHIP_ID detected");
	}

	uint16_t mask;

	/* Chip Powerup and Supply Configurations */

	//--------------- Power Supply Configuration----------------
	// NOTE: This next 2 Write calls is needed ONLY if VDDD is
	// Configure VDDD level to 1.8V (bits 3:0)
	// Write CHIP_LINREG_CTRL 0x????
	// OK, pas touche!
	// Power up internal linear regulator (Set bit 9)
	// Write CHIP_ANA_POWER 0x7260
	// Pas touche non plus

	// NOTE: This next Write call is needed ONLY if VDDD is
	// externally driven
	// Turn off startup power supplies to save power (Clear bit 12 and 13)
	// Write CHIP_ANA_POWER 0x4260
	mask = (1 << 12) | (1 << 13);
	//mask = 0b0111001011111111;
	SGTL5000_i2c_WriteRegister(SGTL5000_CHIP_ANA_POWER, mask);
#if (DEBUG)
	printf("SGTL5000_CHIP_ANA_POWER set as: 0x%04X\r\n", mask);
#endif

	// NOTE: The next Write calls is needed only if both VDDA and
	// VDDIO power supplies are less than 3.1V.
	// Enable the internal oscillator for the charge pump (Set bit 11)
	// Write CHIP_CLK_TOP_CTRL 0x0800
	// Enable charge pump (Set bit 11)
	// Write CHIP_ANA_POWER 0x4A60
	// VDDA and VDDIO = 3.3V so not necessary

	// NOTE: The next modify call is only needed if both VDDA and
	// VDDIO are greater than 3.1 V
	// Configure the charge pump to use the VDDIO rail (set bit 5 and bit 6)
	// Write CHIP_LINREG_CTRL 0x006C
	// VDDA and VDDIO = 3.3V so it IS necessary
	mask = (1 << 5) | (1 << 6);
	SGTL5000_i2c_WriteRegister(SGTL5000_CHIP_LINREG_CTRL, mask);
#if (DEBUG)
	printf("SGTL5000_CHIP_LINREG_CTRL set as: 0x%04X\r\n", mask);
#endif

	//---- Reference Voltage and Bias Current Configuration----
	// NOTE: The value written in the next 2 Write calls is dependent
	// on the VDDA voltage value.
	// Set ground, ADC, DAC reference voltage (bits 8:4). The value should
	// be set to VDDA/2. This example assumes VDDA = 1.8 V. VDDA/2 = 0.9 V.
	// The bias current should be set to 50% of the nominal value (bits 3:1)
	// Write CHIP_REF_CTRL 0x004E
	mask = 0x01FF;	// VAG_VAL = 1.575V, BIAS_CTRL = -50%, SMALL_POP = 1
	SGTL5000_i2c_WriteRegister(SGTL5000_CHIP_REF_CTRL, mask);
#if (DEBUG)
	printf("SGTL5000_CHIP_REF_CTRL set as: 0x%04X\r\n", mask);
#endif

	// Set LINEOUT reference voltage to VDDIO/2 (1.65 V) (bits 5:0)
	// and bias current (bits 11:8) to the recommended value of 0.36 mA
	// for 10 kOhm load with 1.0 nF capacitance
	// Write CHIP_LINE_OUT_CTRL 0x0322
	//	mask = 0x0322;	// LO_VAGCNTRL = 1.65V, OUT_CURRENT = 0.36mA (?)
	mask = 0x031E;
	SGTL5000_i2c_WriteRegister(SGTL5000_CHIP_LINE_OUT_CTRL, mask);
#if (DEBUG)
	printf("SGTL5000_CHIP_LINE_OUT_CTRL set as: 0x%04X\r\n", mask);
#endif

	//------------Other Analog Block Configurations--------------
	// Configure slow ramp up rate to minimize pop (bit 0)
	// Write CHIP_REF_CTRL 0x004F
	// Déjà fait

	// Enable short detect mode for headphone left/right
	// and center channel and set short detect current trip level
	// to 75 mA
	// Write CHIP_SHORT_CTRL 0x1106
	mask = 0x1106;	// MODE_CM = 2, MODE_LR = 1, LVLADJC = 200mA, LVLADJL = 75mA, LVLADJR = 50mA
	SGTL5000_i2c_WriteRegister(SGTL5000_CHIP_SHORT_CTRL, mask);
#if (DEBUG)
	printf("SGTL5000_CHIP_SHORT_CTRL set as: 0x%04X\r\n", mask);
#endif

	// Enable Zero-cross detect if needed for HP_OUT (bit 5) and ADC (bit 1)
	// Write CHIP_ANA_CTRL 0x0133
	mask = 0x0004;	// Unmute all + SELECT_ADC = LINEIN
	//	mask = 0x0000;	// Unmute all + SELECT_ADC = MIC
	SGTL5000_i2c_WriteRegister(SGTL5000_CHIP_ANA_CTRL, mask);
#if (DEBUG)
	printf("SGTL5000_CHIP_ANA_CTRL set as: 0x%04X\r\n", mask);
#endif

	//------------Power up Inputs/Outputs/Digital Blocks---------
	// Power up LINEOUT, HP, ADC, DAC
	// Write CHIP_ANA_POWER 0x6AFF
	mask = 0x6AFF;	// LINEOUT_POWERUP, ADC_POWERUP, CAPLESS_HEADPHONE_POWERUP, DAC_POWERUP, HEADPHONE_POWERUP, REFTOP_POWERUP, ADC_MONO = stereo
	// VAG_POWERUP, VCOAMP_POWERUP = 0, LINREG_D_POWERUP, PLL_POWERUP = 0, VDDC_CHRGPMP_POWERUP, STARTUP_POWERUP = 0, LINREG_SIMPLE_POWERUP,
	// DAC_MONO = stereo
	SGTL5000_i2c_WriteRegister(SGTL5000_CHIP_ANA_POWER, mask);
#if (DEBUG)
	printf("SGTL5000_CHIP_ANA_POWER set as: 0x%04X\r\n", mask);
#endif
	// Power up desired digital blocks
	// I2S_IN (bit 0), I2S_OUT (bit 1), DAP (bit 4), DAC (bit 5),
	// ADC (bit 6) are powered on
	// Write CHIP_DIG_POWER 0x0073
	mask = 0x0073;	// I2S_IN_POWERUP, I2S_OUT_POWERUP, DAP_POWERUP, DAC_POWERUP, ADC_POWERUP
	SGTL5000_i2c_WriteRegister(SGTL5000_CHIP_DIG_POWER, mask);
#if (DEBUG)
	printf("SGTL5000_CHIP_DIG_POWER set as: 0x%04X\r\n", mask);
#endif

	//----------------Set LINEOUT Volume Level-------------------
	// Set the LINEOUT volume level based on voltage reference (VAG)
	// values using this formula
	// Value = (int)(40*log(VAG_VAL/LO_VAGCNTRL) + 15)
	// Assuming VAG_VAL and LO_VAGCNTRL is set to 0.9 V and
	// 1.65 V respectively, the // left LO vol (bits 12:8) and right LO
	// volume (bits 4:0) value should be set // to 5
	// Write CHIP_LINE_OUT_VOL 0x0505
	mask = 0x1111;	// TODO recalculer
	SGTL5000_i2c_WriteRegister(SGTL5000_CHIP_LINE_OUT_VOL, mask);
#if (DEBUG)
	printf("SGTL5000_CHIP_LINE_OUT_VOL set as: 0x%04X\r\n", mask);
#endif

	/* System MCLK and Sample Clock */

	// Configure SYS_FS clock to 48 kHz
	// Configure MCLK_FREQ to 256*Fs
	// Modify CHIP_CLK_CTRL->SYS_FS 0x0002 // bits 3:2
	// Modify CHIP_CLK_CTRL->MCLK_FREQ 0x0000 // bits 1:0
	mask = 0x0004;	// SYS_FS = 48kHz
	SGTL5000_i2c_WriteRegister(SGTL5000_CHIP_CLK_CTRL, mask);
#if (DEBUG)
	printf("SGTL5000_CHIP_CLK_CTRL set as: 0x%04X\r\n", mask);
#endif
	// Configure the I2S clocks in master mode
	// NOTE: I2S LRCLK is same as the system sample clock
	// Modify CHIP_I2S_CTRL->MS 0x0001 // bit 7
	// Non, on reste en slave!
	mask = 0x0130;	// DLEN = 16 bits
	SGTL5000_i2c_WriteRegister(SGTL5000_CHIP_I2S_CTRL, mask);
#if (DEBUG)
	printf("SGTL5000_CHIP_I2S_CTRL set as: 0x%04X\r\n", mask);
#endif

	/* PLL Configuration */
	// Pas utilisé

	/* Input/Output Routing */
	// Laissons tout par défaut pour l'instant
	//	mask = 0x0000;	// ADC -> DAC
	//	SGTL5000_i2c_WriteRegister(SGTL5000_CHIP_SSS_CTRL, mask);

	/* Le reste */
	mask = 0x0000;	// Unmute
	SGTL5000_i2c_WriteRegister(SGTL5000_CHIP_ADCDAC_CTRL, mask);
#if (DEBUG)
	printf("SGTL5000_CHIP_ADCDAC_CTRL set as: 0x%04X\r\n", mask);
#endif

	mask = 0x3C3C;
	//	mask = 0x4747;
	SGTL5000_i2c_WriteRegister(SGTL5000_CHIP_DAC_VOL, mask);
#if (DEBUG)
	printf("SGTL5000_CHIP_DAC_VOL set as: 0x%04X\r\n", mask);
#endif

	mask = 0x0251;	// BIAS_RESISTOR = 2, BIAS_VOLT = 5, GAIN = 1
	SGTL5000_i2c_WriteRegister(SGTL5000_CHIP_MIC_CTRL, mask);
#if (DEBUG)
	printf("SGTL5000_CHIP_MIC_CTRL set as: 0x%04X\r\n", mask);
#endif

	//	for (int i = 0 ; register_map[i] != SGTL5000_DAP_COEF_WR_A2_LSB ; i++)
	//	{
	//		uint16_t reg = 0;
	//		sgtl5000_i2c_read_register(h_sgtl5000, register_map[i], &reg);
	//		printf("%02d: [0x%04x] = 0x%04x\r\n", i, register_map[i], reg);
	//	}

#if (LOGS)
	printf("SGTL5000 initialized successfully, CHIP_ID: 0x%04X\r\n", hSGTL5000.chip_id);
#endif
}

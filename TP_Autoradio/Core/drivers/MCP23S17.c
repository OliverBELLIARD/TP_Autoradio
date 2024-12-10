#include "MCP23S17.h"

#include <stdio.h>
#include <stdlib.h>
#include "spi.h"

#define LOGS 0

typedef struct {
	SPI_HandleTypeDef* hspi;
	uint8_t GPA;	// LED array in GPIOA
	uint8_t GPB;	// LED array in GPIOB
} h_MCP23S17_t;

h_MCP23S17_t hMCP23S17;


// Function to write to a register of MCP23S17 with error handling
void MCP23S17_WriteRegister(uint8_t reg, uint8_t data)
{
	uint8_t control_byte = MCP23S17_CONTROL_BYTE(MCP23S17_CONTROL_ADDR, VU_WRITE); // Address = 0b000

	uint8_t buffer[2] = {reg, data};
	HAL_StatusTypeDef status;

	// Assert chip select
	HAL_GPIO_WritePin(VU_nCS_GPIO_Port, VU_nCS_Pin, GPIO_PIN_RESET);

	// Transmit control byte
	status = HAL_SPI_Transmit(hMCP23S17.hspi, &control_byte, 1, HAL_MAX_DELAY);
	if (status != HAL_OK) {
		HAL_GPIO_WritePin(VU_nCS_GPIO_Port, VU_nCS_Pin, GPIO_PIN_SET); // Deassert chip select
		printf("Error: Failed to transmit control byte (HAL_SPI_Transmit returned %d)\r\n", status);
		Error_Handler(); // Handle the error
		return; // Prevent further execution
	}

#if (LOGS)
	printf("SPI3 control transmission status: %d\r\n", status);
#endif

	// Transmit register address and data
	status = HAL_SPI_Transmit(hMCP23S17.hspi, buffer, 2, HAL_MAX_DELAY);
	if (status != HAL_OK) {
		HAL_GPIO_WritePin(VU_nCS_GPIO_Port, VU_nCS_Pin, GPIO_PIN_SET); // Deassert chip select
		printf("Error: Failed to transmit register data (HAL_SPI_Transmit returned %d)\r\n", status);
		Error_Handler(); // Handle the error
		return; // Prevent further execution
	}

#if (LOGS)
	printf("SPI3 data Ox%X transmission to register 0x%X status: %d\r\n", data, reg, status);
#endif

	// Deassert chip select
	HAL_GPIO_WritePin(VU_nCS_GPIO_Port, VU_nCS_Pin, GPIO_PIN_SET);
}

void MCP23S17_Update_LEDs()
{
	MCP23S17_WriteRegister(MCP23S17_OLATA, hMCP23S17.GPA);
	MCP23S17_WriteRegister(MCP23S17_OLATB, hMCP23S17.GPB);
}

void MCP23S17_Init(void)
{
	hMCP23S17.hspi = &hspi3;

	HAL_SPI_Init(hMCP23S17.hspi);

	// nRESET to base state
	HAL_GPIO_WritePin(VU_nRESET_GPIO_Port, VU_nRESET_Pin, GPIO_PIN_SET);

	// nCS to reset state
	HAL_GPIO_WritePin(VU_nCS_GPIO_Port, VU_nCS_Pin, GPIO_PIN_SET);

	// Set all GPIOA and GPIOB pins as outputs
	MCP23S17_WriteRegister(MCP23S17_IODIRA, MCP23S17_ALL_ON); // GPA as output
	MCP23S17_WriteRegister(MCP23S17_IODIRB, MCP23S17_ALL_ON); // GPB as output

	hMCP23S17.GPA = 0xFF;	// All LEDs on GPIOA OFF
	hMCP23S17.GPB = 0xFF;	// All LEDs on GPIOB OFF

	MCP23S17_Update_LEDs();
}

void MCP23S17_Set_LED_id(uint8_t led)
{
	if (led > 7)
	{
		hMCP23S17.GPB = ~(1 << led%8);
		hMCP23S17.GPA = 0xFF; // All LEDs on GPIOA OFF
	}
	else
	{
		hMCP23S17.GPA = ~(1 << led);
		hMCP23S17.GPB = 0xFF; // All LEDs on GPIOB OFF
	}

	MCP23S17_Update_LEDs();
}

void MCP23S17_Toggle_LED_id(uint8_t led)
{
	if (led > 7)
	{
		hMCP23S17.GPB = (hMCP23S17.GPB & ~(1 << led%8)) | (~hMCP23S17.GPB & (1 << led%8));
	}
	else
	{
		hMCP23S17.GPA = (hMCP23S17.GPA & ~(1 << led)) | (~hMCP23S17.GPA & (1 << led));
	}

	MCP23S17_Update_LEDs();
}

void MCP23S17_Set_LEDs(uint16_t leds)
{
	hMCP23S17.GPB = (0xFF00 & leds) >> 8;
	hMCP23S17.GPA = 0xFF & leds;

	MCP23S17_Update_LEDs();
}

/*
 * @param level in percentage
 */
void MCP23S17_level_R(int level)
{
	if (level > 100) level = 100;
	if (level <= 0) level = 0;

	if (level <= 100)
	{
		hMCP23S17.GPA = 0xFF & (0x00FF << (int)(8*level/100));

		MCP23S17_Update_LEDs();
	}
}

/*
 * @param level in percentage
 */
void MCP23S17_level_L(int level)
{
	if (level > 100) level = 100;
	if (level <= 0) level = 0;

	if (level <= 100)
	{
		hMCP23S17.GPB = 0xFF & (0x00FF << (int)(8*level/100));

		MCP23S17_Update_LEDs();
	}
}

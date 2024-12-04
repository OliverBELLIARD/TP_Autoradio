# TP de Synthèse – Autoradio
TP de Synthèse – Autoradio


## 2 Le GPIO Expander et le VU-Metre

### 2.1 Configuration

1. La référence du GPIO Expander est la suivante : MCP23S17-E/S0. Nous avons ajouté sa datasheet dans notre documentation.
2. C'est le SPI3 qui est utilisé dans ce cas. 
3. Sur STM32CubeIDE Les changements à effectuer sont l'allocation des pins du SPI3 utilisés par le VU-Metre.
4. ![image](https://github.com/user-attachments/assets/7670b71d-7e9e-43da-8ec4-54056be08e00)  
   (le pin VU_nRESET a été également alloué au VU-Metre)

### 2.2 Tests

Pour cette partie nous avons décidé de faire un chenillard en utilisant les 16 LEDs à notre disposition. Pour effectuer ce chenillard nous avons utilisé la connexion SPI3 pour allez contrôler notre GPIO Expander, celui-ci contrôlant directement nos LEDs.  

Voici un aperçu de la fonction que nous avons réalisé à cet effet :  

```C
void test_chenillard(int delay)
{
	int i = 0;

	for (;;)
	{
		MCP23S17_Set_LEDs(~(1 << i%8 | ((1 << i%8) << 8)));
		i++;

		vTaskDelay( delay / portTICK_PERIOD_MS );  // Délai de duree en ms
	}
}
```

### 2.3 Driver

Nous avons par la suite écrit le driver correspondant. Nous avons par ailleurs implémenté ces drivers dans un shell récupéré à partir du TD4 de Noyau temps réel tout en l'adaptant aux besoins de ce TP.

Le voici : (n'est pas encore fini)

```C
#include <stdlib.h>
#include <stdio.h>
#include "spi.h"
#include "driver_MCP23S17.h"

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
void MCP23S17_VUMetre_R(int level)
{
	if (level <= 100)
	{
		hMCP23S17.GPA = 0xFF & (0x00FF << (int)(8*level/100));

		MCP23S17_Update_LEDs();
	}
}

/*
 * @param level in percentage
 */
void MCP23S17_VUMetre_L(int level)
{
	if (level <= 100)
	{
		hMCP23S17.GPB = 0xFF & (0x00FF << (int)(8*level/100));

		MCP23S17_Update_LEDs();
	}
}
```


## 3 Le CODEC Audio SGTL5000

### 3.1 Configurations préalables

Le CODEC utilisant deux protocoles de communication avec l'I2C pour la configuration et l'I2S pour le transfert des échantillons audios, nous allons nous concentrer dans un premier temps sur la partie I2C.


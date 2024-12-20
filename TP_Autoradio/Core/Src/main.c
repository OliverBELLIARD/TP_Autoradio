/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "dma.h"
#include "i2c.h"
#include "sai.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "../drivers/MCP23S17.h"
#include "../drivers/SGTL5000.h"

#include "../shell/shell.h"
#include "../shell/functions.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LOGS 0

#define STACK_DEPTH 256
#define TASK_SHELL_PRIORITY 3
#define TASK_MCP23S17_PRIORITY 2
#define DELAY_LED_TOGGLE 200

#define SAI_BUFFER_LENGTH (512)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
TaskHandle_t h_task_LED = NULL;
TaskHandle_t h_task_shell = NULL;
TaskHandle_t h_task_GPIOExpander = NULL;

uint8_t rxSAI[SAI_BUFFER_LENGTH];
uint8_t txSAI[SAI_BUFFER_LENGTH];
float txSAI_volume;
int VU_level;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/**
 * @brief Transmit a character over UART.
 * @param ch: Character to transmit.
 * @retval int: The transmitted character.
 */
int __io_putchar(int ch)
{
	HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);

	return ch;
}

/**
 * @brief  Gestion d'erreurs de création de tâche.
 * @param	BaseType_t	Retour de xTaskCreate.
 */
void Error_Handler_xTaskCreate(BaseType_t r)
{
	/* Vérification si la tâche a été créée avec succès */
	if (pdPASS == r) {
		/* Si la tâche est créée avec succès, démarrer le scheduler */
#if (LOGS)
		printf("Tâche crée avec succès\r\n");
#endif
	} else if (errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY == r) {
		printf("Erreur: Mémoire insuffisante\r\n");
		Error_Handler();
	} else {
		/* Cas improbable : code d'erreur non prévu pour xTaskCreate */
		printf("Erreur inconnue lors de la création de la tâche\r\n");
		Error_Handler();  	// Gestion d'erreur générique
		NVIC_SystemReset(); // Réinitialiser le microcontrôleur
	}
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == USART2)
	{
		shell_uart_receive_irq_cb();	// Function giving the semaphore!
	}
}

//////////////////////////////////////////////////////////////////////
// TASKS
////////////////////////////////////////////////////////////////////

void task_LED (void * pvParameters) {
	int duree = (int) pvParameters;

#if (LOGS)
	printf("Task %s created\r\n", pcTaskGetName(xTaskGetCurrentTaskHandle()));
#endif
	for (;;)
	{
		HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
		vTaskDelay( duree / portTICK_PERIOD_MS );  // Délai de duree en ms
	}
}

void task_shell(void * unused)
{
#if (LOGS)
	printf("Task %s created\r\n", pcTaskGetName(xTaskGetCurrentTaskHandle()));
#endif

	shell_init();	// Shell initialization

	// Creation of all commands
	shell_add('f', fonction, "Une fonction inutile");
	shell_add('a', addition, "Effectue une somme");
	shell_add('c', calcul, "Opération entre 2 nombres");
	shell_add('t', GPIOExpander_toggle_LED, "Change l'état des LED avec les id");
	shell_add('s', GPIOExpander_set_LED, "Allume une LED avec son id");

	shell_run();	// boucle infinie
}

void test_chenillard(int delay)
{
	int i = 0;

	for (;;)
	{
		MCP23S17_Set_LEDs(~(1 << i%8 | ((1 << i%8) << 8)));
		i++;
		vTaskDelay( delay / portTICK_PERIOD_MS );  // Délai de duree en Dms
	}
}

void task_GPIO_expander (void * unused) {

#if (LOGS)
	printf("Task %s created\r\n", pcTaskGetName(xTaskGetCurrentTaskHandle()));
#endif

	/* VU-Metre test *
	MCP23S17_level_L(70);
	MCP23S17_level_R(50);
	*/

	// Simple test of the array of leds with an animation
	//test_chenillard(100);

	for (;;)
	{
		// VU-Metre
		txSAI_volume = 0;
		for (int i=0; i<SAI_BUFFER_LENGTH; i++)
		{
			txSAI_volume += txSAI[i];
		}
		txSAI_volume = log10f((float)(txSAI_volume)/(SAI_BUFFER_LENGTH*0x7FFF));

		// 0 dB at 70%
		if (VU_level < txSAI_volume+70) VU_level += 1;
		if (VU_level > txSAI_volume+70) VU_level -= 1;

		MCP23S17_level_L(VU_level);
		MCP23S17_level_R(VU_level);

		vTaskDelay( 4/portTICK_PERIOD_MS );  // 1 ms delay
	}
}

///////////////////////////////////////////////////////////////////////
// SAI
/////////////////////////////////////////////////////////////////////

/**
 * @brief Generates a triangular waveform.
 * @param buffer: Pointer to the buffer to hold the waveform.
 * @param length: Number of samples in the waveform.
 * @param amplitude: Peak amplitude of the waveform.
 */
void GenerateTriangleWave(uint8_t* buffer, uint16_t length, uint16_t amplitude)
{
	uint16_t step = (2 * amplitude) / length;
	uint16_t value = 0;
	int8_t direction = 1;

	for (uint16_t i = 0; i < length; i++) {
		buffer[i] = value;
		value += step * direction;

		if (value >= amplitude) {
			value = amplitude;
			direction = -1; // Start decreasing
		} else if (value <= 0) {
			value = 0;
			direction = 1; // Start increasing
		}
	}
}

/**
 * @brief SAI error callback.
 * @param hsai: Pointer to the SAI handle.
 */
void HAL_SAI_ErrorCallback(SAI_HandleTypeDef *hsai)
{
    printf("Error: SAI encountered an error\r\n");

    // Attempt to restart DMA transmission and reception
    if (HAL_SAI_Transmit_DMA(&hsai_BlockA2, (uint8_t*)txSAI, SAI_BUFFER_LENGTH) != HAL_OK) {
        printf("Error: Failed to restart SAI DMA transmission\r\n");
        Error_Handler();
    }

    if (HAL_SAI_Receive_DMA(&hsai_BlockA2, rxSAI, SAI_BUFFER_LENGTH) != HAL_OK) {
        printf("Error: Failed to restart SAI DMA reception\r\n");
        Error_Handler();
    }
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{

	/* USER CODE BEGIN 1 */

	/* USER CODE END 1 */

	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */

	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* Configure the peripherals common clocks */
	PeriphCommonClock_Config();

	/* USER CODE BEGIN SysInit */

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_DMA_Init();
	MX_USART2_UART_Init();
	MX_I2C2_Init();
	MX_SPI3_Init();
	MX_SAI2_Init();
	/* USER CODE BEGIN 2 */
	// Initialize GPIO expander
	MCP23S17_Init();
	__HAL_RCC_SAI2_CLK_ENABLE();
	__HAL_RCC_DMA2_CLK_ENABLE();
	__HAL_SAI_ENABLE(&hsai_BlockA2);
	__HAL_SAI_ENABLE(&hsai_BlockB2);
	SGTL5000_Init();

	// Generate the triangular waveform
	GenerateTriangleWave(txSAI, SAI_BUFFER_LENGTH, 0x7FFF); // 16-bit amplitude (0x7FFF)
	printf("Triangle Wave generation\r\n");

	// Start SAI DMA transmission
	if (HAL_SAI_Transmit_DMA(&hsai_BlockA2, (uint8_t*)txSAI, SAI_BUFFER_LENGTH) != HAL_OK) {
		printf("Error: Failed to start SAI DMA transmission\r\n");
		Error_Handler();
	}

	// Start SAI DMA reception
	if (HAL_SAI_Receive_DMA(&hsai_BlockA2, rxSAI, SAI_BUFFER_LENGTH) != HAL_OK) {
		printf("Error: Failed to start SAI DMA reception\r\n");
		Error_Handler();
	}

	// Test printf
	printf("******* TP Autoradio *******\r\n");

	// Create the task, storing the handle.
	Error_Handler_xTaskCreate(
			xTaskCreate(task_GPIO_expander, // Function that implements the task.
					"GPIO_expander", // Text name for the task.
					STACK_DEPTH, // Stack size in words, not bytes.
					(void *) 500, // 500 ms
					TASK_MCP23S17_PRIORITY, // Priority at which the task is created.
					&h_task_GPIOExpander)); // Used to pass out the created task's handle.

	// Turn on LED2 (Green)
	HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);

	// Create the task, storing the handle.
	Error_Handler_xTaskCreate(
			xTaskCreate(task_LED, // Function that implements the task.
					"LED LD2", // Text name for the task.
					STACK_DEPTH, // Stack size in words, not bytes.
					(void *) DELAY_LED_TOGGLE, // Parameter passed into the task.
					1,// Priority at which the task is created.
					&h_task_LED)); // Used to pass out the created task's handle.
	// Shell task
	Error_Handler_xTaskCreate(
			xTaskCreate(task_shell,
					"Shell",
					STACK_DEPTH,
					NULL,
					TASK_SHELL_PRIORITY,
					&h_task_shell));

	// OS Start
	vTaskStartScheduler();

	/* USER CODE END 2 */

	/* Call init function for freertos objects (in cmsis_os2.c) */
	MX_FREERTOS_Init();

	/* Start scheduler */
	osKernelStart();

	/* We should never get here as control is now taken by the scheduler */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1)
	{
		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

	/** Configure the main internal regulator output voltage
	 */
	if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
	{
		Error_Handler();
	}

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLM = 1;
	RCC_OscInitStruct.PLL.PLLN = 10;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
	RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
	RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
			|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief Peripherals Common Clock Configuration
 * @retval None
 */
void PeriphCommonClock_Config(void)
{
	RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

	/** Initializes the peripherals clock
	 */
	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_SAI2;
	PeriphClkInit.Sai2ClockSelection = RCC_SAI2CLKSOURCE_PLLSAI1;
	PeriphClkInit.PLLSAI1.PLLSAI1Source = RCC_PLLSOURCE_HSI;
	PeriphClkInit.PLLSAI1.PLLSAI1M = 1;
	PeriphClkInit.PLLSAI1.PLLSAI1N = 13;
	PeriphClkInit.PLLSAI1.PLLSAI1P = RCC_PLLP_DIV17;
	PeriphClkInit.PLLSAI1.PLLSAI1Q = RCC_PLLQ_DIV2;
	PeriphClkInit.PLLSAI1.PLLSAI1R = RCC_PLLR_DIV2;
	PeriphClkInit.PLLSAI1.PLLSAI1ClockOut = RCC_PLLSAI1_SAI1CLK;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
	{
		Error_Handler();
	}
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  Period elapsed callback in non blocking mode
 * @note   This function is called  when TIM6 interrupt took place, inside
 * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
 * a global variable "uwTick" used as application time base.
 * @param  htim : TIM handle
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	/* USER CODE BEGIN Callback 0 */

	/* USER CODE END Callback 0 */
	if (htim->Instance == TIM6) {
		HAL_IncTick();
	}
	/* USER CODE BEGIN Callback 1 */

	/* USER CODE END Callback 1 */
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1)
	{
	}
	/* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
	/* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	/* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

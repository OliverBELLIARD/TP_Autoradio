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
#include "i2c.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdlib.h>
#include "../shell/shell.h"
#include "../shell/functions.h"
#include "../drivers/driver_MCP23S17.h"

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

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
TaskHandle_t h_task_LED = NULL;
TaskHandle_t h_task_shell = NULL;
TaskHandle_t h_task_GPIOExpander = NULL;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
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
	shell_add('t', GPIOExpander_toggle_LED, "Change l'état d'une LED avec son id");

	shell_run();	// boucle infinie
}

void task_GPIO_expander (void * pvParameters) {
	int delay = (int) pvParameters;
	int toggle = 1;
	int i = 0;

#if (LOGS)
	printf("Task %s created\r\n", pcTaskGetName(xTaskGetCurrentTaskHandle()));
#endif

	// Initialize MCP23S17 GPIO expander
	MCP23S17_Init();

	/* Test chenillard */
	for (;;)
	{
		MCP23S17_Set_LEDs(~(1 << i%8 | ((1 << i%8) << 8)));
		i++;

		vTaskDelay( delay / portTICK_PERIOD_MS );  // Délai de duree en ms
	}

	/* Test all LEDs at once
	for (;;)
	{

		if (toggle > 0)
		{
			MCP23S17_WriteRegister(MCP23S17_OLATA, 0xFF); // All LEDs on GPIOA ON
			MCP23S17_WriteRegister(MCP23S17_OLATB, 0x00); // All LEDs on GPIOB OFF
		}
		else
		{
			MCP23S17_WriteRegister(MCP23S17_OLATA, 0x00); // All LEDs on GPIOA OFF
			MCP23S17_WriteRegister(MCP23S17_OLATB, 0xFF); // All LEDs on GPIOB ON
		}

		toggle = -toggle;

		vTaskDelay( delay / portTICK_PERIOD_MS );  // Délai de duree en ms
	}
	*/
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

	/* USER CODE BEGIN SysInit */

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_USART2_UART_Init();
	MX_I2C2_Init();
	MX_SPI3_Init();
	/* USER CODE BEGIN 2 */
	HAL_SPI_Init(&hspi3);

	// Test printf
	printf("******* TP Autoradio *******\r\n");

	// Create the task, storing the handle.
	Error_Handler_xTaskCreate(
			xTaskCreate(task_GPIO_expander, // Function that implements the task.
					"GPIO_expander", // Text name for the task.
					STACK_DEPTH, // Stack size in words, not bytes.
					(void *) 500, // 1 ms
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

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

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

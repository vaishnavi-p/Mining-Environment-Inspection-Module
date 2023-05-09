/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body; Mining Environment Inspection Module
  * @instructor     : Prof. Linden McClure
  * @Date           : April 15th, 2023
  * @Assignment     : Final Project
  * Author			: Vaishnavi Patekar, Mrunal Yadav
  * References		: https://www.waveshare.com/wiki/Bme680
  ******************************************************************************
**/

#include "main.h"
#include "stdlib.h"
#include "string.h"
#include "bme680.h"
#include "stdio.h"
#include "fonts.h"
#include "ssd1306.h"
#include "test.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h> //for va_list var arg functions

#include <stdbool.h>
#include "statemachine.h"

I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart2;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART2_UART_Init(void);

void BME680_Read(void);
void user_delay_ms(uint32_t period);

int8_t user_i2c_read(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len);
int8_t user_i2c_write(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data,uint16_t len);

volatile uint8_t set_required_settings;
volatile int8_t rslt = 0;
struct bme680_dev gas_sensor;
struct bme680_field_data data;
char bufbme1[50];
uint16_t min_sampling_period;

/***********************************************************************
 * @name myprintf()
 * @brief to print over a UART
 * @return void
 ***********************************************************************/
void myprintf(const char *fmt, ...) {
  static char buffer[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);

  int len = strlen(buffer);
  HAL_UART_Transmit(&huart2, (uint8_t*)buffer, len, -1);

}


/***********************************************************************
 * @name main()
 * @brief main transition logicc
 * @return void
 ***********************************************************************/
int main(void)
{
	HAL_Init();
	SystemClock_Config();
	MX_GPIO_Init();
	MX_I2C1_Init();
	SSD1306_Init();
	SSD1306_Clear();
	MX_USART2_UART_Init();

	gas_sensor.dev_id = BME680_I2C_ADDR_SECONDARY;
	gas_sensor.intf = BME680_I2C_INTF;
	gas_sensor.read = user_i2c_read;
	gas_sensor.write = user_i2c_write;
	gas_sensor.delay_ms = user_delay_ms;
	gas_sensor.amb_temp = 25;

	rslt = bme680_init(&gas_sensor);

	if (bme680_init(&gas_sensor) != BME680_OK)
	{
	}
	else
	{

	}
	gas_sensor.tph_sett.os_hum = BME680_OS_2X;
	gas_sensor.tph_sett.os_pres = BME680_OS_4X;
	gas_sensor.tph_sett.os_temp = BME680_OS_8X;
	gas_sensor.tph_sett.filter = BME680_FILTER_SIZE_127;
	gas_sensor.gas_sett.run_gas = BME680_ENABLE_GAS_MEAS;
	gas_sensor.gas_sett.heatr_temp = 320; /* degree Celsius */
	gas_sensor.gas_sett.heatr_dur = 150; /* milliseconds */
	gas_sensor.power_mode = BME680_FORCED_MODE;

	set_required_settings = BME680_OST_SEL | BME680_OSP_SEL | BME680_OSH_SEL| BME680_FILTER_SEL | BME680_GAS_SENSOR_SEL;

	rslt = bme680_init(&gas_sensor);

	bme680_get_profile_dur(&min_sampling_period, &gas_sensor);
	struct bme680_field_data data;
	rslt = bme680_set_sensor_settings(set_required_settings, &gas_sensor);
	rslt = bme680_set_sensor_mode(&gas_sensor);
	while (1)
	{
		BME680_Read();
	}
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

	/** Configure the main internal regulator output voltage
	 */
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLM = 8;
	RCC_OscInitStruct.PLL.PLLN = 192;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
	RCC_OscInitStruct.PLL.PLLQ = 8;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK) {
		Error_Handler();
	}
}

/**
 * @brief I2C1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2C1_Init(void) {
	hi2c1.Instance = I2C1;
	hi2c1.Init.ClockSpeed = 400000;
	hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
	hi2c1.Init.OwnAddress1 = 0;
	hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	hi2c1.Init.OwnAddress2 = 0;
	hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN I2C1_Init 2 */

	/* USER CODE END I2C1_Init 2 */

}

static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}


/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	/* USER CODE BEGIN MX_GPIO_Init_1 */
	/* USER CODE END MX_GPIO_Init_1 */

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOH_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(LD5_GPIO_Port, LD5_Pin, GPIO_PIN_RESET);

	 /*Configure GPIO pins : LD4_Pin LD5_Pin LD6_Pin */
	  GPIO_InitStruct.Pin = LD4_Pin|LD5_Pin|LD6_Pin;
	  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	  GPIO_InitStruct.Pull = GPIO_NOPULL;
	  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

}

/***********************************************************************
 * @name BME680_Read()
 * @brief Reads sensor values for every 1s and passes to the state machine
 * @return void
 ***********************************************************************/
void BME680_Read(void)
{
	user_delay_ms(min_sampling_period);
	rslt = bme680_get_sensor_data(&data, &gas_sensor);

	SSD1306_GotoXY(0, 0);
	SSD1306_Puts("ESD PROJECT 2023", &Font_7x10, 1);

	SSD1306_GotoXY(0, 20);
	sprintf(bufbme1, "Temp:%.2fdegC", data.temperature / 100.0f);
	SSD1306_Puts(bufbme1, &Font_7x10, 1);
	myprintf("\r\n\n Temperature: %.2f C ", data.temperature/ 100.0f);

	SSD1306_GotoXY(0, 30);
	sprintf(bufbme1, "Humi:%.2f %%rH ", data.humidity / 1000.0f);
	SSD1306_Puts(bufbme1, &Font_7x10, 1);
	myprintf("\r\n Humidity   : %.2f %%rH ", data.humidity / 1000.0f);

	SSD1306_GotoXY(0, 40);
	sprintf(bufbme1, "Press:%.2fhPa", data.pressure / 100.0f);
	SSD1306_Puts(bufbme1, &Font_7x10, 1);
	myprintf("\r\n Pressure   : %.2f hPa ", data.pressure / 100.0f);

	SSD1306_GotoXY(0, 50);
	sprintf(bufbme1, "AIRQUAL:%.2fKohms ", data.gas_resistance / 1000.0f);
	SSD1306_Puts(bufbme1, &Font_7x10, 1);
	myprintf("\r\n Air Quality: %.2f Kohms ", data.gas_resistance / 1000.0f);


	sensor_statemachine(data.temperature / 100.0f, data.humidity / 1000.0f, data.pressure / 100.0f, data.gas_resistance / 1000.0f );

	SSD1306_UpdateScreen();

	user_delay_ms(5*1000);

	/*  Trigger the next measurement if you would like to read data out continuously*/
	if (gas_sensor.power_mode == BME680_FORCED_MODE)
	{
		rslt = bme680_set_sensor_mode(&gas_sensor);
	}

}

/***********************************************************************
 * @name user_delay_ms()
 * @brief Provides delay in ms
 * @return void
 ***********************************************************************/
void user_delay_ms(uint32_t period)
{
	HAL_Delay(period);
}


/***********************************************************************
 * @name user_i2c_read()
 * @brief reads data from i2c
 * @return void
 ***********************************************************************/
int8_t user_i2c_read(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len)
{
	int8_t result;

	if (HAL_I2C_Master_Transmit(&hi2c1, (dev_id << 1), &reg_addr, 1, HAL_MAX_DELAY) != HAL_OK)
	{
		result = -1;
	}
	else if (HAL_I2C_Master_Receive(&hi2c1, (dev_id << 1) | 0x01, reg_data, len, HAL_MAX_DELAY) != HAL_OK)
	{
		result = -1;
	}
	else
	{
		result = 0;
	}
	return result;
}


/***********************************************************************
 * @name user_i2c_write()
 * @brief reads data from i2c
 * @return void
 ***********************************************************************/
int8_t user_i2c_write(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len)
{
	int8_t result;
	int8_t *buf;

	// Allocate and load I2C transmit buffer
	buf = malloc(len + 1);
	buf[0] = reg_addr;
	memcpy(buf + 1, reg_data, len);

	if (HAL_I2C_Master_Transmit(&hi2c1, (dev_id << 1), (uint8_t*) buf, len + 1, HAL_MAX_DELAY) != HAL_OK)
	{
		result = -1;
	}
	else
	{
		result = 0;
	}

	free(buf);
	return result;
}

/***********************************************************************
 * @name Error_Handler()
 * @brief
 * @return void
 ***********************************************************************/
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

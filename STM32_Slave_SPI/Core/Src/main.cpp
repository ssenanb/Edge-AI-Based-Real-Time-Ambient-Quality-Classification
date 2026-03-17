#include "main.h"
#include "cmsis_os.h"
#include "string.h"
#include <stdarg.h>
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"

typedef struct{
	float temperature;
	float humidity;
	float pressure;
	uint8_t gas_data;
}SensorData;

static SensorData received_sensor_data;
int flag = 0; //for debug

CRC_HandleTypeDef hcrc;

SPI_HandleTypeDef hspi1;
DMA_HandleTypeDef hdma_spi1_rx;

/* Definitions for SPITask */
osThreadId_t SPITaskHandle;
const osThreadAttr_t SPITask_attributes = {
  .name = "SPITask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for EdgeImpulseTask */
osThreadId_t EdgeImpulseTaskHandle;
const osThreadAttr_t EdgeImpulseTask_attributes = {
  .name = "EdgeImpulseTask",
  .stack_size = 2000 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for DataQueue */
osMessageQueueId_t DataQueueHandle;
const osMessageQueueAttr_t DataQueue_attributes = {
  .name = "DataQueue"
};
/* Definitions for BinarySem */
osSemaphoreId_t BinarySemHandle;
const osSemaphoreAttr_t BinarySem_attributes = {
  .name = "BinarySem"
};

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_SPI1_Init(void);
static void MX_CRC_Init(void);
void StartSPITask(void *argument);
void StartEdgeImpulseTask(void *argument);

int get_feature_data(size_t offset, size_t length, float *out_ptr) {
	SensorData local_data = received_sensor_data;
	float sensor_data[4];

    if(osMessageQueueGet(DataQueueHandle, &local_data, 0, 0) == osOK) {
        ei_printf("Data Received\n");

        sensor_data[0] = local_data.temperature;
        sensor_data[1] = local_data.humidity;
        sensor_data[2] = local_data.pressure;
        sensor_data[3] = (float)local_data.gas_data;
    }

    memcpy(out_ptr, sensor_data + offset, length * sizeof(float));

    return 0;
}


int main(void)
{
  HAL_Init();
  
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_SPI1_Init();
  MX_CRC_Init();

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  BinarySemHandle = osSemaphoreNew(1, 1, &BinarySem_attributes);
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of DataQueue */
  DataQueueHandle = osMessageQueueNew (16, sizeof(SensorData), &DataQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of SPITask */
  SPITaskHandle = osThreadNew(StartSPITask, NULL, &SPITask_attributes);

  /* creation of EdgeImpulseTask */
  EdgeImpulseTaskHandle = osThreadNew(StartEdgeImpulseTask, NULL, &EdgeImpulseTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  while (1)
  {
    
  }
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
  RCC_OscInitStruct.PLL.PLLN = 64;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV4;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CRC Initialization Function
  * @param None
  * @retval None
  */
static void MX_CRC_Init(void)
{

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc.Instance = CRC;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_SLAVE;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);

  /*Configure GPIO pin : PA4 */
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
extern "C"{
	int _write(int file, char *ptr, int len)
	{
	  (void)file;
	  int DataIdx;

	  for (DataIdx = 0; DataIdx < len; DataIdx++)
	  {
		ITM_SendChar(*ptr++);
	  }
	  return len;
	}
}

void ei_printf(const char *format, ...) {
    va_list myargs;
    va_start(myargs, format);
    vprintf(format, myargs);
    va_end(myargs);
}
/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartSPITask */
/**
  * @brief  Function implementing the SPITask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartSPITask */
void StartSPITask(void *argument)
{
  for(;;)
  {
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
	if(HAL_SPI_Receive_DMA(&hspi1, (uint8_t*)&received_sensor_data, sizeof(SensorData)) == HAL_OK)
	{
    	flag = 1;
	}
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
	 osSemaphoreRelease(BinarySemHandle);
	 osMessageQueuePut(DataQueueHandle, &received_sensor_data, 0, 0);

    osDelay(20);
  }
}

/* USER CODE BEGIN Header_StartEdgeImpulseTask */
/**
* @brief Function implementing the EdgeImpulseTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartEdgeImpulseTask */
void StartEdgeImpulseTask(void *argument)
{
	signal_t signal;
	signal.total_length = 4; 
	signal.get_data = &get_feature_data;
  for(;;)
  {
	  if(osSemaphoreAcquire(BinarySemHandle, osWaitForever) == osOK)
	  {
		  ei_impulse_result_t result = { 0 };
		  EI_IMPULSE_ERROR res = run_classifier(&signal, &result, true);

		  ei_printf("run_classifier returned: %d\n", res);

		  if (res == EI_IMPULSE_OK) {
			  ei_printf("Predictions (DSP: %d ms., Classification: %d ms.): \n",
					  result.timing.dsp, result.timing.classification);

			  ei_printf("[");
			  for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
				 ei_printf_float(result.classification[ix].value);
				 if (ix != EI_CLASSIFIER_LABEL_COUNT - 1) ei_printf(", ");
			  }

			  ei_printf("]\n");

			  float max_val = 0;
			  int label_idx = 0;
			  for(int i=0; i<EI_CLASSIFIER_LABEL_COUNT; i++) {
				  if(result.classification[i].value > max_val) {
					  max_val = result.classification[i].value;
					 label_idx = i;
				  }
			  }
			  ei_printf("Tespit Edilen Durum: %s\n\n", result.classification[label_idx].label);
		 }
	  }
    osDelay(1000);
  }
  /* USER CODE END StartEdgeImpulseTask */
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
#ifdef USE_FULL_ASSERT
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

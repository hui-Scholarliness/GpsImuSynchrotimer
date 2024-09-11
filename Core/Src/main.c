/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
//进行系统计时
uint8_t ss001=0;//�???? �???? �???? 0.01s
int i=0;//蜂鸣器计数
bool PPS_isArrived=false;//判断PPS是否到达
bool isfirsttime=true;//pps是否第一次来
int  getInitTime_flag=0;//初始时间获取标志位

#define IMU_BUFFER_SIZE 170 //这里应该很小
#define GPS_BUFFER_SIZE 165
#define COMBINED_BUFFER_SIZE 330

char imuBuffer[IMU_BUFFER_SIZE];
char gpsBuffer[GPS_BUFFER_SIZE];
char combinedBuffer[COMBINED_BUFFER_SIZE];


char local_time_ymdsms;//本地年月日时分秒
char fisttime;//
char current_time_integer;//当前时间整数秒——
// 定义全局变量以存储最新的GPS数据
char lastGpsData[180];  // 假设GPS数据最大长度为180字符
// 全局标志位
volatile uint8_t gpsDataReady = 0;
volatile uint8_t imuDataReady = 0;
extern uint8_t u1_buffer[85], u2_buffer[150];
extern uint8_t u1_rx_size, u2_rx_size;
volatile int isTxInProgress = 0;



extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart1_tx;
extern DMA_HandleTypeDef hdma_usart2_rx;
extern DMA_HandleTypeDef hdma_usart2_tx;

volatile bool pps_flag = false;//pps来临标志符
volatile int txComplete = 0; // 传输完成标志

//gpio PE15测GPS来的pps信号，来1次LED0闪烁1次
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	    if(GPIO_Pin==PPS_Pin)
	    {
	        pps_flag = true; // 只设置标志位
	    }
}

//定时器回调函数
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim->Instance == TIM2)
	{

		HAL_GPIO_TogglePin(IMUtrigger_GPIO_Port, IMUtrigger_Pin);//每0.01s触发一次IMU数据——并计时
		ss001++;//进一次回调函数累加一次说明累加了0.01s(10ms）
	}
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart == &huart5) {
        isTxInProgress = 0; // 仅重置标志位
        txComplete = 1;
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

	IMU_data data={0}; // 所有成员初始化为零
	uint8_t dataimu[500];
	char dataimu_time[256];
	char utcTime[14]; // 用于存储提取的UTC时间
	char lastUpdatedTime[14]; // 上一次更新后的时间
	uint8_t byteArray[sizeof(IMU_data)];
	char timestamp[50]; // 添加时间戳
	RingBuffer imuDataBuffer;  // 创建一个环形缓冲区实例
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
  MX_DMA_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_UART5_Init();
  /* USER CODE BEGIN 2 */
  Check_LED();//板子依次亮起，闪烁，最后保持全亮
  Check_BEED();//蜂鸣器响一声，代表计时器启动

  //start GPS USART
  HAL_UARTEx_ReceiveToIdle_DMA(&huart1, u1_buffer, sizeof(u1_buffer));//启动GPS串口  DMA接收
  __HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);
  HAL_UARTEx_ReceiveToIdle_DMA(&huart2, u2_buffer, sizeof(u2_buffer));//启动imu串口  DMA接收
  __HAL_DMA_DISABLE_IT(&hdma_usart2_rx, DMA_IT_HT);//关闭过半中断


  // 初始化GPS缓冲区
  strcpy(gpsBuffer, "No GPS data yet");
  // 初始化环形缓冲区
  RingBuffer_Init(&imuDataBuffer);
  char dataToSend[STRING_LENGTH];
//  HAL_TIM_Base_Start_IT(&htim2);//第一次PPS来开启定期器
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	    if (pps_flag)
	     {
	         pps_flag = false;
	         HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);//PPS来的话led0闪烁
	         if (isfirsttime && getInitTime_flag==0)
	         {
	             isfirsttime = false;
	             getInitTime_flag=1;
	             HAL_TIM_Base_Start_IT(&htim2);//第一次PPS来开启定期器
	         }
	         else {
	        	 ss001 = 0;
	             __HAL_TIM_SET_COUNTER(&htim2, 0);//以后每次来pps就重置定时器，防止晶振计时累计
	             PPS_isArrived=true;
	         }
	     }
//
//
//		/////////////////////////////////////对PPS数据进行处理/////////////////////////////////////////////////
	    ////1.如果是第一次来数据，则获取初始时间
//	    if()

	if(getInitTime_flag !=1 && PPS_isArrived) //不是第一次来pps数据
	{
		PPS_isArrived = false; // 重置
		incrementTime(lastUpdatedTime, 1); // 上一次更新后的时间加1秒
		 long timeDiff = compareTime(lastUpdatedTime,utcTime);
		  if ((timeDiff) <1 && (timeDiff) > -1) // 检查时间差是否在1秒以内
		  {
//			; strcpy(lastUpdatedTime, lastUpdatedTime);//新时间就是上面加一秒后时间
		  }
		  else //非1s内则使用新的utc时间，PPS在信号丢失时候会消失
		  {
			  // 时间差超过1秒的处理逻辑
			  strcpy(lastUpdatedTime, utcTime); // 将lastUpdatedTime更新为当前的UTC时间
		  }
	}
    if (u1_rx_size)//串口获取到imu数据
	{
		u1_rx_size=0;
		//进行数据解析
		 if (u1_buffer[0] != 0xFA && u1_buffer[1] != 0xFF&&u1_buffer[2] != 0x36) {
			printf("Invalid preamble or bus identifier\n");

		}
		int idx = 4;
		uint32_t raw;
		// 欧拉角
		if (u1_buffer[idx] != 0x20 || u1_buffer[idx+1] != 0x30) {
			printf("Invalid data type identifier for Euler angles\n");
		}
		idx += 2;
		if (u1_buffer[idx] != 0x0C) {
			printf("Invalid length for Euler angles data\n");
		}
		idx++;
		for (int i = 0; i < 3; ++i) {
			raw = (u1_buffer[idx + 4 * i] << 24) |
				  (u1_buffer[idx + 1 + 4 * i] << 16) |
				  (u1_buffer[idx + 2 + 4 * i] << 8) |
				  u1_buffer[idx + 3 + 4 * i];
			float *angle = (float*)&raw;
			switch (i) {
				case 0: data.roll = *angle; break;
				case 1: data.pitch = *angle; break;
				case 2: data.yaw = *angle; break;
			}
		}
		idx += 12;

//////////////////////////////////加速度//////////////////////////////////////////////
		if (u1_buffer[idx] != 0x40 || u1_buffer[idx+1] != 0x20) {
			printf("Invalid data type identifier for acceleration\n");
		}
		idx += 2;
		if (u1_buffer[idx] != 0x0C) {
			printf("Invalid length for acceleration data\n");
		}
		idx++;
		for (int i = 0; i < 3; ++i) {
			raw = (u1_buffer[idx + 4 * i] << 24) |
				  (u1_buffer[idx + 1 + 4 * i] << 16) |
				  (u1_buffer[idx + 2 + 4 * i] << 8) |
				  u1_buffer[idx + 3 + 4 * i];
			float *accel = (float*)&raw;
			switch (i) {
				case 0: data.ax = *accel; break;
				case 1: data.ay = *accel; break;
				case 2: data.az = *accel; break;
			}
		}
		idx += 12;
////////////////////////////////// 自由加速度//////////////////////////////////////////////
		if (u1_buffer[idx] != 0x40 || u1_buffer[idx+1] != 0x30) {
			printf("Invalid data type identifier for acc_free\n");
		}
		idx += 2;
		if (u1_buffer[idx] != 0x0C) {
			printf("Invalid length for acc_free data\n");
		}
		idx++;
		for (int i = 0; i < 3; ++i) {
			raw = (u1_buffer[idx + 4 * i] << 24) |
				  (u1_buffer[idx + 1 + 4 * i] << 16) |
				  (u1_buffer[idx + 2 + 4 * i] << 8) |
				  u1_buffer[idx + 3 + 4 * i];
			float *Acc_free = (float*)&raw;
			switch (i) {
				case 0: data.ax_free = *Acc_free; break;
				case 1: data.ay_free = *Acc_free; break;
				case 2: data.az_free = *Acc_free; break;
			}
		}
		idx += 12;
////////////////////////////////// 角速度//////////////////////////////////////////////
		if (u1_buffer[idx] != 0x80 || u1_buffer[idx+1] != 0x20) {
			printf("Invalid data type identifier for gyro\n");
		}
		idx += 2;
		if (u1_buffer[idx] != 0x0C) {
			printf("Invalid length for gyro data\n");
		}
		idx++;
		for (int i = 0; i < 3; ++i) {
			raw = (u1_buffer[idx + 4 * i] << 24) |
				  (u1_buffer[idx + 1 + 4 * i] << 16) |
				  (u1_buffer[idx + 2 + 4 * i] << 8) |
				  u1_buffer[idx + 3 + 4 * i];
			float *gyro = (float*)&raw;
			switch (i) {
				case 0: data.gx = *gyro; break;
				case 1: data.gy = *gyro; break;
				case 2: data.gz = *gyro; break;
			}
		}
		idx += 12;

//////////////////////////////////时间戳//////////////////////////////////////////////////
		// 计算小数秒
		float fractionalSeconds = ss001 * 0.01;
		// 将小数秒转换为字符串
		char fractionalStr[10];
		snprintf(fractionalStr, sizeof(fractionalStr), "%.2f", fractionalSeconds);
		// 提取小数部分（跳过前两个字符，即 '0' 和 '.'）
		char* decimalPart = fractionalStr + 2;
		// 拼接整数秒和小数秒
		snprintf(timestamp, sizeof(timestamp), "%s.%s", lastUpdatedTime, decimalPart);

		char imubuffer[102]; // 足够大以容纳所有数据的字符串
			// 将数据格式化为字符串，包括时间戳
		int len= snprintf(imuBuffer, IMU_BUFFER_SIZE, "%s,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f\r\n",
				timestamp, // 时间戳
					data.roll, data.pitch, data.yaw,
					data.ax, data.ay, data.az,
					data.ax_free, data.ay_free, data.az_free,
					data.gx, data.gy, data.gz);
//					data.mx, data.my, data.mz);
/////////////////////////////////串口发射区//////////////////////////////////////////////////
		if (RingBuffer_Put(&imuDataBuffer, imuBuffer) == 0) {
		    // 处理缓冲区已满的情况（例如，丢弃数据或记录错误）
		}
		// 在主循环或DMA传输完成的中断处理函数中
		if ((!isTxInProgress && RingBuffer_Get(&imuDataBuffer, dataToSend))|| txComplete) {
			isTxInProgress = 1;
			txComplete = 0;
			HAL_UART_Transmit_DMA(&huart5, (uint8_t*)dataToSend, strlen(dataToSend));
		}
	 }
//////////////////////////////////GPS处理区//////////////////////////////////////////////////
    else if(u2_rx_size)//GPS数据来
	{
    	//把GPS也设置成缓冲区进行发射，用于避免imu和gps数据同时出现时候只发送一个的情况
        // 检查环形缓冲区是否有空间
        if (RingBuffer_Put(&imuDataBuffer, (char*)u2_buffer) == 0) {
            // 处理缓冲区已满的情况（例如，丢弃数据或记录错误）
        }
//         检查环形缓冲区是否有待发送的IMU数据
		if ((!isTxInProgress && RingBuffer_Get(&imuDataBuffer, dataToSend))|| txComplete) {
			isTxInProgress = 1;
			txComplete = 0;
			HAL_UART_Transmit_DMA(&huart5, (uint8_t*)dataToSend, strlen(dataToSend));
		}
		memcpy(gpsBuffer, u2_buffer,u2_rx_size);
		u2_rx_size=0;
		extractUTC(gpsBuffer, utcTime);//提取其中的整数秒UTC时间
    	if(getInitTime_flag==1)//第一次来UTC数据
    	{
    		strcpy(lastUpdatedTime,utcTime);//初始化lastUpdatedTime(只有整数秒）
    		getInitTime_flag = 2;
    		PPS_isArrived=false;
    	}
	}
    // 检查环形缓冲区是否有数据，并且串口是否空闲
    if (!isTxInProgress && RingBuffer_Get(&imuDataBuffer, dataToSend)) {
        isTxInProgress = 1; // 设置串口为忙状态
        HAL_UART_Transmit_DMA(&huart5, (uint8_t*)dataToSend, strlen(dataToSend));
    }

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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 96;
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
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
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

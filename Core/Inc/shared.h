#ifndef SHARED_H_
#define SHARED_H_

#include <stdint.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <gpio.h>
#include <stdlib.h> // 用于malloc和free



//定义结构体
typedef struct {
    float roll, pitch, yaw; // 欧拉角
    float ax, ay, az;       // 加速度
    float ax_free,ay_free,az_free;//自由加速度
    float gx, gy, gz;       // 角速度
    float mx, my, mz;       // 磁力计
} IMU_data;

//高低电平宏定�???
#define HIGH GPIO_PIN_SET
#define LOW GPIO_PIN_RESET

#define LED0_ON() HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, HIGH)
#define LED1_ON() HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, HIGH)
#define LED2_ON() HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, HIGH)

//灯熄�????
#define LED0_OFF() HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, LOW)
#define LED1_OFF() HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, LOW)
#define LED2_OFF() HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, LOW)

//状�?�翻�????
#define LED0_TOG() HAL_GPIO_TogglePin(LED0_GPIO_Port,LED0_Pin)
#define LED1_TOG() HAL_GPIO_TogglePin(LED1_GPIO_Port,LED0_Pin)
#define LED2_TOG() HAL_GPIO_TogglePin(LED2_GPIO_Port,LED0_Pin)
void Check_LED();
void Check_BEED();

// 全局变量声明
extern uint8_t hh,mm,ss,ss001;//时分秒 0.01s
extern bool PPS_isArrived;//判断PPS是否到达

#ifndef IMUhz
#define IMUhz 100
#endif
#define BUFFER_SIZE 2048 // 根据需要调整缓冲区大小


//char ImuGpsbuffer[BUFFER_SIZE];
//int bufferIndex = 0;

int isLeapYear(int year);
int getDaysInMonth(int year, int month);
void incrementTime(char *timeStr, int incrementSeconds);
IMU_data analyseImuData(uint8_t imuData[80]);
void IMUDataToBuffer(IMU_data *imu_data, uint8_t *dataimu);
void IMUDataToByteArray(IMU_data *imu_data, uint8_t *byteArray);

#define BUFFER_SIZE 50  // 缓冲区可以存储的字符串数量
#define STRING_LENGTH 200  // 单个字符串的最大长度

typedef struct {
    char buffer[BUFFER_SIZE][STRING_LENGTH];  // 缓冲区数组
    int head;  // 指向下一个要读取的数据
    int tail;  // 指向下一个要写入的数据
    int count;  // 缓冲区中的数据数量
} RingBuffer;

void RingBuffer_Init(RingBuffer *ringBuffer);
int RingBuffer_Put(RingBuffer *ringBuffer, const char *data);
int RingBuffer_Get(RingBuffer *ringBuffer, char *data);

#endif // SHARED_H_

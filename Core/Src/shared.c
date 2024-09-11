/*
 * shared.c
 *
 *  Created on: Nov 30, 2023
 *      Author: Lenovo
 */
#include "shared.h"


int isLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int getDaysInMonth(int year, int month) {
    static const int daysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if (month == 2 && isLeapYear(year)) {
        return 29;
    }
    return daysInMonth[month - 1];
}

void incrementTime(char *timeStr, int incrementSeconds) {
    int year, month, day, hour, minute, second;
    sscanf(timeStr, "%4d%2d%2d%2d%2d%2d", &year, &month, &day, &hour, &minute, &second);

    second += incrementSeconds;
    while (second >= 60) {
        second -= 60;
        minute++;
        if (minute >= 60) {
            minute = 0;
            hour++;
            if (hour >= 24) {
                hour = 0;
                day++;
                if (day > getDaysInMonth(year, month)) {
                    day = 1;
                    month++;
                    if (month > 12) {
                        month = 1;
                        year++;
                    }
                }
            }
        }
    }

    sprintf(timeStr, "%04d%02d%02d%02d%02d%02d", year, month, day, hour, minute, second);
}


void IMUDataToString(const IMU_data *data, char *str, size_t maxSize) {
    snprintf(str, maxSize, "%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f",
             data->roll, data->pitch, data->yaw,
             data->ax, data->ay, data->az,
             data->gx, data->gy, data->gz,
             data->mx, data->my, data->mz);
}



//Check LED
void Check_LED()
{
	LED0_ON();
	HAL_Delay(300);
	LED0_OFF();
	HAL_Delay(300);

	LED1_ON();
	HAL_Delay(300);
	LED1_OFF();
	HAL_Delay(300);

	LED2_ON();
	HAL_Delay(300);
	LED2_OFF();
	HAL_Delay(300);
}

//上电后蜂鸣器响一次，计时器开始启�?
void Check_BEED()
{
	int i=0;
  //Start BEED
  while(i<50000)
  {
	  HAL_GPIO_WritePin(BEED_GPIO_Port, BEED_Pin, HIGH);
	  i++;
  }
  HAL_GPIO_TogglePin(BEED_GPIO_Port, BEED_Pin);//close BEED
}

// 初始化环形缓冲区
void RingBuffer_Init(RingBuffer *ringBuffer) {
    ringBuffer->head = 0;
    ringBuffer->tail = 0;
    ringBuffer->count = 0;
}

// 向环形缓冲区添加数据
int RingBuffer_Put(RingBuffer *ringBuffer, const char *data) {
    if (ringBuffer->count >= BUFFER_SIZE) {
        return 0;  // 缓冲区已满
    }
    strncpy(ringBuffer->buffer[ringBuffer->tail], data, STRING_LENGTH);
    ringBuffer->tail = (ringBuffer->tail + 1) % BUFFER_SIZE;
    ringBuffer->count++;
    return 1;  // 数据添加成功
}

// 从环形缓冲区获取数据
int RingBuffer_Get(RingBuffer *ringBuffer, char *data) {
    if (ringBuffer->count == 0) {
        return 0;  // 缓冲区为空
    }
    strncpy(data, ringBuffer->buffer[ringBuffer->head], STRING_LENGTH);
    ringBuffer->head = (ringBuffer->head + 1) % BUFFER_SIZE;
    ringBuffer->count--;
    return 1;  // 数据获取成功
}


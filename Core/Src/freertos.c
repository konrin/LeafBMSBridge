/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "bridge.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef StaticQueue_t osStaticMessageQDef_t;
/* USER CODE BEGIN PTD */
extern CAN_HandleTypeDef *bmsCanBus;
extern CAN_HandleTypeDef *vcmCanBus;
extern CAN_HandleTypeDef hcan1;
extern CAN_HandleTypeDef hcan2;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for SendToBMSCanMessageTask */
osThreadId_t SendToBMSCanMessageTaskHandle;
const osThreadAttr_t SendToBMSCanMessageTask_attributes = {
  .name = "SendToBMSCanMessageTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for SendToVCMCanMessageTask */
osThreadId_t SendToVCMCanMessageTaskHandle;
const osThreadAttr_t SendToVCMCanMessageTask_attributes = {
  .name = "SendToVCMCanMessageTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for ProcessingVCMCanMessageTask */
osThreadId_t ProcessingVCMCanMessageTaskHandle;
const osThreadAttr_t ProcessingVCMCanMessageTask_attributes = {
  .name = "ProcessingVCMCanMessageTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for ProcessingBMSCanMessageTask */
osThreadId_t ProcessingBMSCanMessageTaskHandle;
const osThreadAttr_t ProcessingBMSCanMessageTask_attributes = {
  .name = "ProcessingBMSCanMessageTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for SendBmbServiceCellVoltageRequestTask */
osThreadId_t SendBmbServiceCellVoltageRequestTaskHandle;
const osThreadAttr_t SendBmbServiceCellVoltageRequestTask_attributes = {
  .name = "SendBmbServiceCellVoltageRequestTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for SendBmbServiceTemperatureRequestTask */
osThreadId_t SendBmbServiceTemperatureRequestTaskHandle;
const osThreadAttr_t SendBmbServiceTemperatureRequestTask_attributes = {
  .name = "SendBmbServiceTemperatureRequestTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for ProcessingVCMCanMessageQueue */
osMessageQueueId_t ProcessingVCMCanMessageQueueHandle;
uint8_t ProcessingVCMCanMessageQueueBuffer[ 4 * sizeof( CanMessage_t ) ];
osStaticMessageQDef_t ProcessingVCMCanMessageQueueControlBlock;
const osMessageQueueAttr_t ProcessingVCMCanMessageQueue_attributes = {
  .name = "ProcessingVCMCanMessageQueue",
  .cb_mem = &ProcessingVCMCanMessageQueueControlBlock,
  .cb_size = sizeof(ProcessingVCMCanMessageQueueControlBlock),
  .mq_mem = &ProcessingVCMCanMessageQueueBuffer,
  .mq_size = sizeof(ProcessingVCMCanMessageQueueBuffer)
};
/* Definitions for ToBMSCanMessageQueue */
osMessageQueueId_t ToBMSCanMessageQueueHandle;
uint8_t ToBMSCanMessageQueueBuffer[ 4 * sizeof( CanMessage_t ) ];
osStaticMessageQDef_t ToBMSCanMessageQueueControlBlock;
const osMessageQueueAttr_t ToBMSCanMessageQueue_attributes = {
  .name = "ToBMSCanMessageQueue",
  .cb_mem = &ToBMSCanMessageQueueControlBlock,
  .cb_size = sizeof(ToBMSCanMessageQueueControlBlock),
  .mq_mem = &ToBMSCanMessageQueueBuffer,
  .mq_size = sizeof(ToBMSCanMessageQueueBuffer)
};
/* Definitions for ToVCMCanMessageQueue */
osMessageQueueId_t ToVCMCanMessageQueueHandle;
uint8_t ToVCMCanMessageQueueBuffer[ 4 * sizeof( CanMessage_t ) ];
osStaticMessageQDef_t ToVCMCanMessageQueueControlBlock;
const osMessageQueueAttr_t ToVCMCanMessageQueue_attributes = {
  .name = "ToVCMCanMessageQueue",
  .cb_mem = &ToVCMCanMessageQueueControlBlock,
  .cb_size = sizeof(ToVCMCanMessageQueueControlBlock),
  .mq_mem = &ToVCMCanMessageQueueBuffer,
  .mq_size = sizeof(ToVCMCanMessageQueueBuffer)
};
/* Definitions for ProcessingBMSCanMessageQueue */
osMessageQueueId_t ProcessingBMSCanMessageQueueHandle;
uint8_t ProcessingBMSCanMessageQueueBuffer[ 10 * sizeof( CanMessage_t ) ];
osStaticMessageQDef_t ProcessingBMSCanMessageQueueControlBlock;
const osMessageQueueAttr_t ProcessingBMSCanMessageQueue_attributes = {
  .name = "ProcessingBMSCanMessageQueue",
  .cb_mem = &ProcessingBMSCanMessageQueueControlBlock,
  .cb_size = sizeof(ProcessingBMSCanMessageQueueControlBlock),
  .mq_mem = &ProcessingBMSCanMessageQueueBuffer,
  .mq_size = sizeof(ProcessingBMSCanMessageQueueBuffer)
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
extern void SendToBMSCanMessageTaskHandler(void *argument);
extern void SendToVCMCanMessageTaskHandler(void *argument);
extern void ProcessingVCMCanMessageTaskHandler(void *argument);
extern void ProcessingBMSCanMessageTaskHandler(void *argument);
extern void SendBmbServiceCellVoltageRequestTaskHandler(void *argument);
extern void SendBmbServiceTemperatureRequestTaskHandler(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of ProcessingVCMCanMessageQueue */
  ProcessingVCMCanMessageQueueHandle = osMessageQueueNew (4, sizeof(CanMessage_t), &ProcessingVCMCanMessageQueue_attributes);

  /* creation of ToBMSCanMessageQueue */
  ToBMSCanMessageQueueHandle = osMessageQueueNew (4, sizeof(CanMessage_t), &ToBMSCanMessageQueue_attributes);

  /* creation of ToVCMCanMessageQueue */
  ToVCMCanMessageQueueHandle = osMessageQueueNew (4, sizeof(CanMessage_t), &ToVCMCanMessageQueue_attributes);

  /* creation of ProcessingBMSCanMessageQueue */
  ProcessingBMSCanMessageQueueHandle = osMessageQueueNew (10, sizeof(CanMessage_t), &ProcessingBMSCanMessageQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of SendToBMSCanMessageTask */
  SendToBMSCanMessageTaskHandle = osThreadNew(SendToBMSCanMessageTaskHandler, NULL, &SendToBMSCanMessageTask_attributes);

  /* creation of SendToVCMCanMessageTask */
  SendToVCMCanMessageTaskHandle = osThreadNew(SendToVCMCanMessageTaskHandler, NULL, &SendToVCMCanMessageTask_attributes);

  /* creation of ProcessingVCMCanMessageTask */
  ProcessingVCMCanMessageTaskHandle = osThreadNew(ProcessingVCMCanMessageTaskHandler, NULL, &ProcessingVCMCanMessageTask_attributes);

  /* creation of ProcessingBMSCanMessageTask */
  ProcessingBMSCanMessageTaskHandle = osThreadNew(ProcessingBMSCanMessageTaskHandler, NULL, &ProcessingBMSCanMessageTask_attributes);

  /* creation of SendBmbServiceCellVoltageRequestTask */
  SendBmbServiceCellVoltageRequestTaskHandle = osThreadNew(SendBmbServiceCellVoltageRequestTaskHandler, NULL, &SendBmbServiceCellVoltageRequestTask_attributes);

  /* creation of SendBmbServiceTemperatureRequestTask */
  SendBmbServiceTemperatureRequestTaskHandle = osThreadNew(SendBmbServiceTemperatureRequestTaskHandler, NULL, &SendBmbServiceTemperatureRequestTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
 * @brief  Function implementing the defaultTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  CAN_TxHeaderTypeDef header;
  uint32_t TxMailbox;

  header.StdId = 0x1DB;
  header.ExtId = 0;
  header.IDE = CAN_ID_STD;
  header.RTR = CAN_RTR_DATA;
  header.DLC = 8;
  header.TransmitGlobalTime = DISABLE;

  // FF60C56A6300025E
  uint8_t data[8] = {0xFF, 0x60, 0xC5, 0x6A, 0x63, 0x00, 0x02, 0x5E};

  for (;;)
  {
    // HAL_CAN_AddTxMessage(vcmCanBus, &header, data, &TxMailbox);

    // osDelay(2000);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */


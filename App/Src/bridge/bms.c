#include "bridge.h"
#include "ev.h"
#include "can.h"
#include "car.h"
#include "bms_service_data.h"
#include "config.h"
#include "cmsis_os2.h"
#include <string.h>

extern BridgeParameters_t bridgeParameters;
extern osMessageQueueId_t ToVCMCanMessageQueueHandle;
extern osMessageQueueId_t ToBMSCanMessageQueueHandle;
extern osMessageQueueId_t ProcessingBMSCanMessageQueueHandle;

CAN_HandleTypeDef *bmsCanBus;
CAN_HandleTypeDef *bmsCanBus;

void settingBMSCanBus(CAN_HandleTypeDef *hcan)
{
    bmsCanBus = hcan;

    CAN_FilterTypeDef sFilterConfig;
    sFilterConfig.FilterBank = 0;
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    sFilterConfig.FilterIdHigh = 0x0000;
    sFilterConfig.FilterIdLow = 0x0000;
    sFilterConfig.FilterMaskIdHigh = 0x0000;
    sFilterConfig.FilterMaskIdLow = 0x0000;
    sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
    sFilterConfig.FilterActivation = ENABLE;

    if (HAL_CAN_ConfigFilter(bmsCanBus, &sFilterConfig) != HAL_OK)
    {
        Error_Handler();
    }
}

void startBmsCanBus(void)
{
    if (HAL_CAN_Start(bmsCanBus) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_CAN_ActivateNotification(bmsCanBus, CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_ERROR | CAN_IT_BUSOFF | CAN_IT_LAST_ERROR_CODE) != HAL_OK)
    {
        Error_Handler();
    }
}

bool isBmsCanBus(CAN_HandleTypeDef *hcan)
{
    return hcan == bmsCanBus;
}

void SendToBMSCanMessageTaskHandler(void *argument)
{
    CanMessage_t message;
    CAN_TxHeaderTypeDef header;
    uint32_t TxMailbox = 0;

    while (1)
    {
        // Очищаем данные сообщения
        memset(message.data, 0, sizeof(message.data));

        if (osMessageQueueGet(ToBMSCanMessageQueueHandle, &message, NULL, osWaitForever) != osOK)
        {
            continue;
        }

        header.StdId = message.stdId;
        header.ExtId = message.extId;
        header.IDE = message.ide;
        header.RTR = message.rtr;
        header.DLC = message.dlc;
        header.TransmitGlobalTime = DISABLE;

        osKernelLock();
        HAL_StatusTypeDef result = HAL_CAN_AddTxMessage(bmsCanBus, &header, message.data, &TxMailbox);
        osKernelUnlock();

        if (result != HAL_OK)
        {
            Error_Handler();
        }
    }
}

void ProcessingBMSCanMessageTaskHandler(void *argument)
{
    CanMessage_t message;

    while (1)
    {
        memset(message.data, 0, sizeof(message.data));
        if (osMessageQueueGet(ProcessingBMSCanMessageQueueHandle, &message, NULL, osWaitForever) != osOK)
        {
            continue;
        }

        if (!bridgeIsActive())
        {
            osMessageQueuePut(ToVCMCanMessageQueueHandle, &message, 0, 0);

            return;
        }

        switch (message.stdId)
        {
        case 0x1DB:
            processingCan1DBMessage(&message);
            break;
        case 0x7BB:
            processingCan7BBMessage(&message);
            break;
        }

        osMessageQueuePut(ToVCMCanMessageQueueHandle, &message, 0, 0);
    }
}

void processingCan1DBMessage(CanMessage_t *message)
{
    Can1DBMessage_t can1DBMessage;

    if (CanDecode1DBMessage(&can1DBMessage, message->data) != canDecodeStatusOk)
    {
        return;
    }

    CanCalcCRC8(message->data);

    osMessageQueuePut(ToVCMCanMessageQueueHandle, &message, 0, 0);

    carOn(false);
    evSetInstantVoltageAndCurrent(can1DBMessage.voltage, can1DBMessage.current);
    carAddEnergySpentForEvState();
}

void processingCan7BBMessage(CanMessage_t *message)
{
    BmsServiceProcessingAnswerFromBms(message);
}

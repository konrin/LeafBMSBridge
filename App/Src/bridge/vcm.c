#include "bridge.h"
#include "car.h"
#include "bms_service_data.h"
#include "config.h"
#include "cmsis_os.h"
#include <string.h>

extern BridgeParameters_t bridgeParameters;
extern osMessageQueueId_t ProcessingVCMCanMessageQueueHandle;
extern osMessageQueueId_t ToBMSCanMessageQueueHandle;
extern osMessageQueueId_t ToVCMCanMessageQueueHandle;

CAN_HandleTypeDef *vcmCanBus;

void settingVcmCanBus(CAN_HandleTypeDef *hcan)
{
    vcmCanBus = hcan;

    CAN_FilterTypeDef sFilterConfig;
    sFilterConfig.FilterBank = 12;
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    sFilterConfig.FilterIdHigh = 0x0000;
    sFilterConfig.FilterIdLow = 0x0000;
    sFilterConfig.FilterMaskIdHigh = 0x0000;
    sFilterConfig.FilterMaskIdLow = 0x0000;
    sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO1;
    sFilterConfig.FilterActivation = ENABLE;
    sFilterConfig.SlaveStartFilterBank = 12;

    if (HAL_CAN_ConfigFilter(vcmCanBus, &sFilterConfig) != HAL_OK)
    {
        Error_Handler();
    }
}

void startVcmCanBus(void)
{
    if (HAL_CAN_Start(vcmCanBus) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_CAN_ActivateNotification(vcmCanBus, CAN_IT_RX_FIFO1_MSG_PENDING | CAN_IT_ERROR | CAN_IT_BUSOFF | CAN_IT_LAST_ERROR_CODE) != HAL_OK)
    {
        Error_Handler();
    }
}

bool isVcmCanBus(CAN_HandleTypeDef *hcan)
{
    return hcan == vcmCanBus;
}

void SendToVCMCanMessageTaskHandler(void *argument)
{
    CanMessage_t message;
    CAN_TxHeaderTypeDef header;
    uint32_t TxMailbox;

    while (1)
    {
        memset(message.data, 0, sizeof(message.data));
        if (osMessageQueueGet(ToVCMCanMessageQueueHandle, &message, 0, osWaitForever) != osOK)
        {
            continue;
        }

        header.StdId = message.stdId;
        header.ExtId = message.extId;
        header.IDE = message.ide;
        header.RTR = message.rtr;
        header.DLC = message.dlc;
        header.TransmitGlobalTime = DISABLE;

        // osKernelLock();
        // HAL_StatusTypeDef result = HAL_CAN_AddTxMessage(vcmCanBus, &header, message.data, &TxMailbox);
        // osKernelUnlock();

        // if (result != HAL_OK)
        // {
        //     Error_Handler();
        // }
    }
}

void ProcessingVCMCanMessageTaskHandler(void *argument)
{
    CanMessage_t message;

    while (1)
    {
        memset(message.data, 0, sizeof(message.data));
        if (osMessageQueueGet(ProcessingVCMCanMessageQueueHandle, &message, 0, osWaitForever) != osOK)
        {
            continue;
        }

        if (!bridgeIsActive())
        {
            osMessageQueuePut(ToBMSCanMessageQueueHandle, &message, 0, 0);

            return;
        }

        switch (message.stdId)
        {
        case 0x11A:
            processingCan11AMessage(&message);
            continue;
        case 0x79B:
            processingCan79BMessage(&message);
            break;
        case 0x50A:
            processingCan50AMessage(&message);
            continue;
        case 0x603:
            // машина только что включилась
            processingCan603Message(&message);
            continue;
        default:
            break;
        }

        osMessageQueuePut(ToBMSCanMessageQueueHandle, &message, 0, 0);
    }
}

void processingCan50AMessage(CanMessage_t *message)
{
    osMessageQueuePut(ToBMSCanMessageQueueHandle, &message, 0, 0);

    if (message->dlc == 6)
    {
        carSetModel(CarLeafZE0);
    }

    if (message->dlc == 8)
    {
        carSetModel(CarLeafAZEO);
    }
}

void processingCan79BMessage(CanMessage_t *message)
{
    if (BmsServiceCanMessageIsServiceRequest(message->data))
    {
        BmsServiceProcessingRequestFromVcm(message->data);

        return;
    }

    // ...
}

void processingCan603Message(CanMessage_t *message)
{
    osMessageQueuePut(ToBMSCanMessageQueueHandle, &message, 0, 0);

    carOn(true);
}

void processingCan11AMessage(CanMessage_t *message)
{
    Can11AMessage_t can11AMessage;

    if (CanDecode11AMessage(&can11AMessage, message->data) != canDecodeStatusOk)
    {
        return;
    }

    if (can11AMessage.carIsOn) {
        carOn(false);
    } else {
        carOff(false);

        // если машина выключена, то остальные данные не актуальны
        // и их не нужно обновлять
        return;
    }

    carUpdateSelectorPosition(can11AMessage.selectorPosition);
}

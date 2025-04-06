#include "bms_service_data.h"
#include "cmsis_os2.h"

extern osMessageQueueId_t ToVCMCanMessageQueueHandle;
extern osMessageQueueId_t ToBMSCanMessageQueueHandle;

static CanMessage_t temperatureRequestCanMessage = {
    .stdId = 0x79B,
    .extId = 0x00,
    .dlc = 8,
    .rtr = false,
    .ide = false,
    .data = {0x02, 0x21, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00},
};

static CanMessage_t cellVoltageRequestCanMessage = {
    .stdId = 0x79B,
    .extId = 0x00,
    .dlc = 8,
    .rtr = false,
    .ide = false,
    .data = {0x02, 0x21, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00},
};

static const uint32_t BmsServiceTemperatureRequestDelay = 10000; // 10 секунд
static const uint32_t BmsServiceCellVoltageRequestDelay = 1000; // 1 секунда

BmsServiceSession_t bmsServiceSession = {
    .isActive = false,
    .group = BmsServiceGroupNone,
    .temperatureGroup = NULL,
    .cellVoltageGroup = NULL,
    .pendingRequestFromVcm = NULL,
    .startTiks = 0,
    .firstAnswerTiks = 0,
    .lastAnswerTiks = 0,
};

static void clearBmsServiceSession()
{
    bmsServiceSession.isActive = false;
    bmsServiceSession.group = BmsServiceGroupNone;
    bmsServiceSession.startTiks = 0;
    bmsServiceSession.firstAnswerTiks = 0;
    bmsServiceSession.lastAnswerTiks = 0;

    if (bmsServiceSession.temperatureGroup != NULL)
    {
        free(bmsServiceSession.temperatureGroup);
        bmsServiceSession.temperatureGroup = NULL;
    }

    if (bmsServiceSession.cellVoltageGroup != NULL)
    {
        free(bmsServiceSession.cellVoltageGroup);
        bmsServiceSession.cellVoltageGroup = NULL;
    }
}

bool BmsServiceCanMessageIsServiceRequest(uint8_t data[8])
{
    if (data[0] == 0x02 && data[1] == 0x21)
    {
        return true;
    }

    return false;
}

/**
 * Это обработчик задачи запроса напряжения ячеек.
 */
void SendBmbServiceCellVoltageRequestTaskHandler(void *argument)
{
    // каждую секунду
    static uint32_t delayTiks = BmsServiceCellVoltageRequestDelay;
    static uint16_t errorCount = 0;

    if (errorCount > 10)
    {
        // если 10 ошибок подряд, то не запрашиваем BMS
        // в течение 10 секунд

        errorCount = 0;
        delayTiks = BmsServiceCellVoltageRequestDelay;
    }

    osDelay(delayTiks);

    BmsServiceRequestStatus_t result = BmbServiceStartGroupCellVoltageRequestSession();

    if (result == BmsServiceRequestError)
    {
        errorCount++;
        // обработка ошибки
    }
    else if (result == BmsServiceRequestNeedResend)
    {
        errorCount++;
        delayTiks = 100;

        return;
    }

    errorCount = 0;
    delayTiks = BmsServiceCellVoltageRequestDelay;
}

/**
 * Это обработчик задачи запроса температуры.
 */
void SendBmbServiceTemperatureRequestTaskHandler(void *argument)
{
    // каждые 10 секунд
    static uint32_t delayTiks = BmsServiceTemperatureRequestDelay;
    static uint16_t errorCount = 0;
    if (errorCount > 10)
    {
        // если 10 ошибок подряд, то не запрашиваем BMS
        // в течение 10 секунд

        errorCount = 0;
        delayTiks = BmsServiceTemperatureRequestDelay;
    }

    osDelay(delayTiks);

    BmsServiceRequestStatus_t result = BmbServiceStartGroupTemperatureRequestSession();

    if (result == BmsServiceRequestError)
    {
        errorCount++;
        // обработка ошибки
    }
    else if (result == BmsServiceRequestNeedResend)
    {
        errorCount++;
        delayTiks = 100;

        return;
    }

    errorCount = 0;
    delayTiks = BmsServiceTemperatureRequestDelay;
}

bool BmbServiceIsAvailableForRequest()
{
    if (bmsServiceSession.isActive)
    {
        return false;
    }

    // если с момента последнего ответа от BMS прошло меньше 150 мс
    if (osKernelGetTickCount() - bmsServiceSession.lastAnswerTiks < 150)
    {
        return false;
    }

    return true;
}

static BmsServiceRequestStatus_t BmbServiceStartGroupCellVoltageRequestSession()
{
    if (!BmbServiceIsAvailableForRequest())
    {
        return BmsServiceRequestNeedResend;
    }

    BmsServiceGroupCellVoltage_t *cellVoltageGroup = malloc(sizeof(BmsServiceGroupCellVoltage_t));
    if (cellVoltageGroup == NULL)
    {
        // обработка ошибки выделения памяти

        return BmsServiceRequestError;
    }
    bmsServiceSession.isActive = true;
    bmsServiceSession.group = BmsServiceGroupCellVoltage;
    bmsServiceSession.cellVoltageGroup = cellVoltageGroup;
    bmsServiceSession.startTiks = osKernelGetTickCount();

    osMessageQueuePut(ToBMSCanMessageQueueHandle, &cellVoltageRequestCanMessage, 0, 0);

    return BmsServiceRequestOk;
}

static BmsServiceRequestStatus_t BmbServiceStartGroupTemperatureRequestSession()
{
    if (!BmbServiceIsAvailableForRequest())
    {
        return BmsServiceRequestNeedResend;
    }

    BmsServiceGroupTemperature_t *temperatureGroup = malloc(sizeof(BmsServiceGroupTemperature_t));

    if (temperatureGroup == NULL)
    {
        // обработка ошибки выделения памяти

        return BmsServiceRequestError;
    }

    bmsServiceSession.isActive = true;
    bmsServiceSession.group = BmsServiceGroupTemperature;
    bmsServiceSession.temperatureGroup = temperatureGroup;
    bmsServiceSession.startTiks = osKernelGetTickCount();

    osMessageQueuePut(ToBMSCanMessageQueueHandle, &temperatureRequestCanMessage, 0, 0);

    return BmsServiceRequestOk;
}

void BmsServiceProcessingAnswerFromBms(CanMessage_t *message)
{
    if (!bmsServiceSession.isActive || bmsServiceSession.group == BmsServiceGroupNone)
    {
        /**
         * Сейчас сессия не активна, значит это ответ на внешний запрос
         * от VCM. Нужно просто отправить его дальше.
         */
        osMessageQueuePut(ToVCMCanMessageQueueHandle, &message, 0, 0);

        return;
    }

    if (bmsServiceSession.group == BmsServiceGroupCellVoltage)
    {
        // обработка ответа на запрос напряжения ячеек
        BmbServiceProcessingAnswerFromBmsCellVoltageRequest(message);

        return;
    }

    if (bmsServiceSession.group == BmsServiceGroupTemperature)
    {
        // обработка ответа на запрос температуры
        BmbServiceProcessingAnswerFromBmsTemperatureRequest(message);

        return;
    }
}

static void BmbServiceProcessingAnswerFromBmsTemperatureRequest(CanMessage_t *message)
{
    if (message->data[0] == 0x10) // First message
    {
        bmsServiceSession.firstAnswerTiks = osKernelGetTickCount();

        bmsServiceSession.temperatureGroup->temperature[0] = (message->data[4] << 8) | message->data[5];
        bmsServiceSession.temperatureGroup->temperature[1] = (message->data[7] << 8);
    }
    else if (message->data[0] == 0x21) // Second message
    {
        bmsServiceSession.temperatureGroup->temperature[1] |= message->data[1];
        bmsServiceSession.temperatureGroup->temperature[2] = (message->data[3] << 8) | message->data[4];
        bmsServiceSession.temperatureGroup->temperature[3] = (message->data[6] << 8) | message->data[7];
    }
    else if (message->data[0] == 0x22) // Third message
    {
        uint16_t *temps = bmsServiceSession.temperatureGroup->temperature;

        if (temps[2] == 65535) // Only three sensors available
        {
            bmsServiceSession.temperatureGroup->max = temps[0];
            bmsServiceSession.temperatureGroup->min = temps[0];

            if (temps[1] > bmsServiceSession.temperatureGroup->max)
                bmsServiceSession.temperatureGroup->max = temps[1];
            if (temps[3] > bmsServiceSession.temperatureGroup->max)
                bmsServiceSession.temperatureGroup->max = temps[3];

            if (temps[1] < bmsServiceSession.temperatureGroup->min)
                bmsServiceSession.temperatureGroup->min = temps[1];
            if (temps[3] < bmsServiceSession.temperatureGroup->min)
                bmsServiceSession.temperatureGroup->min = temps[3];

            bmsServiceSession.temperatureGroup->average = (temps[0] + temps[1] + temps[3]) / 3;
        }
        else // All four sensors available
        {
            bmsServiceSession.temperatureGroup->max = temps[0];
            bmsServiceSession.temperatureGroup->min = temps[0];

            uint32_t sum = temps[0];
            for (int i = 1; i < 4; i++)
            {
                if (temps[i] > bmsServiceSession.temperatureGroup->max)
                    bmsServiceSession.temperatureGroup->max = temps[i];
                if (temps[i] < bmsServiceSession.temperatureGroup->min)
                    bmsServiceSession.temperatureGroup->min = temps[i];

                sum += temps[i];
            }
            bmsServiceSession.temperatureGroup->average = sum / 4;
        }

        // todo сохранить данные в EV состояние

        clearBmsServiceSession();
    }
}

static void BmbServiceProcessingAnswerFromBmsCellVoltageRequest(CanMessage_t *message)
{
    if (message->data[0] == 0x10) // First frame is anomalous
    {
        bmsServiceSession.firstAnswerTiks = osKernelGetTickCount();

        bmsServiceSession.cellVoltageGroup->currentId = 0;
        bmsServiceSession.cellVoltageGroup->voltage[bmsServiceSession.cellVoltageGroup->currentId++] = (message->data[4] << 8) | message->data[5];
        bmsServiceSession.cellVoltageGroup->voltage[bmsServiceSession.cellVoltageGroup->currentId++] = (message->data[6] << 8) | message->data[7];
    }
    else if (message->data[6] == 0xFF && message->data[0] == 0x2C) // Last frame
    {
        // Calculate min/max voltages
        bmsServiceSession.cellVoltageGroup->voltage[bmsServiceSession.cellVoltageGroup->currentId] = 0; // Ensure no overflow
        uint16_t minVoltage = 9999;
        uint16_t maxVoltage = 0;
        uint32_t sumVoltage = 0;

        for (uint8_t i = 0; i < 96; i++)
        {
            if (bmsServiceSession.cellVoltageGroup->voltage[i] < minVoltage)
                minVoltage = bmsServiceSession.cellVoltageGroup->voltage[i];
            if (bmsServiceSession.cellVoltageGroup->voltage[i] > maxVoltage)
                maxVoltage = bmsServiceSession.cellVoltageGroup->voltage[i];

            sumVoltage += bmsServiceSession.cellVoltageGroup->voltage[i];
        }

        bmsServiceSession.cellVoltageGroup->min = minVoltage;
        bmsServiceSession.cellVoltageGroup->max = maxVoltage;
        bmsServiceSession.cellVoltageGroup->average = sumVoltage / 96;

        // todo сохранить данные в EV состояние

        clearBmsServiceSession();
    }
    else if ((message->data[0] % 2) == 0) // Even frames
    {
        bmsServiceSession.cellVoltageGroup->voltage[bmsServiceSession.cellVoltageGroup->currentId++] |= message->data[1];
        bmsServiceSession.cellVoltageGroup->voltage[bmsServiceSession.cellVoltageGroup->currentId++] = (message->data[2] << 8) | message->data[3];
        bmsServiceSession.cellVoltageGroup->voltage[bmsServiceSession.cellVoltageGroup->currentId++] = (message->data[4] << 8) | message->data[5];
        bmsServiceSession.cellVoltageGroup->voltage[bmsServiceSession.cellVoltageGroup->currentId++] = (message->data[6] << 8) | message->data[7];
    }
    else // Odd frames
    {
        bmsServiceSession.cellVoltageGroup->voltage[bmsServiceSession.cellVoltageGroup->currentId++] = (message->data[1] << 8) | message->data[2];
        bmsServiceSession.cellVoltageGroup->voltage[bmsServiceSession.cellVoltageGroup->currentId++] = (message->data[3] << 8) | message->data[4];
        bmsServiceSession.cellVoltageGroup->voltage[bmsServiceSession.cellVoltageGroup->currentId++] = (message->data[5] << 8) | message->data[6];
        bmsServiceSession.cellVoltageGroup->voltage[bmsServiceSession.cellVoltageGroup->currentId] = (message->data[7] << 8);
    }
}

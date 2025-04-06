#ifndef BMS_SERVICE_DATA_H
#define BMS_SERVICE_DATA_H

#include <stdbool.h>
#include "main.h"
#include "can.h"

typedef enum{
    BmsServiceGroupNone,
    // Group 0x02
    BmsServiceGroupCellVoltage,
    // Group 0x04
    BmsServiceGroupTemperature,
} BmsServiceGroup_t;

typedef enum
{
    BmsServiceRequestOk,
    BmsServiceRequestError,
    BmsServiceRequestNeedResend,
} BmsServiceRequestStatus_t;

typedef struct
{
    uint16_t temperature[4];
    uint16_t max;
    uint16_t min;
    uint16_t average;
} BmsServiceGroupTemperature_t;

typedef struct
{
    uint8_t currentId;
    uint16_t voltage[96];
    uint16_t max;
    uint16_t min;
    uint16_t average;
} BmsServiceGroupCellVoltage_t;

typedef struct
{
    bool isActive;
    BmsServiceGroup_t group;

    BmsServiceGroupTemperature_t *temperatureGroup;
    BmsServiceGroupCellVoltage_t *cellVoltageGroup;

    uint8_t *pendingRequestFromVcm;

    // время начала сессии
    uint32_t startTiks;
    // время первого ответа от BMS на запрос
    uint32_t firstAnswerTiks;
    // время последнего любого ответа от BMS (0x7bb)
    uint32_t lastAnswerTiks;
} BmsServiceSession_t;

void BmsServiceProcessingRequestFromVcm(CanMessage_t *message);
void BmsServiceProcessingAnswerFromBms(CanMessage_t *message);

bool BmsServiceCanMessageIsServiceRequest(uint8_t data[8]);

bool BmbServiceIsAvailableForRequest();
// Старт сессии запроса напряжения ячеек
BmsServiceRequestStatus_t BmbServiceStartGroupCellVoltageRequestSession();
// Старт сессии запроса температуры
BmsServiceRequestStatus_t BmbServiceStartGroupTemperatureRequestSession();

// Обработка ответа на запрос температуры
void BmbServiceProcessingAnswerFromBmsTemperatureRequest(CanMessage_t *message);
// Обработка ответа на запрос напряжения ячеек
void BmbServiceProcessingAnswerFromBmsCellVoltageRequest(CanMessage_t *message);


#endif /* BMS_SERVICE_DATA_H */

#ifndef CAN_H
#define CAN_H

#include <stdbool.h>
#include "car_typedef.h"

#ifndef UNIT_TEST
#include "main.h"
#else
#include <stdio.h>
#include <stdlib.h>
#endif

typedef enum
{
    canDecodeStatusOk,
    canDecodeStatusError,
} CanDecodeStatus_t;

typedef struct
{
    float voltage;
    float current;

    uint8_t failsafeStatus;
    bool relayCutRequest;
    bool mainRelayOn;
    bool fullChargeFlag;
} Can1DBMessage_t;

typedef struct
{
    CarSelectorPosition_t selectorPosition;
    bool carIsOn;
    bool ecoModeIsOn;
} Can11AMessage_t;

typedef struct
{
    ChargingStatus_t chargingStatus;
    // Зарядка только до 80%
    bool isEnergyCapacity80Percent;
} Can1F2Message_t;

typedef struct
{
    uint32_t stdId;
    uint32_t extId;
    uint32_t ide;
    uint32_t rtr;
    // кол-во байт данных
    uint32_t dlc;
    uint8_t data[8];
} CanMessage_t;

typedef enum
{
    CAN_MESSAGE_FILTER_OK,
    CAN_MESSAGE_FILTER_SKIP,
} CanMessageFilterResult_t;

// Recalculates the CRC-8 with 0x85 poly
// 0x1DB 0x55B 0x59E
void CanCalcCRC8(uint8_t data[8]);

CanDecodeStatus_t CanDecode1DBMessage(Can1DBMessage_t *message, uint8_t data[8]);
void CanEncode1DBSetRequestChargingStop(uint8_t data[8]);
void CanEncode1DBSetFullChargeCompleted(uint8_t data[8]);

CanDecodeStatus_t CanDecode11AMessage(Can11AMessage_t *message, uint8_t data[8]);

#endif /* CAN_H */

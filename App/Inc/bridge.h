#ifndef BRIDGE_H
#define BRIDGE_H

#include <stdbool.h>
#include "main.h"
#include "cmsis_os2.h"
#include "can.h"

/**
 * @brief Сохраняемая структура
 *
 */
typedef struct
{
    bool isActive;
    // заявленная ёмкость в ватах
    uint16_t totalCapacity;
    // реальная ёмкость в ватах
    uint16_t realCapacity;
    // максимальное общее напряжение ВВБ
    uint16_t maxVoltage;
    // минимальное напряжение ВВБ
    uint16_t minVoltage;
    // максимальное напряжение на одной ячейке
    uint16_t maxCellVoltage;
    // минимальное напряжение на одной ячейке
    uint16_t minCellVoltage;
    // максимальная доступная мощность в процентах
    uint16_t maxPower;
    // временной промежуток между сессиями движения
    // когда они считаются как одна (в секундах)
    uint32_t driveSessionTimeout;
} BridgeParameters_t;

// --- BMS CAN BUS ---
void settingBMSCanBus(CAN_HandleTypeDef *hcan);
void startBmsCanBus(void);
bool isBmsCanBus(CAN_HandleTypeDef *hcan);
void processingCan1DBMessage(CanMessage_t *message);
void processingCan7BBMessage(CanMessage_t *message);

// --- VCM CAN BUS ---
void settingVcmCanBus(CAN_HandleTypeDef *hcan);
void startVcmCanBus(void);
bool isVcmCanBus(CAN_HandleTypeDef *hcan);
void processingCan79BMessage(CanMessage_t *message);
void processingCan603Message(CanMessage_t *message);
void processingCan50AMessage(CanMessage_t *message);
void processingCan11AMessage(CanMessage_t *message);

// --- BRIDGE ---
void startBridge(void);
bool bridgeIsActive();
void convertCanRxHeaderToCanTxHeader(CAN_RxHeaderTypeDef *rxHeader, CAN_TxHeaderTypeDef *txHeader);
/**
 * Обновляет данные сообщения данными из заголовка
 */
void updateCanMessageFromRxHeader(CAN_RxHeaderTypeDef *rxHeader, CanMessage_t *message);

#endif // BRIDGE_H

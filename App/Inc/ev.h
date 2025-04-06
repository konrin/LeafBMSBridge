#ifndef EV_H
#define EV_H

#include <stdbool.h>
#include "main.h"

typedef enum
{
    evChargeTypeAC,
    evChargeTypeDC,
} EvChargeType_t;

typedef enum
{
    // Без статуса
    evFailsafeStatusNone = 0,
    // Это означает, что батарея полностью разряжена и можно прекратить сеанс.
    evFailsafeStatusNormalStopRequest = 1,
    // Это означает, что батарея полностью заряжена и можно прекращать сеанс.
    evFailsafeStatusChargingModeStopRequest = 2,
    // Обычный запрос на остановку.
    evFailsafeStatusChargingModeAndNormalStopRequest = 3,
    // Запрос на включение лампы предупреждения
    evFailsafeStatusCautionLampRequest = 4,
    // Запрос на включение лампы предупреждения и остановку сеанса
    evFailsafeStatusCautionLampAndNormalStopRequest = 5,
    // Запрос на включение лампы предупреждения и остановку зарядной сессии
    evFailsafeStatusCautionLampAndChargingModeStopRequest = 6,
    // Запрос на включение лампы предупреждения, остановку зарядной сессии и обычный запрос на остановку
    evFailsafeStatusCautionLampAndChargingModeAndNormalStopRequest = 7,
} EvFailsafeStatus_t;

typedef struct {
    // Текущее мгновенное напряжение
    float instantVoltage;
    // Текущее Мгновенный ток
    float instantCurrent;
    // Текущая мгновенная мощность в ватах
    float instantPower;

    // Текущий заряд в процентах
    float soc;

    // Температура с датчиков в ВВБ
    uint16_t temperature[4];
    uint16_t temperatureMin;
    uint16_t temperatureMax;
    uint16_t temperatureAverage;
    // Время последнего обновления данных температуры
    uint32_t temperatureUpdatedAt;

    // Напряжение ячеек [минимальное, максимальное, среднее]
    uint16_t cellVoltage[96];
    uint16_t cellVoltageMin;
    uint16_t cellVoltageMax;
    uint16_t cellVoltageAverage;
    // Время последнего обновления данных напряжения ячеек
    uint32_t cellVoltageUpdatedAt;

    EvFailsafeStatus_t failsafeStatus;

    // FAIL. BMS запрашивает отключение и размыкание контакторов
    bool relayCutRequest;
    bool mainRelayOnFlag;

    // Батарея сообщает, что она полностью заряжена, остановите все дальнейшие зарядки, если это еще не сделано
    bool fullChargeFlag;
} EvState_t;

void initEv(void);
void evStateLock(void);
void evStateUnlock(void);

void evSetInstantVoltageAndCurrent(float voltage, float current);

float evCalculateWh(float current, float voltage, uint32_t timeMs);
#endif /* EV_H */

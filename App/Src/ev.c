#include "ev.h"
#include "bridge.h"
#include "can.h"

osMutexId_t EvStateMutex;

EvState_t evState = {
    .instantVoltage = 0.0,
    .instantCurrent = 0.0,
    .instantPower = 0.0,

    .soc = 0.0,
    .temperature = {0},
    .temperatureMin = 0,
    .temperatureMax = 0,
    .temperatureAverage = 0,
    .temperatureUpdatedAt = 0,

    .cellVoltage = {0},
    .cellVoltageMin = 0,
    .cellVoltageMax = 0,
    .cellVoltageAverage = 0,
    .cellVoltageUpdatedAt = 0,

    .failsafeStatus = evFailsafeStatusNone,
    .relayCutRequest = false,
    .mainRelayOnFlag = false,
    .fullChargeFlag = false,
};

void initEvStateMutex(void)
{
    const osMutexAttr_t EvStateMutex_attributes = {
        .name = "EvStateMutex"
    };

    EvStateMutex = osMutexNew(&EvStateMutex_attributes);
}

void initEv(void)
{
    initEvStateMutex();
}

void evStateLock(void)
{
    osMutexAcquire(EvStateMutex, osWaitForever);
}

void evStateUnlock(void)
{
    osMutexRelease(EvStateMutex);
}

void evSetInstantVoltageAndCurrent(float voltage, float current)
{
    evStateLock();
    evState.instantVoltage = voltage;
    evState.instantCurrent = current;
    evState.instantPower = voltage * current;
    // evState.energySpent += (evState.instantPower * 0.01) / 3600;
    evStateUnlock();


}

// Функция для вычисления ватт-часов
float evCalculateWh(float current, float voltage, uint32_t timeMs)
{
    // Переводим время из миллисекунд в часы
    float time_hours = timeMs / (1000.0f * 3600.0f);

    // Вычисляем затраченную энергию в ватт-часах
    float energy = current * voltage * time_hours;

    return energy;
}

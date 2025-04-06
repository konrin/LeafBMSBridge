#ifndef CAR_H
#define CAR_H

#include <stdbool.h>
#include "ev.h"
#include "can.h"
#include "car_typedef.h"

#ifndef UNIT_TEST
#include "cmsis_os2.h"
#else
#include <stdio.h>
#include <stdlib.h>
#endif

typedef struct
{
    // время начала зарядки
    uint32_t startAt;
    // время окончания зарядки
    uint32_t endAt;
    // тип зарядки
    EvChargeType_t chargeType;
    // кол-во полученной энергии в ватах
    int32_t energyReceived;
    // начальная температура батареи
    float temperature[4];
    // конечная температура батареи
    float temperatureEnd[4];
} CarChargingSession_t;

typedef struct
{
    // время начала сессии
    uint32_t startAt;
    // время окончания сессии
    uint32_t endAt;
    // кол-во потраченной энергии в ватах
    float energySpent;
    // кол-во рекуперированной энергии в ватах
    float energyRecuperated;
    // кол-во пройденных километров
    int32_t distance;
} CarDriveSession_t;

typedef struct
{
    bool carIsOn;
    bool ecoModeIsOn;
    CarModel_t carModel;
    CarSelectorPosition_t selectorPosition;

    // статистика по последней зарядке
    CarChargingSession_t *currentChargingSession;

    // история зарядок
    CarChargingSession_t *chargingSessions[5];

    // статус зарядки
    ChargingStatus_t chargingStatus;

    // статистика с момента последней полной зарядки
    CarDriveSession_t *fullChargeDriveSession;

    // статистика с момента последней частичной зарядки
    CarDriveSession_t *partialChargeDriveSession;

    // статистика с момента последнего включения автомобиля
    CarDriveSession_t *lastDriveSession;

    // история статистик с момента последнего включения автомобиля
    CarDriveSession_t *driveSessionsHistory[5];
} CarState_t;

void initCar(void);
void carStateLock(void);
void carStateUnlock(void);

void carSetModel(CarModel_t model);
bool carIsZE0();
bool carIsAZEO();
bool carIsZE1();
bool carIsCharging();

// машина включена
void carOn(bool force);
// машина выключена
void carOff(bool force);
// обновление положения селектора передач
void carUpdateSelectorPosition(CarSelectorPosition_t position);

// учесть новую порцию потреблённой энергии
void carAddEnergySpent(float energy);
void carAddEnergySpentForEvState();

// машина находится не в движении
bool carIsNotMoving();

uint8_t getCountDriveSessions();
CarDriveSession_t *getDriveSession(uint8_t index);
void newDriveSession(CarDriveSession_t *session);

uint8_t getCountChargingSessions();
CarChargingSession_t *getChargingSession(uint8_t index);
void newChargingSession(CarChargingSession_t *session);

#endif /* CAR_H */

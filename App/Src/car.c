#include "car.h"
#include "can.h"
#include "rtc.h"
#include <stdlib.h>
#include <cmsis_os2.h>

osMutexId_t CarStateMutex;

extern EvState_t evState;

CarState_t carState = {
    .carModel = CarModelUndefined,
    .selectorPosition = carSelectorP,
    .carIsOn = false,
    .ecoModeIsOn = false,
    .currentChargingSession = NULL,
    .chargingSessions = {NULL, NULL, NULL, NULL, NULL},
    .fullChargeDriveSession = NULL,
    .partialChargeDriveSession = NULL,
    .lastDriveSession = NULL,
    .driveSessionsHistory = {NULL, NULL, NULL, NULL, NULL}};

void initCarStateMutex(void)
{
    const osMutexAttr_t CarStateMutex_attributes = {
        .name = "CarStateMutex"};

    CarStateMutex = osMutexNew(&CarStateMutex_attributes);
}

void initCar(void)
{
    initCarStateMutex();
}

void carStateLock(void)
{
    osMutexAcquire(CarStateMutex, osWaitForever);
}

void carStateUnlock(void)
{
    osMutexRelease(CarStateMutex);
}

void carSetModel(CarModel_t model)
{
    carStateLock();
    carState.carModel = model;
    carStateUnlock();
}

bool carIsZE0()
{
    return carState.carModel == CarLeafZE0;
}

bool carIsAZEO()
{
    return carState.carModel == CarLeafAZEO;
}

bool carIsZE1()
{
    return carState.carModel == CarLeafZE1;
}

void carOn(bool force)
{
    if (carState.carIsOn && !force)
    {
        return;
    }

    carStateLock();
    carState.carIsOn = true;
    carStateUnlock();

    // Выделяем память для новой сессии
    CarDriveSession_t *newSession = (CarDriveSession_t *)malloc(sizeof(CarDriveSession_t));
    if (newSession == NULL)
    {
        // Обработка ошибки выделения памяти
        return;
    }

    *newSession = (CarDriveSession_t){
        .startAt = getUnixTime(),
        .endAt = 0,
        .energySpent = 0,
        .energyRecuperated = 0,
        .distance = 0};

    newDriveSession(newSession);
}

void carOff(bool force)
{
}

void carUpdateSelectorPosition(CarSelectorPosition_t newPosition)
{
    if (carState.selectorPosition == newPosition)
    {
        return;
    }

    CarSelectorPosition_t prevPosition = carState.selectorPosition;
    carState.selectorPosition = newPosition;

    if (prevPosition == carSelectorP)
    {
        if (newPosition == carSelectorD)
        {

            return;
        }

        if (newPosition == carSelectorR)
        {
            return;
        }

        if (newPosition == carSelectorN)
        {
            return;
        }
    }

    if (prevPosition == carSelectorD)
    {
        if (newPosition == carSelectorP)
        {
            return;
        }

        if (newPosition == carSelectorR)
        {
            return;
        }

        if (newPosition == carSelectorN)
        {
            return;
        }
    }

    if (prevPosition == carSelectorR)
    {
        if (newPosition == carSelectorP)
        {
            return;
        }

        if (newPosition == carSelectorD)
        {
            return;
        }

        if (newPosition == carSelectorN)
        {
            return;
        }
    }

    if (prevPosition == carSelectorN)
    {
        if (newPosition == carSelectorP)
        {
            return;
        }

        if (newPosition == carSelectorD)
        {
            return;
        }

        if (newPosition == carSelectorR)
        {
            return;
        }
    }
}

void carAddEnergySpent(float energy)
{
    if (carState.carIsOn)
    {
        if (energy > 0)
        {
            if (carIsNotMoving())
            {
                return;
            }
            
            if (carState.fullChargeDriveSession != NULL)
            {
                carState.fullChargeDriveSession->energyRecuperated += energy;
            }

            if (carState.partialChargeDriveSession != NULL)
            {
                carState.partialChargeDriveSession->energyRecuperated += energy;
            }

            if (carState.lastDriveSession != NULL)
            {
                carState.lastDriveSession->energyRecuperated += energy;
            }
        }
        else
        {
            if (carState.fullChargeDriveSession != NULL)
            {
                carState.fullChargeDriveSession->energySpent += energy;
            }

            if (carState.partialChargeDriveSession != NULL)
            {
                carState.partialChargeDriveSession->energySpent += energy;
            }

            if (carState.lastDriveSession != NULL)
            {
                carState.lastDriveSession->energySpent += energy;
            }
        }

        return;
    }

    if (carState.currentChargingSession != NULL)
    {
        carState.currentChargingSession->energyReceived += energy;
    }
}

void carAddEnergySpentForEvState()
{
    // считаем сколько энергии было потрачено за 10мс (ватт/час)
    float energySpent = evCalculateWh(evState.instantCurrent, evState.instantVoltage, 10);

    carAddEnergySpent(energySpent);
}

bool carIsNotMoving()
{
    return carState.selectorPosition == carSelectorP ||
           carState.selectorPosition == carSelectorN;
}

// uint8_t getCountDriveSessions()
// {
//     uint8_t count = 0;
//     for (int i = 0; i < 5; i++)
//     {
//         if (carState.driveSessions[i] != NULL)
//         {
//             count++;
//         }
//     }
//     return count;
// }

// CarDriveSession_t *getDriveSession(uint8_t index)
// {
//     if (index >= 5)
//     {
//         return NULL;
//     }

//     return carState.driveSessions[index];
// }

void newDriveSession(CarDriveSession_t *session)
{
    carStateLock();
    if (carState.lastDriveSession == NULL)
    {

        carState.lastDriveSession = session;
        carStateUnlock();

        return;
    }

    if (carState.driveSessionsHistory[0] != NULL)
    {
        // Free memory of the last element if it exists
        if (carState.driveSessionsHistory[4] != NULL)
        {
            free(carState.driveSessionsHistory[4]);
            carState.driveSessionsHistory[4] = NULL;
        }

        // Shift existing sessions to the right
        for (int i = 4; i > 0; i--)
        {
            carState.driveSessionsHistory[i] = carState.driveSessionsHistory[i - 1];
        }
    }

    carState.driveSessionsHistory[0] = carState.lastDriveSession;
    carState.lastDriveSession = session;
    carStateUnlock();
}

// uint8_t getCountChargingSessions()
// {
//     uint8_t count = 0;
//     for (int i = 0; i < 5; i++)
//     {
//         if (carState.chargingSessions[i] != NULL)
//         {
//             count++;
//         }
//     }
//     return count;
// }

// CarChargingSession_t *getChargingSession(uint8_t index)
// {
//     if (index >= 5)
//     {
//         return NULL;
//     }

//     return carState.chargingSessions[index];
// }

// void newChargingSession(CarChargingSession_t *session)
// {
//     carStateLock();
//     // Free memory of the last element if it exists
//     if (carState.chargingSessions[4] != NULL)
//     {
//         free(carState.chargingSessions[4]);
//         carState.chargingSessions[4] = NULL;
//     }
//     // Shift existing sessions to the right
//     for (int i = 4; i > 0; i--)
//     {
//         carState.chargingSessions[i] = carState.chargingSessions[i - 1];
//     }
//     // Add new session at the beginning
//     carState.chargingSessions[0] = session;
//     carState.currentChargingSession = session;
//     carStateUnlock();
// }

bool carIsCharging()
{
    return carState.chargingStatus != canChargingNotConnected && carState.chargingStatus != canChargingIdle;
}

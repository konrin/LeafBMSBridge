#ifndef CAR_TYPEDEF_H
#define CAR_TYPEDEF_H

typedef enum
{
    carSelectorP,
    carSelectorD,
    carSelectorN,
    carSelectorR
} CarSelectorPosition_t;

typedef enum
{
    CarModelUndefined,
    CarLeafZE0,
    CarLeafAZEO,
    CarLeafZE1,
} CarModel_t;

typedef enum
{
    canChargingNotConnected = 0x00,
    canChargingQuickStart = 0x40,
    canChargingQuick = 0xC0,
    canChargingQuickEnd = 0xE0,
    canChargingSlow = 0x20,
    canChargingIdle = 0x60,
} ChargingStatus_t;

#endif /* CAR_TYPEDEF_H */

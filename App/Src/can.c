#include "can.h"

static uint8_t CRCTable[256] = {
    0, 133, 143, 10, 155, 30, 20, 145, 179, 54, 60, 185, 40, 173, 167, 34, 227, 102, 108, 233, 120, 253,
    247, 114, 80, 213, 223, 90, 203, 78, 68, 193, 67, 198, 204, 73, 216, 93, 87, 210, 240, 117, 127, 250,
    107, 238, 228, 97, 160, 37, 47, 170, 59, 190, 180, 49, 19, 150, 156, 25, 136, 13, 7, 130, 134, 3,
    9, 140, 29, 152, 146, 23, 53, 176, 186, 63, 174, 43, 33, 164, 101, 224, 234, 111, 254, 123, 113, 244,
    214, 83, 89, 220, 77, 200, 194, 71, 197, 64, 74, 207, 94, 219, 209, 84, 118, 243, 249, 124, 237, 104,
    98, 231, 38, 163, 169, 44, 189, 56, 50, 183, 149, 16, 26, 159, 14, 139, 129, 4, 137, 12, 6, 131,
    18, 151, 157, 24, 58, 191, 181, 48, 161, 36, 46, 171, 106, 239, 229, 96, 241, 116, 126, 251, 217, 92,
    86, 211, 66, 199, 205, 72, 202, 79, 69, 192, 81, 212, 222, 91, 121, 252, 246, 115, 226, 103, 109, 232,
    41, 172, 166, 35, 178, 55, 61, 184, 154, 31, 21, 144, 1, 132, 142, 11, 15, 138, 128, 5, 148, 17,
    27, 158, 188, 57, 51, 182, 39, 162, 168, 45, 236, 105, 99, 230, 119, 242, 248, 125, 95, 218, 208, 85,
    196, 65, 75, 206, 76, 201, 195, 70, 215, 82, 88, 221, 255, 122, 112, 245, 100, 225, 235, 110, 175, 42,
    32, 165, 52, 177, 187, 62, 28, 153, 147, 22, 135, 2, 8, 141};

void CanCalcCRC8(uint8_t data[8])
{
    uint8_t crc = 0;

    for (uint8_t j = 0; j < 7; j++)
    {
        crc = CRCTable[(crc ^ ((int)data[j])) % 256];
    }

    data[7] = crc;
}

CanDecodeStatus_t CanDecode1DBMessage(Can1DBMessage_t *message, uint8_t data[8])
{
    int16_t current;
    uint16_t voltage;

    // 0.5A/bit
    current = (data[0] << 3) | (data[1] & 0xe0) >> 5;
    if (current & 0x0400)
    {
        current |= 0xf800;
    }

    // 0.5V/bit
    voltage = (data[2] << 2) | ((data[3] & 0xc0) >> 6);

    // 3FF is unavailable value. Can happen directly on reboot.
    if (voltage == 0x3ff)
    {
        return canDecodeStatusError;
    }

    message->failsafeStatus = (data[1] & 0x07);
    message->relayCutRequest = ((data[1] & 0x18) >> 3) > 0;
    message->mainRelayOn = (((data[3] & 0x20) >> 5) != 0);
    message->fullChargeFlag = ((data[3] & 0x10) >> 4) > 0;

    message->voltage = voltage / 2.0f;
    message->current = current / 2.0f;

    return canDecodeStatusOk;
}

void CanEncode1DBSetRequestChargingStop(uint8_t data[8])
{
    data[1] = (data[1] & 0xE0) | 2;
}

void CanEncode1DBSetFullChargeCompleted(uint8_t data[8])
{
    data[3] = (data[3] & 0xEF) | 0x10;
}

CanDecodeStatus_t CanDecode11AMessage(Can11AMessage_t *message, uint8_t data[8])
{
    message->carIsOn = (data[1] == 0x40 || data[1] == 0x50) ? true : false;
    message->ecoModeIsOn = (data[1] == 0x50) ? true : false;

    switch (data[0])
    {
    case 0x3E:
        message->selectorPosition = carSelectorN;
        break;
    case 0x4E:
        message->selectorPosition = carSelectorD;
        break;
    case 0x2E:
        message->selectorPosition = carSelectorR;
        break;
    default:
        // 0x01 and 0x11
        message->selectorPosition = carSelectorP;
    }

    return canDecodeStatusOk;
}

CanDecodeStatus_t CanDecode1F2Message(Can1F2Message_t *message, uint8_t data[8])
{
    message->chargingStatus = (ChargingStatus_t)(data[2]);
    message->isEnergyCapacity80Percent = ((data[0] & 0x80) != 0);

    return canDecodeStatusOk;
}

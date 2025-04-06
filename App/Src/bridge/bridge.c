#include "bridge.h"
#include "config.h"

BridgeParameters_t bridgeParameters = {
    .isActive = true,
    .maxVoltage = 402,
    .minVoltage = 300,
    .maxCellVoltage = 420,
    .minCellVoltage = 290,
    .totalCapacity = 33000,
    .realCapacity = 33000,
    .maxPower = 100,
    .driveSessionTimeout = 3600
};

void startBridge(void)
{
    startBmsCanBus();
    startVcmCanBus();
}

bool bridgeIsActive()
{
    return bridgeParameters.isActive;
}

void convertCanRxHeaderToCanTxHeader(CAN_RxHeaderTypeDef *rxHeader, CAN_TxHeaderTypeDef *txHeader)
{
    txHeader->StdId = rxHeader->StdId;
    txHeader->ExtId = rxHeader->ExtId;
    txHeader->IDE = rxHeader->IDE;
    txHeader->RTR = rxHeader->RTR;
    txHeader->DLC = rxHeader->DLC;
    txHeader->TransmitGlobalTime = DISABLE;
}

void updateCanMessageFromRxHeader(CAN_RxHeaderTypeDef *rxHeader, CanMessage_t *message)
{
    message->stdId = rxHeader->StdId;
    message->extId = rxHeader->ExtId;
    message->ide = rxHeader->IDE;
    message->rtr = rxHeader->RTR;
    message->dlc = rxHeader->DLC;
}

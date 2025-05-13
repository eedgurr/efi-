#ifndef J2534_INTERFACE_H
#define J2534_INTERFACE_H

#include <stdint.h>

/* J2534 API Version */
#define J2534_API_VERSION    04

/* J2534 Protocol IDs */
#define J2534_PROTOCOL_CAN      5
#define J2534_PROTOCOL_ISO9141  3
#define J2534_PROTOCOL_ISO14230 4
#define J2534_PROTOCOL_J1850VPW 2
#define J2534_PROTOCOL_J1850PWM 1

/* J2534 Error Codes */
#define J2534_STATUS_NOERROR    0x00
#define J2534_ERR_NOT_SUPPORTED 0x01
#define J2534_ERR_INVALID_CHANNEL_ID 0x02
#define J2534_ERR_INVALID_PROTOCOL_ID 0x03
#define J2534_ERR_NULL_PARAMETER 0x04
#define J2534_ERR_TIMEOUT       0x05
#define J2534_ERR_INVALID_IOCTL 0x06
#define J2534_ERR_BUFFER_EMPTY  0x07
#define J2534_ERR_BUFFER_FULL   0x08

/* J2534 IOCTL Parameters */
#define J2534_IOCTL_GET_CONFIG  0x01
#define J2534_IOCTL_SET_CONFIG  0x02
#define J2534_IOCTL_READ_VBATT  0x03
#define J2534_IOCTL_READ_PROG_VOLTAGE 0x04

/* J2534 Connection Flags */
#define J2534_CAN_29BIT_ID     0x00000100
#define J2534_ISO9141_NO_CHECKSUM 0x00000200
#define J2534_WAIT_J1939_DTC   0x00000400

typedef struct {
    uint32_t ProtocolID;
    uint32_t Flags;
    uint32_t BaudRate;
} SCONFIG;

typedef struct {
    uint32_t Parameter;
    uint32_t Value;
} SCONFIG_LIST;

/* Function Prototypes */
int J2534_Initialize(void);
int J2534_Connect(uint32_t ProtocolID, uint32_t Flags, uint32_t BaudRate);
int J2534_Disconnect(uint32_t ChannelID);
int J2534_ReadMsgs(uint32_t ChannelID, uint8_t* Data, uint32_t* NumMsgs, uint32_t Timeout);
int J2534_WriteMsgs(uint32_t ChannelID, const uint8_t* Data, uint32_t NumMsgs, uint32_t Timeout);
int J2534_StartPeriodicMsg(uint32_t ChannelID, const uint8_t* Data, uint32_t NumMsgs, uint32_t Period);
int J2534_StopPeriodicMsg(uint32_t ChannelID, uint32_t MsgID);
int J2534_IoctlControl(uint32_t ChannelID, uint32_t IoctlID, const void* Input, void* Output);
const char* J2534_GetErrorText(int ErrorCode);

#endif /* J2534_INTERFACE_H */

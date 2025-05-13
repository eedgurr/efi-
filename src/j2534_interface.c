#include "j2534_interface.h"
#include "obd2_core.h"
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>

/* J2534 DLL/SO handle */
static void* j2534_handle = NULL;
static uint32_t current_channel = 0;

/* Function pointers for J2534 API */
static int (*PassThruOpen)(const char*, uint32_t*);
static int (*PassThruClose)(uint32_t);
static int (*PassThruConnect)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t*);
static int (*PassThruDisconnect)(uint32_t);
static int (*PassThruReadMsgs)(uint32_t, void*, uint32_t*, uint32_t);
static int (*PassThruWriteMsgs)(uint32_t, void*, uint32_t*, uint32_t);
static int (*PassThruStartPeriodicMsg)(uint32_t, void*, uint32_t*, uint32_t);
static int (*PassThruStopPeriodicMsg)(uint32_t, uint32_t);
static int (*PassThruIoctl)(uint32_t, uint32_t, void*, void*);

int J2534_Initialize(void) {
    DEBUG_PRINT(DEBUG_LEVEL_INFO, "Initializing J2534 interface");
    
    /* Load J2534 library */
    #ifdef _WIN32
    j2534_handle = dlopen("J2534.dll", RTLD_NOW);
    #else
    j2534_handle = dlopen("libJ2534.so", RTLD_NOW);
    #endif
    
    if (!j2534_handle) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "Failed to load J2534 library: %s", dlerror());
        return -1;
    }
    
    /* Get function pointers */
    PassThruOpen = dlsym(j2534_handle, "PassThruOpen");
    PassThruClose = dlsym(j2534_handle, "PassThruClose");
    PassThruConnect = dlsym(j2534_handle, "PassThruConnect");
    PassThruDisconnect = dlsym(j2534_handle, "PassThruDisconnect");
    PassThruReadMsgs = dlsym(j2534_handle, "PassThruReadMsgs");
    PassThruWriteMsgs = dlsym(j2534_handle, "PassThruWriteMsgs");
    PassThruStartPeriodicMsg = dlsym(j2534_handle, "PassThruStartPeriodicMsg");
    PassThruStopPeriodicMsg = dlsym(j2534_handle, "PassThruStopPeriodicMsg");
    PassThruIoctl = dlsym(j2534_handle, "PassThruIoctl");
    
    /* Verify all functions were found */
    if (!PassThruOpen || !PassThruClose || !PassThruConnect || !PassThruDisconnect ||
        !PassThruReadMsgs || !PassThruWriteMsgs || !PassThruStartPeriodicMsg ||
        !PassThruStopPeriodicMsg || !PassThruIoctl) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "Failed to load J2534 functions");
        dlclose(j2534_handle);
        j2534_handle = NULL;
        return -1;
    }
    
    uint32_t device_id;
    int result = PassThruOpen(NULL, &device_id);
    if (result != J2534_STATUS_NOERROR) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "Failed to open J2534 device: %s", 
                   J2534_GetErrorText(result));
        dlclose(j2534_handle);
        j2534_handle = NULL;
        return -1;
    }
    
    DEBUG_PRINT(DEBUG_LEVEL_INFO, "J2534 interface initialized successfully");
    return 0;
}

int J2534_Connect(uint32_t ProtocolID, uint32_t Flags, uint32_t BaudRate) {
    if (!j2534_handle) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "J2534 not initialized");
        return -1;
    }
    
    /* Disconnect existing connection if any */
    if (current_channel != 0) {
        J2534_Disconnect(current_channel);
    }
    
    uint32_t channel_id;
    int result = PassThruConnect(1, ProtocolID, Flags, BaudRate, &channel_id);
    if (result != J2534_STATUS_NOERROR) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "Failed to connect: %s", 
                   J2534_GetErrorText(result));
        return -1;
    }
    
    current_channel = channel_id;
    DEBUG_PRINT(DEBUG_LEVEL_INFO, "Connected to channel %d", channel_id);
    return 0;
}

int J2534_Disconnect(uint32_t ChannelID) {
    if (!j2534_handle) {
        return -1;
    }
    
    int result = PassThruDisconnect(ChannelID);
    if (result != J2534_STATUS_NOERROR) {
        DEBUG_PRINT(DEBUG_LEVEL_ERROR, "Failed to disconnect: %s", 
                   J2534_GetErrorText(result));
        return -1;
    }
    
    if (ChannelID == current_channel) {
        current_channel = 0;
    }
    
    return 0;
}

const char* J2534_GetErrorText(int ErrorCode) {
    switch (ErrorCode) {
        case J2534_STATUS_NOERROR:
            return "No error";
        case J2534_ERR_NOT_SUPPORTED:
            return "Function not supported";
        case J2534_ERR_INVALID_CHANNEL_ID:
            return "Invalid channel ID";
        case J2534_ERR_INVALID_PROTOCOL_ID:
            return "Invalid protocol ID";
        case J2534_ERR_NULL_PARAMETER:
            return "NULL parameter";
        case J2534_ERR_TIMEOUT:
            return "Timeout";
        case J2534_ERR_INVALID_IOCTL:
            return "Invalid IOCTL";
        case J2534_ERR_BUFFER_EMPTY:
            return "Buffer empty";
        case J2534_ERR_BUFFER_FULL:
            return "Buffer full";
        default:
            return "Unknown error";
    }
}

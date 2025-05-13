#ifndef DTC_HANDLER_H
#define DTC_HANDLER_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_DTC_COUNT 20

/* DTC details structure */
typedef struct {
    char description[256];
    uint8_t severity;
    char system[64];
} DTCDetails;

/* DTC entry structure */
typedef struct {
    char code[6];
    DTCDetails details;
} DTCEntry;

/* DTC data structure */
typedef struct {
    DTCEntry entries[MAX_DTC_COUNT];
    size_t count;
} DTCData;

/* Initialize DTC database */
int dtc_init_database(const char* database_path);

/* Get info for a specific DTC */
int dtc_get_info(const char* code, DTCDetails* details);

/* Read current DTCs */
int dtc_read_current(DTCData* data);

/* Read freeze frame data */
int dtc_read_freeze_frame(uint8_t frame_id, DTCData* data);

/* Clear all DTCs */
int dtc_clear_all(void);

#endif /* DTC_HANDLER_H */

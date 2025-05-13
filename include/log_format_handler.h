#ifndef LOG_FORMAT_HANDLER_H
#define LOG_FORMAT_HANDLER_H

#include "performance_calc.h"
#include <stdint.h>
#include <time.h>

/* XDF Format Handler */
typedef struct {
    const char* template_path;
    const char* output_path;
    void* xdf_handle;
    struct {
        uint32_t record_count;
        uint64_t start_time;
        uint32_t buffer_size;
    } xdf_state;
} XDFHandler;

/* A2L Format Handler */
typedef struct {
    const char* a2l_path;
    const char* output_path;
    void* a2l_handle;
    struct {
        uint32_t characteristic_count;
        uint32_t measurement_count;
        char project_name[32];
    } a2l_state;
} A2LHandler;

/* Function Declarations */
int xdf_init_logging(XDFHandler* handler, const char* template_path);
int xdf_write_record(XDFHandler* handler, const PerformanceData* data);
int xdf_close_log(XDFHandler* handler);

int a2l_init_logging(A2LHandler* handler, const char* a2l_path);
int a2l_write_record(A2LHandler* handler, const PerformanceData* data);
int a2l_close_log(A2LHandler* handler);

/* Utility Functions */
int convert_log_to_xdf(const char* input_path, const char* output_path);
int convert_log_to_a2l(const char* input_path, const char* output_path);
int validate_xdf_format(const char* xdf_path);
int validate_a2l_format(const char* a2l_path);

#endif /* LOG_FORMAT_HANDLER_H */

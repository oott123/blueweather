#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include "CH58x_common.h"
#include "CH58xBLE_LIB.h"

#define PRINT_RAW(msg) UART1_SendString((uint8_t *)msg, sizeof(msg))
void log_print(const char *fmt, ...);

#define RAW_DEBUG(msg) PRINT_RAW("[DEBUG] " msg "\r\n")
#define RAW_INFO(msg) PRINT_RAW("[INFO] " msg "\r\n")
#define RAW_WARN(msg) PRINT_RAW("[WARN] " msg "\r\n")
#define RAW_ERROR(msg) PRINT_RAW("[ERROR] " msg "\r\n")

#define LOG_DEBUG(msg, ...) log_print("[DEBUG] " msg "\r\n", ##__VA_ARGS__)
#define LOG_INFO(msg, ...) log_print("[INFO] " msg "\r\n", ##__VA_ARGS__)
#define LOG_WARN(msg, ...) log_print("[WARN] " msg "\r\n", ##__VA_ARGS__)
#define LOG_ERROR(msg, ...) log_print("[ERROR] " msg "\r\n", ##__VA_ARGS__)

#endif

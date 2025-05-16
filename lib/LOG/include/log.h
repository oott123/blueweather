#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include "CH58x_common.h"
#include "CH58xBLE_LIB.h"

#define LOG_ENABLE FALSE

void log_print(const char *fmt, ...);

#if LOG_ENABLE

#define PRINT_RAW(msg) UART1_SendString((uint8_t *)msg, sizeof(msg))

#define LOG_DEBUG(msg, ...) log_print("[DEBUG] " msg "\r\n", ##__VA_ARGS__)
#define LOG_INFO(msg, ...) log_print("[INFO] " msg "\r\n", ##__VA_ARGS__)
#define LOG_WARN(msg, ...) log_print("[WARN] " msg "\r\n", ##__VA_ARGS__)
#define LOG_ERROR(msg, ...) log_print("[ERROR] " msg "\r\n", ##__VA_ARGS__)

#else // LOG_ENABLE

#define PRINT_RAW(msg) /* do nothing*/
#define LOG_DEBUG(msg, ...) /* do nothing*/
#define LOG_INFO(msg, ...) /* do nothing*/
#define LOG_WARN(msg, ...) /* do nothing*/
#define LOG_ERROR(msg, ...) /* do nothing*/

#endif // LOG_ENABLE

#define RAW_DEBUG(msg) PRINT_RAW("[DEBUG] " msg "\r\n")
#define RAW_INFO(msg) PRINT_RAW("[INFO] " msg "\r\n")
#define RAW_WARN(msg) PRINT_RAW("[WARN] " msg "\r\n")
#define RAW_ERROR(msg) PRINT_RAW("[ERROR] " msg "\r\n")

#endif // LOG_H

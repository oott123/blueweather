#ifndef MY_APP_H
#define MY_APP_H

#include <stdio.h>

#define PRINT_RAW(msg) UART1_SendString(msg, sizeof(msg))
void bw_print_log(const char *fmt, ...);
void HardFault_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

#define RAW_DEBUG(msg) PRINT_RAW("[DEBUG] " msg "\r\n")
#define RAW_INFO(msg) PRINT_RAW("[INFO] " msg "\r\n")
#define RAW_WARN(msg) PRINT_RAW("[WARN] " msg "\r\n")
#define RAW_ERROR(msg) PRINT_RAW("[ERROR] " msg "\r\n")

#define LOG_DEBUG(msg, ...) bw_print_log("[DEBUG] " msg "\r\n", ##__VA_ARGS__)
#define LOG_INFO(msg, ...) bw_print_log("[INFO] " msg "\r\n", ##__VA_ARGS__)
#define LOG_WARN(msg, ...) bw_print_log("[WARN] " msg "\r\n", ##__VA_ARGS__)
#define LOG_ERROR(msg, ...) bw_print_log("[ERROR] " msg "\r\n", ##__VA_ARGS__)

#endif
#include "log.h"
#include "CH58x_common.h"

#include <stdarg.h>

void log_print(const char *fmt, ...) {
  uint8_t buffer[128];
  int len = 0;

  va_list args;
  va_start(args, fmt);
  len = vsprintf((char *)buffer, fmt, args);
  va_end(args);

  if (len > 0) {
    UART1_SendString(buffer, len);
  } else {
    UART1_SendString((uint8_t *)"Error: print_log\r\n", 17);
  }
}

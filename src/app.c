#include "CH58x_common.h"
#include "CH58xBLE_LIB.h"
#include "HAL.h"
#include "app.h"
#include "bthome.h"
#include "log.h"
#include "app_i2c.h"

#include <stdio.h>
#include <stdarg.h>

#define MASTER_ADDR     (0x21 << 1)
#define AHT10_ADDR      (0x38 << 1)

__attribute__((aligned(4))) uint32_t MEM_BUF[BLE_MEMHEAP_SIZE / 4];

__HIGH_CODE
void run_romisp_force(void) {
  FLASH_ROM_ERASE(0, 4096);
  FLASH_ROM_SW_RESET();
  R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG1;
  R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG2;
  SAFEOPERATE;
  R16_INT32K_TUNE = 0xFFFF;
  R8_RST_WDOG_CTRL |= RB_SOFTWARE_RESET;
  R8_SAFE_ACCESS_SIG = 0;
  while(1);
}

const uint8_t cmd_init[3] = {0xe1, 0x08, 0x00};
const uint8_t cmd_measure[3] = {0xac, 0x33, 0x08};

void key_callback(uint8_t keys) {
  if (keys & HAL_KEY_SW_1) {
    RAW_DEBUG("[KEY] Key 1 pressed");
    run_romisp_force();
    while (1);
  }
}

int main() {
  SetSysClock(CLK_SOURCE_PLL_60MHz);

  GPIOA_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
  GPIOB_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);

  if (LOG_ENABLE) {
    // UART1: TXD-PA9, RXD-PA8
    GPIOA_SetBits(GPIO_Pin_9);
    GPIOA_ModeCfg(GPIO_Pin_8, GPIO_ModeIN_PU);
    GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA);
    UART1_DefInit();
    RAW_DEBUG("[UART] init success!");
  }

  // BLE
  CH58X_BLEInit();
  RAW_DEBUG("[BLE] BLE init success!");
  HAL_Init();
  HalKeyConfig(key_callback);
  RAW_DEBUG("[BLE] HAL init success!");
  GAPRole_BroadcasterInit();
  RAW_DEBUG("[BLE] GAPRole_BroadcasterInit success!");
  bthome_init();
  RAW_DEBUG("[BLE] bthome_init success!");

  // I2C
  i2c_app_init((0x21 << 1));
  RAW_DEBUG("[I2C] init success!");

  // AHT20: PB7 - VOAHT
  GPIOB_ModeCfg(GPIO_Pin_7, GPIO_ModeOut_PP_20mA);
  GPIOB_SetBits(GPIO_Pin_7);
  RAW_DEBUG("[AHT20] power on!");

  RAW_DEBUG("[Main] Starting main loop...");
  while (1) {
    TMOS_SystemProcess();
  }
}

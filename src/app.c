#include "CH58x_common.h"
#include "app_i2c.h"
#include "app.h"

#include <stdio.h>
#include <stdarg.h>

#define MASTER_ADDR     (0x21 << 1)
#define AHT10_ADDR      (0x38 << 1)

void bw_print_log(const char *fmt, ...) {
  char buffer[128];
  int len = 0;

  va_list args;
  va_start(args, fmt);
  len = vsprintf(buffer, fmt, args);
  va_end(args);

  if (len > 0) {
    UART1_SendString(buffer, len);
  } else {
    UART1_SendString("Error: print_log\r\n", 17);
  }
}

// https://www.wch.cn/bbs/thread-114686-1.html
// 把这段代码加入到你的工程里，可以实现不断电，不下拉PB22，直接进入ISP模式做USB升级。
void run_romisp() {
  // 这里读一下CSR, 触发异常进入机器模式。
  // 如果已经是机器模式则继续运行。
  read_csr(mstatus);

  PFIC->IRER[0] = 0xffffffff;
  PFIC->IRER[1] = 0xffffffff;

  // 复制固件代码到ram
  memcpy((void*)0x20003800, (void*)0x000780a4, 0x2500);
  // 0x200038be是检测PB22的。这里让它强制返回1，跳过检测。
  *(uint16_t*)0x200038be = 0x4505; // li a0, 1
  // 清BSS
  memset((void*)0x20005c18, 0, 0x04a8);

  // 设置运行环境并跳转
  __asm__("la gp, 0x20006410\n");
  __asm__("la sp, 0x20008000\n");
  write_csr(mstatus, 0x88);
  write_csr(mtvec, 0x20003801);
  write_csr(mepc, 0x20004ebc);
  __asm__("mret");
  __asm__("nop");
  __asm__("nop");
}

void HardFault_Handler(void)
{
  run_romisp();
  while(1);
}

void write_data(uint8_t *cmd, uint8_t len) {
  while(I2C_GetFlagStatus(I2C_FLAG_BUSY) != RESET);

  I2C_GenerateSTART(ENABLE);
  while(!I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT));

  I2C_Send7bitAddress(AHT10_ADDR, I2C_Direction_Transmitter);
  while(!I2C_CheckEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

  // 写测量命令
  uint8_t i;
  for (i = 0; i < len; i++) {
    while (I2C_GetFlagStatus(I2C_FLAG_TXE) != 1);
    I2C_SendData(cmd[i]);
  }

  while(!I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED));

  I2C_GenerateSTOP(ENABLE);
}

void read_data(uint8_t *rxData, uint8_t len) {
  while (I2C_GetFlagStatus(I2C_FLAG_BUSY) != RESET);
  I2C_GenerateSTART(ENABLE);
  while (!I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT));
  I2C_Send7bitAddress(AHT10_ADDR, I2C_Direction_Receiver);
  while (!I2C_CheckEvent(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));

  uint8_t i;
  for (i = 0; i < len; i++) {
    while (I2C_GetFlagStatus(I2C_FLAG_RXNE) != 1);
    rxData[i] = I2C_ReceiveData();
    if (i == len - 1) {
      // 主设备为了能在收到最后一个字节后产生一个 NACK 脉冲，必须在读取倒数第二个字节之后（倒数第二个
      // RxNE 事件之后）清除 ACK 位(ACK=0)
      I2C_AcknowledgeConfig(DISABLE);
      // 主设备为了能在收到最后一个字节后产生一个停止/重起始信号，必须在读取倒数第二个字节之后（倒
      // 数第二个 RxNE 事件之后）设置 STOP/START 位(STOP=1/START=1)
      I2C_GenerateSTOP(ENABLE);
    }
  }  
}

const uint8_t cmd_init[3] = {0xe1, 0x08, 0x00};
const uint8_t cmd_measure[3] = {0xac, 0x33, 0x08};

int main() {
  SetSysClock(CLK_SOURCE_PLL_60MHz);

  // UART1: TXD-PA9, RXD-PA8
  GPIOA_SetBits(GPIO_Pin_9);
  GPIOA_ModeCfg(GPIO_Pin_8, GPIO_ModeIN_PU);
  GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA);
  UART1_DefInit();
  RAW_DEBUG("[UART] init success!");

  // I2C: SCL-PB13, SDA-PB12
  GPIOB_ModeCfg(GPIO_Pin_13 | GPIO_Pin_12, GPIO_ModeIN_PU);

  I2C_Init(I2C_Mode_I2C, 400000, I2C_DutyCycle_16_9, I2C_Ack_Enable, I2C_AckAddr_7bit, MASTER_ADDR);
  RAW_DEBUG("[I2C] init success!");

  uint8_t calEnable = 0;
  uint8_t busy = 0;
  uint8_t rxData[6];

  while (1) {
    RAW_DEBUG("[AHT10] Sending measure command...");

    write_data(cmd_measure, sizeof(cmd_measure));

    // 等 75ms
    RAW_DEBUG("[AHT10] Waiting for measurement to complete...");
    DelayMs(75);

    RAW_DEBUG("[AHT10] Reading data...");
    read_data(rxData, sizeof(rxData));

    LOG_DEBUG("[AHT10]   rxData: %02x %02x %02x %02x %02x %02x", rxData[0], rxData[1], rxData[2], rxData[3], rxData[4], rxData[5]);

    // Bit[3] 校准使能位
    calEnable = rxData[0] & 0x08;
    // Bit[7] 忙位
    busy = rxData[0] & 0x80;
    if (calEnable == 0) {
      RAW_DEBUG("[AHT10] Need calibration");
      write_data(cmd_init, sizeof(cmd_init));
      RAW_DEBUG("[AHT10] Calibration done");
    } else if (busy == 1) {
      RAW_DEBUG("[AHT10] Busy");
      DelayMs(75);
    } else {
      RAW_DEBUG("[AHT10] Measurement done");
      uint32_t SRH = (rxData[1]<<12) | (rxData[2]<<4) | (rxData[3]>>4);
      uint32_t ST = ((rxData[3]&0x0f)<<16) | (rxData[4]<<8) | rxData[5];
      float humidity = (SRH * 100.0) / 1024.0 / 1024.0;
      float temperature = (ST * 200.0) / 1024.0 / 1024.0 - 50;

      char buffer[128];
      int len = sprintf(buffer, "[AHT10] Temperature: %.2f, Humidity: %.2f", temperature, humidity);
      buffer[len] = '\0';
      LOG_INFO("%s", buffer);

      break;
    }
  }

  RAW_DEBUG("[MAIN] Program end, entering flash mode...");
  run_romisp();
  RAW_DEBUG("[MAIN] Flash mode failed.");
  while (1);
}

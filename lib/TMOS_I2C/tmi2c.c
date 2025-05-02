#include "CH58x_common.h"
#include "CH58xBLE_LIB.h"
#include "HAL.h"
#include "tmi2c.h"
#include "app_i2c.h"
#include "log.h"

static uint16_t i2c_process_event(uint8_t task_id, uint16_t events);

static tmosTaskID i2c_task_id;
void tmi2c_init() {
  // I2C: SCL-PB13, SDA-PB12
  GPIOB_ModeCfg(GPIO_Pin_13 | GPIO_Pin_12, GPIO_ModeIN_PU);

  I2C_Init(I2C_Mode_I2C, 400, I2C_DutyCycle_16_9, I2C_Ack_Enable, I2C_AckAddr_7bit, (0x21 << 1));

  RAW_DEBUG("[I2C] init success!");

  i2c_task_id = TMOS_ProcessEventRegister(i2c_process_event);
  if (i2c_task_id == 0xff) {
    LOG_ERROR("[TMI2C] failed to register event");
  }
}

static uint8_t i2c_process_address = 0;
static uint8_t i2c_process_direction = 0;
static uint8_t i2c_process_data[64];
static uint8_t i2c_process_data_index = 0;
static uint8_t i2c_process_data_len = 0;
static tmi2c_data_cb_t i2c_process_data_cb = NULL;

uint16_t i2c_process_event(uint8_t task_id, uint16_t events) {
  if (events & I2C_EVENT_GENERATE_START) {
    if (I2C_GetFlagStatus(I2C_FLAG_BUSY) == RESET) {
      RAW_DEBUG("[TMI2C] i2c generate start");
      I2C_GenerateSTART(ENABLE);
      tmos_set_event(i2c_task_id, I2C_EVENT_SEND_ADDRESS);
      return events ^ I2C_EVENT_GENERATE_START;
    } else {
      return events;
    }
  }

  if (events & I2C_EVENT_SEND_ADDRESS) {
    if (
      I2C_CheckEvent(
        i2c_process_direction == I2C_Direction_Transmitter ?
          I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED :
          I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED
      )
    ) {
      LOG_DEBUG("[TMI2C] i2c send address: %02x, direction: %d", i2c_process_address, i2c_process_direction);
      I2C_Send7bitAddress(i2c_process_address, i2c_process_direction);
      tmos_set_event(
        i2c_task_id,
        i2c_process_direction == I2C_Direction_Transmitter ?
          I2C_EVENT_DATA_TX :
          I2C_EVENT_DATA_RX
        );
      return events ^ I2C_EVENT_SEND_ADDRESS;
    } else {
      return events;
    }
  }

  if (events & I2C_EVENT_DATA_TX) {
    if (I2C_GetFlagStatus(I2C_FLAG_TXE) == SET) {
      LOG_DEBUG("[TMI2C] i2c data tx: #%02d", i2c_process_data_index, i2c_process_data[i2c_process_data_index]);
      I2C_SendData(i2c_process_data[i2c_process_data_index++]);
      if (i2c_process_data_index == i2c_process_data_len) {
        tmos_set_event(i2c_task_id, I2C_EVENT_GENERATE_STOP);
        return events ^ I2C_EVENT_DATA_TX;
      } else {
        return events;
      }
    } else {
      return events;
    }
  }

  if (events & I2C_EVENT_DATA_RX) {
    if (I2C_GetFlagStatus(I2C_FLAG_RXNE) == SET) {
      LOG_DEBUG("[TMI2C] i2c data rx: #%02d", i2c_process_data_index);
      i2c_process_data[i2c_process_data_index++] = I2C_ReceiveData();
      LOG_DEBUG("[TMI2C] i2c data rx: #%02d %02x", i2c_process_data_index, i2c_process_data[i2c_process_data_index]);
      if (i2c_process_data_index == i2c_process_data_len - 1) {
        I2C_AcknowledgeConfig(DISABLE);
        I2C_GenerateSTOP(ENABLE);
      } else if (i2c_process_data_index == i2c_process_data_len) {
        tmos_set_event(i2c_task_id, I2C_EVENT_DATA_CALLBACK);
        return events ^ I2C_EVENT_DATA_RX;
      } else {
        return events;
      }
    } else {
      return events;
    }
  }

  if (events & I2C_EVENT_GENERATE_STOP) {
    RAW_DEBUG("[TMI2C] i2c generate stop");
    I2C_GenerateSTOP(ENABLE);
    tmos_set_event(i2c_task_id, I2C_EVENT_DATA_CALLBACK);
    return events ^ I2C_EVENT_GENERATE_STOP;
  }

  if (events & I2C_EVENT_DATA_CALLBACK) {
    RAW_DEBUG("[TMI2C] i2c data callback");
    i2c_process_data_cb(i2c_process_data, i2c_process_data_len);
    i2c_process_address = 0;
    i2c_process_direction = 0;
    i2c_process_data_index = 0;
    i2c_process_data_len = 0;
    i2c_process_data_cb = NULL;
    return events ^ I2C_EVENT_DATA_CALLBACK;
  }

  return events;
}

uint8_t tmi2c_write(uint8_t address, const uint8_t *data, uint8_t len, tmi2c_data_cb_t cb) {
  if (i2c_process_data_cb != NULL) {
    return 1;
  }

  i2c_process_address = address << 1;
  i2c_process_direction = I2C_Direction_Transmitter;
  i2c_process_data_len = len;
  i2c_process_data_cb = cb;
  tmos_set_event(i2c_task_id, I2C_EVENT_GENERATE_START);
  return 0;
}

uint8_t tmi2c_read(uint8_t address, uint8_t len, tmi2c_data_cb_t cb) {
  if (i2c_process_data_cb != NULL) {
    return 1;
  }

  i2c_process_address = address << 1;
  i2c_process_direction = I2C_Direction_Receiver;
  i2c_process_data_len = len;
  i2c_process_data_cb = cb;
  tmos_set_event(i2c_task_id, I2C_EVENT_GENERATE_START);
  return 0;
}

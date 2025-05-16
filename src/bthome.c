#include "CH58xBLE_LIB.h"
#include "bthome.h"
#include "app_i2c.h"
#include "log.h"

static uint8_t broadcaster_task_id;

// GAP - SCAN RSP data (max size = 31 bytes)
static uint8_t scanRspData[] = {
  // complete name
  0x0c, // length of this data
  GAP_ADTYPE_LOCAL_NAME_COMPLETE, 0x42, // 'B'
  0x72,                                 // 'r'
  0x6f,                                 // 'o'
  0x61,                                 // 'a'
  0x64,                                 // 'd'
  0x63,                                 // 'c'
  0x61,                                 // 'a'
  0x73,                                 // 's'
  0x74,                                 // 't'
  0x65,                                 // 'e'
  0x72,                                 // 'r'

  // Tx power level
  0x02, // length of this data
  GAP_ADTYPE_POWER_LEVEL, 0 // 0dBm
};

// GAP - Advertisement data (max size = 31 bytes, though this is
// best kept short to conserve power while advertisting)
static uint8_t advertData[] = {
  2,
  GAP_ADTYPE_FLAGS,
  GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,

  12,
  GAP_ADTYPE_LOCAL_NAME_COMPLETE, 'B', 'l', 'u', 'e', 'w', 'e', 'a', 't', 'h', 'e', 'r',
};

static uint8_t advertData_update[] = {
  // Flags
  /* 0 */ 2,
  /* 1 */ GAP_ADTYPE_FLAGS,
  /* 2 */ GAP_ADTYPE_FLAGS_GENERAL | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,

  // Service Data
  /* 3 */ 10,
  /* 4 */ GAP_ADTYPE_SERVICE_DATA,
  // UUID(D2FC) info(40) temperature(020000) humidity(030000)
  /* 5 */ 0xd2, 0xfc, 0x40, 0x02,
  /* 9 */ 0x00, 0x00, 0x03,
  /* 12 */ 0x00, 0x00,

  // Local Name
  /* 14 */ 12,
  /* 15 */ GAP_ADTYPE_LOCAL_NAME_COMPLETE,
  /* 16 */ 'B', 'l', 'u', 'e', 'w', 'e', 'a', 't', 'h', 'e', 'r',
};

uint16_t bthome_broadcaster_process_event(uint8_t task_id, uint16_t events);
static void bthome_process_tmos_msg(tmos_event_hdr_t *pMsg);
static void bthome_state_notification_cb(gapRole_States_t newState);

static gapRolesBroadcasterCBs_t bthome_broadcaster_callbacks = {
  bthome_state_notification_cb, // Profile State Change Callbacks
  NULL
};

static void bthome_process_tmos_msg(tmos_event_hdr_t *pMsg) {
  // noop
}

static void bthome_state_notification_cb(gapRole_States_t newState) {
  // noop
}

const uint8_t aht10_addr = 0x38;
const uint8_t aht10_cmd_init[3] = {0xbe, 0x08, 0x00};
const uint8_t aht10_cmd_measure[3] = {0xac, 0x33, 0x00};

static uint8_t aht10_status = 0;
static uint8_t aht10_rx_buffer[6];

static float humidity = 0.0;
static float temperature = 0.0;

uint8_t aht10_process_rx(uint8_t *rxData, uint8_t len) {
  if (len != 1 && len != 6) {
    LOG_ERROR("[AHT10] read failed, expected 1 or 6 bytes, got %d", len);
    return 1;
  }

  // Bit[3] 校准使能位
  uint8_t calEnable = rxData[0] & 0x08;
  // Bit[7] 忙位
  uint8_t busy = rxData[0] & 0x80;

  if (calEnable == 0) {
    LOG_DEBUG("[AHT10] Need calibration %d %x xxx", len, rxData[0]);
    return 2;
  } else if (busy == 1) {
    RAW_DEBUG("[AHT10] Busy");
    return 3;
  } else {
    if (len < 6) {
      LOG_ERROR("[AHT10] read failed, expected 6 bytes, got %d", len);
      return 1;
    }
    uint32_t SRH = (rxData[1]<<12) | (rxData[2]<<4) | (rxData[3]>>4);
    uint32_t ST = ((rxData[3]&0x0f)<<16) | (rxData[4]<<8) | rxData[5];
    humidity = (SRH * 100.0) / 1024.0 / 1024.0;
    temperature = (ST * 200.0) / 1024.0 / 1024.0 - 50;
    (*(uint16_t *)(advertData_update + 9)) = (uint16_t) (temperature * 100);
    (*(uint16_t *)(advertData_update + 12)) = (uint16_t) (humidity * 100);
    return 0;
  }
}

uint16_t bthome_broadcaster_process_event(uint8_t task_id, uint16_t events) {
  if (events & SYS_EVENT_MSG) {
    uint8_t *pMsg;

    if ((pMsg = tmos_msg_receive(broadcaster_task_id)) != NULL) {
      bthome_process_tmos_msg((tmos_event_hdr_t *) pMsg);

      // Release the TMOS message
      tmos_msg_deallocate(pMsg);
    }

    // return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }

  if (events & SBP_START_DEVICE_EVT) {
    // Start the Device
    GAPRole_BroadcasterStartDevice(&bthome_broadcaster_callbacks);

    return (events ^ SBP_START_DEVICE_EVT);
  }

  if (events & SBP_ADV_UPDATE) {
    int ret = 0;
    if (aht10_status == 0) {
      // 写测量命令
      ret = i2c_write_to(aht10_addr, aht10_cmd_measure, sizeof(aht10_cmd_measure), 1, 1);
      if (ret == 0) {
        aht10_status = 1;
        // 等 80ms 后读
        tmos_start_task(broadcaster_task_id, SBP_ADV_UPDATE, MS1_TO_SYSTEM_TIME(80));
      } else {
        LOG_ERROR("[AHT10] write failed, ret: %d", ret);
        // 写入有问题，重启 I2C
        aht10_status = 0;
        I2C_SoftwareResetCmd(ENABLE);
        I2C_SoftwareResetCmd(DISABLE);
        tmos_start_task(broadcaster_task_id, SBP_ADV_UPDATE, MS1_TO_SYSTEM_TIME(1000));
      }
    } else if (aht10_status == 1) {
      // 读测量值
      ret = i2c_read_from(aht10_addr, aht10_rx_buffer, sizeof(aht10_rx_buffer), 1, 600);
      ret = aht10_process_rx(aht10_rx_buffer, ret);
      aht10_status = 0;
      if (ret == 2) {
        // 校准
        ret = i2c_write_to(aht10_addr, aht10_cmd_init, sizeof(aht10_cmd_init), 1, 1);
        if (ret != 0) {
          LOG_ERROR("[AHT10] calibration failed, ret: %d", ret);
        }
        tmos_start_task(broadcaster_task_id, SBP_ADV_UPDATE, MS1_TO_SYSTEM_TIME(1000));
      } else if (ret != 0) {
        LOG_ERROR("[AHT10] process rx failed, ret: %d", ret);
        tmos_start_task(broadcaster_task_id, SBP_ADV_UPDATE, MS1_TO_SYSTEM_TIME(1000));
      } else {
        // 更新成功，等比较长时间后进行下一次更新
        GAP_UpdateAdvertisingData(broadcaster_task_id, TRUE, sizeof(advertData_update), advertData_update);
        tmos_start_task(broadcaster_task_id, SBP_ADV_UPDATE, MS1_TO_SYSTEM_TIME(10000));
      }
    }

    return (events ^ SBP_ADV_UPDATE);
  }

  // Discard unknown events
  return 0;
}


void bthome_init() {
  broadcaster_task_id = TMOS_ProcessEventRegister(bthome_broadcaster_process_event);

  uint8_t initial_advertising_enable = TRUE;
  uint8_t initial_adv_event_type = GAP_ADTYPE_ADV_NONCONN_IND;
  GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t), &initial_advertising_enable);
  GAPRole_SetParameter(GAPROLE_ADV_EVENT_TYPE, sizeof(uint8_t), &initial_adv_event_type);
  GAPRole_SetParameter(GAPROLE_SCAN_RSP_DATA, sizeof(scanRspData), scanRspData);
  GAPRole_SetParameter(GAPROLE_ADVERT_DATA, sizeof(advertData), advertData);

  uint16_t advInt = DEFAULT_ADVERTISING_INTERVAL;
  GAP_SetParamValue(TGAP_DISC_ADV_INT_MIN, advInt);
  GAP_SetParamValue(TGAP_DISC_ADV_INT_MAX, advInt);

  tmos_set_event(broadcaster_task_id, SBP_START_DEVICE_EVT);
  tmos_start_task(broadcaster_task_id, SBP_ADV_UPDATE, MS1_TO_SYSTEM_TIME(1000));
}

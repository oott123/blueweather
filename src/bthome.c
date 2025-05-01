#include "CH58xBLE_LIB.h"
#include "bthome.h"

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
  // Flags; this sets the device to use limited discoverable
  // mode (advertises for 30 seconds at a time) instead of general
  // discoverable mode (advertises indefinitely)
  0x02, // length of this data
  GAP_ADTYPE_FLAGS,
  GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,

  // Broadcast of the data
  0x03, // length of this data including the data type byte
  GAP_ADTYPE_MANUFACTURER_SPECIFIC, // manufacturer specific advertisement data type
  0, 0,
  12,
  GAP_ADTYPE_LOCAL_NAME_COMPLETE, 'B', 'l', 'u', 'e', 'w', 'e', 'a', 't', 'h', 'e', 'r',
};

static uint8_t advertData_update[] = {
  // Flags; this sets the device to use limited discoverable
  // mode (advertises for 30 seconds at a time) instead of general
  // discoverable mode (advertises indefinitely)
  0x02,// length of this data
  GAP_ADTYPE_FLAGS,
  GAP_ADTYPE_FLAGS_GENERAL | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,

  0x0a,
  GAP_ADTYPE_SERVICE_DATA,
  // 0xD2FC4002C40903BF13 
  0xd2, 0xfc, 0x40, 0x02, 0xc4, 0x09, 0x03, 0xbf, 0x13,

  12,
  GAP_ADTYPE_LOCAL_NAME_COMPLETE, 'B', 'l', 'u', 'e', 'w', 'e', 'a', 't', 'h', 'e', 'r',
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
    // Update advert
    GAP_UpdateAdvertisingData(broadcaster_task_id, TRUE, sizeof(advertData_update), advertData_update);
    tmos_start_task(broadcaster_task_id, SBP_ADV_UPDATE, MS1_TO_SYSTEM_TIME(1000));

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

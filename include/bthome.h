#ifndef __BTHOME_H__
#define __BTHOME_H__

#include "CH58xBLE_LIB.h"

// What is the advertising interval when device is discoverable (units of 625us, min is 160=100ms)
#define DEFAULT_ADVERTISING_INTERVAL 160


// Simple BLE Broadcaster Task Events
#define SBP_START_DEVICE_EVT         0x0001
#define SBP_PERIODIC_EVT             0x0002
#define SBP_ADV_IN_CONNECTION_EVT    0x0004

#define SBP_ADV_UPDATE               0x0008

void bthome_init();

#endif

#ifndef TMI2C_H
#define TMI2C_H

#define I2C_EVENT_GENERATE_START (1 << 0)
#define I2C_EVENT_SEND_ADDRESS (1 << 1)
#define I2C_EVENT_DATA_TX (1 << 2)
#define I2C_EVENT_DATA_RX (1 << 3)
#define I2C_EVENT_GENERATE_STOP (1 << 4)
#define I2C_EVENT_DATA_CALLBACK (1 << 5)

void tmi2c_init();
typedef void (*tmi2c_data_cb_t)(const uint8_t *data, uint8_t len);
uint8_t tmi2c_write(uint8_t address, const uint8_t *data, uint8_t len, tmi2c_data_cb_t cb);
uint8_t tmi2c_read(uint8_t address, uint8_t len, tmi2c_data_cb_t cb);

#endif

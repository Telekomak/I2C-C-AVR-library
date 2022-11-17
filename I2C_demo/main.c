#define F_CPU 16000000

#include "../I2C/I2C.h"
#include "HD44780\HD44780_LCD.h"
#include <util\delay.h>

int main(void)
{
	uint8_t tx_result = 0x0F;
	uint8_t rx_result = 0x0F;
	
	PinConfig lcd_config = {.ddr = &DDRD, .port = &PORTD, .d0 = 32, .d1 = 16, .d2 = 128, .d3 = 64, .rs = 4, .en = 8};
	lcd_init(&lcd_config);
	lcd_on();
	lcd_clear();
	lcd_home();
	
	I2CConfig config = {.frequency = 100000, .mode = MASTER};
	uint8_t init_result = I2C_init(&config);
	I2C_enable();
	
	char* tx_buffer = malloc(12);
	strcpy(tx_buffer, "Hello world?");
	
	I2CTransmission tx_transmission = {.buffer = tx_buffer, .buffer_start = 0, .buffer_length = 12, .rw = 0, .slave_address = 0x0F};
	
	uint8_t* rx_buffer = malloc(12);
	I2CTransmission rx_transmission = {.buffer = rx_buffer, .buffer_start = 0, .buffer_length = 12, .rw = 1, .slave_address = 0x0F};
	
	rx_result = I2C_start_transmission(&rx_transmission);
	tx_result = I2C_start_transmission(&tx_transmission);
	
	lcd_write_string(itoa(init_result, "00", 16), init_result > 0xF? 2 : 1);
	lcd_write_string(itoa(tx_result, " 00" + 1, 16) - 1, tx_result > 0xF? 3 : 2);
	lcd_write_string(itoa(rx_result, " 00" + 1, 16) - 1, rx_result > 0xF? 3 : 2);
	
	lcd_write_string(itoa(tx_transmission.status, " 00" + 1, 16) - 1, tx_transmission.status > 0xF? 3 : 2);
	lcd_write_string(itoa(rx_transmission.status, " 00" + 1, 16) - 1, rx_transmission.status > 0xF? 3 : 2);
	
	lcd_set_cursor(1, 0);
	
	lcd_write_string(rx_transmission.buffer, 12);
}


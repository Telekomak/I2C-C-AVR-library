#define F_CPU 16000000

#include "../I2C/I2C.h"
#include "HD44780\HD44780_LCD.h"
#include <util\delay.h>

void test(I2CMasterTransmission* transmission);

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
	
	I2C_on_receive_subscribe(test);
	I2C_enable();
	
	//char* tx_buffer = malloc(13);
	//strcpy(tx_buffer, "Hello world?");
	I2CMasterTransmission tx_transmission = {.buffer = "Hello world?", .transmission_config = TCONFIG_MODE_WRITE | TCONFIG_STOP_TERMINATOR, .terminator = 0x0, .buffer_length = 20, .slave_address = 0x0F};
	
	uint8_t* rx_buffer = calloc(13, 1);
	I2CMasterTransmission rx_transmission = {.buffer = rx_buffer, .transmission_config = TCONFIG_MODE_READ | TCONFIG_STOP_TERMINATOR, .terminator = 0x0, .buffer_length = 20, .slave_address = 0x0F};
	
	rx_result = I2C_start_transmission(&rx_transmission);
	
	tx_result = I2C_start_transmission(&tx_transmission);
	
	lcd_write_string(itoa(rx_transmission.bytes_transmitted, "000", 10), 3);
	lcd_write_string(itoa(tx_transmission.bytes_transmitted, "000", 10), 3);
	
	lcd_write_string(itoa(init_result, "00", 16), init_result > 0xF? 2 : 1);
	lcd_write_string(itoa(rx_result, " 00" + 1, 16) - 1, rx_result > 0xF? 3 : 2);
	lcd_write_string(itoa(tx_result, " 00" + 1, 16) - 1, tx_result > 0xF? 3 : 2);
	
	lcd_write_string(itoa(rx_transmission.status, " 00" + 1, 16) - 1, rx_transmission.status > 0xF? 3 : 2);
	lcd_write_string(itoa(tx_transmission.status, " 00" + 1, 16) - 1, tx_transmission.status > 0xF? 3 : 2);
	
	lcd_set_cursor(1, 0);
	
	lcd_write_string(rx_transmission.buffer, rx_transmission.bytes_transmitted);
}

void test(I2CMasterTransmission* transmission)
{
	DDRB |= 0x20;
	PORTB |= 0x20;
}


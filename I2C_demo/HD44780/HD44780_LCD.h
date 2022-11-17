#ifndef HD44780_LCD_H_
#define HD44780_LCD_H_

#include <avr/io.h>

#ifndef F_CPU
	#define F_CPU 16000000u
#endif

typedef struct PinConfig{
	uint8_t* ddr;
	uint8_t* port;
	uint8_t rs;
	uint8_t en;
	uint8_t d0;
	uint8_t d1;
	uint8_t d2;
	uint8_t d3;
} PinConfig;

int lcd_init(PinConfig* config);
void lcd_command(uint8_t command);
void lcd_write_char(char character);
void lcd_clear();
void lcd_set_cursor(uint8_t row, uint8_t collumn);
void lcd_write_string(char* string, unsigned long length);
void lcd_pulse_en();
void lcd_pulse_en_repeat(int repeat);
void lcd_show_cursor(uint8_t blink);
void lcd_hide_cusror();
void lcd_on();
void lcd_off();
void lcd_home();
#endif
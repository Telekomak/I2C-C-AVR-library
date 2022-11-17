#include <avr/io.h>
#include "HD44780_LCD.h"
#include <util/delay.h>

#define LCD_DELAY 1500u

void clear_data_pins();
uint8_t my_log2(uint8_t x);
void write_value(uint8_t value, uint8_t rs_value);
void init_linear();
void init_nonlinear();
uint8_t verify_config();

uint8_t _d0_pin_offset = 0;
uint8_t _cursor_visible = 0;
uint8_t _cursor_blink = 0;
uint8_t _display_on = 0;
uint8_t _two_line = 1;
PinConfig* _config;

int lcd_init(PinConfig* config)
{
	_config = config;
	
	if (verify_config())
	{
		uint8_t ddr_value = ( _config -> rs | _config -> en
		| _config -> d0 | _config -> d1
		| _config -> d2 | _config -> d3);
		
		//Label LCD pins as output
		*(_config -> ddr) |= ddr_value;
		
		//Set all pins labeled as output to LOW
		*(_config -> port) &= ~ddr_value;
		
		//4-bit mode initialization sequence
		*(_config -> port) |= (_config -> d0 | _config -> d1);
		lcd_pulse_en_repeat(3);
		
		clear_data_pins();
		
		*(_config -> port) |= _config -> d1;
		lcd_pulse_en();
	}
	else return 1;
	
	//display config
	lcd_command(_two_line? 0x2C : 0x24);
	lcd_command(0x06);
	lcd_command(0x08);
	
	return 0;
}

uint8_t verify_config()
{
	uint8_t current = 0, previous = 0;
	
	//cycle trough all members the _config struct
	//skip first two because they are pointers (pointer is 2 bytes long)
	for (uint8_t i = 2 * sizeof(uint8_t*); i < sizeof(PinConfig); i++)
	{
		//access the _config member on address _config + i
		current |= *(((uint8_t*)_config) + i);
		
		//if nothing has changed, one of the previous iterations has already
		//set the bit to 1, which means that at least two values are the same,
		//or the _config struct member has value of 0
		if (current == previous) return 0;
		previous = current;
	}
	
	return 1;
}

void lcd_pulse_en()
{
	*(_config -> port) |= _config -> en;
	_delay_us(LCD_DELAY);
	*(_config -> port) &= ~_config -> en;
}

void lcd_pulse_en_repeat(int repeat)
{
	for (int i = 0; i < repeat; i++) lcd_pulse_en();
}

uint8_t my_log2(uint8_t x)
{
	//shamelessly stolen from https://stackoverflow.com/questions/3064926/how-to-write-log-base2-in-c-c
	return (uint8_t)(log10(x) / log10(2));
}

void write_value(uint8_t value, uint8_t rs_value)
{
	clear_data_pins();
	
	if (rs_value) *(_config -> port) |= _config -> rs;
	
	*(_config -> port) |= value & 0x80? _config -> d3 : 0;
	*(_config -> port) |= value & 0x40? _config -> d2 : 0;
	*(_config -> port) |= value & 0x20? _config -> d1 : 0;
	*(_config -> port) |= value & 0x10? _config -> d0 : 0;
	
	lcd_pulse_en();
	
	clear_data_pins();
	
	*(_config -> port) |= value & 0x08? _config -> d3 : 0;
	*(_config -> port) |= value & 0x04? _config -> d2 : 0;
	*(_config -> port) |= value & 0x02? _config -> d1 : 0;
	*(_config -> port) |= value & 0x01? _config -> d0 : 0;
	
	lcd_pulse_en();
	
	*(_config -> port) &= ~_config -> rs;
	
	clear_data_pins();
}

void lcd_command(uint8_t command)
{
	write_value(command, 0);
}

void lcd_write_char(char character)
{
	write_value(character, 1);
}

void lcd_write_string(char* string, unsigned long length)
{
	for (unsigned long i = 0; i < length; i++) lcd_write_char(*(string + i));
}

void clear_data_pins()
{
	*(_config -> port) &= ~(_config -> d0 | _config -> d1 | _config -> d2 | _config -> d3);
}

void lcd_clear()
{
	lcd_command(1);
}

void lcd_set_cursor(uint8_t row, uint8_t collumn)
{
	uint8_t command = 128;
	
	if (!_two_line) command += collumn % 80;
	else
	{
		 command += row? 64 : 0;
		 command += collumn % 40;
	}
	
	lcd_command(command);
}

void lcd_show_cursor(uint8_t blink)
{
	_cursor_visible = 1;
	_cursor_blink = blink? 1 : 0;
	
	uint8_t command = 0x0A;
	if (_cursor_blink) command |= 0x01;
	if (_display_on) command |= 0x04;
	
	lcd_command(command);
}

void lcd_hide_cusror()
{
	_cursor_visible = 0;
	
	lcd_command(_display_on? 0x0C : 0x08);
}

void lcd_on()
{
	_display_on = 1;
	
	uint8_t command = 0x0C;
	if (_cursor_visible) command |= 0x02;
	if (_cursor_blink) command |= 0x01;
	
	lcd_command(command);
}

void lcd_off()
{
	_display_on = 0;
	
	uint8_t command = 0x08;
	if (_cursor_visible) command |= 0x02;
	if (_cursor_blink) command |= 0x01;
	
	lcd_command(command);
}

void lcd_home()
{
	lcd_command(2);
	//this operation requires 1.52ms delay
	_delay_us(1600 - LCD_DELAY);
}
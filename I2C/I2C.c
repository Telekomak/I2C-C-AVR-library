#include "I2C.h"

//https://www.arnabkumardas.com/arduino-tutorial/i2c-register-description/
//https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf#G1199017
#define TWCR_INT 0x80 //TWI interrupt flag
#define TWCR_EA 0x40 //TWI Enable acknowledge bit
#define TWCR_STA 0x20 //TWI START condition bit
#define TWCR_STO 0x10 //TWI STOP condition bit
#define TWCR_WC 0x08 //TWI Write collision flag
#define TWCR_EN 0x04 //TWI Enable bit
#define TWCR_INTEN 0x01 //TWI Interrupt enable bit

#define TWSR_STATUS 0xF8 //TWI Status
#define TWSR_PRS 0x03 //TWI Prescaler

#define TWAR_GCE 0x01 //TWI General call recognition enable bit

uint8_t set_frequency(uint32_t frequency);
void write_value(uint8_t value);

I2CConfig* _config;
I2CTransmission* _transmition;
I2CTransmissionStatus _expected_status;

//Return codes:
//1: Invalid frequency
//2: Invalid address
int I2C_init(I2CConfig* config)
{
	_config = config;
	I2C_transmission_ended = 1;
	
	if (!config -> mode == I2CMode::SLAVE) if (!set_frequency(config -> frequency)) return 1;
	if (!config -> mode == I2CMode::MASTER) 
	{
		if (config -> address < 0x08 || config -> address > 0x77) return 2;
		
		TWAR = 0;
		
		//Load address into TWAR register
		TWAR = config -> address << 1;
		TWAR |= config -> recognize_general_call? 1 : 0;
	}
	
	TWCR = 0;
	//Enable ACK
	TWCR |= TWCR_EA;
	
	if (config -> recognize_general_call) TWAR |= TWAR_GCE;
	
	return 0;
}

uint8_t set_frequency(uint32_t frequency)
{
	if (frequency == 0) return 1;
	
	TWBR = 0;
	TWSR = 0;
	
	//Frequency formula:
	//SCL frequency = CPU clock frequency / (16 + 2 * TWBR * PrescalerValue)
	
	//Get TWBR * Prescaler
	uint32_t TWBRP = (F_CPU / (2 * frequency)) - 8;
	
	//Get prescaler and TWBR value
	uint8_t PSCLR = 1;
	
	//Prescaler values:
	//TWSR1...TWSR0...Prescaler value
	//	0.......0............1
	//	0.......1............4
	//	1.......0...........16
	//	1.......1...........64
	
	for (uint8_t i = 0; i < 4; i++)
	{
		if (TWBRP / PSCLR <= 255)
		{
			TWBR = TWBRP / PSCLR;
			TWSR = i;
			
			return 0;
		}
		
		PSCLR = PSCLR >> 2;
	}
	
	return 1;
}

void I2C_enable()
{
	TWCR |= TWCR_EN;
}

void I2C_disable()
{
	TWCR &= ~TWCR_EN;
}

void I2C_enable_GC_recognition()
{
	TWAR |= TWAR_GCE;
}

void I2C_disable_GC_recognition()
{
	TWAR &= ~TWAR_GCE;
}

void write_value(uint8_t value)
{
	
}

ISR(TWI_vect)
{
	
}

void I2C_read(I2CTransmission* transmission)
{
	if (TWCR &= ~TWCR_EN)
	{
		transmission -> status = I2CTransmissionStatus::C_ERR_TWI_DISABLED;
		return;
	}
	
	if (TWCR &= ~TWCR_EA)
	{
		transmission -> status = I2CTransmissionStatus::C_ERR_ACK_DISABLED;
		return;
	}
	
	//Disable interrupt
	TWCR &= ~TWCR_INTEN;
	
	//Send start condition
	_expected_status = I2CTransmissionStatus::MTR_START
	
	for (uint16_t i = transmission -> buffer_start; i < transmission -> buffer_length; i++)
	{
	}
}

void I2C_write(I2CTransmission* transmission)
{
	
}
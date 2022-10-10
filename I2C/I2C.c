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
uint8_t m_transmit(I2CTransmission* transmission);
uint8_t m_receive(I2CTransmission* transmission);

I2CConfig* _config;
I2CTransmission* _transmition;
I2CTransmission** _transmission_queue;
enum I2CTransmissionStatus _expected_status;
uint8_t _current_queue_position = 0;
uint8_t _last_queue_position = 0;

//Return codes:
//1: Invalid frequency
//2: Invalid address
int I2C_init(I2CConfig* config)
{
	_transmission_queue = malloc(sizeof(I2CTransmission*) * 256);
	_config = config;
	I2C_transmission_ended = 1;
	
	if (config -> mode != SLAVE) if (!set_frequency(config -> frequency)) return 1;
	if (config -> mode != MASTER) 
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

void I2C_free()
{
	free(_transmission_queue);
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
	if(_config -> mode != MASTER) TWAR |= TWAR_GCE;
}

void I2C_disable_GC_recognition()
{
	if(_config -> mode != MASTER) TWAR &= ~TWAR_GCE;
}

void I2C_queue_transmission(I2CTransmission* transmission)
{
	_last_queue_position++;
	_transmission_queue[_last_queue_position] = transmission;
}

ISR(TWI_vect)
{
	
}

//Return codes:
//1: Unexpected TWI state
uint8_t m_transmit(I2CTransmission* transmission)
{	
	//START:
	//Send start condition
	TWCR |= TWCR_STA;
	
	//Clear the interrupt flag
	TWCR |= TWCR_INT;
	
	//Wait for interrupt flag
	while(TWCR & ~TWCR_INT);
	
	transmission -> status = TWSR & TWSR_STATUS;
	if((TWSR & TWSR_STATUS) != (uint8_t)MTR_START) return 1;
	
	//SLA+W:
	//Load SLA+W
	TWDR = transmission -> slave_address << 1;
	
	//Clear the start flag
	TWCR &= ~TWCR_STA;
	
	//Clear the interrupt flag
	TWCR |= TWCR_INT;
	
	while(TWCR & ~TWCR_INT);
	
	transmission -> status = TWSR & TWSR_STATUS;
	if (transmission -> status != MT_SLAW_ACK) return 1;
	
	//DATA:
	for(uint16_t i = transmission -> buffer_start; (i - transmission -> buffer_start) < transmission -> buffer_length; i++)
	{
		TWDR = transmission -> buffer[i];
		TWCR |= TWCR_INT;
		
		while(TWCR & ~TWCR_INT);
		
		transmission -> status = TWSR & TWSR_STATUS;
		if (transmission -> status != MT_DATA_ACK) return 1;
	}
	
	transmission -> status = C_SUCCESS;
	return 0;
}

//Return codes:
//1: Unexpected TWI state
uint8_t m_receive(I2CTransmission* transmission)
{
	//START:
	//Send start condition
	TWCR |= TWCR_STA;
	
	//Clear the interrupt flag
	TWCR |= TWCR_INT;
	
	//Wait for interrupt flag
	while(TWCR & ~TWCR_INT);
	
	transmission -> status = TWSR & TWSR_STATUS;
	if((TWSR & TWSR_STATUS) != (uint8_t)MTR_START) return 1;
	
	//SLA+R:
	//Load SLA+R
	TWDR = (transmission -> slave_address << 1) | 1;
	
	//Clear the start flag
	TWCR &= ~TWCR_STA;
	
	//Clear the interrupt flag
	TWCR |= TWCR_INT;
	
	while(TWCR & ~TWCR_INT);
	
	transmission -> status = TWSR & TWSR_STATUS;
	if (transmission -> status != MR_SLAR_ACK) return 1;
	
	//DATA:
	for(uint16_t i = transmission -> buffer_start; (i - transmission -> buffer_start) < transmission -> buffer_length; i++)
	{
		TWCR |= TWCR_INT;
		
		while(TWCR & ~TWCR_INT);
		
		transmission -> status = TWSR & TWSR_STATUS;
		if (transmission -> status != MR_DATA_ACK) return 1;
		
		transmission -> buffer[i] = TWDR;
	}
	
	transmission -> status = C_SUCCESS;
	return 0;
}

//Return codes:
//1: TWI is disabled
//2: ACK is disabled
uint8_t I2C_start_transmission(I2CTransmission* transmission, uint8_t attempts)
{	
	if (TWCR &= ~TWCR_EN) return 1;
	if (TWCR &= ~TWCR_EA) return 2;
	if (attempts < 1) attempts = 1;
	
	I2C_queue_transmission(transmission);
	while(!I2C_transmission_ended);
	
	I2C_transmission_ended = 0;
	
	//Disable interrupt
	TWCR &= ~TWCR_INTEN;
	
	//Clear STOP flag
	TWCR &= ~TWCR_STO;
	
	while(_current_queue_position != _last_queue_position)
	{
		for (uint8_t i = 0; i < attempts; i++)
		{
			if (_transmission_queue[_current_queue_position] -> rw)
				if (!m_receive(_transmission_queue[_current_queue_position])) break;
			else
				if (!m_transmit(_transmission_queue[_current_queue_position])) break;
		}
		
		_current_queue_position++;
	}
	
	TWCR |= TWCR_STO;
	
	I2C_transmission_ended = 1;
	
	return 0;
}

uint8_t I2C_start_transmission_async(I2CTransmission* transmission, uint8_t attempts)
{
	if (TWCR &= ~TWCR_EN) return 1;
	if (TWCR &= ~TWCR_EA) return 2;
	if (attempts < 1) attempts = 1;
	
	I2C_queue_transmission(transmission);
	if (!I2C_transmission_ended) return 0;
	
	//Enable interrupt
	TWCR |= TWCR_INTEN;
	
	//Clear STOP flag
	TWCR &= ~TWCR_STO;
	
	I2C_transmission_ended = 0;
	
	I2C_transmission_ended = 1;
	return 0;
}
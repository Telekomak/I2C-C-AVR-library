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

uint8_t _I2C_set_frequency(uint32_t frequency);
enum I2CTransmissionResult _I2C_m_send(I2CTransmission* transmission);
enum I2CTransmissionResult _I2C_m_receive(I2CTransmission* transmission);

I2CConfig* _I2C_config;
I2CTransmission** _I2C_transmission_queue;
enum I2CTransmissionStatus _I2C_expected_status;
uint8_t _I2C_current_queue_position = 0;
uint8_t _I2C_last_queue_position = 0;

//Return codes:
//0: Success
//1: Invalid frequency
//2: Invalid address
uint8_t I2C_init(I2CConfig* config)
{
	PORTC |= 0x30;
	
	_I2C_transmission_queue = calloc(128, sizeof(I2CTransmission*));
	_I2C_config = config;
	I2C_transmission_ended = 1;
	
	if (config -> mode != SLAVE) if (_I2C_set_frequency(config -> frequency)) return 1;
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
	
	return 0;
}

void I2C_free()
{
	free(_I2C_transmission_queue);
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
	if(_I2C_config -> mode != MASTER) TWAR |= TWAR_GCE;
}

void I2C_disable_GC_recognition()
{
	if(_I2C_config -> mode != MASTER) TWAR &= ~TWAR_GCE;
}

void I2C_queue_transmission(I2CTransmission* transmission)
{
	transmission -> status = C_PENDING;
	_I2C_transmission_queue[_I2C_last_queue_position] = transmission;
	_I2C_last_queue_position++;
}

enum I2CTransmissionResult I2C_start_transmission(I2CTransmission* transmission)
{
	if (!(TWCR & TWCR_EN)) return ERR_TWI_DISABLED;
	if (!(TWCR & TWCR_EA)) return ERR_ACK_DISABLED;
	if (_I2C_config -> mode == SLAVE) return ERR_SLAVE;
	
	while(!I2C_transmission_ended)
	I2C_transmission_ended = 0;
	
	//Disable interrupt
	TWCR &= ~TWCR_INTEN;
	
	//Clear STOP flag
	TWCR &= ~TWCR_STO;
	
	enum I2CTransmissionResult result;
	
	if (transmission -> rw) result = _I2C_m_receive(transmission);
	else result = _I2C_m_send(transmission);
	
	if(result == ARB_LOST_SLA)
	{
		//Enable interrupt
		TWCR |= TWCR_INTEN;
		return result;
	}
	
	TWCR |= TWCR_STO;
	
	TWCR |= TWCR_INT;
	
	I2C_transmission_ended = 1;

	//Enable interrupt
	TWCR |= TWCR_INTEN;
	
	return result;
}

/*WIP
//Return codes:
//0: Success
//1: TWI is disabled
//2: ACK is disabled
//3: Device is in slave mode
uint8_t I2C_start_transmission_async(I2CTransmission* transmission)
{
	if (TWCR &= ~TWCR_EN) return 1;
	if (TWCR &= ~TWCR_EA) return 2;
	if (_I2C_config -> mode == SLAVE) return 3;
	
	I2C_queue_transmission(transmission);
	if (!I2C_transmission_ended) return 0;
	
	//Enable interrupt
	TWCR |= TWCR_INTEN;
	
	//Clear STOP flag
	TWCR &= ~TWCR_STO;
	
	I2C_transmission_ended = 0;
	
	_I2C_expected_status = MTR_START;
	
	//START:
	//Load start condition
	TWCR |= TWCR_STA;
	
	//Clear the interrupt flag
	TWCR |= TWCR_INT;
	
	return 0;
}
*/

//Return codes:
//0: Success
//1: Invalid address
uint8_t _I2C_set_frequency(uint32_t frequency)
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

/*WIP
ISR(TWI_vect)
{
	_I2C_transmission_queue[_I2C_current_queue_position] -> status = TWSR & TWSR_STATUS;
	
	if ((TWSR & TWSR_STATUS) != _I2C_expected_status)
	{
		_I2C_current_queue_position++;
		if (_I2C_current_queue_position == _I2C_last_queue_position) return;
		
		_I2C_expected_status = C_PENDING;
		TWCR |= TWCR_INT;
	}
	
	switch(_I2C_expected_status)
	{
		
	}
}
*/
enum I2CTransmissionResult _I2C_m_send(I2CTransmission* transmission)
{	
	//START:
	//Load start condition
	TWCR |= TWCR_STA;
	
	//Clear the interrupt flag
	TWCR |= TWCR_INT;
	
	//Wait for interrupt flag
	while(!(TWCR & TWCR_INT));
	
	transmission -> status = TWSR & TWSR_STATUS;
	if(!((TWSR & TWSR_STATUS) == MTR_START || (TWSR & TWSR_STATUS) == MTR_REPSTART))
	{	
		switch (TWSR & TWSR_STATUS)
		{
			case MTR_ARB_LOST:
				return ARB_LOST;
				
			case SR_ARB_LOST_SLAW_ACK:
				return ARB_LOST_SLA;
				
			case SR_ARB_LOST_GC_ACK:
				return ARB_LOST_SLA;
				
			case ST_ARB_LOST_SLAR_ACK:
				return ARB_LOST_SLA;
				
			default:
				return UNEXPECTED_STATE;
		}
	}
	
	//SLA+W:
	//Load SLA+W
	TWDR = transmission -> slave_address << 1;
	
	//Clear the start flag
	TWCR &= ~TWCR_STA;
	
	//Clear the interrupt flag
	TWCR |= TWCR_INT;
	
	while(!(TWCR & TWCR_INT));
	
	transmission -> status = TWSR & TWSR_STATUS;
	if ((TWSR & TWSR_STATUS) != MT_SLAW_ACK) return UNEXPECTED_STATE;
	
	//DATA:
	for(uint16_t i = transmission -> buffer_start; (i - transmission -> buffer_start) < transmission -> buffer_length; i++)
	{
		TWDR = transmission -> buffer[i];
		TWCR |= TWCR_INT;
		
		while(!(TWCR & TWCR_INT));
		
		transmission -> status = TWSR & TWSR_STATUS;
		if ((TWSR & TWSR_STATUS) != MT_DATA_ACK) return UNEXPECTED_STATE;
		
		transmission -> bytes_transmitted++;
	}
	
	transmission -> status = C_SUCCESS;
	return SUCCESS;
}

enum I2CTransmissionResult _I2C_m_receive(I2CTransmission* transmission)
{
	//START:
	//Load start condition
	TWCR |= TWCR_STA;
	
	//Clear the interrupt flag
	TWCR |= TWCR_INT;
	
	//Wait for interrupt flag
	while(!(TWCR & TWCR_INT));
	
	transmission -> status = TWSR & TWSR_STATUS;
	if(!((TWSR & TWSR_STATUS) == MTR_START || (TWSR & TWSR_STATUS) == MTR_REPSTART))
	{
		switch (TWSR & TWSR_STATUS)
		{
			case MTR_ARB_LOST:
				return ARB_LOST;
			
			case SR_ARB_LOST_SLAW_ACK:
				return ARB_LOST_SLA;
			
			case SR_ARB_LOST_GC_ACK:
				return ARB_LOST_SLA;
			
			case ST_ARB_LOST_SLAR_ACK:
				return ARB_LOST_SLA;
			
			default:
				return UNEXPECTED_STATE;
		}
	}
	
	//SLA+R:
	//Load SLA+R
	TWDR = (transmission -> slave_address << 1) | 1;
	
	//Clear the start flag
	TWCR &= ~TWCR_STA;
	
	//Clear the interrupt flag
	TWCR |= TWCR_INT;
	
	while(!(TWCR & TWCR_INT));
	
	transmission -> status = TWSR & TWSR_STATUS;
	if (transmission -> status != MR_SLAR_ACK) return UNEXPECTED_STATE;
	
	//DATA:
	for(uint16_t i = transmission -> buffer_start; (i - transmission -> buffer_start) < transmission -> buffer_length; i++)
	{
		TWCR |= TWCR_INT;
		
		while(!(TWCR & TWCR_INT));
		
		transmission -> status = TWSR & TWSR_STATUS;
		if (transmission -> status != MR_DATA_ACK) return UNEXPECTED_STATE;
		
		transmission -> buffer[i] = TWDR;
		transmission -> bytes_transmitted++;
	}
	
	transmission -> status = C_SUCCESS;
	return SUCCESS;
}
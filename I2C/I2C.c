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
enum I2CTransmissionResult _I2C_m_request(I2CTransmission* transmission);
void _I2C_on_receive_invoke();
void _I2C_write_to_stream(I2CStream* stream, uint8_t value);
void _I2C_trim_stream(I2CStream* stream);

//SR status handlers
//very painful
void _I2C_status_SR_SLAW_ACK();
void _I2C_status_SR_ARB_LOST_SLAW_ACK();
void _I2C_status_SR_ARB_LOST_GC_ACK();
void _I2C_status_SR_GC_ACK();
void _I2C_status_SR_DATA_ACK();
void _I2C_status_SR_DATA_NACK();
void _I2C_status_SR_GC_DATA_ACK();
void _I2C_status_SR_GC_DATA_NACK();
void _I2C_status_SR_STOP_REPSTART();

I2CConfig* _I2C_config;
enum I2CTransmissionStatus _I2C_last_status;
I2CStream* _I2C_current_receive_stream;
uint16_t _I2C_bytes_received;
void (*_I2C_on_receive_handler)(I2CStream);

//Return codes:
//0: Success
//1: Invalid frequency
//2: Invalid address
uint8_t I2C_init(I2CConfig* config)
{
	PORTC |= 0x30;
	
	_I2C_on_receive_handler = 0;
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
		
		//Enable interrupt
		TWCR |= TWCR_INTEN;
	}
	
	TWCR = 0;
	//Enable ACK
	TWCR |= TWCR_EA;
	
	return 0;
}

void I2C_enable() {TWCR |= TWCR_EN;}
void I2C_disable() {TWCR &= ~TWCR_EN;}
void I2C_enable_GC_recognition() {if(_I2C_config -> mode != MASTER) TWAR |= TWAR_GCE;}
void I2C_disable_GC_recognition() {if(_I2C_config -> mode != MASTER) TWAR &= ~TWAR_GCE;}
void I2C_on_receive_subscribe(void* handler) {_I2C_on_receive_handler = handler;}
void I2C_on_receive_unsubscribe() {_I2C_on_receive_handler = 0;}

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
	
	if (transmission -> transmission_config & TCONFIG_MODE) result = _I2C_m_request(transmission);
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

//Passing whole struct because pointer can change in next ISR
void _I2C_on_receive_invoke() { if(_I2C_on_receive_handler) _I2C_on_receive_handler(*_I2C_current_receive_stream);}

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

ISR(TWI_vect)
{
	I2CTransmissionStatus status = TWSR & TWSR_STATUS;
	
	//PAIN
	switch(status)
	{
		case SR_SLAW_ACK:
			_I2C_status_SR_SLAW_ACK();
			break;
		
		case SR_ARB_LOST_SLAW_ACK:
			_I2C_status_SR_ARB_LOST_SLAW_ACK();
			break;
		
		case SR_ARB_LOST_GC_ACK:
			_I2C_status_SR_GC_ACK();
			break;
		
		case SR_GC_ACK:
			_I2C_status_SR_GC_ACK();
			break;
		
		case SR_DATA_ACK:
			_I2C_status_SR_DATA_ACK();
			break;
		
		case SR_DATA_NACK:
			_I2C_status_SR_DATA_NACK();
			break;
		
		case SR_GC_DATA_ACK:
			_I2C_status_SR_GC_DATA_ACK()
			break;
		
		case SR_GC_DATA_NACK:
			_I2C_status_SR_GC_DATA_NACK();
			break;
		
		case SR_STOP_REPSTART:
			_I2C_status_SR_STOP_REPSTART();
			break;
	}
}

void _I2C_status_SR_SLAW_ACK()
{
	if(_I2C_current_receive_stream) free(_I2C_current_receive_stream);
	_I2C_bytes_received = 0;
	
	_I2C_current_receive_stream = calloc(sizeof(I2CStream), 1);
	_I2C_current_receive_stream -> buffer = calloc(1, 8);
	_I2C_current_receive_stream -> length = 8;
}

//This works similar to array list
void _I2C_write_to_stream(I2CStream* stream, uint8_t value)
{
	if (stream -> length == _I2C_bytes_received)
	{
		//copy the buffer
		char* tmp = calloc(1, stream -> length);
		memcpy(tmp, stream -> buffer, stream -> length);
		
		//free and resize
		free(stream -> buffer);
		stream -> buffer = calloc(1, stream -> length * 2);
		
		//move data back and free tmp
		memcpy(stream -> buffer, tmp, stream -> length);
		free(tmp);
		
		//update size
		stream -> length *= 2; 
	}
	
	stream -> buffer[_I2C_bytes_received] = value;
}

void _I2C_trim_stream(I2CStream* stream)
{
	char* tmp = calloc(1, _I2C_bytes_received);
	memcpy(tmp, stream -> buffer, _I2C_bytes_received);
	
	free(stream -> buffer);
	stream -> buffer = calloc(1, _I2C_bytes_received);
	
	memcpy(stream -> buffer, tmp, _I2C_bytes_received);
	free(tmp);
	
	stream -> length = _I2C_bytes_received;
}

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
	
	//DATA
	for(uint16_t i = 0; i < transmission -> stream.length; i++)
	{
		TWDR = transmission -> stream.buffer[i];
		TWCR |= TWCR_INT;
		
		while(!(TWCR & TWCR_INT));
		
		transmission -> status = TWSR & TWSR_STATUS;
		if ((TWSR & TWSR_STATUS) != MT_DATA_ACK) return UNEXPECTED_STATE;
		
		transmission -> bytes_transmitted++;
		if (transmission -> stream.buffer[i] == transmission -> terminator && !(transmission -> transmission_config & TCONFIG_STOP)) break;
	}
	
	transmission -> status = C_SUCCESS;
	return SUCCESS;
}

enum I2CTransmissionResult _I2C_m_request(I2CTransmission* transmission)
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
	for(uint16_t i = 0; i < transmission -> stream.length; i++)
	{
		TWCR |= TWCR_INT;
		
		while(!(TWCR & TWCR_INT));
		
		transmission -> status = TWSR & TWSR_STATUS;
		if (transmission -> status != MR_DATA_ACK) return UNEXPECTED_STATE;
		
		transmission -> stream.buffer[i] = TWDR;
		transmission -> bytes_transmitted++;
		if (TWDR == transmission -> terminator && !(transmission -> transmission_config & TCONFIG_STOP)) break;
	}
	
	transmission -> status = C_SUCCESS;
	return SUCCESS;
}
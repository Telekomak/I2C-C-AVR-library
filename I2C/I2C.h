#ifndef I2C_H_
#define I2C_H_

#include <avr/io.h>
#include <avr/interrupt.h>

#define F_CPU 16000000

enum I2CMode{
	MASTER = 0,
	MULTI_MASTER = 1,
	SLAVE = 2
};

enum I2CTransmissionStatus{
	//MTR - Master transmitter/receiver
	//MT - Master transmitter
	//MR - Master receiver
	//SR - Slave receiver
	//ST - Slave transmitter
	//M - Miscellaneous
	//C - Custom (not defined in datasheet)
	
	MTR_START = 0x08, //A START condition has been transmitted
	MTR_REPSTART = 0x10, //A repeated START condition has been transmitted
	MTR_ARB_LOST = 0x38, //Double meaning:
	//1. MR: Arbitration lost in SLA+R or NOT ACK bit
	//2. MT: Arbitration lost in SLA+W or data bytes
	
	MT_SLAW_ACK = 0x18, //SLA+W has been transmitted; ACK has been received
	MT_SLAW_NACK = 0x20, //SLA+W has been transmitted; NOT ACK has been received
	MT_DATA_ACK = 0x28, //Data byte has been transmitted; ACK has been received
	MT_DATA_NACK = 0x30, //Data byte has been transmitted; NOT ACK has been received
	
	MR_SLAR_ACK = 0x40, //SLA+R has been transmitted; ACK has been received
	MR_SLAR_NACK = 0x48, //SLA+R has been transmitted; NOT ACK has been received
	MR_DATA_ACK = 0x50, //Data byte has been received; ACK has been returned
	MR_DATA_NACK = 0x58, //Data byte has been received; NOT ACK has been returned
	
	SR_SLAW_ACK = 0x60, //Own SLA+W has been received; ACK has been returned
	SR_ARB_LOST_SLAW_ACK = 0x68, //Arbitration lost in SLA+R/W as Master; own SLA+W has been received; ACK has been returned
	SR_ARB_LOST_GC_ACK = 0x78, //General call address has been received; ACK has been returned
	SR_GC_ACK = 0x70, //Arbitration lost in SLA+R/W as Master; General call address has been received; ACK has been returned
	SR_DATA_ACK = 0x80, //Previously addressed with own SLA+W; data has been received; ACK has been returned
	SR_DATA_NACK = 0x88, //Previously addressed with own SLA+W; data has been received; NOT ACK has been returned
	SR_GC_DATA_ACK = 0x90, //Previously addressed with general call; data has been received; ACK has been returned
	SR_GC_DATA_NACK = 0x90, //Previously addressed with general call; data has been received; NOT ACK has been returned
	SR_STOP_REPSTART = 0xA0, //A STOP condition or repeated START condition has been received while still addressed as slave
	
	ST_SLAR_ACK = 0xA8, //Own SLA+R has been received; ACK has been returned
	ST_ARB_LOST_SLAR_ACK = 0xB0, //Arbitration lost in SLA+R/W as Master; own SLA+R has been received; ACK has been returned
	ST_DATA_ACK = 0xB8, //Data byte in TWDR has been transmitted; ACK has been received
	ST_DATA_NACK = 0xC0, //Data byte in TWDR has been transmitted; NOT ACK has been received
	ST_DATA_DONE = 0xC8, //Last data byte in TWDR has been transmitted (TWEA = “0”); ACK has been received
	
	M_NO_INFO = 0xF8, //No relevant state information available; TWINT = “0”
	M_ERR_ILLEGAL_START_STOP = 0x00, //Bus error due to an illegal START or STOP condition
	
	C_ERR_ACK_DISABLED = 0x01, //TWCR ACK enable bit is 0 (bit 6)
	C_ERR_TWI_DISABLED = 0x02, //TWCR TWI enable bit is 0 (bit 2)
	C_ERR_GENERAL = 0x03, //Undefined error
	C_SUCCESS = 0x04 //Transmission was successful
};

typedef struct I2CConfig{
	uint32_t frequency;
	uint8_t address;
	enum I2CMode mode;
	uint8_t recognize_general_call;
} I2CConfig;

typedef struct I2CTransmission{
	uint8_t slave_address;
	uint8_t rw;
	uint8_t* buffer;
	uint16_t buffer_start;
	uint16_t buffer_length;
	uint16_t bytes_transmitted;
	enum I2CTransmissionStatus status;
} I2CTransmission;

uint8_t I2C_transmission_ended;

int I2C_init();
void I2C_enable();
void I2C_disable();
void I2C_enable_GC_recognition();
void I2C_disable_GC_recognition();
void I2C_start_transmission(I2CTransmission* transmission);
void I2C_start_transmission_async(I2CTransmission* transmission);
#endif
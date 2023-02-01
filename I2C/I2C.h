#ifndef I2C
#define I2C

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>

#define F_CPU 16000000

//transmission config
#define TCONFIG_MODE 0x01
#define TCONFIG_STOP 0x02

#define TCONFIG_MODE_READ 0x01
#define TCONFIG_MODE_WRITE 0x00
#define TCONFIG_STOP_LENGTH 0x02
#define TCONFIG_STOP_TERMINATOR 0x00

enum I2CMode{
	MASTER = 0,
	MULTI_MASTER = 1,
	SLAVE = 2
};

enum I2CTransmissionResult{
	SUCCESS = 0,
	ERR_TWI_DISABLED = 1,
	ERR_ACK_DISABLED = 2,
	ERR_SLAVE = 3,
	ARB_LOST = 4,
	ARB_LOST_SLA = 5,
	UNEXPECTED_STATE = 6
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
	C_PENDING = 0x03, //Transmission in queue
	C_SUCCESS = 0x04 //Transmission was successful
};

typedef struct I2CConfig{
	uint32_t frequency;
	uint8_t address;
	enum I2CMode mode;
	uint8_t recognize_general_call;
} I2CConfig;

typedef struct I2CTransmission{
	I2CStream stream;
	uint8_t slave_address;
	uint8_t transmission_config;
	uint8_t terminator;//TODO change terminator behavior
	uint16_t bytes_transmitted;
	enum I2CTransmissionStatus status;
} I2CTransmission;

typedef struct I2CStream{
	char* buffer;
	uint16_t length;
} I2CStream;

volatile uint8_t I2C_transmission_ended;

uint8_t I2C_init();
void I2C_enable();
void I2C_disable();
void I2C_enable_GC_recognition();
void I2C_disable_GC_recognition();
enum I2CTransmissionResult I2C_start_transmission(I2CTransmission* transmission);
void I2C_on_receive_subscribe(void* handler);
void I2C_on_receive_unsubscribe();

#endif
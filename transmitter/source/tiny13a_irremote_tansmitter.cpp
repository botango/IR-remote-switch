#define F_CPU 4800000UL
//#define F_CPU 9600000UL

#include <avr/io.h>
#include <util/delay.h>

#ifndef cbi
#define cbi(PORT, BIT) (_SFR_BYTE(PORT) &= ~_BV(BIT))
#endif
#ifndef sbi
#define sbi(PORT, BIT) (_SFR_BYTE(PORT) |= _BV(BIT))
#endif

#define IR_LED			PB1
#define _IR_LED_HIGH	(TCCR0A |= _BV(COM0B1))
#define _IR_LED_LOW		(TCCR0A &= ~_BV(COM0B1))

#define HEADER_H		9000
#define HEADER_L		4500
#define STOP_H			600
#define STOP_L			40000

#define PULSE0_H		600
#define PULSE0_L		600
#define PULSE1_H		600
#define PULSE1_L		1800

#define REP_HEADER_H	9000
#define REP_HEADER_L	2250
#define REP_STOP_H		600
#define REP_STOP_L		95000

#define DATA_BYTE_COUNT 4
#define DATA_BIT_COUNT  (DATA_BYTE_COUNT*8)

uint8_t remote_code[DATA_BYTE_COUNT]=
{0x01,0x00,0x00,0x01};
//{0x01,0x00,0x00,0x02};
//{0x01,0x00,0x00,0x03};

//{0x02,0x00,0x00,0x01};
//{0x02,0x00,0x00,0x02};
//{0x02,0x00,0x00,0x03};

//{0x03,0x00,0x00,0x01};
//{0x03,0x00,0x00,0x02};
//{0x03,0x00,0x00,0x03};

void send(uint8_t *code)
{
	////////////////
	// Header send
	_IR_LED_HIGH;
	_delay_us(HEADER_H);
	
	_IR_LED_LOW;
	_delay_us(HEADER_L);
	
	///////////////
	// Code send
	uint8_t bitCount,byteCount=0;
	uint8_t fBit=0x80;
	for(bitCount=0; bitCount<DATA_BIT_COUNT; bitCount++)
	{
		if(code[byteCount]&fBit)
		{
			_IR_LED_HIGH;
			_delay_us(PULSE1_H);
			
			_IR_LED_LOW;
			_delay_us(PULSE1_L);
		}
		else
		{
			_IR_LED_HIGH;
			_delay_us(PULSE0_H);
			
			_IR_LED_LOW;
			_delay_us(PULSE0_L);
		}
		
		fBit>>=1;
		if(!fBit)
		{
			byteCount++;
			fBit=0x80;
		}
	}
	
	///////////////
	// Stop send
	_IR_LED_HIGH;
	_delay_us(STOP_H);
	
	_IR_LED_LOW;
	_delay_us(STOP_L);
	
	
	///////////////
	// Repeat send
	/*
	while(1)
	{
		_IR_LED_HIGH
		_delay_us(REP_HEADER_H);
		
		_IR_LED_LOW
		_delay_us(REP_HEADER_L);
		
		_IR_LED_HIGH
		_delay_us(REP_STOP_H);
		
		_IR_LED_LOW
		_delay_us(REP_STOP_L);
	}
	*/
	
}

int main(void)
{
	DDRB |= _BV(IR_LED);

	cbi(TCCR0A,COM0B1);
	
	TCCR0A = _BV(WGM00);
	TCCR0B = _BV(WGM02) | _BV(CS00);
	
	OCR0A = (F_CPU / 2 / 38300)-1;	// freq 38.3kHz
	OCR0B = OCR0A / 3;				// duty 1/3

	send(remote_code);
	
	while(true);	// infinite loop
}

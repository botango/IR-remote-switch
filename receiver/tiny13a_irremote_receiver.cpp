//#define F_CPU 9600000UL
#define F_CPU 4800000UL

#include <avr/io.h>
#include <util/delay.h>

#ifndef cbi
#define cbi(PORT, BIT) (_SFR_BYTE(PORT) &= ~_BV(BIT))
#endif
#ifndef sbi
#define sbi(PORT, BIT) (_SFR_BYTE(PORT) |= _BV(BIT))
#endif

#define LED1			PB3		// LED
#define IR_SENSOR1		PB4		// IR sensor
#define SW1				PB2		// Push switch
#define FET_DRV			PB1		// FET Drive
#define APOC_DRV		PB0		// Auto Power Off Canceler

#define THRESHOLD_HEADER_HIGH_MIN		144		// 9000 * 0.8 / 50
#define THRESHOLD_HEADER_HIGH_MAX		216		// 9000 * 1.2 / 50
#define THRESHOLD_HEADER_LOW_MIN		72		// 4500 * 0.8 / 50
#define THRESHOLD_HEADER_LOW_MAX		108		// 4500 * 1.2 / 50
#define THRESHOLD_HEADER_REP_LOW_MIN	36		// 2250 * 0.8 / 50
#define THRESHOLD_HEADER_REP_LOW_MAX	54		// 2250 * 1.2 / 50
#define THRESHOLD_STOP_LOW_MIN			72

#define MAX_DATA_BYTE_COUNT		4
#define MAX_DATA_BIT_COUNT		(MAX_DATA_BYTE_COUNT*8)

uint8_t ch_data[][MAX_DATA_BYTE_COUNT]=
{
	{0x01,0x00,0x00,0x01},	// toggle
	{0x01,0x00,0x00,0x02},	// on
	{0x01,0x00,0x00,0x03},	// off
};
/*
{
	{0x02,0x00,0x00,0x01},	// toggle
	{0x02,0x00,0x00,0x02},	// on
	{0x02,0x00,0x00,0x03},	// off
};
*/
/*
{
	{0x03,0x00,0x00,0x01},	// toggle
	{0x03,0x00,0x00,0x02},	// on
	{0x03,0x00,0x00,0x03},	// off
};
*/

typedef enum
{
	MODE_CHECK_HEADER = 0,
	MODE_ANALYZE,
	MODE_RECEIVED,
	MODE_REPEAT,
	MODE_REPEAT_RECEIVED,
	MODE_OVERFLOW = 0x10,
} modeStatus;



int main(void)
{
	DDRB &= ~_BV(SW1);			// pinmode input
	sbi(PORTB,SW1);				// pull up
	DDRB &= ~_BV(IR_SENSOR1);	// pinmode input
	
	DDRB |= _BV(LED1);			// pinmode output
	cbi(PORTB,LED1);
	DDRB |= _BV(FET_DRV);		// pinmode output
	cbi(PORTB,FET_DRV);
	DDRB |= _BV(APOC_DRV);		// pinmode output
	cbi(PORTB,APOC_DRV);
	
	uint8_t data[MAX_DATA_BYTE_COUNT];
	uint8_t mode;
	uint8_t timeHigh,timeLow;
	uint8_t fBit;
	uint8_t byteCount;
	uint8_t bitCount=0;
	uint8_t i;
	uint16_t count = 0;

	for(i=0;i<3;i++)
	{
		sbi(PORTB,LED1);
		_delay_ms(150);
		cbi(PORTB,LED1);
		_delay_ms(150);
	}

	while(1)
	{
		while((PINB&_BV(IR_SENSOR1)))
		{
			if(count<300)
			{
				sbi(PORTB,APOC_DRV);
			}
			else if(count>8000)
			{
				count=0;
			}
			else
			{
				cbi(PORTB,APOC_DRV);
			}
			for(i=0;i<20;i++)
			{
				_delay_us(50);	// 50us * 20 = 1000us = 1ms
				if(!(PINB&_BV(IR_SENSOR1)))	break;
				
				if(!(PINB & _BV(SW1)))
				{
					_delay_ms(10);
					if(!(PINB & _BV(SW1)))
					{
						PORTB^=_BV(FET_DRV);
						if(PORTB&_BV(FET_DRV))
						{
							sbi(PORTB, LED1);
						}
						else
						{
							cbi(PORTB, LED1);
						}

						while(!(PINB & _BV(SW1)));	// スイッチを押し続けるとUSB電源が切れる可能性が有る
					}
				}
			}
			count++;
		}
		cbi(PORTB,APOC_DRV);
			
		mode=MODE_CHECK_HEADER;
		
		while(1)
		{
			timeHigh=0;
			timeLow=0;
			
			while(!(PINB&_BV(IR_SENSOR1)))
			{
				sbi(PORTB, LED1);
				timeHigh++;
				if(timeHigh==0)
				{
					timeHigh=0xFF;
					while(!(PINB&_BV(IR_SENSOR1)));
					break;
				}
				_delay_us(50);
			}
			while((PINB&_BV(IR_SENSOR1)))
			{
				cbi(PORTB, LED1);
				timeLow++;
				if(timeLow==0)
				{
					timeLow=0xFF;
					timeHigh=0xFF;
					break;
				}
				_delay_us(50);
			}

			if(mode==MODE_CHECK_HEADER)
			{
				if(timeHigh>=THRESHOLD_HEADER_HIGH_MIN && timeHigh<=THRESHOLD_HEADER_HIGH_MAX)
				{
					if(timeLow>=THRESHOLD_HEADER_LOW_MIN && timeLow<=THRESHOLD_HEADER_LOW_MAX)
					{
						mode=MODE_ANALYZE;
						fBit=0x80;
						byteCount=0;
						bitCount=0;
						for(i=0;i<MAX_DATA_BYTE_COUNT;i++)	data[i]=0;
					}
					else if(timeLow>=THRESHOLD_HEADER_REP_LOW_MIN && timeLow<=THRESHOLD_HEADER_REP_LOW_MAX)
					{
						mode=MODE_REPEAT;
					}
				}
			}
			else if(mode==MODE_ANALYZE)
			{
				if(timeLow>=THRESHOLD_STOP_LOW_MIN)
				{
					mode=MODE_RECEIVED;
				}
				else
				{
					if(timeLow>=(timeHigh+timeHigh))
					{
						data[byteCount]|=fBit;
					}
					fBit>>=1;
					if(fBit==0)
					{
						byteCount++;
						fBit=0x80;
						if(byteCount>=MAX_DATA_BYTE_COUNT)
						{
							mode=MODE_OVERFLOW;
						}
					}
					bitCount++;
				}
			}
			else if(mode==MODE_REPEAT)
			{
				if(timeLow>=THRESHOLD_STOP_LOW_MIN)
				{
					mode=MODE_REPEAT_RECEIVED;
				}
				else
				{
					mode=MODE_OVERFLOW;
				}
			}
			else if(mode==MODE_OVERFLOW)
			{
				if(timeLow>=THRESHOLD_STOP_LOW_MIN)
				{
					mode=MODE_RECEIVED;
				}
			}
			
			if(mode==MODE_RECEIVED)
			{
				if(bitCount==MAX_DATA_BIT_COUNT)
				{
					uint8_t ch;
					for(ch=0;ch<3;ch++)
					{
						for(i=0;i<MAX_DATA_BYTE_COUNT;i++)
						{
							if(ch_data[ch][i]!=data[i])
							{
								break;
							}
						}
						if(i==MAX_DATA_BYTE_COUNT)
						{
							if(ch==0)
							{
								PORTB^=_BV(FET_DRV);
							}
							else if(ch==1)
							{
								sbi(PORTB, FET_DRV);
							}
							else if(ch==2)
							{
								cbi(PORTB, FET_DRV);
							}
							break;
						}
					}
				}
				break;
			}
			else if(mode==MODE_REPEAT_RECEIVED)
			{
				if(bitCount!=0)
				{
					// リピートコード受信時の処理
				}
				break;
			}
			else if(mode==MODE_CHECK_HEADER)
			{
				bitCount=0;
				break;
			}
		}
		
		if(PORTB&_BV(FET_DRV))
		{
			sbi(PORTB, LED1);
		}
		else
		{
			cbi(PORTB, LED1);
		}

		
		while(!(PINB&_BV(IR_SENSOR1)));
	}
	
}

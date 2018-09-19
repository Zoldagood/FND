/*
 * Ex_UltrasonicWave.c
 *
 * Created: 2018-09-14 오전 10:15:04
 * Author : kccistc
 */ 
#define F_CPU 16000000L
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include "UART0.h"
#include "I2C_LCD.h"


FILE OUTPUT = FDEV_SETUP_STREAM(UART0_transmit, NULL, _FDEV_SETUP_WRITE);
FILE INPUT = FDEV_SETUP_STREAM(NULL, UART0_receive, _FDEV_SETUP_READ);

#define PRESCALER 1024


void display_digit(uint8_t position, uint8_t number);
void displayNumber(int cnt);


volatile uint8_t timerCounter0 = 0;
volatile unsigned int counter= 0 ;
uint8_t distance;
char buff[20];

ISR(TIMER0_OVF_vect){
	if(timerCounter0 % 4 == 0)
		display_digit(0, counter%10);
	else if(timerCounter0 % 4 == 1)
		display_digit(1, counter%100/10);
	else if(timerCounter0 % 4 == 2)
		display_digit(2, counter%1000/100);
	else if(timerCounter0 % 4 == 3)
		display_digit(3, counter/1000);
	
	timerCounter0++;
	if(timerCounter0 >= 200)
	{
		
		timerCounter0 = 0;
	}
}

void SegmentInit(void){
	DDRC = 0xff;				// segment 제어핀 8개 출력 설정
	DDRF = 0xf0;				// 자릿수 선택 핀4개 출력 설정
	PORTF = 0x00;
}
void UWInit(void)
{
	DDRG |= (1 << PORTG4);			// 트리거핀 출력으로 설정1
	DDRG &= ~(1 << PORTG3);			// 에코핀 입력 설정0
	
}
void TimerInterruptInit(void)
{
	TCCR0 |= (1 << CS02);
	TIMSK |= (1 << TOIE0);
}
void Timer_init(void){
	// 16비트 타이머 1번 타이머/카운터 분주비를 1024로 설정
	TCCR1B |= (1 << CS12) | (1 << CS10);
}

uint8_t measure_distance(void)
{
	
	// 트리거 핀으로 펄스 출력
	PORTG &= ~(1 << PORTG4);			// low값 출력
	_delay_us(1);
	PORTG |= (1 << PORTG4);				// high 출력
	_delay_us(10);						// 10 마이크로초대기
	PORTG &= ~(1 << PORTG4);			// low값 출력
	
	// 에코핀이 high가 될때까지 대기
	TCNT1 = 0;
	while(!(PING & 0x08))
		if(TCNT1 > 65000) return 0;		// 장애물이 없는경우
	
	// 에코핀이 low가 될때까지 시간측정
	TCNT1 = 0;
	while(PING & 0x08)
	{
		if(TCNT1 > 650000)
		{
			TCNT1 = 0;
			break;
		}
	}
	
	// 에코핀의 펄스폭을 마이크로초 단위로 계산
	double pulse_width = 1000000.0 * TCNT1 * PRESCALER / F_CPU;
	return pulse_width / 58;			// 센티미터 단위 거리 반환
}



int main(void)
{

	stdout = &OUTPUT;
	stdin = &INPUT;
	SegmentInit();
	UWInit();
	UART0_init();
	I2C_LCD_init();
	Timer_init();			// 1번 타이머/카운터 활성화
	TimerInterruptInit();
	sei();
	
    while (1) 
    {
		distance = measure_distance();		// 거리측정

		printf("Distance : %d cm\r\n", distance);
		sprintf(buff,"Distance:%03d cm", distance);
		I2C_LCD_write_string_XY(1,0,buff);
		
		_delay_ms(500);
		
		counter = distance;
	
    }
	return 0;
}


void display_digit(uint8_t position, uint8_t number)
{
	// position : 출력할 자리(1~4)
	// number : 출력할 숫자(0~9)
	uint8_t numbers[] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x27, 0x7f, 0x67};
	uint8_t fndSelect[]= {0x80, 0x40, 0x20, 0x10};
	
	
	
	PORTF = PORTF | 0xf0;			// 4~7번핀만 사용하기위해 상위에 |연산하여 4~7만 1로 나머지는 건들지않겠다!
	PORTF &= ~fndSelect[position];
	PORTC = numbers[number];
}

void displayNumber(int cnt)
{
	display_digit(0,cnt%10);
	_delay_ms(1);
	display_digit(1,cnt%100/10);
	_delay_ms(1);
	display_digit(2,cnt%1000/100);
	_delay_ms(1);
	display_digit(3,cnt/1000);
	_delay_ms(1);
}
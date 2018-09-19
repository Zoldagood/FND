#include	<avr/io.h>
#include	<avr/interrupt.h>
#include	<stdio.h>
#include	"usart.h"

volatile unsigned char rx1_buffer[LENGTH_RX_BUFFER], tx1_buffer[LENGTH_TX_BUFFER];
volatile unsigned char rx1_head=0, rx1_tail=0, tx1_head=0, tx1_tail=0;

// 인터럽트 USART 초기화
void USART1_init(unsigned int ubrr_baud)
{
	UCSR1B |= 1<<RXEN1 | 1<<TXEN1 | 1<<RXCIE1;		// RXCIE0 입력이 들어오면 인터럽트 발생을 허용한다.
	UBRR1H = 0;
	UBRR1L = ubrr_baud;
}

// 인터럽트에 의한 문자 전송 호출
int USART1_send(char data)
{
	// txbuffer[] full, 한 개라도 빌 때까지 기다림
	while( (tx1_head+1==tx1_tail) || ((tx1_head==LENGTH_TX_BUFFER-1) && (tx1_tail==0)) );
	
	tx1_buffer[tx1_head] = data;
	tx1_head = (tx1_head==LENGTH_TX_BUFFER-1) ? 0 : tx1_head+1;
	UCSR1B = UCSR1B | 1<<UDRIE1;	// 보낼 문자가 있으므로 UDRE1 빔 인터럽트 활성화

	return data;
}

// 인터럽트에 의한 문자 수신 호출
int USART1_receive()
{	unsigned char data;
	
	while( rx1_head==rx1_tail );	// 수신 문자가 없음 tail이 앞서가지 못하게 방지

	data = rx1_buffer[rx1_tail];
	rx1_tail = (rx1_tail==LENGTH_RX_BUFFER-1) ? 0 : rx1_tail + 1;			// tail과 LENGTH_TX_BUFFER-1이 같으면 0으로 아니면 tail+1
	
	return data;
}

// USART1 UDR empty interrupt service
ISR(USART1_UDRE_vect)
{
	UDR1 = tx1_buffer[tx1_tail];
	tx1_tail = (tx1_tail==LENGTH_TX_BUFFER-1) ? 0 : tx1_tail+1;			// tail과 LENGTH_TX_BUFFER-1이 같으면 0으로 아니면 tail+1
	//							7
	if( tx1_tail==tx1_head)		// 보낼 문자가 없으면 UDRE1 빔 인터럽트 비활성화
	UCSR1B = UCSR1B & ~(1<<UDRIE1);
}

// USART1 RXC interrupt service
ISR(USART1_RX_vect)
{
	volatile unsigned char data;
	
	// rx_buffer[] full, 마지막 수신 문자 버림
	if( (rx1_head+1==rx1_tail) || ((rx1_head==LENGTH_RX_BUFFER-1) && (rx1_tail==0)) ){
		data = UDR1;		// 버리기위해서 data에 udr0
		}else{
		rx1_buffer[rx1_head] = UDR1;		//  buffer의 rx_head의 위치로 데이터입력
		rx1_head = (rx1_head==LENGTH_RX_BUFFER-1) ? 0 : rx1_head+1;			// LENGTH_RX_BUFFER-1은 인덱스가 0부터 시작해서-1/ rx_head+1해서 헤드의 위치 증가
	}
}

// USART1 receive char check
int	USART1_rx_check(void)
{
	return (rx1_head != rx1_tail) ? 1 : 0;
}


void USART1_print_string(char *str){
	for(int i=0; str[i]; i++)
	{
		USART1_send(str[i]);
	}
}
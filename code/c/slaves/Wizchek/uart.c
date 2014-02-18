
//----------------------------------------------------------------------------------------
// USART
// Library functions adapted from Peter Fleury
//----------------------------------------------------------------------------------------

/** Size of the circular receive buffer, must be power of 2 */
#ifndef UART_RX_BUFFER_SIZE
#define UART_RX_BUFFER_SIZE 64
#endif

/* test if the size of the circular buffers fits into SRAM */
#if ( (UART_RX_BUFFER_SIZE) >= (RAMEND-0x60 ) )
#error "size of UART_RX_BUFFER_SIZE larger than size of SRAM"
#endif

/* 
** high byte error return code of uart_getc()
*/
#define UART_FRAME_ERROR      ((1 << FE0) << 8)              /* Framing Error by UART       */
#define UART_OVERRUN_ERROR    ((1 << DOR0) << 8)              /* Overrun condition by UART   */
#define UART_BUFFER_OVERFLOW  0x0200              /* receive ringbuffer overflow */
#define UART_NO_DATA          0x0100              /* no receive data available   */

#define UART_RX_BUFFER_MASK ( UART_RX_BUFFER_SIZE - 1)

#if ( UART_RX_BUFFER_SIZE & UART_RX_BUFFER_MASK )
#error RX buffer size is not a power of 2
#endif

#define UART0_RECEIVE_INTERRUPT   SIG_USART_RECV
#define ATMEGA_USART0
#define UART0_STATUS   UCSR0A
#define UART0_CONTROL  UCSR0B
#define UART0_DATA     UDR0
#define UART0_UDRIE    UDRIE0

static volatile unsigned char UART_RxBuf[UART_RX_BUFFER_SIZE];
static volatile unsigned char UART_RxHead;
static volatile unsigned char UART_RxTail;
static volatile unsigned char UART_LastRxError;

/*************************************************************************
Function: UART Receive Complete interrupt
Purpose:  called when the UART has received a character
**************************************************************************/
ISR (UART0_RECEIVE_INTERRUPT) 
{
    unsigned char tmphead;
    unsigned char data;
    unsigned char usr;
    unsigned char lastRxError;
  
    /* read UART status register and UART data register */ 
    usr  = UART0_STATUS;
    data = UART0_DATA;
    
    /* */
#if defined( AT90_UART )
    lastRxError = (usr & (_BV(FE)|_BV(DOR)) );
#elif defined( ATMEGA_USART )
    lastRxError = (usr & (_BV(FE)|_BV(DOR)) );
#elif defined( ATMEGA_USART0 )
    lastRxError = (usr & (_BV(FE0)|_BV(DOR0)) );
#elif defined ( ATMEGA_UART )
    lastRxError = (usr & (_BV(FE)|_BV(DOR)) );
#endif
        
    /* calculate buffer index */ 
    tmphead = ( UART_RxHead + 1) & UART_RX_BUFFER_MASK;
    
    if ( tmphead == UART_RxTail ) {
        /* error: receive buffer overflow */
        lastRxError = UART_BUFFER_OVERFLOW >> 8;
    }else{
        /* store new index */
        UART_RxHead = tmphead;
        /* store received data in buffer */
        UART_RxBuf[tmphead] = data;
    }
    UART_LastRxError = lastRxError;
}

//----------------------------------------------------------------------------------------
// Initialisierung USART
// 
//----------------------------------------------------------------------------------------
void Uart_Init(void)
{
    UART_RxHead = 0;
    UART_RxTail = 0;

	UBRR0H = ((F_CPU + BAUDRATE * 8L) / 16 / BAUDRATE - 1) >> 8;
	UBRR0L = ((F_CPU + BAUDRATE * 8L) / 16 / BAUDRATE - 1) & 0xFF;

	UART0_STATUS = 0x00;
	UART0_CONTROL = (1 << RXCIE0) | (1 << RXEN0);
	UCSR0C = 0x06;		// 8 bits
}

/*************************************************************************
Function: uart_getc()
Purpose:  return byte from ringbuffer  
Returns:  lower byte:  received byte from ringbuffer
          higher byte: last receive error
**************************************************************************/
unsigned int uart_getc(void)
{    
    unsigned char tmptail;
    unsigned char data;

    if ( UART_RxHead == UART_RxTail ) {
        return UART_NO_DATA;   /* no data available */
    }
    
    /* calculate /store buffer index */
    tmptail = (UART_RxTail + 1) & UART_RX_BUFFER_MASK;
    UART_RxTail = tmptail; 
    
    /* get data from receive buffer */
    data = UART_RxBuf[tmptail];
    
    return (UART_LastRxError << 8) + data;

}/* uart_getc */



/*************************************************************************
Function: uart_putc()
Purpose:  bit banging TX
Input:    byte to be transmitted
Returns:  none          
**************************************************************************/
void uart_putc(unsigned char value)
{
#define CR_TAB "\n\t" 
#define STOP_BITS 1 

#define SWUART_PORT PORTC	//TX Port
#define SWUART_PIN	3		//TX Pin

	// implemented via bit-banging (Atmel Note AVR305)
   volatile uint8_t bitcnt = 1+8+STOP_BITS; 
   volatile uint8_t delay;

	// fixed baudrate: 9600
	delay = 135;

   volatile unsigned char val = ~value; 

   cli(); 

   __asm__ __volatile__( 
      "      sec             ; Start bit           " CR_TAB 

      "0:    brcc 1f              ; If carry set   " CR_TAB 
      "      cbi   %[port],%[pad] ; send a '0'     " CR_TAB 	// logisch 0 (TTL-Pegel)
      "      rjmp 2f              ; else           " CR_TAB 

      "1:    sbi   %[port],%[pad] ; send a '1'     " CR_TAB 	// logisch 1 (TTL-Pegel)
      "      nop"                                    CR_TAB 

      "2:    %~call bit_delay_%=  ; One bit delay  " CR_TAB 
      "      %~call bit_delay_%="                    CR_TAB 

      "      lsr   %[val]   ; Get next bit        " CR_TAB 
      "      dec   %[bitcnt] ; If not all bit sent " CR_TAB 
      "      brne 0b         ; send next           " CR_TAB 
      "      rjmp 5f         ; else: done          " CR_TAB 
                                                     CR_TAB 
      "bit_delay_%=:"                                CR_TAB 
      "      mov   __zero_reg__, %[delay]"           CR_TAB 
      "4:    dec   __zero_reg__"                     CR_TAB 
      "      brne 4b"                                CR_TAB 
      "      ret"                                    CR_TAB 
      "5:"                                           CR_TAB 

      : [bitcnt] "=r" (bitcnt), [val] "=r" (val) 
      : "1" (val), "0" (bitcnt), [delay] "r" (delay), [port] "M" (_SFR_IO_ADDR(SWUART_PORT)), [pad] "M" (SWUART_PIN) 
   ); 

   sei(); 
}/* uart_putc */


void uart_puts (char *s)
{
    while (*s)
    {
        uart_putc(*s);
        s++;
    }
}
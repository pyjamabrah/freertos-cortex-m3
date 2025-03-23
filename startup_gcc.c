/*
 * FreeRTOS V202212.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

/* FreeRTOS interrupt handlers. */
extern void vPortSVCHandler( void );
extern void xPortPendSVHandler( void );
extern void xPortSysTickHandler( void );
extern void TIMER0_Handler( void );
extern void TIMER1_Handler( void );

/* Exception handlers. */
static void HardFault_Handler( void ) __attribute__( ( naked ) );
static void Default_Handler( void ) __attribute__( ( naked ) );
void Reset_Handler( void ) __attribute__( ( naked ) );

extern int main( void );
extern uint32_t _estack;

/* Vector table. */
const uint32_t* isr_vector[] __attribute__((section(".isr_vector"), used)) =
{
    ( uint32_t * ) &_estack,
    ( uint32_t * ) &Reset_Handler,     // Reset                -15
    ( uint32_t * ) &Default_Handler,   // NMI_Handler          -14
    ( uint32_t * ) &HardFault_Handler, // HardFault_Handler    -13
    ( uint32_t * ) &Default_Handler,   // MemManage_Handler    -12
    ( uint32_t * ) &Default_Handler,   // BusFault_Handler     -11
    ( uint32_t * ) &Default_Handler,   // UsageFault_Handler   -10
    0, // reserved   -9
    0, // reserved   -8
    0, // reserved   -7
    0, // reserved   -6
    ( uint32_t * ) &vPortSVCHandler,    // SVC_Handler          -5
    ( uint32_t * ) &Default_Handler,    // DebugMon_Handler     -4
    0, // reserved   -3
    ( uint32_t * ) &xPortPendSVHandler, // PendSV handler       -2
    ( uint32_t * ) &xPortSysTickHandler,// SysTick_Handler      -1
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    ( uint32_t * ) TIMER0_Handler,     // Timer 0
    ( uint32_t * ) TIMER1_Handler,     // Timer 1
    0,
    0,
    0,
    0, // Ethernet   13
};

void Reset_Handler( void )
{
    main();
}

/* Variables used to store the value of registers at the time a hardfault
 * occurs.  These are volatile to try and prevent the compiler/linker optimizing
 * them away as the variables never actually get used. */
volatile uint32_t r0;
volatile uint32_t r1;
volatile uint32_t r2;
volatile uint32_t r3;
volatile uint32_t r12;
volatile uint32_t lr; /* Link register. */
volatile uint32_t pc; /* Program counter. */
volatile uint32_t psr;/* Program status register. */

/* Called from the hardfault handler to provide information on the processor
 * state at the time of the fault.
 */
__attribute__( ( used ) ) void prvGetRegistersFromStack( uint32_t *pulFaultStackAddress )
{
    r0 = pulFaultStackAddress[ 0 ];
    r1 = pulFaultStackAddress[ 1 ];
    r2 = pulFaultStackAddress[ 2 ];
    r3 = pulFaultStackAddress[ 3 ];

    r12 = pulFaultStackAddress[ 4 ];
    lr = pulFaultStackAddress[ 5 ];
    pc = pulFaultStackAddress[ 6 ];
    psr = pulFaultStackAddress[ 7 ];

    printf( "Calling prvGetRegistersFromStack() from fault handler" );
    fflush( stdout );

    /* When the following line is hit, the variables contain the register values. */
    for( ;; );
}


void Default_Handler( void )
{
    __asm volatile
    (
        ".align 8                                \n"
        " ldr r3, =0xe000ed04                    \n" /* Load the address of the interrupt control register into r3. */
        " ldr r2, [r3, #0]                       \n" /* Load the value of the interrupt control register into r2. */
        " uxtb r2, r2                            \n" /* The interrupt number is in the least significant byte - clear all other bits. */
        "Infinite_Loop:                          \n" /* Sit in an infinite loop - the number of the executing interrupt is held in r2. */
        " b  Infinite_Loop                       \n"
        " .ltorg                                 \n"
    );
}

void HardFault_Handler( void )
{
    __asm volatile
    (
        ".align 8                                                   \n"
        " tst lr, #4                                                \n"
        " ite eq                                                    \n"
        " mrseq r0, msp                                             \n"
        " mrsne r0, psp                                             \n"
        " ldr r1, [r0, #24]                                         \n"
        " ldr r2, =prvGetRegistersFromStack                         \n"
        " bx r2                                                     \n"
        " .ltorg                                                    \n"
    );
}


#define UART0_ADDRESS 	( 0x40004000UL )
#define UART0_DATA		( * ( ( ( volatile unsigned int * )( UART0_ADDRESS + 0UL ) ) ) )
#define putchar(c)      UART0_DATA = c

static int tiny_print( char **out, const char *format, va_list args, unsigned int buflen );

static void printchar(char **str, int c, char *buflimit)
{
	if (str) {
		if( buflimit == ( char * ) 0 ) {
			/* Limit of buffer not known, write charater to buffer. */
			**str = (char)c;
			++(*str);
		}
		else if( ( ( unsigned long ) *str ) < ( ( unsigned long ) buflimit ) ) {
			/* Within known limit of buffer, write character. */
			**str = (char)c;
			++(*str);
		}
	}
	else
	{
		putchar(c);
	}
}

#define PAD_RIGHT 1
#define PAD_ZERO 2

static int prints(char **out, const char *string, int width, int pad, char *buflimit)
{
	register int prt_ch = 0, padchar = ' ';

	if (width > 0) {
		register int len = 0;
		register const char *ptr;
		for (ptr = string; *ptr; ++ptr) ++len;
		if (len >= width) width = 0;
		else width -= len;
		if (pad & PAD_ZERO) padchar = '0';
	}
	if (!(pad & PAD_RIGHT)) {
		for ( ; width > 0; --width) {
			printchar (out, padchar, buflimit);
			++prt_ch;
		}
	}
	for ( ; *string ; ++string) {
		printchar (out, *string, buflimit);
		++prt_ch;
	}
	for ( ; width > 0; --width) {
		printchar (out, padchar, buflimit);
		++prt_ch;
	}

	return prt_ch;
}

/* the following should be enough for 32 bit int */
#define PRINT_BUF_LEN 12

static int printi(char **out, int i, int b, int sg, int width, int pad, int letbase, char *buflimit)
{
	char print_buf[PRINT_BUF_LEN];
	register char *s;
	register int t, neg = 0, prt_ch = 0;
	register unsigned int u = (unsigned int)i;

	if (i == 0) {
		print_buf[0] = '0';
		print_buf[1] = '\0';
		return prints (out, print_buf, width, pad, buflimit);
	}

	if (sg && b == 10 && i < 0) {
		neg = 1;
		u = (unsigned int)-i;
	}

	s = print_buf + PRINT_BUF_LEN-1;
	*s = '\0';

	while (u) {
		t = (unsigned int)u % b;
		if( t >= 10 )
			t += letbase - '0' - 10;
		*--s = (char)(t + '0');
		u /= b;
	}

	if (neg) {
		if( width && (pad & PAD_ZERO) ) {
			printchar (out, '-', buflimit);
			++prt_ch;
			--width;
		}
		else {
			*--s = '-';
		}
	}

	return prt_ch + prints (out, s, width, pad, buflimit);
}

static int tiny_print( char **out, const char *format, va_list args, unsigned int buflen )
{
	register int width, pad;
	register int prt_ch = 0;
	char scr[2], *buflimit;

	if( buflen == 0 ){
		buflimit = ( char * ) 0;
	}
	else {
		/* Calculate the last valid buffer space, leaving space for the NULL
		terminator. */
		buflimit = ( *out ) + ( buflen - 1 );
	}

	for (; *format != 0; ++format) {
		if (*format == '%') {
			++format;
			width = pad = 0;
			if (*format == '\0') break;
			if (*format == '%') goto out;
			if (*format == '-') {
				++format;
				pad = PAD_RIGHT;
			}
			while (*format == '0') {
				++format;
				pad |= PAD_ZERO;
			}
			for ( ; *format >= '0' && *format <= '9'; ++format) {
				width *= 10;
				width += *format - '0';
			}
			if( *format == 's' ) {
				register char *s = (char *)va_arg( args, int );
				prt_ch += prints (out, s?s:"(null)", width, pad, buflimit);
				continue;
			}
			if( *format == 'd' ) {
				prt_ch += printi (out, va_arg( args, int ), 10, 1, width, pad, 'a', buflimit);
				continue;
			}
			if( *format == 'x' ) {
				prt_ch += printi (out, va_arg( args, int ), 16, 0, width, pad, 'a', buflimit);
				continue;
			}
			if( *format == 'X' ) {
				prt_ch += printi (out, va_arg( args, int ), 16, 0, width, pad, 'A', buflimit);
				continue;
			}
			if( *format == 'u' ) {
				prt_ch += printi (out, va_arg( args, int ), 10, 0, width, pad, 'a', buflimit);
				continue;
			}
			if( *format == 'c' ) {
				/* char are converted to int then pushed on the stack */
				scr[0] = (char)va_arg( args, int );
				scr[1] = '\0';
				prt_ch += prints (out, scr, width, pad, buflimit);
				continue;
			}
		}
		else {
		out:
			printchar (out, *format, buflimit);
			++prt_ch;
		}
	}
	if (out) **out = '\0';
	va_end( args );
	return prt_ch;
}

int printf(const char *format, ...)
{
        va_list args;

        va_start( args, format );
        return tiny_print( 0, format, args, 0 );
}

int sprintf(char *out, const char *format, ...)
{
        va_list args;

        va_start( args, format );
        return tiny_print( &out, format, args, 0 );
}


int snprintf( char *buf, unsigned int count, const char *format, ... )
{
        va_list args;

        ( void ) count;

        va_start( args, format );
        return tiny_print( &buf, format, args, count );
}

/* To keep linker happy. */
int	write( int i, char* c, int n)
{
	(void)i;
	(void)n;
	(void)c;
	return 0;
}



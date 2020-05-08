#include <stdint.h>
#include <stdbool.h>
#include "Lab9_UART.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"				// This and below manually added
#include "inc/tm4c123gh6pm.h"
#include "driverlib/uart.h"
#include <string.h>
#include <stdio.h>
#include <utils/uartstdio.h>

//*****************************************************************************
volatile unsigned long discount= 0 ;

volatile uint32_t letter;
volatile unsigned long count= 0 ;
void
PortFunctionInit(void)	//ENABLE LEDS
{
    //
    // Enable Peripheral Clocks 
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

    //
    // Enable pin PF3 for GPIOOutput (green)
    //
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_3);

    //
    // Enable pin PF2 for GPIOOutput (blue)
    //
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_2);

    //
    // Enable pin PF1 for GPIOOutput (red)
    //
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1); 
}

void UART_Init(void)
{
	// Enable peripheral clocks for UART0 
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	
	//
  // Enable pin PA0 & PA1 for UART0 U0RX
  //
  GPIOPinConfigure(GPIO_PA0_U0RX);
  GPIOPinConfigure(GPIO_PA1_U0TX);
  GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	
	// Configure UART 115200 baud rate, clk freq, 8 bit word leng, 1 stop bit, & no parity.
	UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 115200,
        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
	
	// Disable FIFO (First In First Out) registers. 
	UARTFIFODisable(UART0_BASE);
	
	//enable the UART interrupt
	IntEnable(INT_UART0); 
	
	//only enable RX and TX interrupts
  UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_TX); 
}

// baud rate is bits/sec. Bit time is reciprocal of baud rate. bandwidth is amnt of useful info transmitted/sec


void UARTIntHandler(void)
{
	uint32_t count;
	
	// get interrupt status
	count = UARTIntStatus(UART0_BASE, true);
	
	// clear the asserted interrupts
	UARTIntClear(UART0_BASE, count);
	count++;
	while(UARTCharsAvail(UART0_BASE)) //loop while there are chars
  {
		letter = UARTCharGet(UART0_BASE);
	
		//echo character (reads from receive register and sends to transmit registers)
    UARTCharPutNonBlocking(UART0_BASE, UARTCharGetNonBlocking(UART0_BASE)); 
	
		// go through letters RrBbGg, default is LED off and invalid option

		}
  }






int main(void)
{
	// Create 50MHz system clock frequency
	SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
	
	// initialize GPIO F ports
	PortFunctionInit();
	
	// Call UART0A interrupt
	UART_Init();
	
	// globally enable interrupt, enable processor interrupt
	IntMasterEnable();
	
	//\n\r
	char str[]="Welcome\n\r";

for (int i = 0; i<(sizeof(str)); i++)
	{	
		UARTCharPut(UART0_BASE, str[i]);
	}
//	
	
	while (1) //let interrupt handler do the UART echo function
	{
	// print Initial prompt message
		if (count&0x03)==0x03 & ((discount&0x1)==0x01)
			UARTCharPut(UART0_BASE, '\n');
			UARTCharPut(UART0_BASE, '\r');
				char str[]="Regular:3/3\n\r Disabled:1/1";
						for (int i = 0; i<(sizeof(str)); i++)
	{	
		UARTCharPut(UART0_BASE, str[i]);
	}
			break;
				if (count&0x02)==0x02 & ((discount&0x1)==0x01)
			UARTCharPut(UART0_BASE, '\n');
			UARTCharPut(UART0_BASE, '\r');
				char str0[]="Regular:2/3\n\r Disabled:1/1";
						for (int i = 0; i<(sizeof(str0)); i++)
	{	
		UARTCharPut(UART0_BASE, str0[i]);
	}
			break;
				if (count&0x01)==0x01 & ((discount&0x1)==0x01)
			UARTCharPut(UART0_BASE, '\n');
			UARTCharPut(UART0_BASE, '\r');
				char str1[]="Regular:1/3\n\r Disabled:1/1";
						for (int i = 0; i<(sizeof(str1)); i++)
	{	
		UARTCharPut(UART0_BASE, str1[i]);
	}
			break;
				if (count&0x00)==0x00 & ((discount&0x1)==0x01)
			UARTCharPut(UART0_BASE, '\n');
			UARTCharPut(UART0_BASE, '\r');
				char str2[]="Regular:0/3\n\r Disabled:1/1";
						for (int i = 0; i<(sizeof(str2)); i++)
	{	
		UARTCharPut(UART0_BASE, str2[i]);
	}
			break;
				if (count&0x03)==0x03 & ((discount&0x1)==0x00)
			UARTCharPut(UART0_BASE, '\n');
			UARTCharPut(UART0_BASE, '\r');
				char str3[]="Regular:3/3\n\r Disabled:0/1";
						for (int i = 0; i<(sizeof(str3)); i++)
	{	
		UARTCharPut(UART0_BASE, str3[i]);
	}
			break;
				if (count&0x02)==0x02 & ((discount&0x1)==0x00)
			UARTCharPut(UART0_BASE, '\n');
			UARTCharPut(UART0_BASE, '\r');
				char str4[]="Regular:2/3\n\r Disabled:0/1";
						for (int i = 0; i<(sizeof(str4)); i++)
	{	
		UARTCharPut(UART0_BASE, str4[i]);
	}
			break;
			if (count&0x01)==0x01 & ((discount&0x1)==0x00)
			UARTCharPut(UART0_BASE, '\n');
			UARTCharPut(UART0_BASE, '\r');
				char str5[]="Regular:1/3\n\r Disabled:0/1";
						for (int i = 0; i<(sizeof(str5)); i++)
	{	
		UARTCharPut(UART0_BASE, str5[i]);
	}
			break;
			if (count&0x00)==0x00 & ((discount&0x1)==0x00)
			UARTCharPut(UART0_BASE, '\n');
			UARTCharPut(UART0_BASE, '\r');
				char str6[]="Regular:0/3\n\r Disabled:0/1";
						for (int i = 0; i<(sizeof(str6)); i++)
	{	
		UARTCharPut(UART0_BASE, str6[i]);
	}
			break;
	
	}
}

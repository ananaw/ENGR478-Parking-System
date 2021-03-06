//***********************************************************
// Hardware PWM proof of concept using
// the Tiva C Launchpad
//
// If sensors detect object, on/off red. If object leaving, on/off blue.
// For disabled, turn on green LED if car is there, else turn green off.
// Display message on Tera Term with UART
// 
// by Ananda Aw, CanXian Su, YunJie Li
//
// This drives servo motors 90 degrees to left on PWM PB6, PB7 when 
// ultrasonic sensor senses object around 4.5cm away.
//***********************************************************

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>		//
#include <string.h>
#include "inc/hw_gpio.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_timer.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"  
#include "driverlib/pwm.h"
#include "driverlib/uart.h"
#include "driverlib/timer.h"					// manually added
#include "driverlib/interrupt.h" 			// manually added
#include "inc/tm4c123gh6pm.h"					// manually added
//***********************************************************
volatile bool trigok0 = 1;			// sensor 1
volatile bool trigok1 = 1;			// sensor 2
volatile bool trigok2 = 1;			// sensor 3
volatile bool trigok3 = 1;			// sensor 4
volatile bool trigok4 = 1;			// sensor 5
volatile uint32_t cars_count = 0;
volatile uint32_t Disabled_count = 0;
volatile bool object1 = 0;			// flag to see if object present or not
volatile bool object2 = 0;
volatile bool object3 = 0;
volatile float duration0;
volatile float distance0;
volatile float duration1;
volatile float distance1;
volatile float duration2;
volatile float distance2;
volatile float duration3;
volatile float distance3;
volatile float duration4;
volatile float distance4;

void PortFunctionInit(void)
{
	// Enable clock for Port F peripheral
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	// Enable pin PF1,2,3 for GPIOOutput (green) 
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_3|GPIO_PIN_1|GPIO_PIN_2);
	// Enable pin PF4 for GPIOInput SW1
	GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_4);
	//Enable pull-up on PF4 
	GPIO_PORTF_PUR_R |= 0x10; 
	
	// Enable clock for Port D peripheral
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	// Enable pin PD0,2 for GPIOOutput Trig
	GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, GPIO_PIN_0|GPIO_PIN_2);
	// Enable pin PD1,3 for GPIOInput  Echo
	GPIOPinTypeGPIOInput(GPIO_PORTD_BASE, GPIO_PIN_1|GPIO_PIN_3);
	// Set output 2mA current strength, push-pull pin *STD, and weak pull down *WPD.
	GPIOPadConfigSet(GPIO_PORTD_BASE, GPIO_PIN_1|GPIO_PIN_3, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPD);
	
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);            		// enable GPIO port B
	GPIOPinConfigure(GPIO_PB6_T0CCP0);					// enable timer0A mode
	GPIOPinConfigure(GPIO_PB4_T1CCP0);					// enable timer1A mode
	GPIOPinConfigure(GPIO_PB0_T2CCP0);					// enable timer2A mode
	GPIOPinConfigure(GPIO_PB2_T3CCP0);					// enable timer3A mode
	GPIOPinTypeTimer(GPIO_PORTB_BASE, GPIO_PIN_0|GPIO_PIN_2|GPIO_PIN_4|GPIO_PIN_6);		// configure the GPIO pins for timer
	
	// Enable peripheral clocks for UART0 
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	
	// Enable pin PA2,4,6 for Output (Trig)
	GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE, GPIO_PIN_2|GPIO_PIN_4|GPIO_PIN_6);
	// Enable pin PA3,5,7 for Input	 (Echo)
	GPIOPinTypeGPIOInput(GPIO_PORTA_BASE, GPIO_PIN_3|GPIO_PIN_5|GPIO_PIN_7);
	GPIOPadConfigSet(GPIO_PORTA_BASE, GPIO_PIN_3|GPIO_PIN_5|GPIO_PIN_7, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPD);
	//
  // Enable pin PA0 & PA1 for UART0 U0RX
  //
  GPIOPinConfigure(GPIO_PA0_U0RX);
  GPIOPinConfigure(GPIO_PA1_U0TX);
  GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	
	// Configure UART 115200 baud rate, clk freq, 8 bit word leng, 1 stop bit, & no parity.
	UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 115200,
        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
	
	//UART1_IBRD_R = 27;     // IBRD = int(50,000,000/(16*115,200)) = int(27.12674)
	//UART1_FBRD_R = 8;     // FBRD = round(0.12674 * 64) = 8
	//UART1_LCRH_R = 0x00000060;  // 8 bit, no parity bits, one stop, no FIFOs
	
	// Disable FIFO (First In First Out) registers. 
	//UARTFIFODisable(UART0_BASE);
}

void
ConfigurePWM(uint32_t period, uint32_t duty)
{
    //Configure PWM Clock divide system clock by 32
    SysCtlPWMClockSet(SYSCTL_PWMDIV_32);

    // Enable E peripheral used by this program.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);  //Tiva Launchpad has two modules (0 and 1). Module 0 covers the E peripheral
	
    //Configure PE4,PE5 Pins as PWM
    GPIOPinConfigure(GPIO_PE4_M0PWM4);
    GPIOPinConfigure(GPIO_PE5_M0PWM5);
    GPIOPinTypePWM(GPIO_PORTE_BASE, GPIO_PIN_4|GPIO_PIN_5);

    //Configure PWM Options
    //PWM_GEN_2 Covers M0PWM4 and M0PWM5 for PE4,5, with count down mode, no comparator update synchronization mode
    PWMGenConfigure(PWM0_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN|PWM_GEN_MODE_NO_SYNC); 

    //Set the Period (expressed in clock ticks)
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, period);

    //Set PWM duty for M0PWM4 and M0PWM5
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, duty);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_5, duty);

    // Enable the PWM generator
    PWMGenEnable(PWM0_BASE, PWM_GEN_2);

		// Enable PWM Output pins 4 & 5: M0PWM4 and M0PWM5
    PWMOutputState(PWM0_BASE, PWM_OUT_4_BIT|PWM_OUT_5_BIT, true);		
}

void Configure_Timers()		//unsigned long reload
{
	//Timer0A count
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);			// enable peripheral clock for Timer0A
	TimerDisable(TIMER0_BASE, TIMER_A);								// disable timer during setup
	TimerConfigure(TIMER0_BASE, TIMER_CFG_A_PERIODIC_UP);	// configure for one shot up count mode													
	TimerEnable(TIMER0_BASE, TIMER_A);								// Enable Timer0A
	
	// Timer1A count
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);			// enable peripheral clock for Timer1A
	TimerDisable(TIMER1_BASE, TIMER_A);								// disable timer during setup
	TimerConfigure(TIMER1_BASE, TIMER_CFG_A_PERIODIC_UP);	// configure for one shot up count mode 
	TimerEnable(TIMER1_BASE, TIMER_A);								// Enable Timer1A

	// Timer2A count
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER2);			// enable peripheral clock for Timer2A
	TimerDisable(TIMER2_BASE, TIMER_A);								// disable timer during setup
	TimerConfigure(TIMER2_BASE, TIMER_CFG_A_PERIODIC_UP);	// configure for one shot up count mode 
	TimerEnable(TIMER2_BASE, TIMER_A);								// Enable Timer2A
	
	// Timer3A count
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER3);			// enable peripheral clock for Timer3A
	TimerDisable(TIMER3_BASE, TIMER_A);								// disable timer during setup
	TimerConfigure(TIMER3_BASE, TIMER_CFG_A_PERIODIC_UP);	// configure for one shot up count mode

	TimerEnable(TIMER3_BASE, TIMER_A);								// Enable Timer3A

	// Timer4A count
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER4);			// enable peripheral clock for Timer2A
	TimerDisable(TIMER4_BASE, TIMER_A);								// disable timer during setup
	TimerConfigure(TIMER4_BASE, TIMER_CFG_A_ONE_SHOT_UP);	// configure for one shot up count mode
	TimerEnable(TIMER4_BASE, TIMER_A);								// Enable Timer4A
}

void InterruptF_Init(void)
{
	IntEnable(INT_GPIOF);								                          // Enable interrupt 30 in NVIC, Code: NVIC_EN0_R |= 0x40000000;
	IntPrioritySet(INT_GPIOF, 0x00);		                          // Set interrupt priority 0 bits 23-21:010, Code: NVIC_PRI7_R &= ~0x00E00000
	GPIOIntEnable(GPIO_PORTF_BASE, GPIO_INT_PIN_1|GPIO_INT_PIN_2|GPIO_INT_PIN_3|GPIO_INT_PIN_4);//GPIO_PORTF_IM_R; arm interrupt on PF1-4 (Echo)
	GPIOIntTypeSet(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2, GPIO_RISING_EDGE);		// PF1 & 2 rising edged interrupt
	GPIOIntTypeSet(GPIO_PORTF_BASE, GPIO_PIN_3, GPIO_BOTH_EDGES);								// PF3 Both edged interrupt
	GPIOIntTypeSet(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_FALLING_EDGE);							// PF4 falling edged interrupt
	//GPIO_PORTF_IS_R;	 edge sensitive
	//GPIO_PORTF_IBE_R;  both edge triggered, depends on IEV register
	//GPIO_PORTF_IEV_R; rising-edged/falling edged event
}

void InterruptA_Init(void)
{
	IntEnable(INT_GPIOA);								                          // Enable interrupt 
	IntPrioritySet(INT_GPIOA, 0x20);		                          // Set interrupt priority 1 
	GPIOIntEnable(GPIO_PORTA_BASE, GPIO_INT_PIN_3|GPIO_INT_PIN_5|GPIO_INT_PIN_7);		//arm interrupt on PA3,5,7 (Echo)
	GPIOIntTypeSet(GPIO_PORTA_BASE, GPIO_PIN_3|GPIO_PIN_5|GPIO_PIN_7, GPIO_BOTH_EDGES);		// interrupt on both edges
}

void InterruptD_Init(void)
{
	IntEnable(INT_GPIOD);								                          // Enable interrupt 
	IntPrioritySet(INT_GPIOD, 0x20);		                          // Set interrupt priority 1 
	GPIOIntEnable(GPIO_PORTD_BASE, GPIO_INT_PIN_1|GPIO_INT_PIN_3);								// arm interrupt on PD1,3 (Echo)
	GPIOIntTypeSet(GPIO_PORTD_BASE, GPIO_PIN_1|GPIO_PIN_3, GPIO_BOTH_EDGES);			// interrupt on both edges
}

void GPIOPortD_Handler(void)			// D interrupt to count cars
{
	// Disabled spot case
	if(GPIO_PORTD_RIS_R&0x02)																	// if PD1 has action 
	{	
		GPIOIntClear(GPIO_PORTD_BASE, GPIO_INT_PIN_1);					// Clear trigger flag echo pin
	
		// start count at rising edged
		if(GPIOPinRead(GPIO_PORTD_BASE, GPIO_PIN_1)== GPIO_PIN_1)		// if PD1 high
		{
        TIMER4_TAV_R = 0xFFFFFFFF;			// Initialize Timer4A with value 0, HWREG(TIMER4_BASE + TIMER_O_TAV) = 0;
        TimerEnable(TIMER4_BASE, TIMER_A);									// Enable Timer4 to start counting for when Echo Pin is High
		}
		// falling edge
		else
		{
			TimerDisable(TIMER4_BASE, TIMER_A);										// disable timer
			duration4 = TimerValueGet(TIMER4_BASE, TIMER_A);			// get value of timer
			distance4 = duration4/464;														//convert to distance in cm, *17000/SysCtlClockGet(); 
			
			// if car is at parking spot, distance less than 4cm
			if(distance4 < 4)
			{
				GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, GPIO_PIN_3);		// turn green LED on 
			}
			else
			{
				GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0);							// turn green LED off
			}
      trigok4 = 1;						// Set condition for Trigger Pulse
		}
	}
	
	if(GPIO_PORTD_RIS_R&0x08)				// if PD3 has action
	{
		// Clear flag echo pin PF3
		GPIOIntClear(GPIO_PORTD_BASE, GPIO_INT_PIN_3);
	
		// count at rising edged
		if(GPIOPinRead(GPIO_PORTD_BASE, GPIO_PIN_3)== GPIO_PIN_3)
		{
			// Initialize Timer0A with value 0, HWREG(TIMER0_BASE + TIMER_O_TAV) = 0;
      TIMER0_TAV_R = 0xFFFFFFFF;
      // Enable Timer0A to start measuring duration for which Echo Pin is High
      TimerEnable(TIMER0_BASE, TIMER_A);
		}
		// falling edge
		else
		{
			TimerDisable(TIMER0_BASE, TIMER_A);
			duration0 = TimerValueGet(TIMER0_BASE, TIMER_A);
			distance0 = duration0/464;			//*17000/SysCtlClockGet();
			
			// if car park not full and there's a car in front
			if(((cars_count!=3)||(Disabled_count!=1))&&(distance0 < 4))
			{
				PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, 650);				// Turn 90 deg to left
				SysCtlDelay((2*SysCtlClockGet()-2)/3 + 1);					// delay 2s
			}	
			else
			{
				PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, 375);				// Turn back to 0 deg
			}
			
			// Set condition for Trigger Pulse
      trigok0 = 1;
		}
	}
}

// interrupt for counting cars leaving
void GPIOPortA_Handler(void)
{
	if(GPIO_PORTA_RIS_R&0x08)																	// if PA3 has action 
	{	
		GPIOIntClear(GPIO_PORTA_BASE, GPIO_INT_PIN_3);					// Clear flag echo pin PA3
	
		// count while there's pulse from echo pin rising
		if(GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_3)== GPIO_PIN_3)		// if PA3 high
		{
        TIMER1_TAV_R = 0xFFFFFFFF; 													// Initialize Timer1A with value 0, HWREG(TIMER1_BASE + TIMER_O_TAV) = 0;
        TimerEnable(TIMER1_BASE, TIMER_A);									// Enable Timer1A to start counting for which Echo Pin is High
		}
		// falling edge
		else
		{
			duration1 = TimerValueGet(TIMER1_BASE, TIMER_A);			// get value of timer
			TimerDisable(TIMER1_BASE, TIMER_A);										// disable timer
			distance1 = duration1/464;														// convert to distance in cm, *17000/SysCtlClockGet();	
			
			// if there's no car and distance less than 4cm
			if((object1!=1)&&(distance1 < 4))
			{
				GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_PIN_1);		// turn on red LED
				SysCtlDelay((0.375*SysCtlClockGet()-2)/3 + 1);						// delay 0.375s
				GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0);							// turn off red LED
				object1 = 1;																							// set flag to indicate a car is present
			}
			
			// if car present and distance > 10cm 
			if((object1)&&(distance1>10))
			{
				GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);		// turn on blue LED
				SysCtlDelay((0.375*SysCtlClockGet()-2)/3 + 1);						// delay 0.375s
				GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);							// turn off blue LED
				object1 = 0;																							// clear flag to indicate a car is not present
			}
      trigok1 = 1;						// Set condition for Trigger Pulse
		}
	}
	
	if(GPIO_PORTA_RIS_R&0x20)																	// if PA5 has action 
	{	
		GPIOIntClear(GPIO_PORTA_BASE, GPIO_INT_PIN_5);					// Clear trigger flag echo pin
	
		// count while there's pulse from echo pin rising
		if(GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_5)== GPIO_PIN_5)		// if PA5 high
		{
        TIMER2_TAV_R = 0xFFFFFFFF;													// Initialize Timer2A with value 0, HWREG(TIMER2_BASE + TIMER_O_TAV) = 0;
        TimerEnable(TIMER2_BASE, TIMER_A);									// Enable Timer2A to start counting for which Echo Pin is High
		}
		// falling edge
		else
		{
			TimerDisable(TIMER2_BASE, TIMER_A);										// stop timer								
			duration2 = TimerValueGet(TIMER2_BASE, TIMER_A);			// get timer value
			distance2 = duration2/464;														// convert to distance in cm, *17000/SysCtlClockGet();
			
			// if car not present and distance less than 4cm
			if((object2!=1)&&(distance2<4))	
			{
				GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_PIN_1);		// turn on red LED
				SysCtlDelay((0.375*SysCtlClockGet()-2)/3 + 1);						// delay 0.375s
				GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0);							// turn off red LED
				object2 = 1;																							// Set flag to indicate car is present
			}
			
			// if car present and distance > 10cm
			if((object2)&&(distance2>10))
			{
				GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);		// turn on blue LED
				SysCtlDelay((0.375*SysCtlClockGet()-2)/3 + 1);						// delay 0.375s
				GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);							// turn off blue LED
				object2 = 0;																							// Clear flag to indicate car is not present
			}
      trigok2 = 1;						// Set condition for Trigger Pulse
		}
	}
	
	
	if(GPIO_PORTA_RIS_R&0x80)																	// if PA7 has action 
	{	
		GPIOIntClear(GPIO_PORTA_BASE, GPIO_INT_PIN_7);					// Clear trigger flag echo pin
	
		// count while there's pulse from echo pin rising
		if(GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_7)== GPIO_PIN_7)		// if PA7 high
		{
        TIMER3_TAV_R = 0xFFFFFFFF;													// Reset Timer3A with value 0, HWREG(TIMER3_BASE + TIMER_O_TAV) = 0;
        TimerEnable(TIMER3_BASE, TIMER_A);									// Enable Timer3A to start counting for which Echo Pin is High
		}
		// falling edge
		else
		{
			TimerDisable(TIMER3_BASE, TIMER_A);										// stop timer
			duration3 = TimerValueGet(TIMER3_BASE, TIMER_A);			// Get timer value
			distance3 = duration3/464;														// convert to distance in cm, *17000/SysCtlClockGet();
			
			// if car not present and distance < 4cm
			if((object3!=1)&&(distance3 < 4))
			{
				GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_PIN_1);		// turn on red LED
				SysCtlDelay((0.375*SysCtlClockGet()-2)/3 + 1);						// delay 0.375s
				GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0);							// turn off red LED
				object3 = 1;																							// Set flag to indicate car is present
			}
			
			// if car present and distance > 10cm (car leaving)
			if((object3)&&(distance3>10))						
			{
				GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);		// turn on blue LED
				SysCtlDelay((0.375*SysCtlClockGet()-2)/3 + 1);						// delay 0.375s
				GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);							// turn off blue LED
				object3 = 0;																							// clear flag to indicate car is not present
			}
      trigok3 = 1;						// Set condition for Trigger Pulse
		}
	}
}

void GPIOPortF_Handler(void)
{
	// If red LED on 
	if(GPIO_PORTF_RIS_R&0x02)
	{
		// clear flag for PF1
		GPIOIntClear(GPIO_PORTF_BASE, GPIO_INT_PIN_1);
		
		// increment at rising edge
		if(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_1)==GPIO_PIN_1)
		{
			cars_count++;
		}		
	}
	
	// Check RIS flag, if blue LED on 
	if(GPIO_PORTF_RIS_R&0x04)
	{
		// clear flag for PF2
		GPIOIntClear(GPIO_PORTF_BASE, GPIO_INT_PIN_2);
		
		// decrement at rising edge
		if(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_2)==GPIO_PIN_2)
		{
			cars_count--;
		}
	}
	
	if(GPIO_PORTF_RIS_R&0x08)
	{
		// clear flag for PF3
		GPIOIntClear(GPIO_PORTF_BASE, GPIO_INT_PIN_3);
		// increment at rising edge
		if(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_3)==GPIO_PIN_3)
		{
			Disabled_count++;
		}
		// decrement at falling edge
		else
		{
			Disabled_count--;
		}
	}
	
	if(GPIO_PORTF_RIS_R&0x10)
	{
		// Clear flag for PF4
		GPIOIntClear(GPIO_PORTF_BASE, GPIO_INT_PIN_4);
		// If SW1 is pressed
		if(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4)== 0)
		{
			PWMPulseWidthSet(PWM0_BASE, PWM_OUT_5, 650);			// Turn servo 90 deg left
			SysCtlDelay((2*SysCtlClockGet()-2)/3 + 1);				// Delay 2s
			PWMPulseWidthSet(PWM0_BASE, PWM_OUT_5, 375);			// Turn servo back to 0 deg
		}
	}
}

// print message for the number of cars
void printmessage(uint32_t cars_count, uint32_t Disabled_count)
{
	char cars = cars_count + '0';								// convert int to string with ASCII
	char disabled = Disabled_count + '0';
	char message2[] = "Regular Spot: ";
	for (int k = 0; k<sizeof(message2); k++)
	{
		UARTCharPut(UART0_BASE, message2[k]);
	}
	
	UARTCharPut(UART0_BASE, cars);
	char message3[] = "/3 Disabled Spot: ";
	for (int k = 0; k<sizeof(message3); k++)
	{
		UARTCharPut(UART0_BASE, message3[k]);
	}

	UARTCharPut(UART0_BASE, disabled);
	
	char message1[] = "/1    ";
	for (int k = 0; k<sizeof(message1); k++)	
	{
		UARTCharPut(UART0_BASE, message1[k]);				
	}
}

int main(void)
{
	// Set clock rate to 400M/2/25 = 8MHz. (or 40MHz, divide by 5)
	SysCtlClockSet(SYSCTL_SYSDIV_25|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);
	
	uint32_t period = 5000; 						//20ms (8MHz / 32pwm_divider / 50Hz)
	uint32_t duty = 375;     						//1.5ms pulse width (0 degrees), 0(1.5ms) mid, +90(~2.4ms) left, -90(0.5ms) right	
	
	// Initialize GPIOportF, D, B, & A
	PortFunctionInit();
	
	// Configure PE4,PE5 as PWM										
	ConfigurePWM(period, duty);
	
	// Configure timers0-3A
	Configure_Timers();
	
	// Run PortD interrupt
	InterruptD_Init();	
	
	// Run PortF interrupt
	InterruptF_Init();
	
	// Run PortC interrupt
	InterruptA_Init();
	
	// Globally enable interrupt
	IntMasterEnable();
	
//	char message0[] = "Welcome to Pi Parking Garage! All spots are empty!\n\r";
//				for (int n = 0; n<sizeof(message0); n++)
//				{
//					UARTCharPut(UART0_BASE, message0[n]);
//				}

	while(1)
  {
		if(trigok0)
		{			
			// Create 10us pulse
			GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_2, GPIO_PIN_2); // Set Trigger pin high
			SysCtlDelay(27);				 															 // Make 10us delay, 27, (10e-6*SysCtlClockGet()-2)/3 + 1
			GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_2, 0x00); 			 // Set Trigger pin low
			
			// Disable trig flag
			trigok0 = 0;
		}
		
		if(trigok1)
		{
			GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_2, GPIO_PIN_2); // Set Trigger pin high
			SysCtlDelay(27);				 															 // Make 10us delay, 27, (10e-6*SysCtlClockGet()-2)/3 + 1
			GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_2, 0x00); 			 // Set Trigger pin low
			
			// Disable trig flag
			trigok1 = 0;
		}
		
		if(trigok2)
		{
			GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_4, GPIO_PIN_4); // Set Trigger pin high
			SysCtlDelay(27);				 															 // Make 10us delay, 27, (10e-6*SysCtlClockGet()-2)/3 + 1
			GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_4, 0x00); 			 // Set Trigger pin low
			
			// Disable trig flag
			trigok2 = 0;
		}
		
		if(trigok3)
		{
			GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_6, GPIO_PIN_6); // Set Trigger pin high
			SysCtlDelay(27);				 															 // Make 10us delay, 27, (10e-6*SysCtlClockGet()-2)/3 + 1
			GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_6, 0x00); 			 // Set Trigger pin low
			
			// Disable trig flag
			trigok3 = 0;
		}
		
		if(trigok4)
		{
			GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_0, GPIO_PIN_0); // Set Trigger pin high
			SysCtlDelay(27);				 															 // Make 10us delay, 27, (10e-6*SysCtlClockGet()-2)/3 + 1
			GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_0, 0x00); 			 // Set Trigger pin low
			
			// Disable trig flag
			trigok4 = 0;
		}
		
		printmessage(cars_count, Disabled_count);
		
		// display full message when carpark is full. 
		if((cars_count==3)&&(Disabled_count==1))
		{
			char message4[] = "Carpark FULL!\n\r";
			for (int k = 0; k<sizeof(message4); k++)
			{
				UARTCharPut(UART0_BASE, message4[k]);
			}
		}
			
		else 
		{
			// if 1st regular spot taken only
			if((object1)&&(object2!=1)&&(object3!=1))
			{
				char message4[] = "2nd & 3rd regular spots open.\n\r";
				for (int k = 0; k<sizeof(message4); k++)
				{
					UARTCharPut(UART0_BASE, message4[k]);
				}
			}
			// if 2nd regular spot taken only
			else if((object1!=1)&&(object2)&&(object3!=1))
			{
				char message4[] = "1st & 3rd regular spots open.\n\r";
				for (int k = 0; k<sizeof(message4); k++)
				{
					UARTCharPut(UART0_BASE, message4[k]);
				}
			}
			// if 3rd regular spot taken only
			else if((object1!=1)&&(object2!=1)&&(object3))
			{
				char message4[] = "1st & 2nd regular spots open.\n\r";
				for (int k = 0; k<sizeof(message4); k++)
				{
					UARTCharPut(UART0_BASE, message4[k]);
				}
			}
			// if 1st & 2nd regular spots taken
			else if((object1)&&(object2)&&(object3!=1))
			{
				char message4[] = "3rd spot regular open.\n\r";
				for (int k = 0; k<sizeof(message4); k++)
				{
					UARTCharPut(UART0_BASE, message4[k]);
				}
				
			}
			// if 1st & 3rd regular spots taken
			else if((object1)&&(object2!=1)&&(object3))
			{
				char message4[] = "2nd spot regular open.\n\r";
				for (int k = 0; k<sizeof(message4); k++)
				{
					UARTCharPut(UART0_BASE, message4[k]);
				}
			}
			// if 2nd & 3rd regular spots taken
			else if((object1!=1)&&(object2)&&(object3))
			{
				char message4[] = "1st spot regular open.\n\r";
				for (int k = 0; k<sizeof(message4); k++)
				{
					UARTCharPut(UART0_BASE, message4[k]);
				}
			}
			// if all regular spots taken
			else if((object1)&&(object2)&&(object3))
			{
				char message4[] = "No regular spots open.\n\r";
				for (int k = 0; k<sizeof(message4); k++)
				{
					UARTCharPut(UART0_BASE, message4[k]);
				}
			}
			// if no regular spots taken
			else
			{
				char message4[] = "All regular spots open.\n\r";
				for (int k = 0; k<sizeof(message4); k++)
				{
					UARTCharPut(UART0_BASE, message4[k]);
				}
			}
		}
	
//		while(prev_count != cars_count)
//		{
//			char cars = cars_count + '0';
//			char disabled = Disabled_count + '0';
//				char message2[] = "Regular Spot: ";
//				for (int k = 0; k<sizeof(message2); k++)
//				{
//					UARTCharPut(UART0_BASE, message2[k]);
//				}
//				
//				UARTCharPut(UART0_BASE, cars);
//				char message3[] = "/3 Disabled Spot: ";
//				for (int k = 0; k<sizeof(message3); k++)
//				{
//					UARTCharPut(UART0_BASE, message3[k]);
//				}

//				UARTCharPut(UART0_BASE, disabled);
//				
//				char message1[] = "/1\n\r";
//				for (int k = 0; k<sizeof(message1); k++)
//				{
//					UARTCharPut(UART0_BASE, message1[k]);
//				}
//			}
  }
}

//***********************************************************
// Hardware PWM proof of concept using
// the Tiva C Launchpad
//
// 
//
//
// This drives servo motors 90 degrees to left on PWM PB6, PB7 when 
// ultrasonic sensor senses object around 4.5cm away.
//***********************************************************

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_gpio.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/pwm.h"
#include "driverlib/timer.h"					// manually added
#include "driverlib/interrupt.h" 			// manually added
#include "inc/tm4c123gh6pm.h"					// manually added
//***********************************************************
volatile uin32_t cars_count;
volatile uint32_t count;
//volatile uint32_t duration;
//volatile uint32_t distance;

void PortFunctionInit(void)
{
	// Enable clock for Port F peripheral
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	//
	// Enable pin PF3 for GPIOOutput (green) Trig
	//
	GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_3|GPIO_PIN_1);

	//
	// Enable pin PF2 for GPIOInput (blue) Echo
	//
	GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_2);
	
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0);
	
	// Set output 2mA current strength, push-pull pin *STD, and weak pull down *WPD.
	GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPD);
}

void
ConfigurePWM(uint32_t period, uint32_t duty)
{
    //Configure PWM Clock divide system clock by 32
    SysCtlPWMClockSet(SYSCTL_PWMDIV_32);

    // Enable the peripherals used by this program.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);  //Tiva Launchpad has two modules (0 and 1). Module 0 covers the B peripheral

    //Configure PB6,PB7 Pins as PWM
    GPIOPinConfigure(GPIO_PB6_M0PWM0);
    GPIOPinConfigure(GPIO_PB7_M0PWM1);
    GPIOPinTypePWM(GPIO_PORTB_BASE, GPIO_PIN_6|GPIO_PIN_7);

    //Configure PWM Options
    //PWM_GEN_0 Covers M0PWM0 and M0PWM1 for PB6,7, with count down mode, no comparator update synchronization mode
    PWMGenConfigure(PWM0_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN|PWM_GEN_MODE_NO_SYNC); 

    //Set the Period (expressed in clock ticks)
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, period);

    //Set PWM duty for M0PWM0 and M0PWM1
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, duty);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, duty);

    // Enable the PWM generator
    PWMGenEnable(PWM0_BASE, PWM_GEN_0);

		// Enable PWM Output pins 6 & 7: M0PWM0 and M0PWM1
    PWMOutputState(PWM0_BASE, PWM_OUT_0_BIT|PWM_OUT_1_BIT, true);		
}

void Timer0A_Init(unsigned long reload)
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);			// enable peripheral clock for Timer0A
	TimerDisable(TIMER0_BASE, TIMER_A);								// disable timer during setup
	TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);	// configure for periodic mode
	TimerLoadSet(TIMER0_BASE, TIMER_A, reload-1);			// reload value of 60ms
	IntPrioritySet(INT_TIMER0A, 0x20);								// Set priority as 1
	IntEnable(INT_TIMER0A);														// Enable interrupt 19 in NVIC
	TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);	// arm interrupt at timeout
	TimerEnable(TIMER0_BASE, TIMER_A);								// Enable Timer0A
}

void Interrupt_Init(void)
{
	IntEnable(INT_GPIOF);								                          // Enable interrupt 30 in NVIC, Code: NVIC_EN0_R |= 0x40000000;
	IntPrioritySet(INT_GPIOF, 0x00);		                          // Set interrupt priority 0 bits 23-21:010, Code: NVIC_PRI7_R &= ~0x00E00000
	GPIOIntEnable(GPIO_PORTF_BASE, GPIO_INT_PIN_2);		//GPIO_PORTF_IM_R |= 0x04; arm interrupt on PF2 (Echo)
	GPIOIntTypeSet(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_RISING_EDGE);
	                                                              //GPIO_PORTF_IS_R &= ~0x04;	PF2 are edge sensitive
	                                                              //GPIO_PORTF_IBE_R &= ~0x04;  PF2 not both edge triggered, depends on IEV register
	                                                              //GPIO_PORTF_IEV_R |= 0x04;PF2 are rising-edged event
}

void Timer0A_Handler(void)
{
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);				 // Clear trigger flag for Timer0A
	IntRegister(INT_TIMER0A, Timer0A_Handler);
	
	// Create 10us pulse
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, GPIO_PIN_3); // Set Trigger pin high
	SysCtlDelay(27);				 															 // Make 10us delay, 27, (10e-6*SysCtlClockGet()-2)/3 + 1
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0x00); 			 // Set Trigger pin low
}

void GPIOPortF_Handler(void)
{
		// Clear trigger flag echo pin
		GPIOIntClear(GPIO_PORTF_BASE, GPIO_INT_PIN_2);
		
		// count while there's pulse from echo pin
		while(GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_2)== GPIO_PIN_2)
		{
			count++;
		}
		
		// if less than around 4.5cm, turn servo by 90 degrees, 100 == 4.5cm
		if(count < 100)
		{
			GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_PIN_1);
			
      cars_count++;
      
			//Drive servo to 90 left (2.6ms, or 650)	
			PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, 625);
			PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, 625);
			
			// 2s delay, 5333334
			SysCtlDelay((2*SysCtlClockGet()-2)/3 + 1);
		}
    
		else
		{
			GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0);

			PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, 375);
			PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, 375);
		}
		count = 0;	
}

int main(void)
{
	// Set clock rate to 400M/2/25 = 8MHz. (or 40MHz, divide by 5)
	SysCtlClockSet(SYSCTL_SYSDIV_25|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);
	
	unsigned long reload = 480000;			// reload value to generate 60ms delay
	uint32_t period = 5000; 						//20ms (8MHz / 32pwm_divider / 50Hz)
	uint32_t duty = 375;     						//1.5ms pulse width (0 degrees), 0(1.5ms) mid, +90(~2.4ms) left, -90(0.5ms) right	
	
	// Initialize GPIOportF, PF2, PF3
	PortFunctionInit();
	
	// Configure PB6,PB7 as PWM										
	ConfigurePWM(period, duty);
	
	// Run PortF interrupt
	Interrupt_Init();
	
	// Run Timer interrupt
	Timer0A_Init(reload);
	
	// Globally enable interrupt
	IntMasterEnable();

	while(1)
  {
		
  }
}

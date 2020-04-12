//***********************************************************
// Hardware PWM proof of concept using
// the Tiva C Launchpad
//
// 
//
//
//This drives servo motors on PB6, PB7
//
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
//***********************************************************

void
ConfigurePWM(uint32_t period, uint32_t duty)
{
		// Set clock rate to 400M/2/25 = 8MHz. (or 40MHz, divide by 5)
	  SysCtlClockSet(SYSCTL_SYSDIV_25|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);

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
    //PWM_GEN_0 Covers M0PWM0 and M0PWM1 for PB6,7
    PWMGenConfigure(PWM0_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN|PWM_GEN_MODE_NO_SYNC); 

    //Set the Period (expressed in clock ticks)
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, period);

    //Set PWM duty for M0PWM0 and M0PWM1
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, duty);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, duty);

    // Enable the PWM generator
    PWMGenEnable(PWM0_BASE, PWM_GEN_0);

		// Enable PWM Output pins 6 & 7: M0PWM0 and M0PWM1
    PWMOutputState(PWM0_BASE, PWM_OUT_0_BIT | PWM_OUT_1_BIT, true);		
}

int
main(void)
{
	uint32_t period = 5000; 	//20ms (8MHz / 32pwm_divider / 50Hz)
	uint32_t duty = 375;     	//1.5ms pulse width (0 degrees), 0(1.5ms) mid, +90(~2.4ms) left, -90(0.5ms) right	
	
	// Configure PB6,PB7 as PWM
	ConfigurePWM(period, duty);
	
	while(1)
  {
		//(0.5*SysCtlClockGet()-2)/3 + 1
		SysCtlDelay(2666667);

    //Drive servo to 0 (mid)
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, duty);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, duty);

    SysCtlDelay(2666667);

    //Drive servo to 90 left (2.6ms, or 650)	
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, 650);
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, duty+duty);
  }
}
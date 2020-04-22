#include <stdint.h>
#include "sensor.h"
#include "../inc/Clock.h"
#include "../inc/CortexM.h"
#include "../inc/LaunchPad.h"
#include "../inc/Motor.h"
#include "../inc/TimerA1.h"



int distance;

//Trigger pin conected to P6.4
uint8_t trigBit = 0x10;

//Echo pin onnected to P6.5
uint8_t echoBit = 0x20;

void Distance_Init(void){
    //Trigger as GPIO, P6.4
    P6->SEL0 &= ~trigBit;
    P6->SEL1 &= ~trigBit;
    //Trigger as Output
    P6->DIR |= trigBit;

    //Echo as GPIO P6.5
    P6->SEL0 &= ~echoBit;
    P6->SEL1 &= ~echoBit;
    //Echo as input
    P6->DIR &= ~echoBit;
}


int GetDistance(int timeOut){
    //timeOut is a value in microseconds
    //it is used to exit the while loop if the signal is not found

    int time;
    //int distance;
    uint8_t echoValue;

    //Trigger off
    P6->OUT &= ~trigBit;
    Clock_Delay1us(2);

    //Trigger on
    P6->OUT |= trigBit;
    Clock_Delay1us(10);

    //Trigger off
    P6->OUT &= ~trigBit;

    time = 0;

    //Store the value of the echo sensor ANDing the pin vector with the bit that the eacho sensor is connected to
    echoValue = (P6->IN & echoBit);

    //wait for triggers signal to be received by the echo sensor
    while( echoValue == 0x00 && time < timeOut){
        //Delay for one microsecond so we can keep track of how much time it took to receive signal
        Clock_Delay1us(1);
        time++;
        echoValue = (P6->IN & echoBit);
    }

    time = 0;
    echoValue = (P6->IN & echoBit);

    //wait for echo signal to return to LOW
    while( echoValue != 0x00 && time < timeOut  ){
        //Delay for 1 microsecond and increment time variable so the time variable will store how much time it took for the echo signal to return to 0
        Clock_Delay1us(1);
        time++;
        echoValue = (P6->IN & echoBit);
    }

    //Rate of sound in air is approximately 0.0343 centimeters per microsecond
    //Distance = Rate * time
    distance = (0.0343 * time);

    return distance;
}

int objectDetected = 0;


void CheckDistance(void){

    distance = GetDistance(10000);
    uint8_t LED = 0x00;

    if(distance < 10){
        //If sensor detects an object, make the LED light RED
        LED = RED;

        //objectDetected is a global variable, and is referenced in the AvoidObject function
        objectDetected = 1;

    }else{
        //set LED to green if no object is in front
        LED = GREEN;

        objectDetected = 0;
    }

    //set LED's color
    LaunchPad_Output(LED);
}

void AvoidObject(void){

    //Turn on RED led on Robot
    LaunchPad_LED(0x01);

    //stop the motor
    Motor_Stop();
    Clock_Delay1ms(250);

    //Robot turns right when an object is detected
    Motor_Right(4000,4000);

    while(objectDetected == 1){
        //The robot will turn right until an object is no longer detected
    }

    //Stop motor when an object is no longer in front of the distance sensor
    Motor_Stop();

    //Turn off RED led
    LaunchPad_LED(0x00);

    //Wait for a short period of time before the function completes
    //This is not needed, however, I used it so the robot came to a short stop before continuing forward
    Clock_Delay1ms(500);
}

int main(void){

    Clock_Init48MHz();
    LaunchPad_Init(); // built-in switches and LEDs
    Motor_Init();     // your function
    Distance_Init();
    TimerA1_Init(&CheckDistance,50000);
    Clock_Delay1ms(1000);
    EnableInterrupts();

    while(1)
    {
        if(objectDetected == 0){
            Motor_Forward(5000,5000);
        }
        else{
            AvoidObject();
            Clock_Delay1ms(250);
        }
    }
}

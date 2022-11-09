/* 
 * File:   DigitalThermometer Control.c
 * Author: cesar colina
 *
 * Created on November 29, 2021, 10:15 PM
 */


#include "defines.h"

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>

#include <avr/io.h>
#include <avr/pgmspace.h>

#include <util/delay.h>

#include "lcd.h"
#include <math.h>

// holds the full decimal value
float external_raw;
// holds the full decimal value
float internal_raw;
// holds the integer numbers
int External_tempint;
// holds the decimal numbers
int External_tempdec;
// holds the integer numbers
int Internal_tempint;
// holds the decimal numbers
int Internal_tempdec;
//initial digit for user temp selection (button: 2)
int MSB = 1;
//secondary digit for user temp selection (button: 3)
int LSB = 0;

//the tracker will be used to select which arrow output to choose from.
uint16_t Mode_Tracker = 0;

/*
 * 
 */

FILE lcd_str = FDEV_SETUP_STREAM(lcd_putchar, NULL, _FDEV_SETUP_WRITE);
//file stream for output

int main(int argc, char** argv) {
    IO_init(); //initialize the buttons
    
    while(1){
      
      lcd_init(); // initialize the LCD
      External_init(); // obtain the values for the external temp sensor
      Internal_init(); // obtain the values for the internal temp sensor
      ExternalInternal_display(Mode_Tracker); // display the values of the temp sensor
      ModeShift(); // switch between the temperature sensors
      DigitSelect(); // code to take in the inputs of the user 
      
      
      _delay_ms(400); //refreshes screen 
    }
    

    
    return (1);
}

void External_init(){
    ADCSRA |= (1<<7); // enable the ADC
    ADMUX |= (1<<REFS0); //set a reference voltage from the external capacitor
    // ADMUX has the MUX registers from 3:0 set to 0 which sets the analog input to ADC0
    ADCSRA |= (1<<ADSC); // begin the conversion
    while(!(ADCSRA & (1<<ADIF))){} // if the flag is set (successful conversion) move on. If not wait in the loop
    ADCSRA &= ~(1<<ADSC); //turn the ADC conversion off
    
    // hold the raw temperature value
    external_raw = (ADC * 5) / (1024 * 0.01); // ref voltage is 5V
    External_tempint = (int) external_raw; // save the first digits of the temperature
    External_tempdec = (int) (fmod(external_raw, 1.0) * 10); //isolate and store the decimal portion for printing
    // reset the admux and adcsra to prevent the other temperature sensor from using the values
    ADMUX = 0x00;
    ADCSRA = 0x00;
    
    
    
}

void ExternalInternal_display(){
    
    
    switch(Mode_Tracker % 2)
        {
            case 0: //even number
               //select internal temp
                fprintf(&lcd_str, "%d.%d\xDF",Internal_tempint, Internal_tempdec);
                fprintf(&lcd_str, "C \x7F\x2D %d.%d\xDF", External_tempint, External_tempdec); // point arrow <-
                fprintf(&lcd_str, "C");
                fprintf(&lcd_str, "\x1b\xc0");
                //top part
                fprintf(&lcd_str, "      %d%d\xDF",MSB,LSB);
                if (TempEvaluation() == 2){
                    //turn on Furnace
                    fprintf(&lcd_str, "C   On");
                    PORTB |= (1<<PORTB0); //turn on LED
                }
                if (TempEvaluation() == 1){
                    //turn off Furnace
                    fprintf(&lcd_str, "C   Off");
                    PORTB &= ~(1<<PORTB0); //turn off LED
                }
                if (TempEvaluation() == 0){
                    //keep things as is
                }
                //bottom part
                
                break;
                
            case 1: //odd number
                //select external temp
                
                fprintf(&lcd_str, "%d.%d\xDF",Internal_tempint, Internal_tempdec);
                fprintf(&lcd_str, "C \x2D\x7E %d.%d\xDF", External_tempint, External_tempdec); // point arrow ->
                fprintf(&lcd_str, "C");
                fprintf(&lcd_str, "\x1b\xc0");
                //top part
                fprintf(&lcd_str, "      %d%d\xDF",MSB,LSB);
                
                if (TempEvaluation() == 2){
                    //turn on Furnace
                    fprintf(&lcd_str, "C   On");
                    PORTB |= (1<<PORTB0); //turn on LED
                }
                if (TempEvaluation() == 1){
                    //turn off Furnace
                    fprintf(&lcd_str, "C   Off");
                    PORTB &= ~(1<<PORTB0); //turn off LED
                }
                if (TempEvaluation() == 0){
                    //keep things as is
                } 
                //bottom part
                
                break;
                
            default: // shouldn't need to run
                break;
        }
    
    
}

void Internal_init(){
    
    ADCSRA |= (1<<7); // enable the ADC
    ADMUX |= (1<<REFS1) | (1<<REFS0)| (1<<MUX3); // internal voltage and using the temperature sensor
    _delay_ms(5); // add a delay to stabilize the voltage
    ADCSRA |= (1<<ADSC); // begin the conversion
    while(!(ADCSRA & (1<<ADIF))){} // if the flag is set (successful conversion) move on. If not wait in the loop
    ADCSRA &= ~(1<<ADSC); //turn the ADC conversion off
    
     // hold the raw temperature value
    internal_raw = (ADC * 1.1) / (1024 * 0.01); // ref voltage is 1.1V
    internal_raw = (internal_raw - 35.49)/0.0833; // the calibrated voltage using a calculated slope and constant C
    Internal_tempint = (int) internal_raw; // save the first digits of the temperature
    Internal_tempdec = (int) (fmod(internal_raw, 1.0) * 10); //isolate and store the decimal portion for printing
    // reset the admux and adcsra to prevent the other temperature sensor from using the values
    ADMUX = 0x00;
    ADCSRA = 0x00;
    
    
}

void IO_init(){
    DDRC &= ~(1<<DDC3); //input switch 1
    PORTC |= (1<<PORTC3); //pullup switch 1
    
    DDRC &= ~(1<<DDC4); //input switch 2
    PORTC |= (1<<PORTC4); //pullup switch 2
    
    DDRC &= ~(1<<DDC5); //input switch 3
    PORTC |= (1<<PORTC5); //pullup switch 3
    
    DDRB |= (1<<DDB0); //output to LED
}

void ModeShift(){
    // if button 1 is pressed switch the arrow direction
    if(!(PINC & (1<<PINC3))){
        while(!(PINC & (1<<PINC3))){} //wait for user to let go
        Mode_Tracker++; 
         
    }
    
}

// return a 2 if LED turns on. return a 1 if it doesn't. return a 0 if in deadband
int TempEvaluation(){
    
    uint8_t user = (MSB * 10) + LSB; // user input
    switch(Mode_Tracker % 2)
        {
            case 0: //even number
                //select internal temp
                // difference between the user number and the temperature is greater than 0.5
                if((user - internal_raw) >= 0.5){
                    return(2); // turn on furnace/LED
                }
                // difference between the user number and the temperature is less than 0.5
                if((user - internal_raw) <= 0.5){
                    return(1); //turn off furnace/LED
                }
                //if in between nothing happens
                return(0);
                break;
                
            case 1: //odd number
                //select external temp
                // difference between the user number and the temperature is greater than 0.5
                if((user - external_raw) >= 0.5){
                    return(2); // turn on furnace/LED
                }
                // difference between the user number and the temperature is less than 0.5
                if((user - external_raw) <= 0.5){
                    return(1); //turn off furnace/LED
                }
                //if in between nothing happens
                return(0);
                break;
                
            default: // shouldn't need to run
                break;
        }
    
    
}

void DigitSelect(){
    // if button 2 is pressed (MSB)
    if (!(PINC & (1<<PINC4)))
    {
        while(!(PINC & (1<<PINC4))){} //wait for user to let go
        LSB++;
        if(LSB > 9){ //increment
            LSB = 0;
            MSB++;
        }
        if(MSB == 3 && LSB > 5){ // if the value is 35 then reset to 10
            MSB = 1;
            LSB = 0;
        }
       
    }
    
    // if button 3 is pressed (LSB)
    if (!(PINC & (1<<PINC5)))
    {
        while(!(PINC & (1<<PINC5))){} //wait for user to let go
        LSB--;
        if(MSB == 1 && LSB < 0){ //if the value is 10 reset to 35
            MSB = 3;
            LSB = 5;
        }
        if(LSB < 0){ // increment
            LSB = 9;
            MSB--;
        }
        
    }
}

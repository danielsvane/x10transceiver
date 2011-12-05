#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

int START_MESSAGE[4] = {1,1,1,0};

int interrupts = 0;
int data[4] = {0,0,0,0};
int receiving = 0;
int sending = 0;
int received = 0;
int clockCounter = 0;


// Compares 2 int arrays, returns 1 if equal, 0 if not equal
int isEqual(int arr1[], int arr2[]){
	if(sizeof(arr1) != sizeof(arr2)) return 0;
	int i = 0;
	while(i<sizeof(arr1)){
		if(arr1[i] != arr2[i]) return 0;
		i++;
	}
	return 1;

}

void checkData(void){
	// If start message received, turn on LED
	if(isEqual(data, START_MESSAGE)) turnOnLED(7);
}

void sendBit(int bit){
	// Start first timer for 1 ms. 3686,4 clock cycles needed for 1 ms. A prescaler of 64 will give the value that should be written to the compare register of 57,6
	// Rounded to 58 will make the timer run for 3712 clock cycles, or 0,993 ms
	// Start second timer responsible for 200kHz, to toggle on compare match.
	// OCR should be set to 8, will give the closest possible frequency of 204800Hz. 3686400/(2(1+8))
	// When 1 ms has passed, kill both timers

	// OCR2 should be set to 14, for a 123 kHz frequency

	// Enables timer0 and timer2
	TIMSK |= 0b10000010;
}

void setupTimers(){
	// Set timer0 to output compare mode
	TCCR0 |= 0b00001000;
	// Give timer0 a prescaler of 64
	TCCR0 |= 0b00000011;
	// Set timer0 top to 58
	OCR0 = 58;

	// timer2 has no prescaler
	// Set timer2 to CTC mode
	TCCR2 |= 0b00001000;
	// Set timer2 to toggle on compare match
	TCCR2 |= 0b00010000;
	// Set timer2 top to 14;
	OCR2 = 14;
}

ISR(TIMER0_COMP_vect){
	// When 1 ms has passed, disable timer0 and timer2
	TIMSK &= 0b01111101;
}

void turnOnLED(int led){
	PORTB &= ~(0b00000001 << led);
}

void toggleLED(int led){
	PORTB ^= (0b00000001 << led);
}

// Clock interrupt0
ISR(_VECTOR(1)){
	// Toggles LED0 for visual feedback
	toggleLED(0);
	sendBit(1);
	// If a data interrupt has occurred
	if(receiving){
		// Count data interrupts, and save to data array
		if(interrupts == 1) data[clockCounter] = 1;
		else if(interrupts == 0) data[clockCounter] = 0;
		interrupts = 0;
		// Toggles LED1 for visual feedback
		toggleLED(1);
		// If receiving and 4 clocks detected
		if(++clockCounter == 4){
			checkData();
			// Reset clock counter
			clockCounter = 0;
			// Stop receiving
			receiving = 0;
		}
	}
	// If send command occurred
	else if(sending){
		sendBit(START_MESSAGE[clockCounter]);
		// If 4 bits are sent, stop sending
		if(++clockCounter == 4){
			sending = 0;
			// Enable data interrupt
			GICR = 0b11000000;
			// Reset clock counter
			clockCounter = 0;
		}
	}
}

// Data interrupt1
ISR(_VECTOR(2)){
	receiving = 1;
	interrupts++;
}

// Button interrupt2
ISR(_VECTOR(18)){
	// Only receive if not sending
	sendStartMessage();
}

// UART receive interrupt
ISR(USART_RXC_vect){
	// If received an 'a', send a start message through X10
	if(UDR == 'a'){
		sendStartMessage();
	}
}

void sendStartMessage(){
	// Disable UART interrupt
	UCSRB = 0b01000000;
	// Disable data interrupt
	GICR &= 0b01111111;

	sending = 1;
}

int main(void)
{
    DDRB = 0xFF;
    PORTB = 0xFF;

    // Enable INT0, INT1 and INT2 interrupt
    GICR = 0b11100000;
    // Sets INT0 and INT1 to rising edge
    MCUCR = 0b00001111;
    // Enable global interrupts
    sei();

    while(1){}
}




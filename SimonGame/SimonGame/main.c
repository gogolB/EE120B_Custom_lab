/*
 * SimonGame.c
 *
 * Created: 8/23/2017 1:08:39 PM
 * Author : Gogol
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include "PRNG.c"
#include "io.c"

// ********************************************************************************
//								Timing functions
// ********************************************************************************
volatile unsigned char TimerFlag = 0;
// Internal variables for mapping AVR's ISR to our cleaner TimerISR model.
unsigned long _avr_timer_M = 1; // Start count from here, down to 0. Default 1 ms.
unsigned long _avr_timer_cntcurr = 0; // Current internal count of 1ms ticks

void TimerOn() {
	// AVR timer/counter controller register TCCR1
	TCCR1B = 0x0B;// bit3 = 0: CTC mode (clear timer on compare)
	// bit2bit1bit0=011: pre-scaler /64
	// 00001011: 0x0B
	// SO, 8 MHz clock or 8,000,000 /64 = 125,000 ticks/s
	// Thus, TCNT1 register will count at 125,000 ticks/s

	// AVR output compare register OCR1A.
	OCR1A = 125;	// Timer interrupt will be generated when TCNT1==OCR1A
	// We want a 1 ms tick. 0.001 s * 125,000 ticks/s = 125
	// So when TCNT1 register equals 125,
	// 1 ms has passed. Thus, we compare to 125.
	// AVR timer interrupt mask register
	TIMSK1 = 0x02; // bit1: OCIE1A -- enables compare match interrupt

	//Initialize avr counter
	TCNT1=0;

	_avr_timer_cntcurr = _avr_timer_M;
	// TimerISR will be called every _avr_timer_cntcurr milliseconds

	//Enable global interrupts
	SREG |= 0x80; // 0x80: 1000000
}

void TimerOff() {
	TCCR1B = 0x00; // bit3bit1bit0=000: timer off
}

void TimerISR() {
	TimerFlag = 1;
}

// In our approach, the C programmer does not touch this ISR, but rather TimerISR()
ISR(TIMER1_COMPA_vect) {
	// CPU automatically calls when TCNT1 == OCR1 (every 1 ms per TimerOn settings)
	_avr_timer_cntcurr--; // Count down to 0 rather than up to TOP
	if (_avr_timer_cntcurr == 0) { // results in a more efficient compare
		TimerISR(); // Call the ISR that the user uses
		_avr_timer_cntcurr = _avr_timer_M;
	}
}

// Set TimerISR() to tick every M ms
void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}
// ********************************************************************************
//								Sound functions
// ********************************************************************************
void set_PWM(double frequency) {
	static double current_frequency;
	if (frequency != current_frequency) {
		if (!frequency) { TCCR3B &= 0x08; }
		else { TCCR3B |= 0x03; }
		if (frequency < 0.954) { OCR3A = 0xFFFF; }
		else if (frequency > 31250) { OCR3A = 0x0000; }
		else { OCR3A = (short)(8000000 / (128 * frequency)) - 1; }
		TCNT3 = 0;
		current_frequency = frequency;
	}
}

void PWM_on() {
	TCCR3A = (1 << COM3A0);
	TCCR3B = (1 << WGM32) | (1 << CS31) | (1 << CS30);
	set_PWM(0);
}

void PWM_off() {
	TCCR3A = 0x00;
	TCCR3B = 0x00;
}

void playSound(char sequence)
{
	if(sequence == 0x01)
	{
		// C4
		set_PWM(261.63);
	}
	else if(sequence == 0x02)
	{
		// D4
		set_PWM(293.66);
	}
	else if (sequence == 0x04)
	{
		// E4
		set_PWM(329.63);
	}
	else if(sequence == 0x08)
	{
		// F4
		set_PWM(349.23);
	}
	else
	{
		// Silence
		set_PWM(0);
	}
}

// ********************************************************************************
//								Game functions
// ********************************************************************************
// Boolean value for button 1
unsigned char button1 = 0x00;
// Boolean value for button 2
unsigned char button2 = 0x00;
// Boolean value for button 3
unsigned char button3 = 0x00;
// Boolean value for button 4
unsigned char button4 = 0x00;

// The value for (~PINB & 0x0F) or (button1) | (button2 << 1) | (button3 << 2) | (button4 << 3)
unsigned char currentValue;


// Gets the inputs from the four buttons.
// Sets appropriate button variable to 1 or 0.
// button1 = 1 means button 1 is pressed, 0 otherwise.
void getInput()
{
	unsigned char tmpB = (~PINB & 0x0F);
	button1 = ((tmpB & 0x01) == 0x01);
	button2 = ((tmpB & 0x02) == 0x02);
	button3 = ((tmpB & 0x04) == 0x04);
	button4 = ((tmpB & 0x08) == 0x08);
}

// Game state.
enum SIMON_GAME_STATE{WAIT_TO_START, INIT_GAME, GENERATE_NEXT_SEQUENCE_COMPONENT, PLAYBACK_SEQUENCE, GET_INPUT, GET_INPUT_FE, LOSE_GAME, WIN_GAME, UPDATE_GAME}GAME_STATE;

// The maximum number of rounds
#define MAX_ROUNDS 9
// How long to way during playback.
#define MAX_WAIT_PER_LED 2

// Generated Sequence
unsigned char GeneratedSequence[MAX_ROUNDS];

// Next location for generation.
unsigned char nextLocationToGenerate = 0;
// How many values have been generated so far.
unsigned char maxGeneratedSoFar = 0;
// The next location for playback.
unsigned char nextLocationToPlayback = 0;
// The next location for program to check the user input against/
unsigned char nextLocationToCheck = 0;

// Counters for timing .
unsigned char PlayBackCount = 0;

void GAME_TICK()
{
	// Transition Table
	switch(GAME_STATE)
	{
		case WAIT_TO_START:
			if(button1 || button2 || button3 || button4)
			{
				// Push any button to begin the game.
				GAME_STATE = INIT_GAME;
				LCD_ClearScreen();
				LCD_DisplayString(1, "Let go of button to start game.");
			}
		break;
		
		case INIT_GAME:
			if(button1 || button2 || button3 || button4)
			{
				// They held done the button and they need to let go
				GAME_STATE = INIT_GAME;
			}
			else
			{
				// They let go of the button, now its time to start the game.
				GAME_STATE = GENERATE_NEXT_SEQUENCE_COMPONENT;

			}
		break;
		
		case GENERATE_NEXT_SEQUENCE_COMPONENT:
			GAME_STATE = PLAYBACK_SEQUENCE;
			LCD_ClearScreen();
			LCD_DisplayString(1, "Can you remember this? S: ");
			LCD_WriteData('0' + maxGeneratedSoFar - 1);
		break;
		
		case PLAYBACK_SEQUENCE:
			// We have played back the whole generated sequence.
			if(nextLocationToPlayback >= maxGeneratedSoFar)
			{
				// Go wait for their input.
				GAME_STATE = GET_INPUT;
				// Reset everything.
				nextLocationToPlayback = 0;
				PORTA = 0x00;
				LCD_ClearScreen();
				LCD_DisplayString(1, "Your turn,do you remember? S: ");
				LCD_WriteData('0' + maxGeneratedSoFar - 1);
			}
			else
			{
				// We have given them enough time to look at the LED.
				if(PlayBackCount > MAX_WAIT_PER_LED)
				{
					GAME_STATE = PLAYBACK_SEQUENCE;
					nextLocationToPlayback++;
					PlayBackCount = 0;
				}
				else
				{
					// Wait for them to see the LED.
					PlayBackCount++;
				}
			}
		break;
		
		case UPDATE_GAME:
			// Update all the things as necessary.
			PORTA = 0;
			if(button1 || button2 || button3 || button4)
			{
				GAME_STATE = UPDATE_GAME;
				LCD_ClearScreen();
				LCD_DisplayString(1, "Nice Job! Score: ");
				LCD_WriteData('0' + (maxGeneratedSoFar));
			}
			else if(nextLocationToCheck < MAX_ROUNDS)
			{
				nextLocationToCheck = 0;
				GAME_STATE = GENERATE_NEXT_SEQUENCE_COMPONENT;
			}
			else
			{
				// They won the game!
				GAME_STATE = WIN_GAME;
				LCD_ClearScreen();
				LCD_DisplayString(1, "Congratulation! You're a winner!");
			}
		break;
		
		case WIN_GAME:
			if(button1 || button2 || button3 || button4)
			{
				// Restart the game.
				GAME_STATE = WAIT_TO_START;
				LCD_ClearScreen();
				LCD_DisplayString(1, "Push button to  start...");
				playSound(0);
			}
		break;
		
		case LOSE_GAME:
			if(button1 || button2 || button3 || button4)
			{
				// Restart the game.
				GAME_STATE = WAIT_TO_START;
				LCD_ClearScreen();
				LCD_DisplayString(1, "Push button to  start...");
				playSound(0);
			}
		break;
		
		case GET_INPUT:
			playSound(0);
			if(nextLocationToCheck >= maxGeneratedSoFar)
			{
				// Round was won because all points matched.
				GAME_STATE = UPDATE_GAME;
			}
			else
			{
				GAME_STATE = GET_INPUT;
				playSound(0);
			}
		break;
		
		case GET_INPUT_FE:
			if(button1 || button2 || button3 || button4)
			{
				GAME_STATE = GET_INPUT_FE;
			}
			else
			{
				nextLocationToCheck++;
				GAME_STATE = GET_INPUT;
				playSound(0);
			}
		break;
		
		default:
			GAME_STATE = WAIT_TO_START;
		break;
	}
	
	// Action Table
	switch(GAME_STATE)
	{
		case WAIT_TO_START:
			playSound(0);
		break;
		case GET_INPUT_FE:
		break;
		case LOSE_GAME:
		break;
		case WIN_GAME:
		break;
		case UPDATE_GAME:
		break;
		
		case INIT_GAME:
			// Reset all game states.
			nextLocationToGenerate = 0;
			nextLocationToPlayback = 0;
			maxGeneratedSoFar = 0;
			nextLocationToCheck = 0;
		break;
		
		case GENERATE_NEXT_SEQUENCE_COMPONENT:
			PORTA = 0;
			if(nextLocationToGenerate < MAX_ROUNDS)
			{
				GeneratedSequence[nextLocationToGenerate] = getRandomLed();
				nextLocationToGenerate++;
				maxGeneratedSoFar++;
			}
			else
			{
				// Some kind of win sequence.
				GAME_STATE = WIN_GAME;
				playSound(0);
			}
		break;
		
		case PLAYBACK_SEQUENCE:
			if(nextLocationToPlayback < maxGeneratedSoFar)
			{
				PORTA = GeneratedSequence[nextLocationToPlayback] << 1;
				playSound(GeneratedSequence[nextLocationToPlayback]);
				//nextLocationToPlayback++;
			}
		break;
		
		case GET_INPUT:
			currentValue = (button1) | (button2 << 1) | (button3 << 2) | (button4 << 3);
			PORTA = currentValue << 1;
			playSound(currentValue);
			if(currentValue == 0)
			{
				// No input.
				break;
			}
			else if(currentValue == GeneratedSequence[nextLocationToCheck])
			{
				// It matched, keep moving forward to check next location.
				//nextLocationToCheck++;
				GAME_STATE = GET_INPUT_FE;
			}
			else
			{
				// There was input and it was wrong.
				// Lost the game.
				GAME_STATE = LOSE_GAME;
				playSound(0);
				LCD_ClearScreen();
				LCD_DisplayString(1, "Better luck next time!");
				PORTA = 0x00;
			}
		break;
	}
}

int main(void)
{
	// Init the outputs.
	DDRA = 0xFF; PORTA = 0x00;
	// Init the Inputs.
	DDRB = 0xF0; PORTB = 0x0F;
	DDRC = 0xFF; PORTC = 0x00;
	DDRD = 0xFF; PORTD = 0x00;

	// Init the LCD.
	LCD_init();
	PWM_on();
	// Set the seed for the PNR.
	Initialize(1);				// Use a predetermined seed to allow for repeatability.
	TimerSet(200);
	
	TimerOn();
	GAME_STATE = WAIT_TO_START;
	
	LCD_ClearScreen();
	LCD_DisplayString(1, "Push button to  start...");
	set_PWM(0);
    while (1)
    {
		// Get inputs.
		getInput();
		GAME_TICK();
		//PORTD = GAME_STATE;
		while(!TimerFlag);
		TimerFlag = 0;
    }
}


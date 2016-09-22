/******************************************************************************
/	SHORT DESCRIPTION:
/ 	This memory game program shows a random sequence of blinks with 4 LEDs, 
/	waits for the player to remember and input the same sequence using buttons 
/	that correspond to each colour of LED, judges the correctness of the
/	answer, and continues this process until the player wins or loses.
/******************************************************************************

/******************************************************************************
/	LONG DESCRIPTION:
/	A button in front of a lit red LED mustbe pressed to start the game
/
/	The player chooses a difficulty at the start of the game, which decides
/	the number of rounds to be played. (8, 10, or 12 by default, with a
/	'hidden' option to play 16 rounds) They do this by pressing one of four
/	buttons.
/
/	The game then sets the random sequence and flashes the LEDs, and the player
/	attempts to remember the sequence of flashes, which increases by 1 every
/	round, and inputs their answer using the buttons set in front of the LEDs
/
/	If the answer is false, the game flashes a red LED and allows the player
/	to restart the game. If the answer is true, the game continues until the
/	round set in the variable 'difficulty', and then tells the player they've
/	won by showing a series of flashing LEDs.
******************************************************************************/

#include <msp430.h>

// Used to nable inputs and set the appropriate timers
#define ENABLE_PINS	0xFFFE
#define ACLK		0x0100
#define SMCLK		0x0200
#define	CONTINUOUS	0x0020
#define	UP 			0x0010

// The LEDs and buttons used to stop the timer and get the random number
#define RED_LED			0x40						// Red LED 			(P3.6)
#define	GREEN_LED		0x08						// Green LED		(P3.3)
#define BLUE_LED		0x40						// Blue LED 		(P2.6)
#define YELLOW_LED		0x80 						// Yellow LED 		(P2.7)

#define	RED_BTN			0x08 						// Red button 		(P2.3)
#define GREEN_BTN		0x02						// Green button 	(P3.1)
#define BLUE_BTN		0x01 						// Blue button 	 	(P3.0)
#define YELLOW_BTN		0x08 						// Yellow button 	(P1.3)

// Different length pauses for the Wait() function
#define TEN_MS		1
#define	CENTI_SEC	10
#define	QUART_SEC	25
#define HALF_SEC	50
#define ONE_SEC		100
#define	BLINK		20
#define PAUSE		30

// The on-board LED used to indicate a correct answer
#define P9_GREEN_LED	0x80 					// Launchpad green LED 	(P9.7)

// The number of rounds played depending on the chosen difficulty
#define EASY			8
#define NORMAL 			10
#define HARD			12
#define	EXTREME			16

// The functions used to set up, play and reset the game
void Reset(void);
int ChooseDifficulty(void);
void Wait(int t);

int GetFirstNumber(void);
int GetSecondNumber(void);
void MakeSequence(int sequence[16], int first_number, int second_number);

void BlinkLeds(int sequence[16], int round);
int	GetAnswer(int sequence[16], int round);
void CorrectAnswer(void);

void Win(void);
void Loss(void);

/******************************************************************************
/	The main function calls the different functions to go through the stages 
/	and rounds of the game, and repeats after a win or a loss occurs
******************************************************************************/
void main(void)
{
	// Turn off the WatchDog Timer and enable I/O pins
	WDTCTL 	= WDTPW | WDTHOLD;
	PM5CTL0	= ENABLE_PINS;

	// Start timer A0 with ACLK, counting UP to 10 ms (400 * 25 µsec)
	TA0CTL |= ACLK | UP;
	TA0CCR0 = 400;

	// Start timer A1 with ACLK and A2 with SMCLK, counting continuously
	TA1CTL |= ACLK | CONTINUOUS;
	TA2CTL |= SMCLK | CONTINUOUS;

	// Enable LED outputs and button inputs
   	P2DIR |= BLUE_LED | YELLOW_LED;
   	P3DIR |= RED_LED | GREEN_LED;
   	P9DIR |= P9_GREEN_LED;

   	P1OUT |= YELLOW_BTN;
   	P1REN |= YELLOW_BTN;
   	P2OUT |= RED_BTN;
   	P2REN |= RED_BTN;
   	P3OUT |= GREEN_BTN | BLUE_BTN;
   	P3REN |= GREEN_BTN | BLUE_BTN;

	// Start the gameplay loop
	while(1)
	{
		// Waits for the player to start the game, then gets random number
		int first_number = 0;
		Reset();
		first_number = GetFirstNumber();
		Wait(QUART_SEC);
		// Waits for the player to choose a difficulty, the gets second number
		int difficulty;
		int second_number = 0;
		difficulty = ChooseDifficulty();
		second_number = GetSecondNumber();
		// Fills an array with a combination of the two random numbers
		int sequence[16] = {0.0};
		MakeSequence(sequence, first_number, second_number);

		// Starts the game, keep playing until a win or loss is indicated
		int game_state = 1;
		int round = 0;
		while(game_state == 1)
		{
			// Shows the appropriate sequence for the round
			Wait(ONE_SEC);
			BlinkLeds(sequence, round);

			// Waits for the player to input an answer, then checks correctness
			Wait(TEN_MS);
			game_state = GetAnswer(sequence, round);
			Wait(TEN_MS);

			// If right answer was given, blink green, continue to next round
			if (game_state == 1)
			{
				CorrectAnswer();
				round++;
			}
			Wait(TEN_MS);

			// If enough rounds have been won, indicate a Win()
			if (round == difficulty)
				game_state = 2;
		}
		// If a win was indicated, show the player they've won
		if (game_state == 2)
		{
			Win();
		}
		// If not, then the loop quit because game_state == 0; show a loss
		else
		{
			Loss();
		}
	}
}

/******************************************************************************
/	This function shows a red LED, and continues the game preparation when the
/	corresponding button is pressed.
******************************************************************************/
void Reset(void)
{
	P3OUT |= RED_LED;
	int x = 0;
	while (x < 1)
	{
		if ((P2IN & RED_BTN) == 0)
		{
			P3OUT &= (~RED_LED);
			x = 1;
		}
	}
}

/******************************************************************************
/	This function reads Timer A1 to get a random-ish number after the previous
/	function's button is pressed, and returns it as an integer
******************************************************************************/
int GetFirstNumber(void)
{
	int first_num = 0;
	first_num = TA1R;
	return first_num;
}

/******************************************************************************
/	Shows the user 3 lit LEDs (green, blue yellow); they can press the 
/	corresponding buttons to select a difficulty, which decides the number
/	of rounds that have to be successfully played to win the game
******************************************************************************/
int ChooseDifficulty(void)
{
	P3OUT |= GREEN_LED;
	P2OUT |= (BLUE_LED | YELLOW_LED);
	int x = 0;
	int i;
	while (x < 1)
	{
		if ((P3IN & GREEN_BTN) == 0)
		{
			P3OUT &= (~GREEN_LED);
			P2OUT &= (~(BLUE_LED | YELLOW_LED));
			x = 8;
			for (i = 0; i < 8; i++)
			{
				P3OUT = (P3OUT ^ GREEN_LED);
				Wait(CENTI_SEC);
			}
		}
		if ((P3IN & BLUE_BTN) == 0)
		{
			P3OUT &= (~GREEN_LED);
			P2OUT &= (~(BLUE_LED | YELLOW_LED));
			x = 10;
			for (i = 0; i < 8; i++)
			{
				P2OUT = (P2OUT ^ BLUE_LED);
				Wait(CENTI_SEC);
			}
		}
		if ((P1IN & YELLOW_BTN) == 0)
		{
			P3OUT &= (~GREEN_LED);
			P2OUT &= (~(BLUE_LED | YELLOW_LED));
			x = 12;
			for (i = 0; i < 8; i++)
			{
				P2OUT = (P2OUT ^ YELLOW_LED);
				Wait(CENTI_SEC);
			}
		}
		if ((P2IN & RED_BTN) == 0)
		{
			P3OUT &= (~GREEN_LED);
			P2OUT &= (~(BLUE_LED | YELLOW_LED));
			x = 16;
			for (i = 0; i < 8; i++)
			{
				P3OUT = (P3OUT ^ RED_LED);
				Wait(CENTI_SEC);
			}
		}
	}
	return x;
}

/******************************************************************************
/	After a button is pressed in the previous function, this function reads
/	a random-ish value from Timer A2 and returns it as an integer
******************************************************************************/
int GetSecondNumber(void)
{
	int second_num = 0;
	second_num = TA2R;
	return second_num;
}


/******************************************************************************
/	This function uses the random numbers obtained in previous functions to
/ 	fill an array with a 16-number sequence, where each number is either 0, 1,
/	3 or 4, corresponding to the Red, Green, Blue and Yellow LEDs
/
/	It does this by iterating through the two random ints, moving each i-th bit
/	into the least-significant space, masking the rest, and storing that bit in
/	the i-th element of an array
/
/	The arrays are then combined (one as-is, the other *3) to form a new array
/	filled with a string of digits where each digit is either 0, 1, 3 or 4
******************************************************************************/
void MakeSequence(int sequence[16], int first_number, int second_number)
{
	int i;
	int first_array[16] = {0.0};
	int second_array[16] = {0.0};
	// Use bit shifting and masking to fill the new arrays bit-by-bit (ha!)
	for (i = 0; i < 16; i++)
	{
		first_array[(15 - i)] = ((first_number >> i) & 0x01);
		second_array[(15 - i)] = ((second_number >> i) & 0x01);
	}
	// Fill the pre-made sequence array with a new random sequence
	for (i = 0; i < 16; i++)
	{
		sequence[i] = (first_array[i]) + (second_array[i] * 3);
	}
}

/******************************************************************************
/	This function is used to blink LEDs and to wait for short periods of time
/	in between actions or messages
/
/	It counts up to the given (t) in steps of 10 milliseconds each, where
/ 	t is the number of 10-millisecond delays required
******************************************************************************/
void Wait(int t)
{
	int i = 0;
	// While the count set has not been reached
	while (i <= t)
	{	// When another 10 milliseconds have expired
		if (TA0CTL & TAIFG)
		{
			// Increase the count and start another 10-millisecond round
			i++;
			TA0CTL &= (~TAIFG);
		}
	}
}

/******************************************************************************
/	This function blinks the LEDs in the order indicated by the random number
/ 	sequence stored in the 'sequence' array
/
/	It will start by blinking only one light, then increment the number of
/	lights by one for each round played, up to a maximum of 16 rounds/blinks
******************************************************************************/
void BlinkLeds(int sequence[16], int round)
{
	int i = 0;
	// Start executing once, then checks whether it needs to loop again
	do
	{
		// Checks i-th element's value, blinks corresponding LED
		switch (sequence[i])
		{
			case (0):	P3OUT |= RED_LED;
						Wait(BLINK);
						P3OUT &= (~RED_LED);
						Wait(PAUSE);
						break;

			case (1):	P3OUT |= GREEN_LED;
						Wait(BLINK);
						P3OUT &= (~GREEN_LED);
						Wait(PAUSE);
						break;

			case (3):	P2OUT |= BLUE_LED;
						Wait(BLINK);
						P2OUT &= (~BLUE_LED);
						Wait(PAUSE);
						break;

			case (4):	P2OUT |= YELLOW_LED;
						Wait(BLINK);
						P2OUT &= (~YELLOW_LED);
						Wait(PAUSE);
						break;
		}

		i = i + 1;

	}
	while (i <= round);
}

/******************************************************************************
/ This function will wait for the player's answer, and judge its correctness
/
/ It does this by waiting for input, and checking whether it's the right input
/ for the i-th element of the sequence array, where i is the n-th LED shown
******************************************************************************/
int GetAnswer(int sequence[16], int round)
{
	int i = 0;
	int game_over = 0;
	// Loops as often as the round requires, or until a wrong answer is given
	while (i <= round && game_over == 0)
	{
		if ((P2IN & RED_BTN) == 0)
		{
			if (sequence[i] == 0)
				i++;
			else
				game_over = 1;
			Wait(QUART_SEC);
		}
		if ((P3IN & GREEN_BTN) == 0)
		{
			if (sequence[i] == 1)
				i++;
			else
				game_over = 1;
			Wait(QUART_SEC);
		}
		if ((P3IN & BLUE_BTN) == 0)
		{
			if (sequence[i] == 3)
				i++;
			else
				game_over = 1;
			Wait(QUART_SEC);
		}
		if ((P1IN & YELLOW_BTN) == 0)
		{
			if (sequence[i] == 4)
				i++;
			else
				game_over = 1;
			Wait(QUART_SEC);
		}
	}
	// If a wrong answer is given, signal the main() function to show a Loss()
	if (game_over == 0)
		return 1;
	// Else, signal that main() should show a 'correct response' and continue
	else
		return 0;
}

/******************************************************************************
/ This function blinks a green led 4 times quickly to indicate a correct answer
******************************************************************************/
void CorrectAnswer(void)
{
	int i;
	for (i = 0; i < 8; i++)
	{
		P9OUT = (P9OUT ^ P9_GREEN_LED);
		Wait(CENTI_SEC);
	}
}

/******************************************************************************
/ This function tells the player he/she has won by showing a sequence of rapid
/ LED blinks
******************************************************************************/
void Win(void)
{
	int i;
	for (i = 0; i < 3; i++)
	{
		P3OUT |= RED_LED;
		Wait(CENTI_SEC);
		P3OUT &= (~RED_LED);
		Wait(CENTI_SEC);
		P3OUT |= GREEN_LED;
		Wait(CENTI_SEC);
		P3OUT &= (~GREEN_LED);
		Wait(CENTI_SEC);
		P2OUT |= BLUE_LED;
		Wait(CENTI_SEC);
		P2OUT &= (~BLUE_LED);
		Wait(CENTI_SEC);
		P2OUT |= YELLOW_LED;
		Wait(CENTI_SEC);
		P2OUT &= (~YELLOW_LED);
		Wait(CENTI_SEC);
	}
}

/******************************************************************************
/ This function shows 3 slow red LED blinks to indicate a loss
******************************************************************************/
void Loss(void)
{
	int i;
	for (i = 0; i < 3; i++)
	{
		P3OUT |= RED_LED;
		Wait(HALF_SEC);
		P3OUT &= (~RED_LED);
		Wait(QUART_SEC);
	}
	Wait(ONE_SEC);
}

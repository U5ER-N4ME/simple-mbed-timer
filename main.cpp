#include <mbed.h> // nobody will forget it, right?

// about "ushort"
// to make codes beautiful, I use lower-case, whihc against requirements that I learned before
#undef ushort
#define ushort unsigned short

// a constant to denote the whole number of songs in case any song will be add or delect
#define MAX_SONG 3

Timer t; // a timer...

// Using 4041BH-33 (common cathode)
//					   A    B    C    D    E    F    G
BusOut segments_anaode(p10, p11, p12, p13, p14, p15, p16);
//                      0    1    2    3
BusOut segments_cathode(p17, p18, p19, p20);
// BusIn  keys(p5, p6, p7, p8); 
InterruptIn keyFunction(p9); // fn
// 			min+10     min+1      sec+10     sec+1
InterruptIn keyA1(p5), keyA2(p6), keyB1(p7), keyB2(p8);
PwmOut buzzer(p21); 

// a variable to determine the state: 0 - free, 1 - input time, 2 - select music, 3 - timing, 4 - playing
ushort timeState = 0;
ushort estArray[4] = {0, 0, 0, 1}, // an array to store the number on 4-digit 7-segment LED display
		est = 1, // calculated estArray
		song = 1; // id of song

// prototypes of functions
void interrupt_rise(void);
ushort inputTime(void);
ushort selectSong(void);
void displayBit(ushort, ushort, ushort);
void displayNum(ushort, ushort, ushort, ushort);
void displayTime(ushort);
void playNote(ushort, ushort);
void playMusic(ushort);

/** main function
 * 
 */
int main(void)
{
	// mute the buzzer
	buzzer = 0; 

	// attach interruptions
	keyFunction.rise(&interrupt_rise); 
	keyA1.rise(&interrupt_rise); 
	keyA2.rise(&interrupt_rise); 
	keyB1.rise(&interrupt_rise); 
	keyB2.rise(&interrupt_rise); 

	// start the timer
	t.start(); 

	while(true)
	{
		switch (timeState)
		{
			/* free */
			case 0: 
				displayNum(10, 10, 10, 10); // display: ----
				break; 
			
			/* input time */
			case 1: 
				displayNum(estArray[0], estArray[1], estArray[2], estArray[3]); // display estArray
				break; 
			
			/* select music */
			case 2: 
				displayBit(0, song, 20); // display selected song
				break; 
			
			/* timing */
			case 3: 
				// when time not reached, keep showing remaining time
				while(t.read() < est) displayTime(est); 
				// when timeout, jump to the next state (playing)
				timeState = 4; 
				break; 
			
			/* playing */
			case 4: 
				playMusic(song); 
				// after playing music, go to initial state (free)
				timeState = 0; 
				break; 
			
			default: break;
		} // end of switch
		
	} // end of while
} // end of main

/** the function to handle interruption
 * 
 */
void interrupt_rise(void)
{
	wait_ms(10); 
	/* when touch function key */
	if(keyFunction == 1) 
	{
		switch (timeState)
		{
			case 0: timeState=1; wait_ms(100); break; // go to next state
			case 1: // calculate est from estArray
					est = estArray[0]*60*10 + estArray[1]*60 + estArray[2]*10 + estArray[3]; 
					timeState=2; wait_ms(100); break; // go to next state
			case 2: t.reset(); // reset the timer in order to let it begin from zero
					timeState=3; wait_ms(100); break; // go to next state
			default:timeState=0; wait_ms(100); break; // go to default state
		}
	}

	/* Since following four "else if" statements are quite similar
	   I only place comments on keyA1
	 */

	/* when touch key A1: min+10 */
	else if(keyA1 == 1) 
	{
		switch (timeState)
		{
			case 1: 
				// min+10. if decade is bigger than 6 then return to 0
				estArray[0]++; if(estArray[0]>=6) estArray[0] = 0; 
				// the wait is used to prevent mistouch in case when some shake are there
				wait_ms(100); break;
			case 2: 
				// song id increase by 1. if bigger than maximum number then return to 1
				song++; if(song>MAX_SONG) song = 1;
				// the same to above one
				wait_ms(100); break;
			default: break;
		}
	}

	/* when touch key A2: min+1 */
	else if(keyA2 == 1) 
	{
		switch (timeState)
		{
			case 1: // timeState: input time
				estArray[1]++; if(estArray[1]>=10) estArray[1] = 0;
				wait_ms(100); break;
			case 2: // timeState: select music
				song++; if(song>MAX_SONG) song = 1;
				wait_ms(100); break;
			default: break;
		}
	}

	/* when touch key B1: sec+10 */
	else if(keyB1 == 1) 
	{
		switch (timeState)
		{
			case 1: 
				estArray[2]++; if(estArray[2]>=6) estArray[2] = 0;
				wait_ms(100); break;
			case 2: 
				song++; if(song>MAX_SONG) song = 1;
				wait_ms(100); break;
			default: break;
		}
	}

	/* when touch key B2: sec+1 */
	else if(keyB2 == 1) 
	{
		switch (timeState)
		{
			case 1: 
				estArray[3]++; if(estArray[3]>=10) estArray[3] = 0;
				wait_ms(100); break;
			case 2: 
				song++; if(song>MAX_SONG) song = 1;
				wait_ms(100); break;
			default: break;
		}
	}

}

/** select a bit of LED to display a number
 * 
 * @param bit the position that a number will appear: 0 - 3
 * @param number the number that should be shown: 0 - 10
 * @param time how long the number will exist (ms)
 * 
 */
void displayBit(ushort bit, ushort number, ushort time)
{
	// to check which bit should be illuminated
	// sorry to use a long and annoying expression, but it can be replaced by "switch" and so on
	segments_cathode = (bit==0) ? 1 : ((bit==1) ? 2 : ((bit==2) ? 4 : 8)); 

	// to check which number should be displayed
	//* this is not a public accepted encoding, just to simplify the program *
	switch (number)
	{										   // GFE DCBA
		case 0: segments_anaode = 0x40; break; // 100 0000, 0
		case 1: segments_anaode = 0x79; break; // 111 1001, 1
		case 2: segments_anaode = 0x24; break; // 010 0100, 2
		case 3: segments_anaode = 0x30; break; // 011 0000, 3
		case 4: segments_anaode = 0x19; break; // 001 1001, 4
		case 5: segments_anaode = 0x12; break; // 001 0010, 5
		case 6: segments_anaode = 0x02; break; // 000 0010, 6
		case 7: segments_anaode = 0x78; break; // 111 1000, 7
		case 8: segments_anaode = 0x00; break; // 000 0000, 8
		case 9: segments_anaode = 0x10; break; // 001 0000, 9
		case 10:segments_anaode = 0x3F; break; // 011 1111, -
	}
	wait_ms(time);
	
}

/** to display a 4-bit number
 * 
 * @param num3 the 4th bit: 0 - 10
 * @param num2 the 3rd bit: 0 - 10
 * @param num1 the 2nd bit: 0 - 10
 * @param num0 the 1st bit: 0 - 10
 * 
 */
void displayNum(ushort num3, ushort num2, ushort num1, ushort num0)
{
	displayBit(3, num3, 5);
	displayBit(1, num1, 5);
	displayBit(2, num2, 5);
	displayBit(0, num0, 5);
}

/** to display time remaining
 * 
 * @param estimate estimated time(second)
 * 
 */
void displayTime(ushort estimate)
{
	ushort timeRMN = estimate - t.read(); // get remaining time
	/*
	 * minute = timeRMN / 60 % 60
	 * minute(decade) = minute / 10 = timeRMN / 60 % 60 / 10
	 * minute(unit) = minute % 10 = timeRMN / 60 % 60 % 10 = timeRMN / 60 % 10
	 * 
	 * second = timeRMN % 60
	 * second(decade) = second / 10 = timeRMN % 60 / 10
	 * second(unit)	= second % 10 = timeRMN % 60 % 10 = timeRMN % 10
	 */
	//displayNum(((timeRMN/60)%60)/10,(timeRMN/60)%10 ,(timeRMN%60)/10 , timeRMN%10);
	displayNum(timeRMN/60%60/10,timeRMN/60%10 ,timeRMN%60/10 , timeRMN%10);
}

/** to play a single note
 * 
 * @param tune the id of note
 * @param time how long the note will exist (ms)
 * 
 * @note the param "time" is not exact, to be honest, because some other lines also need time
 */
void playNote(ushort tune, ushort time)
{
	/* 
	 * an array that store periods of tunes (in unit us)
	 * data source: https://pages.mtu.edu/~suits/notefreqs.html
	 */
	ushort lot[36] = {	0, 	// rest
					/*  do     re     m      fa     so     la     si    */
						15288, 13620, 12134, 11453, 10204, 9091,  8099, 	// C2 - B2 [ 1  - 7  ]
						7645,  6811,  6068,  5727,  5102,  4545,  4290, 	// C3 - B3 [ 8  - 15 ]
						3822,  3405,  3034,  2863,  2551,  2273,  2025, 	// C4 - B4 [ 16 - 22 ]
						1911,  1703,  1517,  1433,  1276,  1136,  1012, 	// C5 - B5 [ 23 - 29 ]
						956,   851,   758,   716,   638,   568,   506   	// C6 - B6 [ 30 - 36 ]
						};

	// ensure that is inside the range
	if(tune < 36 && tune !=0) 
	{
		buzzer = 0.5; // duty cycle: unmute the buzzer
		buzzer.period_us(lot[tune]);
		wait_ms(time);
		buzzer = 0; // duty cycle: mute the buzzer again

	}
	else if(tune == 0)
	{
		buzzer = 0; 
		wait_ms(time);
	}
	
}

/** to play a full music
 * 
 * @param id the order of a music
 * 
 */
void playMusic(ushort id)
{
	ushort* redirection; // see its usage after these arrays

	/* about the format of a song
	 * 
	 * music[0] tells the program approximately how long each note should last
	 * then the number keeps the same to the function "playNote"
	 * "999" denotes the end of a song
	 * 
	 */

	ushort music1[] = { 500, 
						1,  2,  3,  4,  5,  6,  7, 
						8,  9,  10, 11, 12, 13, 14, 
						15, 16, 17, 18, 19, 20, 21, 
						22, 23, 24, 25, 26, 27, 28, 
						29, 30, 31, 32, 33, 34, 35, 
						0, 0, 999};

	ushort music2[] = {	200, 
						23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 0,  0,  20, 20,
						23, 23, 23, 23, 20, 20, 20, 21, 22, 22, 22, 22, 18, 18, 18, 18, 
						21, 21, 21, 21, 20, 20, 20, 19, 20, 20, 20, 20, 16, 16, 16, 16, 
						17, 17, 17, 17, 17, 17, 17, 18, 19, 19, 19, 19, 19, 19, 19, 20, 

						21, 21, 21, 21, 22, 22, 22, 23, 24, 24, 24, 24, 20, 20, 20, 20, 
						25, 25, 25, 25, 24, 24, 24, 23, 24, 24, 24, 24, 20, 20, 20, 20, 
						23, 23, 23, 23, 22, 22, 21, 21, 22, 22, 22, 22, 18, 18, 18, 18, 
						21, 21, 21, 21, 20, 20, 20, 20, 19, 19, 19, 19, 20, 20, 20, 20, 

						16, 16, 16, 16, 23, 23, 23, 23, 22, 22, 22, 21, 20, 20, 20, 20, 
						20, 20, 20, 20, 25, 25, 25, 25, 25, 25, 25, 25, 24, 24, 23, 23, 
						23, 23, 23, 23, 22, 22, 23, 23, 24, 24, 24, 24, 20, 20, 20, 20, 
						20, 20, 20, 20, 20, 20, 20, 20, 23, 23, 23, 23, 23, 23, 23, 23, 

						22, 22, 21, 21, 20, 20, 21, 21, 22, 22, 22, 22, 18, 18, 18, 18, 
						18, 18, 18, 18, 0,  0,  0,  0,  23, 23, 23, 23, 21, 21, 21, 22, 
						23, 23, 23, 23, 21, 21, 21, 22, 23, 23, 23, 23, 21, 21, 21, 21, 
						23, 23, 23, 23, 26, 26, 26, 26, 26, 26, 26, 26, 0,  0,  0,  0,  

						26, 26, 26, 26, 26, 26, 26, 26, 25, 25, 24, 24, 23, 23, 24, 24, 
						25, 25, 25, 25, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 
						24, 24, 24, 24, 24, 24, 24, 24, 23, 23, 22, 22, 21, 21, 22, 22, 
						23, 23, 23, 23, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 

						23, 23, 23, 23, 22, 22, 21, 21, 20, 20, 20, 20, 16, 16, 16, 16, 
						23, 23, 23, 23, 22, 22, 22, 21, 20, 20, 20, 20, 0,  0,  0,  0, 
						23, 23, 23, 23, 22, 22, 21, 21, 20, 20, 20, 20, 16, 16, 16, 16, 
						21, 21, 21, 21, 21, 21, 22, 22, 23, 23, 23, 23, 23, 23, 23, 23, 

						999
						};

	ushort music3[] = {	250, 
						0,  0,  0,  0,  21, 21, 21, 21, 21, 21, 21, 21, 23, 23, 23, 23, 
						23, 23, 23, 23, 25, 25, 25, 25, 24, 24, 24, 24, 27, 27, 27, 27, 

						28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 24, 23, 22, 20, 
						21, 21, 21, 25, 24, 24, 23, 23, 22, 22, 22, 22, 24, 24, 23, 22,

						21, 21, 21, 30, 29, 30, 29, 30, 21, 21, 21, 30, 29, 30, 29, 30, 
						21, 21, 21, 25, 24, 24, 23, 23, 22, 22, 22, 22, 24, 24, 23, 22,

						21, 21, 21, 30, 29, 30, 29, 30, 21, 21, 21, 30, 29, 30, 29, 30, 
						21, 21, 21, 25, 24, 24, 23, 23, 22, 22, 22, 22, 24, 24, 23, 22,

						21, 21, 21, 30, 29, 30, 29, 30, 21, 21, 21, 30, 29, 30, 29, 30, 
						21, 21, 21, 21, 25, 25, 25, 25, 24, 24, 24, 24, 27, 27, 27, 27, 

						28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 24, 23, 22, 20, 
						21, 21, 21, 25, 24, 24, 23, 23, 22, 22, 22, 22, 24, 24, 23, 22,

						21, 21, 21, 30, 29, 30, 29, 30, 21, 21, 21, 30, 29, 30, 29, 30, 
						21, 21, 21, 25, 24, 24, 23, 23, 22, 22, 22, 22, 24, 24, 23, 22,

						21, 21, 21, 30, 29, 30, 29, 30, 21, 21, 21, 30, 29, 30, 29, 30, 
						21, 21, 21, 21, 25, 25, 25, 25, 24, 24, 24, 24, 27, 27, 27, 27, 

						28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 
						999
						};

	
	switch (id)
	{
		/* 
		 * since the identifier of an array shows only its first value's position in RAM
		 * we can give its value to "redirection" then it is the same to music{id}
		 * by doing this will decrease the number of codes
		 * 
		 */
		case 1: redirection = music1; break;
		case 2: redirection = music2; break;
		case 3: redirection = music3; break;
		default: redirection = music1; break; 
	}
	// play music by playing notes one by one
	for(ushort i = 1; (redirection[i] < 999) && (timeState == 4); i++) playNote(redirection[i], redirection[0]);

}

// I dislike 404 :(, so this is the 405th line

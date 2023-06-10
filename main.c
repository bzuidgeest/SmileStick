#include <stdio.h>
#include "pico/stdlib.h"
#include "core2.h"

#define HIGH true
#define LOW false

#define UART_ID uart1
#define BAUD_RATE 4800
#define UART1_TX_PIN 4
#define UART1_RX_PIN 5

//#define CTS PICO_DEFAULT_LED_PIN
#define CTS 6
#define RTS 7

char data = 0x00;

// What is the maximum number of bytes per transmission? 
// Haven't seen more then two byte per lowering of RTS.
char packet[4] = { 0, 0, 0, 0 };
char* packetPointer = packet;


bool repeating_timer_callback(struct repeating_timer *t) {
	
	// Wait for any current activity to finish
	while (gpio_get(RTS) == false && gpio_get(CTS) == true);

	printf("Sending 20s wake up!");
    gpio_put(CTS, 1);
	uart_putc_raw(UART_ID, 0xB9);
	busy_wait_ms(3);
	gpio_put(CTS, 0);
	busy_wait_ms(15);
    return true;
}


// RX interrupt handler
void on_uart_rx() {

	// Read all bytes offered by the controller in one go.
    while (uart_is_readable(UART_ID) || gpio_get(RTS) == LOW) {
        
		*packetPointer = uart_getc(UART_ID);

		printf("in: %d\n", *packetPointer);
		data = *packetPointer;
		packetPointer++;
		
		if (gpio_get(RTS) == HIGH)
		{
			stickData_t stickData;
			memcpy(stickData.message, packet, 2);
			stickData.length = 2;

			queue_add_blocking(&call_queue, &stickData);

			// reset packet pointer
			packetPointer = packet;
			// if RTS is no longer low pull CTS to low.
			gpio_put(CTS, 0);
		}
    }
}

void rts_callback(uint gpio, uint32_t events) {
    // Put the GPIO event(s) that just happened into event_str
    // so we can print it
	if (gpio == RTS)
	{
		if (events = GPIO_IRQ_EDGE_FALL)
		{
			gpio_put(CTS, 1);
    		printf("RTS Fall");
		}
	}
}

bool sendByte(char data)
{
	// Check if no transmission is ongoing
	// Not sure if this is needed. Can we send while the controller is transmitting?
	//while (gpio_get(RTS) == LOW && gpio_get(CTS) == HIGH);

    gpio_put(CTS, HIGH);
	uart_putc_raw(UART_ID, data);
	uart_tx_wait_blocking(UART_ID);
	//busy_wait_ms(3);

	// Set CTS low if no transmission from the controller is ongoing.
	if (gpio_get(RTS) == HIGH)
		gpio_put(CTS, LOW);

	// Remove this wait?
	//busy_wait_ms(1);
}

int main() {

	char state = 0;
	int leds = 0x60;
	bool blink = true;
	bool startup = true;
	struct repeating_timer timer;

    stdio_init_all();


    queue_init(&call_queue, sizeof(stickData_t), 10);
    //queue_init(&results_queue, sizeof(int32_t), 2);

    multicore_launch_core1(coreUSBMain);

	// CTS
 	gpio_init(CTS);
    gpio_set_dir(CTS, GPIO_OUT);
	gpio_put(CTS, 0);
	gpio_pull_down(CTS);
	
	// RTS
	gpio_init(RTS);
    gpio_set_dir(RTS, GPIO_IN);
	gpio_pull_up(RTS);


	uart_init(UART_ID, BAUD_RATE);
	uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);

    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART1_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART1_RX_PIN, GPIO_FUNC_UART);


    //uart_set_hw_flow(UART_ID, true, true);
	// Turn off FIFO's - we want to do this character by character
    uart_set_fifo_enabled(UART_ID, false);

   	// Set up a RX interrupt
    // We need to set up the handler first
    // Select correct interrupt for the UART we are using
	int UART_IRQ = UART_ID == uart0 ? UART0_IRQ : UART1_IRQ;

    // And set up and enable the interrupt handlers
    irq_set_exclusive_handler(UART_IRQ, on_uart_rx);
    irq_set_enabled(UART_IRQ, true);

    // Now enable the UART to send interrupts - RX only
    uart_set_irq_enables(UART_ID, true, false);
	
	gpio_set_irq_enabled_with_callback(RTS, GPIO_IRQ_EDGE_FALL, true, &rts_callback);


    // Send out a character without any conversions
    //uart_putc_raw(UART_ID, 'A');

    // Send out a character but do CR/LF conversions
    //uart_putc(UART_ID, 'B');

    // Send out a string, with CR/LF conversions
    //uart_puts(UART_ID, " Hello, UART!\n");


	// 0x78 only send once on "startup", before that no stick data
	// 0xBX about every 20 ms. No idea about function. keep-alive?
    while (true) {
		if (data == 0x55 && gpio_get(RTS) == true)
		{
			sleep_ms(20);

			gpio_put(CTS, 1);
			uart_putc_raw(UART_ID, 0xE6);
			sleep_ms(3);
			gpio_put(CTS, 0);
			sleep_ms(15);

			gpio_put(CTS, 1);
			uart_putc_raw(UART_ID, 0xD6);
			sleep_ms(3);
			gpio_put(CTS, 0);
			sleep_ms(15);

			gpio_put(CTS, 1);
			uart_putc_raw(UART_ID, leds);
			sleep_ms(3);
			gpio_put(CTS, 0);
			sleep_ms(15);

			data = 0x00;

			if (startup == true)
			{
				sleep_ms(200);

				if (gpio_get(RTS) == true)
				{
					gpio_put(CTS, 1);
					uart_putc_raw(UART_ID, 0x78);
					sleep_ms(3);
					gpio_put(CTS, 0);
					sleep_ms(15);
					startup = false;

					gpio_put(CTS, 1);
					uart_putc_raw(UART_ID, 0xB9);
					sleep_ms(3);
					gpio_put(CTS, 0);
					sleep_ms(15);
					startup = false;

					
					// repeat the 0xBX value every 20 seconds.
					// SDK uses negative values to indicate callback execution time should not affect the interval
					add_repeating_timer_ms(-20000, repeating_timer_callback, NULL, &timer);
				}

			}
		}

		if (gpio_get(RTS) == true)
		{
			gpio_put(CTS, 1);
			leds = blink ? 0x61 : 0x60;
			blink = !blink;
			uart_putc_raw(UART_ID, leds);
			sleep_ms(3);
			gpio_put(CTS, 0);
			sleep_ms(15);
			
		}
		sleep_ms(200);
    }
    return 0;
}




/*


4.1.23.4.18. uart_set_hw_flow
static void uart_set_hw_flow (uart_inst_t *uart,
bool cts,
bool rts)
Set UART flow control CTS/RTS.
Parameters
• uart UART instance. uart0 or uart1
• cts If true enable flow control of TX by clear-to-send input
• rts If true enable assertion of request-to-send output by RX flow control

uart1 gpio7 RTS
gpio6  CTS



https://vtech.pulkomandy.tk/doku.php?id=controllers

Controller port

6 pin mini DIN (like PS/2 keyboards and mouse). It is a serial port at 4800 baud, 8N1.

Pinout (using the standard numbering for mini-DIN connectors) :

    VCC
    CTS (from V.Smile)
    Tx (from V.Smile)
    GND
    Rx (from controller)
    RTS (from controller)

PulseView captures for some controllers

PulseView captures of booting with no controller, with a joystick (both with "Le Roi Lion") and with a keyboard (with "Clavier Tip Tap" cartridge)
Flow control

The CPU has a single UART that is used to communicate with both controllers. This requires flow control to make sure the two controllers don't try to communicate at the same time.

A controller is activated (both for transmission and reception) by setting its CTS pin high. Controllers can request attention from the console when they have data to send using their RTS line.

The RTS line changes can be detected using IRQ5. Events on the UART (transmit or receive complete) can be detected using IRQ3.

The general way to handle this is as follows, starting from an idle state with all CTS low and no pending data transfers

    If a controller has its RTS pin low, select it by setting the corresponding CTS high
    The controller will start transmitting data, receive that from the Rx register until “Rx ready” is cleared
    If needed, send a reply to the controller
    Make sure the reply is completely sent (“Tx buffer empty” in status register)
    Wait until RTS goes high (the controller has nothing to send anymore)
    Put CTS low again
    Wait a little (in case the controller was sending something just as you set CTS low) and read a possible last byte from the UART
    You are back to idle state

It is possible to send something to a controller even if it was not requesting RTS:

    Select the controller by setting the corresponding CTS high
    Send data to the controller
    Make sure the reply is completely sent (“Tx buffer empty” in status register)
    Check that RTS did not become low
    Put CTS high again
    Back to idle state

The controller keeps RTS down as long as it has more bytes to send.
Messages from the controller

When idle (no buttons touched), the console sends a byte every 20ms, it seems to be partially random. I've seen E6, D6, or 96.

Every second the controller sends 55 if nothing else is happening.
Common to joystick, dance mat and keyboard
Button 	Press 	Release
OK 	A1 	A0
Quit 	A2 	A0
Help 	A3 	A0
ABC 	A4 	A0
Idle (nothing) 	55
Joystick

The joystick has 5 levels of precision in each direction. For example, C3 is “slightly up”, C7 is “all the way up”.

The 4 color buttons are allocated one bit each in the 9x range so it's possible to manage multiple of them being pressed at once.

The other buttons are Ax with x just being the button number, so it's not possible to handle multiple of them being pressed at the same time.
Joystick
Button 	Press 	Release
Green 	91 	90
Blue 	92 	90
Yellow 	94 	90
Red 	98 	90
Up 	C0 83 to C0 87 	C0 80
Down 	C0 8B to C0 8F 	C0 80
Left 	CB 80 to CF 80 	C0 80
Right 	C3 80 to C7 80 	C0 80
Dance mat

Every press sends at least a “joystick position” 2-byte pair, and possibly an extra byte for the button itself (some buttons report as joystick moves, other as separate buttons). The mapping is not at all compatible with the joystick and seems a bit random. Note that for example 8B and 8D are different buttons, where on the joystick it would be different positions in the same direction.
Dance mat
Button 	Press 	Release
1 / Red 	C0 8B 	C0 80
2 / Up 	92 C0 80 	90 C0 80
3 / Yellow 	CB 80 	C0 80
4 / Left 	C0 8D 	C0 80
5 / Middle 	91 C0 80 	90 C0 80
6 / Right 	CD 80 	C0 80
7 / Blue 	A4 C0 80 	A0 C0 80
8 / Down 	94 C0 80 	90 C0 80
9 / Green 	98 C0 80 	90 C0 80
Smart Keyboard (Clavier Tip Tap)

(sorry, I have the French/azerty version so key labels may not match up. The table is in row/column order)
Row 1 (top) 	Row 2 	Row 3 	Row 4 	Row 5
Key 	Code 	Key 	Code 	Key 	Code 	Key 	Code 	Key 	Code
Esc 	A2 	Dactylo 	22 	Caps 	1A 	Shift 	A9/AA 	Player 1 	04
1 	33 	A 	23 			W 	13 	Help 	A3
2 	34 	Z 	24 	Q 	1B 	X 	14 	Symbol 	2C
3 	35 	E 	25 	S 	1C 	C 	15 	Space 	05
4 	37 	R 	27 	D 	1D 	V 	17 	Player 2 	0E
5 	36 	T 	26 	F 	1F 	B 	16 	Left 	06
6 	30 	Y 	20 	G 	1E 	N 	08 	Down 	0F
7 	31 	U 	21 	H 	18 	, 	11 	Right 	0D
8 	3E 	I 	3A 	J 	19 	; 	0C 		
9 	3F 	O 	3B 	K 	0A 	: 	2F 		
0 	38 	P 	3C 	L 	0B 	Up 	12 		
º 	29 	¨ 	2A 	M 	01 				
Backspace 	39 	Erase 	3D 	Enter 	A1 				

    Escape is mapped to Quit and works the same
    Help is mapped to Help and works the same
    Enter is mapped to OK and works the same
    Shift sends A9 on press and AA on release
    Other keys send their code on press, and code | C0 on release (so no code will be in the 90-AF range for either press or release to not conflict with the special buttons)

The joystick at the bottom of the keyboard is similar to the normal Joystick but uses different values (to avoid clashing with the keyboard release keycode range). There does not seem to be different values possible, it's all on or all off.

    Left: 7F 80
    Right: 77 80
    Down: 70 8F
    Up: 70 87

Boot sequence:

    Keyboard sends 52 52 52
    Console sends 0x02 0x02 0xE6 0xD6 0x60
    Keyboard sends language code
    Console: 0x70
    Keyboard: 0xBA

The language codes:

    0x40: US
    0x41: UK
    0x42: French
    0x44: German

Commands from the console

61, 62, 64 and 68 are sent in reply to color buttons presses. I suspect this controls the lights in the buttons. 60 is sent to turn the light off.

These are repeated every 20ms. After the controller sends 55 (idle), the game also returns to its idle reply (E6 followed by D6 for example) every 20ms.


https://github.com/retrospy/RetroSpy/blob/11525c9ed5e92da9c91c7adbfdad396e5f2ec5c5/firmware/sketches/VSmile.cpp
https://retro-spy.com/wiki/vtech-vsmile-on-teensy-getting-started/

Mini-Din 6 Pin	Teensy (3.5) Digital Pin
1	Not Connected
2	Not Connected
3	Not Connected
4	0 (RX1)
5	Not Connected
6	9 (RX2)

from retrospy firmware:

//
// VSmile.cpp
//
// Author:
//       Christopher "Zoggins" Mallery <zoggins@retro-spy.com>
//
// Copyright (c) 2020 RetroSpy Technologies
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "VSmile.h"

#if defined(__arm__) && defined(CORE_TEENSY) && defined(ARDUINO_TEENSY35)

static bool redButton = false;
static bool yellowButton = false;
static bool blueButton = false;
static bool greenButton = false;
static bool enterButton = false;
static bool helpButton = false;
static bool exitButton = false;
static bool learningZoneButton = false;
static byte x = 0;
static byte y = 0;

void VSmileSpy::setup() {
	
	Serial1.begin(4800);
	Serial2.begin(4800);
	
}

void VSmileSpy::loop() {
	updateState();
#if !defined(DEBUG)
	writeSerial();
#else
	debugSerial();
#endif
	delay(5);
}

void VSmileSpy::writeSerial() {
	Serial.write(redButton ? ONE : ZERO);
	Serial.write(yellowButton ? ONE : ZERO);
	Serial.write(blueButton ? ONE : ZERO);
	Serial.write(greenButton ? ONE : ZERO);
	Serial.write(enterButton ? ONE : ZERO);
	Serial.write(helpButton ? ONE : ZERO);
	Serial.write(exitButton ? ONE : ZERO);
	Serial.write(learningZoneButton ? ONE : ZERO);
	Serial.write(x == 10 ? 11 : x);
	Serial.write(y == 10 ? 11 : y);
	Serial.write(SPLIT);
}

void VSmileSpy::debugSerial() {
	Serial.print(redButton ? "1" : "0");
    Serial.print(yellowButton ? "1" : "0");
    Serial.print(blueButton ? "1" : "0");
    Serial.print(greenButton ? "1" : "0");
    Serial.print(enterButton ? "1" : "0");
    Serial.print(helpButton ? "1" : "0");
    Serial.print(exitButton ? "1" : "0");
    Serial.print(learningZoneButton ? "1" : "0");
    Serial.print("|");
    Serial.print(x);
    Serial.print("|");
    Serial.println(y);
}

void VSmileSpy::updateState() {
	if (Serial1.available())
	{
		char c = Serial1.read();
		if ((c & 0b11110000) == 0b11000000)
		{
			x = (c & 0b00001111);
		}
		if ((c & 0b11110000) == 0b10000000)
		{
			y = (c & 0b00001111);
		}
		if ((c & 0b11110000) == 0b10100000)
		{
			if ((c & 0b00001111) == 0b00000011)
			{
				helpButton = true;
				enterButton = false;
				exitButton = false;
				learningZoneButton = false;
			}
			else if ((c & 0b00001111) == 0b00000001)
			{
				helpButton = false;
				enterButton = true;
				exitButton = false;
				learningZoneButton = false;
			}
			else if ((c & 0b00001111) == 0b00000010)
			{
				helpButton = false;
				enterButton = false;
				exitButton = true;
				learningZoneButton = false;
			}
			else if ((c & 0b00001111) == 0b00000100)
			{
				helpButton = false;
				enterButton = false;
				exitButton = false;
				learningZoneButton = true;
			}
			else 
			{ 
				helpButton = false;
				enterButton = false;
				exitButton = false;
				learningZoneButton = false;                 
			}
		}
	}
	if (Serial2.available())
	{
		char c = Serial2.read();
		if ((c & 0b11110000) == 0b01100000)
		{
			redButton = ((c & 0b00001000) != 0) ? true : false;
			yellowButton = ((c & 0b00000100) != 0) ? true : false;
			blueButton = ((c & 0b00000010) != 0) ? true : false;
			greenButton = ((c & 0b00000001) != 0) ? true : false;
		}
	}
}

#else

void VSmileSpy::setup() {
}

void VSmileSpy::loop() {
}

void VSmileSpy::writeSerial() {
}

void VSmileSpy::debugSerial() {
}

void VSmileSpy::updateState() {
}

#endif

const char* VSmileSpy::startupMsg()
{
	return "V.Smile";
}



*/
/*
 * Copyright (c) 2012 Erik Bosman
 *
 * Permission  is  hereby  granted,  free  of  charge,  to  any  person
 * obtaining  a copy  of  this  software  and  associated documentation
 * files (the "Software"),  to deal in the Software without restriction,
 * including  without  limitation  the  rights  to  use,  copy,  modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the
 * Software,  and to permit persons to whom the Software is furnished to
 * do so, subject to the following conditions:
 *
 * The  above  copyright  notice  and this  permission  notice  shall be
 * included  in  all  copies  or  substantial portions  of the Software.
 *
 * THE SOFTWARE  IS  PROVIDED  "AS IS", WITHOUT WARRANTY  OF ANY KIND,
 * EXPRESS OR IMPLIED,  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY,  FITNESS  FOR  A  PARTICULAR  PURPOSE  AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM,  DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT,  TORT OR OTHERWISE,  ARISING FROM, OUT OF OR IN
 * CONNECTION  WITH THE SOFTWARE  OR THE USE  OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * (http://opensource.org/licenses/mit-license.html)
 */


#define SCL_PIN (2)         /* keep them adjacent */
#define SDA_PIN (SCL_PIN+1)

#define SCL_BIT (1<<SCL_PIN)
#define SDA_BIT (1<<SDA_PIN)
#define INMASK (SCL_BIT|SDA_BIT)

#define INMASK_SHIFT(x) ( ( x & INMASK ) >> SCL_PIN )

#define BUFSIZE (1536 * 8)

#define MSG_START (2)

#define SAMPLE_MODE (0)
#define I2C_MODE (1)

unsigned char buf[BUFSIZE/8];
int bufsize = BUFSIZE;
int mode = SAMPLE_MODE;

void setup(void)
{
	Serial.begin(57600);

	DDRD &= ~INMASK;
}

void sample_capture(void)
{
	uint8_t state;

	int i, index;

	for (i=0; i*8<bufsize; i++)
		buf[i]=0;

	/* wait for interesting bits */

	while(PORTD & SDA_BIT);

	/* capture */

	for (i=0; i<bufsize; i+=2)
		buf[ i >> 3 ] |= INMASK_SHIFT(PORTD) << ( i & 7 );
}

void sample_print(void)
{
	uint8_t state;
	int i;

	for (i=0; i<bufsize; i+=2)
	{
		state = buf[ i >> 3 ] >> ( i & 7 );

		Serial.print(i >> 1);
		Serial.print( (state & 2) ? ", 1" : ", 0" );
		Serial.print( (state & 1) ? ", 1" : ", 0" );
		Serial.println();
	}
}

void i2c_capture(void)
{
	uint8_t state, last;
	int i;

	for (i=0; i*8<bufsize; i++)
		buf[i]=0;

	i=0;

	state = PORTD & INMASK;
	for( ;; )
	{
		/* wait for the start of a message */
		for(;;)
		{
			last = state;
			state = PORTD & INMASK;
			if ( (last == (SCL_BIT|SDA_BIT)) && (state == SCL_BIT) )
				break;
		}

		/* new message */

		buf[ i >> 3 ] |= MSG_START << ( i & 7 );

		while ( PORTD & SCL_BIT );

		for(;;)
		{
			while ( ! ((last = (PORTD&INMASK)) & SCL_BIT) );
			while ( (state = (PORTD&INMASK)) == last );

			if (state & SCL_BIT) /* end of message */
				break;

			buf[ i >> 3 ] |= (last & SDA_BIT ? 1 : 0) << ( i & 7 );
			i+=2;

			if (i >= bufsize)
				return;
		}
	}
}

void i2c_print(void)
{
	uint8_t state;
	int i;

	for (i=0; i<bufsize; i+=2)
	{
		state = buf[ i >> 3 ] >> ( i & 7 );

		if (state & MSG_START)
			Serial.print("\nMessage: ");

		Serial.print( (state & 1) ? "1" : "0" );
	}

	Serial.print("...\n");
}

void loop(void)
{
	int c;

	Serial.print("Current mode: ");
	Serial.println(mode==I2C_MODE?"I2C":"Sample");
	Serial.print("Current sample size: ");
	Serial.println(bufsize >> 1);

	Serial.println("Available commands: (r)un, switch (m)ode, (i)ncrease buffer, (d)ecrease buffer");

	while ( (c = Serial.read()) < 0 );

	if (c == 'm')
		mode = I2C_MODE+SAMPLE_MODE-mode;
	else if ( (c == 'i') && (bufsize+0x400 <= BUFSIZE) )
		bufsize += 0x400;
	else if ( (c == 'd') && (bufsize-0x400 > 0) )
		bufsize -= 0x400;
	else if ( c == 'r' )
	{
		if (mode == I2C_MODE)
		{
			i2c_capture();
			i2c_print();
		}
		else
		{
			sample_capture();
			sample_print();
		}
	}
}

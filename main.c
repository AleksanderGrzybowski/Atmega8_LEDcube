#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#include "595.h"

void delay_ms(uint16_t count) {
	while (count--) {
		_delay_ms(1);
	}
}
void delay_us(uint16_t count) {
	while (count--) {
		_delay_us(1);
	}
}

/* Multiplexing */

typedef struct cube {
	char tab[4][16]; // 4 layers, 16 in each layer
} cube;

cube main_cube;
volatile int current_layer = 0;
volatile int pwm_iter = 0;

/* Pushing data to shift registers. There are 3 74595's
 * connected in series (Qs - DS), two of them drive 16 N-transistors
 * that turn on vertical columns, one of them drives P-transistors
 * that turn on layers.
 */

ISR(TIMER0_OVF_vect) {
	TCNT0 = 210;
	// TEST ROUTINE, WORKS:
	//	cur++;
	//	transmit(cur);
	//	transmit(2);
	//	commit();

	// 'ripple' through
	pwm_iter++;
	if (pwm_iter == 16) {
		pwm_iter = 0;
		current_layer++;
	}
	if (current_layer == 4) {
		current_layer = 0;
	}

	// layers
	char tosend_pmos = 0xff;
	tosend_pmos &= ~(2 << current_layer); // P-transistors active-low

	// columns
	char tosend_left = 0, tosend_right = 0;

	int r;
	for (r = 0; r < 8; ++r) {
		if (main_cube.tab[current_layer][r] > pwm_iter)
			tosend_left |= (1 << r);
	}
	for (r = 8; r < 16; ++r) {
		if (main_cube.tab[current_layer][r] > pwm_iter)
			tosend_right |= (1 << (r - 8));
	}

	transmit(tosend_pmos);
	transmit(tosend_right);
	transmit(tosend_left);
	commit();
}

void inline set1(int layer, int pos) {
	main_cube.tab[layer][pos] = 16;
}

void inline set0(int layer, int pos) {
	main_cube.tab[layer][pos] = 0;
}

void inline set(int layer, int pos, int brig) {
	main_cube.tab[layer][pos] = brig;
}

void fill(int what) {
	int i, j;
	for (i = 0; i < 4; ++i)
		for (j = 0; j < 16; ++j)
			set(i, j, what);

}

void inline setLayer(int layer, int brig) {
	int i;
	for (i = 0; i <= 15; ++i) {
		main_cube.tab[layer][i] = brig;
	}
}

#define TEST_DELAY 200

void test_all() {
	int i, j;

	fill(0);

	for (i = 0; i < 4; ++i) {
		for (j = 0; j < 16; ++j) {
			set1(i, j);
			delay_ms(TEST_DELAY);
			set0(i, j);
		}
	}
	fill(15);
	for (i = 0; i < 4; ++i) {
		for (j = 0; j < 16; ++j) {
			set0(i, j);
			delay_ms(TEST_DELAY);
			set1(i, j);
		}
	}
	fill(0);

	for (i = 0; i < 4; ++i) {
		for (j = 0; j < 16; ++j) {
			set1(i, j);
		}
		delay_ms(TEST_DELAY * 10);
	}
	for (i = 0; i < 4; ++i) {
		for (j = 0; j < 16; ++j) {
			set0(i, j);
		}
		delay_ms(TEST_DELAY * 10);
	}

	fill(15);
	delay_ms(1000);
	fill(0);
	delay_ms(1000);
	fill(15);
	delay_ms(1000);
	fill(0);
	delay_ms(1000);
}

//////////////////////////////////////////////////
void e_full_bright() {
	fill(15);
}
void e_little_bright() {
	fill(1);
}

void e_smooth_dimming(int count, int del) {
	int i;

	int c;
	for (c = 0; c < count; c++) {
		for (i = 0; i < 4; ++i) {
			fill(i);
			delay_ms(del);
		}
		for (i = 4; i >= 0; --i) {
			fill(i);
			delay_ms(del);
		}
	}
	fill(0);

}

void e_raindrops(int count, int del) {

	int i;
	int brightab[] = { 1, 2, 5, 15 };

	int c;
	for (c = 0; c < count; c++) {
		int rnd = TCNT1 % 16;
		for (i = 3; i >= 0; --i) {
			set(i, rnd, brightab[i]);
			delay_ms(del);
			set(i, rnd, 0);
		}
	}
}

void e_snake(int count, int del) {
	int x, y, z;
	int spos = TCNT1 % 64;
	x = spos / 16;
	y = spos / 4 % 4;
	z = spos % 4;

	int i;
	for (i = 0; i < count; ++i) {
		int rnd = TCNT1 % 6;

		switch (rnd) {
		case 0: // w lewo
			x--;
			if (x == -1)
				x = 1;
			break;
		case 1: // w prawo
			x++;
			if (x == 4)
				x = 2;
			break;
		case 2: // w głąb
			y++;
			if (y == 4)
				y = 2;
			break;
		case 3: // w moją stronę
			y--;
			if (y == -1)
				y = 1;
			break;
		case 4: // w dół
			z--;
			if (z == -1)
				z = 1;
			break;
		case 5: // do góry
			z++;
			if (z == 4)
				z = 2;
			break;
		}
		set1(z, 4 * x + y);
		delay_ms(del);
		set0(z, 4 * x + y);

	}
}

void e_random(int repeats, int count, int del) {
	int i, c;
	fill(0);

	for (c = 0; c < repeats; ++c) {
		for (i = 0; i < count; ++i) {
			int x, y, z;
			int spos = TCNT1 % 64;
			x = spos / 16;
			y = spos / 4 % 4;
			z = spos % 4;

			set1(z, 4 * x + y);
			delay_ms(del);
		}
		fill(0);
	}
}

void e_layers(int del) {
	fill(0);
	delay_ms(del);

	setLayer(0, 15);
	delay_ms(del);
//	setLayer(0, 0);

	setLayer(1, 15);
	delay_ms(del);
//	setLayer(1, 0);

	setLayer(2, 15);
	delay_ms(del);
//	setLayer(2, 0);

	setLayer(3, 15);
	delay_ms(del);
//	setLayer(3, 0);

	fill(0);
	delay_ms(del);

}
int main(void) {

	DDRC = 0xff;

	TIMSK = (1 << TOIE0);
	TCCR0 = (1 << CS01) | (1 << CS00); // 64 jak na razie
	sei();

	TCCR1B |= (1 << CS12); // do losowania

	int ii;
	while (1) {
		e_smooth_dimming(5, 20);
		e_random(10, 8, 50);

		e_raindrops(20, 50);
	}

}

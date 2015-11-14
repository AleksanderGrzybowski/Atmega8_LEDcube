#include "595.h"

void DS_1() {
	PORTC |= (1 << PC0);
}
void DS_0() {
	PORTC &= ~(1 << PC0);
}
void SHCP_1() {
	PORTC |= (1 << PC1);
}
void SHCP_0() {
	PORTC &= ~(1 << PC1);
}
void STCP_1() {
	PORTC |= (1 << PC2);
}
void STCP_0() {
	PORTC &= ~(1 << PC2);
}

void transmit(char b) {
	int cur;
	for (cur = 128; cur != 0; cur /= 2) {
		if (b & cur) {
			DS_1();
		} else {
			DS_0();
		}

		SHCP_1();
//		delay_us(1); // should not be needed in typical case
		SHCP_0();
//		delay_us(1);

	}
}

void commit() {
	STCP_1();
//	delay_us(1);
	STCP_0();
//	delay_us(1);
}

/*
 * PRNG.c
 *
 * Created: 8/23/2017 1:47:48 PM
 *  Author: Gogol
 */ 
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdint.h>
#include "PRNG.h"

uint8_t state[1];

// Inits the system. Seed must not be 0.
void Initialize(uint8_t seed)
{
	state[0] = seed;
}

// Algorithm adapted from:
// http://www.arklyffe.com/main/2010/08/29/xorshift-pseudorandom-number-generator/
// https://en.wikipedia.org/wiki/Xorshift
uint8_t xorshift8(uint8_t state[static 1])
{
	uint8_t x = state[0];
	x ^= x << 7;
	x ^= x >> 5;
	x ^= x << 3;
	state[0] = x;
	return x;
}

// Returns a Random Number.
uint8_t getRandomNumber()
{
	return xorshift8(state);
}

// Returns one of the following sequences:
// 1) 0000 0001
// 2) 0000 0010
// 3) 0000 0100
// 4) 0000 1000
uint8_t getRandomLed()
{
	uint8_t rn = getRandomNumber();
	return (0x01 << (rn % 4));
}

/*
 * PRNG.h
 * Generates a pseduorandom number using the XORshift algorithm
 * 
 *  Created: 8/23/2017 1:47:03 PM
 *  Author: Gogol
 */ 


#ifndef PRNG_H_
#define PRNG_H_

// Re-init with a given seed
void Initialize(uint8_t  seed);

// Returns a random number between 0 and 255
uint8_t getRandomNumber();

// Returns a random LED pattern from the following
// 1) 0000 0001
// 2) 0000 0010
// 3) 0000 0100
// 4) 0000 1000
uint8_t getRandomLed();

#endif /* PRNG_H_ */

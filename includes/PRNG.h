/*
 * PRNG.h
 *
 * Created: 8/23/2017 1:47:03 PM
 *  Author: Gogol
 */ 


#ifndef PRNG_H_
#define PRNG_H_

// Re-init with a given seed
void Initialize(uint8_t  seed);

uint8_t getRandomNumber();

uint8_t getRandomLed();

#endif /* PRNG_H_ */
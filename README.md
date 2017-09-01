# EE120B Custom Lab

In this lab I created an copy of the Simon game(https://www.youtube.com/watch?v=4YhVyt4q5HI). This game was designed test the memory of the player. My implementation generates a random sequence using the XorShift algorithm adapted for 8 bit numbers. It then plays back this sequence by flashing an LED and playing a sound. Then it waits for the user to input the same sequence using the buttons. When the user pushes a button it also plays a sound. Once the player beats all 9 levels they win! However, any single mistake will cause the player to instantly lose.

In this lab I had to get very familiar with the capibilities of the ATmega1284P including its various onboard features such as the timers and the pwm. I also had to use several different banks of its I/O for everything from the input buttons to the LEDS to the LCD display. In this project I used pins A1-A4 as the LEDS control pins, pins B0-B4 as the button pins, each one hooked up to a different push button, and all of PORTC and pin D7 to control an LCD display. The LCD contrast is also controlled by a potentiometer. I also used a piezo buzzer to generate sound.

A video of my game in action can be found here(https://drive.google.com/open?id=0B7pVYtHnjUqmRUl6MHkwOEhVQlU)

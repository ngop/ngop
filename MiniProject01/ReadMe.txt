Miniproject 01
MiniProject01bone.c by Peter Ngo
header.h: credit given to Andrew Miller.

To run the code with ./a.out there needs to be 5 arguments after it.
Each one is the default values such as the first two arguments are the input for the push button and the output to the LED. While the last 3 are for i2c sensor.

As follows the correct arguments to run the program is:

./a.out 7 60 3 75 0


There will be 4 cases all interrupted by the push button to cycle through each one. Each case is displayed on the terminal whenever the case is selected to keep track of which case is currently selected.

Case 0: The gpio output to show a blinking LED which toggles on and off after a certain time in milliseconds.

Case 1: This is the PWM output also used to show the flashing of an LED with the added values that the rate of the flashing of the LED is shown by the frequency and duty cycle given in the said case statement.

Case 2: This is the Analog in where the voltage from the potentiometer is read and displayed in the terminal when ran and changes whenever the pot is turned.

Case 3: This is the i2c device where the Temperature is read in Celsius and displayed in the terminal and also changes whenever the sensor detects a change in the temperature.

All these cases are placed in an infinite while loop which can be exited out with an interrupt from Ctrl + C.

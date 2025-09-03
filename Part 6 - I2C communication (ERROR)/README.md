Complement to [Part 6 - I2C communication](https://github.com/rtek1000/CH32_Microcontroller_Tutorials/tree/main/Part%206%20-%20I2C%20communication)


I2C can cause the program to crash if a communication failure occurs, due to the wait and acknowledge loops. Adding a timeout to the loops prevents code from crashing and allows the program to check the communication progress.

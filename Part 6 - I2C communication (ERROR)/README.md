Complement to Part 6 - I2C communication


I2C can cause the program to crash if a communication failure occurs, due to the wait and acknowledge loops. Adding a timeout to the loops prevents code from crashing and allows you to check the communication progress.

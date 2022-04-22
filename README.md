# Meteorological station
Simple meteorological station student project made using mbed OS.

MCU used is ST NUCLEO-F446RE. Sensor for temperature, humidity and pressure readouts is BME680 (naming for the library was left from BME280 that was previously meant to be used in the project). Display is a generic 1.3 inch 128x64 OLED. Library for OLED display is used from and can be found on the following link: https://os.mbed.com/users/Anaesthetix/code/SSH1106/. Wind speed is measured using anemometer and hall sensor. Encoder is used to navigate between all those measured values, which you can then select and modify their maximum values using the (10k) potentiometer. Should the measured values go above the maximum values, buzzer is going to play a preset "buzz", each measured value having its unique one.

Communication between MCU and BME680 is I2C while the display uses SPI.

![image](https://user-images.githubusercontent.com/103043443/161752707-2b3b16d4-b333-4b74-9b8b-5ae38411873c.png)

![image](https://user-images.githubusercontent.com/103043443/161752734-329a43a9-d7ce-4491-9f90-0b0a4162ab65.png)

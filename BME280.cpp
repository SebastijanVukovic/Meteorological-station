#include "mbed.h"
#include "BME280.h"

BME280::BME280(PinName sda, PinName scl, char slave_adr)
    :
    i2c_p(new I2C(sda, scl)), 
    i2c(*i2c_p),
    address(slave_adr),
    t_fine(0)
{
    initialize();
}

BME280::BME280(I2C &i2c_obj, char slave_adr)
    :
    i2c_p(NULL), 
    i2c(i2c_obj),
    address(slave_adr),
    t_fine(0)
{
    initialize();
}

BME280::~BME280()
{
    if (NULL != i2c_p)
        delete  i2c_p;
}
    
void BME280::initialize()
{
    char cmd[36];
    
    cmd[0] = 0xE0; // (soft) reset register
    cmd[1] = 0xB6; // Reset when 0xB6 written
    i2c.write(address, cmd, 2);
    
    cmd[0] = 0x72; // ctrl_hum register
    cmd[1] = 0x01; // Humidity oversampling x1
    i2c.write(address, cmd, 2);
 
    cmd[0] = 0x74; // ctrl_meas register
    cmd[1] = 0x27; // Temparature oversampling x1, Pressure oversampling x1, Normal mode
    i2c.write(address, cmd, 2);
 
    cmd[0] = 0x75; // config register
    cmd[1] = 0x40; // Standby 125ms, Filter off
    i2c.write(address, cmd, 2);

    cmd[0] = 0x70; // ctrl_gas_0 register
    cmd[1] = 0x08; // Turn off gas heater
    i2c.write(address, cmd, 2);
    
    cmd[0] = 0x71; // ctrl_gas_1 register
    cmd[1] = 0x0F; // Turn off gas measurements
    i2c.write(address, cmd, 2);

    cmd[35] = 0x89; // register for 1st group of coefficients
    i2c.write(address, &cmd[35], 1);
    i2c.read(address, &cmd[0], 25);
    cmd[35] = 0xE1; // register for 2nd group of coefficients
    i2c.write(address, &cmd[35], 1);
    i2c.read(address, &cmd[25], 10);
    
    // Temperature related coefficients
    dig_T1 = (cmd[34] << 8) | cmd[33];
    dig_T2 = (cmd[2] << 8) | cmd[1];
    dig_T3 =  cmd[3];
    
    // Pressure related coefficients
    dig_P1 = (cmd[6] << 8) | cmd[5];
    dig_P2 = (cmd[8] << 8) | cmd[7];
    dig_P3 =  cmd[9];
    dig_P4 = (cmd[12] << 8) | cmd[11];
    dig_P5 = (cmd[14] << 8) | cmd[13];
    dig_P6 =  cmd[16];
    dig_P7 =  cmd[15];
    dig_P8 = (cmd[20] << 8) | cmd[19];
    dig_P9 = (cmd[22] << 8) | cmd[21];
    dig_P10 = cmd[23];
    
    // Humidity related coefficients
    dig_H1 = (cmd[27] << 4) | (cmd[26] << 4 & 0x0f);
    dig_H2 = (cmd[25] << 4) | (cmd[26] >> 4);
    dig_H3 = cmd[28];
    dig_H4 = cmd[29];
    dig_H5 = cmd[30];
    dig_H6 = cmd[31];
    dig_H7 = cmd[32];
}
 
float BME280::getTemperature()
{
    uint32_t temp_raw;
    float tempf;
    char cmd[4];
 
    cmd[0] = 0x22; // temp_msb
    i2c.write(address, &cmd[0], 1);
    i2c.read(address, &cmd[1], 1);
    cmd[0] = 0x23; // temp_lsb
    i2c.write(address, &cmd[0], 1);
    i2c.read(address, &cmd[2], 1);
    cmd[0] = 0x24; // temp_xlsb
    i2c.write(address, &cmd[0], 1);
    i2c.read(address, &cmd[3], 1);
 
    temp_raw = (cmd[1] << 12) | (cmd[2] << 4) | (cmd[3] >> 4);
 
    int64_t var1, var2, var3;
    int16_t temp;

    var1 = (temp_raw >> 3) - (dig_T1 << 1);
    var2 = (var1 * dig_T2) >> 11;
    var3 = ((var1 >> 1) * (var1 >> 1)) >> 12;
    var3 = ((var3) * (dig_T3 << 4)) >> 14;
    t_fine = (int32_t)(var2 + var3);
    temp = (int16_t)(((t_fine * 5) + 128) >> 8);
    
    tempf = (float)temp;
 
    return (tempf/100.0f);   
}
 
float BME280::getPressure()
{
    uint32_t press_raw;
    float pressf;
    char cmd[4];
 
    cmd[0] = 0x1F; // press_msb
    i2c.write(address, &cmd[0], 1);
    i2c.read(address, &cmd[1], 1);
    cmd[0] = 0x20; // press_lsb
    i2c.write(address, &cmd[0], 1);
    i2c.read(address, &cmd[2], 1);
    cmd[0] = 0x21; // press_xlsb
    i2c.write(address, &cmd[0], 1);
    i2c.read(address, &cmd[3], 1);
 
    press_raw = (cmd[1] << 12) | (cmd[2] << 4) | (cmd[3] >> 4);
    
    int32_t var1, var2, var3;
    uint32_t press;
 
    var1 = (t_fine >> 1) - 64000;
    var2 = ((((var1 >> 2) * (var1 >> 2)) >> 11) * dig_P6) >> 2;
    var2 = var2 + ((var1 * dig_P5) << 1);
    var2 = (var2 >> 2) + (dig_P4 << 16);
    var1 = (((dig_P3 * (((var1 >> 2)*(var1 >> 2)) >> 13)) >> 3) + ((dig_P2 * var1) >> 1)) >> 18;
    var1 = ((32768 + var1) * dig_P1) >> 15;
    
    if (var1 == 0) {
        return 0;
    }
    
    press = (((1048576 - press_raw) - (var2 >> 12))) * 3125;
    
    if(press < 0x80000000) {
        press = (press << 1) / var1;
    } else {
        press = (press / var1) * 2;
    }
    
    var1 = (dig_P9 * ((int32_t)(((press >> 3) * (press >> 3)) >> 13))) >> 12;
    var2 = (((int32_t)(press >> 2)) * dig_P8) >> 13;
    var3 = ((int32_t)(press >> 8) * (int32_t)(press >> 8) * (int32_t)(press >> 8) * dig_P10) >> 17;
    press = (press + ((var1 + var2 + var3 + (dig_P7 << 7)) >> 4));
 
    pressf = (float)press;
    return (pressf/100.0f);
}
 
float BME280::getHumidity()
{
    uint16_t hum_raw;
    float humf;
    char cmd[3];
 
    cmd[0] = 0x25; // hum_msb
    i2c.write(address, &cmd[0], 1);
    i2c.read(address, &cmd[1], 1);
    cmd[0] = 0x26; // hum_lsb
    i2c.write(address, &cmd[0], 1);
    i2c.read(address, &cmd[2], 1);
 
    hum_raw = (cmd[1] << 8) | cmd[2];
    
    int64_t var1, var2, var3, var4, var5, var6, temp_scaled;
    uint16_t hum;
    
    temp_scaled = ((t_fine * 5) + 128) >> 8;
    var1 = (int32_t)(hum_raw - ((int32_t)(dig_H1 << 4))) - ((temp_scaled * (dig_H3) / ((int32_t)100)) >> 1);
    var2 = (dig_H2 * (((temp_scaled * dig_H4) / ((int32_t)100)) + (((temp_scaled * ((temp_scaled * dig_H5) / ((int32_t)100))) >> 6) / ((int32_t)100)) + (int32_t)(1 << 14))) >> 10;
    var3 = var1 * var2;
    var4 = dig_H6 << 7;
    var4 = ((var4) + ((temp_scaled * dig_H7) / ((int32_t)100))) >> 4;
    var5 = ((var3 >> 14) * (var3 >> 14)) >> 10;
    var6 = (var4 * var5) >> 1;
    hum = (((var3 + var6) >> 10) * ((int32_t)1000)) >> 12;
    
    if (hum > 100000) hum = 100000; // Cap at 100%rH

    else if (hum < 0) hum = 0;

    humf = (float)hum;
 
    return (humf/1024.0f);
}

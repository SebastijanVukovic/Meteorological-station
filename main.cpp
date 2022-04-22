#include "mbed.h"
#include "BME280.h"
#include "SSH1106.h"

BME280 sensor(D14, D15); // sda, scl (I2C)
                         // senzor je BME680, ali je library modificiran i kombiniran sa drugima
                         // na bazi BME280 library-ja od Toyomasa Watarai-a, kako bih dobio trenutni

SPI lcd(D11, NC, D13); // mosi, miso(nc), sclk (SPI)
DigitalOut cs(D10); // chip select (active low)
DigitalOut cd(D9); // command/data (0=command, 1=data)
DigitalOut rst(D8); // reset (active low)
SSH1106 ekran(lcd, cs, cd, rst);

BusIn enc(D3, D4); // DT, CLK linije
InterruptIn enc_sw(D7); // tipka na enkoderu

AnalogIn pot(A0); // potenciometar 10k ohm

DigitalIn hallSensor(D5); // hall sensor

PwmOut buzzer(D6); // buzzer

Timer debounce; // debounce za enc_sw tipku
Timer brzina; // vrijeme za izracun brzine vjetra
Ticker zvoni; // pozivanje buzzera

// "Globalne" vrijednosti
bool edit = false, edit_prev = false, chk = false;
float temp, vlaga, tlak, brzvj; // ocitane vrijednosti
int temp_max = 40, vlaga_max = 80, tlak_max = 1050, brzvj_max = 10; // zadane granicne vrijednosti
char str1[40], str2[40], str3[40], str4[40], str5[40], str6[40], str7[40], str8[40], str9[40],
str10[40], str11[40], str12[40], str13[40], str14[40], str15[40], str16[40], str17[40], str18[40];
int te, vl, tl, brzvj1, brzvj2, pok, te_jed, vl_jed, tl_jed, brzvj_jed, te_max, vl_max, tl_max, brzvj2_max;

// Buzzer note
float freq1[5] = {
    27.5,
    29.14,
    27.5,
    29.14,
    30.87
}; // temperatura
float freq2[5] = {
    23.12,
    21.83,
    21.83,
    21.83,
    20.60
}; // vlaga
float freq3[5] = {
    20.60,
    21.83,
    27.5,
    20.60,
    19.45
}; // tlak
float freq4[5] = {
    30.87,
    23.12,
    30.87,
    21.83,
    23.12
}; // brzina vjetra

// Kontruktori za funkcije
void deb();
void buzz();
void tekst();

int main() {
    ekran.init();
    wait(0.2);
    ekran.clear();
    wait(0.2);

    debounce.start();
    brzina.start();

    enc_sw.mode(PullUp);
    enc_sw.fall(&deb);

    // Vrijednosti za ispis na ekran
    te = sprintf(str1, "Temperatura");
    vl = sprintf(str3, "Vlaga");
    tl = sprintf(str5, "Tlak");
    brzvj1 = sprintf(str7, "Brzina");
    brzvj2 = sprintf(str8, "vjetra");
    pok = sprintf(str10, "<");
    te_jed = sprintf(str11, "[*C]");
    vl_jed = sprintf(str12, "[%% r.H.]");
    tl_jed = sprintf(str13, "[hPa]");
    brzvj_jed = sprintf(str14, "[km/h]");

    tekst();

    // Dodatne vrijednosti
    int x = enc,
    x_cur,
    linija = 0,
    hs,
    hs_prev = hallSensor,
    skip = 0;
    float pote,
    te_vr,
    vl_vr,
    tl_vr,
    brzvj_vr,
    vrijeme;

    while(1) {
        // Ocitavanje vrijednosti senzora BME680
        temp = sensor.getTemperature();
        vlaga = sensor.getHumidity();
        tlak = sensor.getPressure();

        // Izracun brzine vjetra
        hs = hallSensor;

        if((hs_prev == 0) && (hs == 1)) {
            brzina.reset();
            hs_prev = hs;
            skip = 0;
        }

        else if((hs_prev == 1) && (hs == 0)) {
            if(skip == 0) {
                vrijeme = brzina.read_ms();
                brzvj = ((5.6*3.14159)/vrijeme)*36; // pretvaranje cm/ms (koristeno radi vece preciznosti) u km/h
            }
            hs_prev = hs;
        }

        else if(brzina.read_ms() > 2000) {
            brzvj = 0;
            skip = 1;
        }

        // Ekran - pridruzivanje vrijednosti
        te_vr = sprintf(str2, "%4.1f", temp);
        vl_vr = sprintf(str4, "%4.1f", vlaga);
        tl_vr = sprintf(str6, "%4.1f", tlak);
        brzvj_vr = sprintf(str9, "%4.1f", brzvj);
        te_max = sprintf(str15, "%4d", temp_max);
        vl_max = sprintf(str16, "%4d", vlaga_max);
        tl_max = sprintf(str17, "%4d", tlak_max);
        brzvj2_max = sprintf(str18, "%4d", brzvj_max);

        // Ekran - ispis vrijednosti
        ekran.writeText(50, 0, font_6x6, str2, te_vr);
        ekran.writeText(50, 2, font_6x6, str4, vl_vr);
        ekran.writeText(50, 4, font_6x6, str6, tl_vr);
        ekran.writeText(50, 6, font_6x6, str9, brzvj_vr);
        ekran.writeText(90, 0, font_6x6, str15, te_max);
        ekran.writeText(90, 2, font_6x6, str16, vl_max);
        ekran.writeText(90, 4, font_6x6, str17, tl_max);
        ekran.writeText(90, 6, font_6x6, str18, brzvj2_max);
        ekran.writeText(122, linija, font_6x6, str10, pok);

        // Enkoder
        if(edit == true) x = enc;
        x_cur = enc;

        if(x != x_cur && edit == false) {
            if((x == 1 && x_cur == 3) || (x == 2 && x_cur == 0)) {
                // lijevo/gore
                if(linija == 0) linija = 6;
                else linija = linija - 2;
                ekran.clear();
                tekst();
            }
            else if((x == 2 && x_cur == 3) || (x == 1 && x_cur == 0)) {
                // desno/dolje
                if(linija == 6) linija = 0;
                else linija = linija + 2;
                ekran.clear();
                tekst();
            }
            x = x_cur;
        }

        // Odredivanje granicnih vrijednosti
        pote = pot;

        if(edit == true) {
            edit_prev = edit;
            if(linija == 0) {
                temp_max = ((125*pote)-40);
            }
            else if(linija == 2) {
                vlaga_max = (pote*100);
            }
            else if(linija == 4) {
                tlak_max = ((800*pote)+300);
            }
            else if(linija == 6) {
                brzvj_max = (pote*50);
            }
        }

        if((edit_prev == true) && (edit == false)) {
            ekran.clear();
            tekst();
            edit_prev = edit;
        }

        // Buzzer
        if(!chk) {
            // osigurava da ce se samo jednom izvrsiti .attach() Tickera dok ne pozovemo .detach()
            if((temp > temp_max) || (vlaga > vlaga_max) || (tlak > tlak_max) || (brzvj > brzvj_max)) {
                zvoni.attach(&buzz, 4.0);
                chk = !chk;
            }
        }

        if(chk) {
            if((temp < temp_max) && (vlaga < vlaga_max) && (tlak < tlak_max) && (brzvj < brzvj_max)) {
                zvoni.detach();
                buzzer = 0;
                chk = !chk;
            }
        }
    }
}

void deb() {
    if (debounce.read_ms() > 230) {
        edit = !edit;
        debounce.reset();
    }
}

void buzz() {
    if(temp > temp_max) {
        for(int i = 0; i < 5; i++) {
            buzzer.period(1 / freq1[i]);
            buzzer = 0.5;
            wait(0.4);
        }
        buzzer = 0;
    }
    else if(tlak > tlak_max) {
        for(int i = 0; i < 5; i++) {
            buzzer.period(1 / freq2[i]);
            buzzer = 0.5;
            wait(0.4);
        }
        buzzer = 0;
    }
    else if(vlaga > vlaga_max) {
        for(int i = 0; i < 5; i++) {
            buzzer.period(1 / freq3[i]);
            buzzer = 0.5;
            wait(0.4);
        }
        buzzer = 0;
    }
    else if(brzvj > brzvj_max) {
        for(int i = 0; i < 5; i++) {
            buzzer.period(1 / freq4[i]);
            buzzer = 0.5;
            wait(0.4);
        }
        buzzer = 0;
    }
    else buzzer = 0;
}

void tekst() {
    // Ekran - ispis nepromjenjivog teksta
    ekran.writeText(0, 0, font_4x5, str1, te);
    ekran.writeText(0, 2, font_6x6, str3, vl);
    ekran.writeText(0, 4, font_6x6, str5, tl);
    ekran.writeText(0, 6, font_6x6, str7, brzvj1);
    ekran.writeText(0, 7, font_6x6, str8, brzvj2);
    ekran.writeText(0, 1, font_4x5, str11, te_jed);
    ekran.writeText(0, 3, font_4x5, str12, vl_jed);
    ekran.writeText(0, 5, font_4x5, str13, tl_jed);
    ekran.writeText(38, 7, font_4x5, str14, brzvj_jed);

    // Podcrtavanje granicnih vrijednosti
    ekran.drawLineHor(88, 9, 1, 27);
    ekran.drawLineHor(88, 25, 1, 27);
    ekran.drawLineHor(88, 41, 1, 27);
    ekran.drawLineHor(88, 57, 1, 27);
}
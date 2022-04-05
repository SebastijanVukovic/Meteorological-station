1   #include "mbed.h"
2   #include "BME280.h"
3   #include "SSH1106.h"
4   
5   BME280 sensor(D14, D15); // sda, scl (I2C)
6                            // senzor je BME680, ali je library modificiran i kombiniran sa drugima
7                            // na bazi BME280 library-ja od Toyomasa Watarai-a, kako bih dobio trenutni
8   
9   SPI lcd(D11, NC, D13);   // mosi, miso(nc), sclk (SPI)
10  DigitalOut cs(D10);      // chip select (active low)
11  DigitalOut cd(D9);       // command/data (0=command, 1=data)
12  DigitalOut rst(D8);      // reset (active low)
13  SSH1106 ekran(lcd, cs, cd, rst);
14  
15  BusIn enc(D3, D4);       // DT, CLK linije
16  InterruptIn enc_sw(D7);  // tipka na enkoderu
17  
18  AnalogIn pot(A0);        // potenciometar 10k ohm
19  
20  DigitalIn hallSensor(D5);// hall sensor
21  
22  PwmOut buzzer(D6);       // buzzer
23  
24  Timer debounce;          // debounce za enc_sw tipku
25  Timer brzina;            // vrijeme za izracun brzine vjetra
26  Ticker zvoni;            // pozivanje buzzera
27  
28  // "Globalne" vrijednosti
29  bool edit = false, edit_prev = false, chk = false;
30  float temp, vlaga, tlak, brzvj; // ocitane vrijednosti
31  int temp_max = 40, vlaga_max = 80, tlak_max = 1050, brzvj_max = 10; // zadane granicne vrijednosti
32  char str1[40], str2[40], str3[40], str4[40], str5[40], str6[40], str7[40], str8[40], str9[40],
33       str10[40], str11[40], str12[40], str13[40], str14[40], str15[40], str16[40], str17[40], str18[40];
34  int te, vl, tl, brzvj1, brzvj2, pok, te_jed, vl_jed, tl_jed, brzvj_jed, te_max, vl_max, tl_max, brzvj2_max;
35  
36  // Buzzer note
37  float freq1[5]={27.5, 29.14, 27.5, 29.14, 30.87};   // temperatura
38  float freq2[5]={23.12, 21.83, 21.83, 21.83, 20.60}; // vlaga
39  float freq3[5]={20.60, 21.83, 27.5, 20.60, 19.45};  // tlak
40  float freq4[5]={30.87, 23.12, 30.87, 21.83, 23.12}; // brzina vjetra
41  
42  // Kontruktori za funkcije
43  void deb();
44  void buzz();
45  void tekst();
46  
47  int main(){
48      ekran.init();
49      wait(0.2);
50      ekran.clear();
51      wait(0.2);
52      
53      debounce.start();
54      brzina.start();
55      
56      enc_sw.mode(PullUp);
57      enc_sw.fall(&deb);
58      
59      // Vrijednosti za ispis na ekran
60      te = sprintf(str1, "Temperatura");
61      vl = sprintf(str3, "Vlaga");
62      tl = sprintf(str5, "Tlak");
63      brzvj1 = sprintf(str7, "Brzina");
64      brzvj2 = sprintf(str8, "vjetra");
65      pok = sprintf(str10, "<");
66      te_jed = sprintf(str11, "[*C]");
67      vl_jed = sprintf(str12, "[%% r.H.]");
68      tl_jed = sprintf(str13, "[hPa]");
69      brzvj_jed = sprintf(str14, "[km/h]");
70      
71      tekst();
72      
73      // Dodatne vrijednosti
74      int x = enc, x_cur, linija = 0, hs, hs_prev = hallSensor, skip = 0;
75      float pote, te_vr, vl_vr, tl_vr, brzvj_vr, vrijeme;
76      
77      while(1){
78          // Ocitavanje vrijednosti senzora BME680
79          temp = sensor.getTemperature();
80          vlaga = sensor.getHumidity();
81          tlak = sensor.getPressure();
82          
83          // Izracun brzine vjetra
84          hs = hallSensor;
85          
86          if((hs_prev == 0) && (hs == 1)){ 
87              brzina.reset();
88              hs_prev = hs;
89              skip = 0;
90          }
91          
92          else if((hs_prev == 1) && (hs == 0)){
93              if(skip == 0){
94                  vrijeme = brzina.read_ms();
95                  brzvj = ((5.6*3.14159)/vrijeme)*36; // pretvaranje cm/ms (koristeno radi vece preciznosti) u km/h
96              }
97              hs_prev = hs;
98          }
99          
100         else if(brzina.read_ms() > 2000){
101             brzvj = 0;
102             skip = 1;
103         }
104         
105         // Ekran - pridruzivanje vrijednosti
106         te_vr = sprintf(str2, "%4.1f", temp);
107         vl_vr = sprintf(str4, "%4.1f", vlaga);
108         tl_vr = sprintf(str6, "%4.1f", tlak);
109         brzvj_vr = sprintf(str9, "%4.1f", brzvj);
110         te_max = sprintf(str15, "%4d", temp_max);
111         vl_max = sprintf(str16, "%4d", vlaga_max);
112         tl_max = sprintf(str17, "%4d", tlak_max);
113         brzvj2_max = sprintf(str18, "%4d", brzvj_max);
114         
115         // Ekran - ispis vrijednosti        
116         ekran.writeText(50,0, font_6x6, str2, te_vr);
117         ekran.writeText(50,2, font_6x6, str4, vl_vr);
118         ekran.writeText(50,4, font_6x6, str6, tl_vr);
119         ekran.writeText(50,6, font_6x6, str9, brzvj_vr);
120         ekran.writeText(90,0, font_6x6, str15, te_max);
121         ekran.writeText(90,2, font_6x6, str16, vl_max);
122         ekran.writeText(90,4, font_6x6, str17, tl_max);
123         ekran.writeText(90,6, font_6x6, str18, brzvj2_max);
124         ekran.writeText(122,linija, font_6x6, str10, pok);
125         
126         // Enkoder
127         if(edit == true) x = enc;
128         x_cur = enc;
129         
130         if(x != x_cur && edit == false){
131             if((x==1 && x_cur==3) || (x==2 && x_cur==0)){ // lijevo/gore
132                 if(linija==0) linija = 6;
133                 else linija = linija - 2;
134                 ekran.clear();
135                 tekst();
136             }
137             else if((x==2 && x_cur==3) || (x==1 && x_cur==0)){ // desno/dolje
138                 if(linija==6) linija = 0;
139                 else linija = linija + 2;
140                 ekran.clear();
141                 tekst();
142             }
143             x = x_cur;
144         }
145         
146         // Odredivanje granicnih vrijednosti
147         pote = pot;
148         
149         if(edit == true){
150             edit_prev = edit;
151             if(linija == 0){
152                 temp_max = ((125*pote)-40);
153             }
154             else if(linija == 2){
155                 vlaga_max = (pote*100);
156             }
157             else if(linija == 4){
158                 tlak_max = ((800*pote)+300);
159             }
160             else if(linija == 6){
161                 brzvj_max = (pote*50);
162             }
163         }
164         
165         if((edit_prev == true) && (edit == false)){
166             ekran.clear();
167             tekst();
168             edit_prev = edit;
169         }
170         
171         // Buzzer
172         if(!chk){ // osigurava da ce se samo jednom izvrsiti .attach() Tickera dok ne pozovemo .detach()
173             if((temp > temp_max) || (vlaga > vlaga_max) || (tlak > tlak_max) || (brzvj > brzvj_max)){
174                 zvoni.attach(&buzz, 4.0);
175                 chk = !chk;
176             }
177         }
178         
179         if(chk){
180             if((temp < temp_max) && (vlaga < vlaga_max) && (tlak < tlak_max) && (brzvj < brzvj_max)){
181                 zvoni.detach();
182                 buzzer = 0;
183                 chk = !chk;
184             }
185         }
186     }
187 }
188 
189 void deb(){
190     if (debounce.read_ms() > 230) {
191         edit = !edit;
192         debounce.reset();
193     }
194 }
195 
196 void buzz(){
197     if(temp > temp_max){
198         for(int i=0; i<5; i++){
199             buzzer.period(1 / freq1[i]);
200             buzzer = 0.5;
201             wait(0.4);
202         }
203         buzzer = 0;
204     }
205     else if(tlak > tlak_max){
206         for(int i=0; i<5; i++){
207             buzzer.period(1 / freq2[i]);
208             buzzer = 0.5;
209             wait(0.4);
210         }
211         buzzer = 0;
212     }
213     else if(vlaga > vlaga_max){
214         for(int i=0; i<5; i++){
215             buzzer.period(1 / freq3[i]);
216             buzzer = 0.5;
217             wait(0.4);
218         }
219         buzzer = 0;
220     }
221     else if(brzvj > brzvj_max){
222         for(int i=0; i<5; i++){
223             buzzer.period(1 / freq4[i]);
224             buzzer = 0.5;
225             wait(0.4);
226         }
227         buzzer = 0;
228     }
229     else buzzer = 0;
230 }
231 
232 void tekst(){
233     // Ekran - ispis nepromjenjivog teksta
234     ekran.writeText(0,0, font_4x5, str1, te);
235     ekran.writeText(0,2, font_6x6, str3, vl);
236     ekran.writeText(0,4, font_6x6, str5, tl);
237     ekran.writeText(0,6, font_6x6, str7, brzvj1);
238     ekran.writeText(0,7, font_6x6, str8, brzvj2);
239     ekran.writeText(0,1, font_4x5, str11, te_jed);
240     ekran.writeText(0,3, font_4x5, str12, vl_jed);
241     ekran.writeText(0,5, font_4x5, str13, tl_jed);
242     ekran.writeText(38,7, font_4x5, str14, brzvj_jed);
243     
244     // Podcrtavanje granicnih vrijednosti
245     ekran.drawLineHor(88, 9, 1, 27);
246     ekran.drawLineHor(88, 25, 1, 27);
247     ekran.drawLineHor(88, 41, 1, 27);
248     ekran.drawLineHor(88, 57, 1, 27);
249 }
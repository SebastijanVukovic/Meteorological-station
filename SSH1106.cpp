1   #include "SSH1106.h"
2   
3   SSH1106::SSH1106(SPI &lcd, DigitalOut &lcd_cs, DigitalOut &lcd_cd, DigitalOut &lcd_rst)
4   {
5       _lcd = &lcd;
6       _lcd_cs = &lcd_cs;
7       _lcd_cd = &lcd_cd;
8       _lcd_rst = &lcd_rst;
9   }
10  
11  void SSH1106::init()
12  {
13      _lcd_cs->write(1);
14      _lcd->frequency(24000000);
15      _lcd->format(8,3);
16      _lcd_cs->write(0);              // enable SPI
17      _lcd_cd->write(0);              // COMMAND mode
18  
19      _lcd_rst->write(0);
20      wait_ms(100);
21      _lcd_rst->write(1);
22  
23      wait_ms(50);
24  
25      _lcd->write(0xAE);              // Display off
26      _lcd->write(0x02);              // Set lower column address
27      _lcd->write(0x10);              // Set higher column address
28      _lcd->write(0x40);              // Set display start line
29      _lcd->write(0xB0);              // Set page address
30      _lcd->write(0x81);              // Set contrast to
31      _lcd->write(0x80);              // 128
32      _lcd->write(0xA1);              // Segment remap
33      _lcd->write(0xA6);              // Inverse: normal (A7 = inverse)
34      _lcd->write(0xA8);              // Multiplex ratio
35      _lcd->write(0x3F);              // Duty = 1/32
36      _lcd->write(0xAD);              // Charge pump enable
37      _lcd->write(0x8B);              // External VCC
38      _lcd->write(0x33);              // VPP: 9v
39      _lcd->write(0xC8);              // Com scan direction
40      _lcd->write(0xD3);              // Display offset
41      _lcd->write(0x00);              // 0x20
42      _lcd->write(0xD5);              // Osc division
43      _lcd->write(0x80);
44      _lcd->write(0xD9);              // Pre-charge period
45      _lcd->write(0x1F);              // 0x22
46      _lcd->write(0xDA);              // Set com pins
47      _lcd->write(0x12);
48      _lcd->write(0xDB);              // Set vcomh
49      _lcd->write(0x40);
50      _lcd->write(0xAF);              // Display ON
51      _lcd_cs->write(1);
52      _lcd_cd->write(1);
53  }
54  
55  void SSH1106::setContrast(char contrast)
56  {
57      _lcd_cs->write(0);              // enable SPI
58      _lcd_cd->write(0);              // command mode
59      _lcd->write(0x81);              // command to set contrast
60      _lcd->write(contrast);          // set contrast
61      _lcd_cs->write(1);
62      _lcd_cd->write(1);
63  }
64  
65  void SSH1106::setCursor(char column, char line)
66  {
67      int i, j;
68      column = column+2; // column+4
69  
70      i=(column&0xF0)>>4;
71      j=column&0x0F;
72      _lcd_cd->write(0);
73      _lcd->write(0xb0+line);
74      _lcd->write(0x10+i);
75      _lcd->write(j);
76      _lcd_cd->write(1);
77  }
78  
79  void SSH1106::clear()
80  {
81      _lcd_cs->write(0);
82      _lcd_cd->write(1);
83  
84      for(unsigned short j = 0; j < LCDPAGES; j++) {
85          SSH1106::setCursor(0, j);
86          for(unsigned short i = 0; i < LCDWIDTH ; i++) {
87              _lcd->write(0x00);
88          }
89      }
90  
91      SSH1106::setCursor(0, 0);
92  
93      _lcd_cs->write(1);
94  }
95  
96  void SSH1106::writeText2d(char column, char page, const char font_address[96][8], const char *text, int size)
97  {
98      _lcd_cs->write(0);
99      SSH1106::setCursor(column, page);
100     for(int i=0; i<size; i++) {
101         for(int a=0; a<8; a++) {
102             _lcd->write((font_address[(text[i]-32)][a]));
103         }
104     }
105     _lcd_cs->write(1);
106 }
107 
108 void SSH1106::writeText(char column, char page, const char *font_address, const char *text, const uint8_t size)
109 {
110     // Position of character data in memory array
111     uint16_t pos_array;
112     // temporary column, page address, and column_cnt are used
113     // to stay inside display area
114     uint8_t i,y, column_cnt = 0;
115     uint8_t count = 0;
116 
117     // font information, needed for calculation
118     uint8_t start_code, last_code, width, page_height, bytes_p_char;
119 
120     uint8_t *txtbuffer;
121 
122     start_code   = font_address[2];  // get first defined character
123     last_code    = font_address[3];  // get last defined character
124     width        = font_address[4];  // width in pixel of one char
125     page_height  = font_address[6];  // page count per char
126     bytes_p_char = font_address[7];  // bytes per char
127 
128     _lcd_cs->write(0);  // Enable SPI
129     _lcd_cd->write(1);  // Data mode
130 
131     if(page_height + page > LCDPAGES) //stay inside display area
132         page_height = LCDPAGES - page;
133 
134     // The string is displayed character after character. If the font has more then one page,
135     // the top page is printed first, then the next page and so on
136     for(y = 0; y < page_height; y++) {
137         txtbuffer = &_lcdbuffer[page*LCDWIDTH + column];
138         column_cnt = 0;                 // clear column_cnt start point
139         i = 0;
140         while(( i < size) && ((column_cnt + column) < LCDWIDTH)) {
141             if(text[i] < start_code || (uint8_t)text[i] > last_code) //make sure data is valid
142                 i++;
143             else {
144                 // calculate position of ASCII character in font array
145                 // bytes for header + (ASCII - startcode) * bytes per char)
146                 pos_array = 8 + (uint8_t)(text[i++] - start_code) * bytes_p_char;
147 
148                 // get the dot pattern for the part of the char to print
149                 pos_array += y*width;
150 
151                 // stay inside display area
152                 if((column_cnt + width + column) > LCDWIDTH)
153                     column_cnt = LCDWIDTH-width;
154 
155                 // copy character data to buffer
156                 memcpy (txtbuffer+column_cnt,font_address+pos_array,width);
157             }
158 
159             column_cnt += width;
160         }
161         SSH1106::setCursor(column,page);  // set start position x and y
162 
163         do {
164             _lcd->write(txtbuffer[count]);
165             count++;
166         } while ((count <= column_cnt));
167     }
168 
169     _lcd_cs->write(1);  // Disable SPI
170 
171 }
172 
173 void SSH1106::drawBitmap(const char *data)
174 {
175     int cnt = 0;
176     _lcd_cs->write(0);
177     _lcd_cd->write(1);
178     SSH1106::setCursor(0,0);
179     for(int row=0; row<LCDPAGES; row++) {
180         SSH1106::setCursor(0, row);
181         for(int column=0; column<LCDWIDTH; column++) {
182             _lcd->write(data[cnt]);
183             cnt++;
184         }
185     }
186     _lcd_cs->write(1);
187 }
188 
189 void SSH1106::drawLineHor(char posx, char posy, char height, char width)
190 {
191     char page, offset, offset2;
192     char buffer[2] = {0xFF, 0xFF};
193 
194     _lcd_cs->write(0);
195     _lcd_cd->write(1);
196 
197     if(width+posx > LCDWIDTH) width = (LCDWIDTH-posx); // keep inside display area
198 
199     page = posy/8;
200     offset = posy - (page*8);
201     buffer[0] = buffer[0] >> (8-height);
202     buffer[0] = buffer[0] << offset;
203 
204     if((offset + height) > 8) {
205         offset2 = ((offset+height)-8);
206         buffer[1] = buffer[1] - (0xFF << (offset2));
207     }
208 
209     SSH1106::setCursor(posx, page);
210 
211     for(int i=0; i<width; i++) _lcd->write(buffer[0]);
212 
213     if(buffer[1] != 0xFF && (page+1) < 8) {         // only write if line takes up > 1 page & keep inside display area
214         SSH1106::setCursor(posx, (page+1));
215         for(int i=0; i<width; i++) _lcd->write(buffer[1]);
216     }
217     _lcd_cs->write(1);
218 }
219 
220 void SSH1106::drawLineVert(char posx, char posy, char height, char width)
221 {
222     char page, pagecount, offset, offset2;
223     
224     _lcd_cs->write(0);
225     _lcd_cd->write(1);
226 
227     page = posy/8;
228     pagecount = height/8;
229     offset2 = height - (pagecount*8);
230 
231     SSH1106::setCursor(posx, page);
232     for(int i=0; i<width; i++) _lcd->write((0xFF>>offset));
233 
234     for(; pagecount > 1; pagecount--) {
235         page++;
236         SSH1106::setCursor(posx, page);
237         for(int i=0; i<width; i++) _lcd->write(0xFF);
238     }
239 
240     SSH1106::setCursor(posx, (page+1));
241     for(int i=0; i<width; i++) _lcd->write((0xFF<<offset2));
242 
243     _lcd_cs->write(1);
244 }
245 
246 void SSH1106::clearBuffer(void)
247 {
248     for(int i=0; i<(LCDWIDTH*LCDPAGES); i++) buff[i] = 0;
249 }
250 
251 void SSH1106::update(void)
252 {
253     int cnt = 0;
254     _lcd_cs->write(0);
255     _lcd_cd->write(1);
256     SSH1106::setCursor(0,0);
257     for(int row=0; row<LCDPAGES; row++) {
258         SSH1106::setCursor(0, row);
259         for(int column=0; column<LCDWIDTH; column++) {
260             _lcd->write(buff[cnt]);
261             cnt++;
262         }
263     }
264     _lcd_cs->write(1);
265 }
266 
267 void SSH1106::drawbufferLineHor(char posx, char posy, char height, char width)
268 {
269     char page, offset, offset2;
270     int cursor;
271     char buffer[2] = {0xFF, 0xFF};
272 
273     if(width+posx > LCDWIDTH) width = (LCDWIDTH-posx); // keep inside display area
274 
275     page = posy/LCDPAGES;
276     offset = posy - (page*LCDPAGES);
277     buffer[0] = buffer[0] >> (8-height);
278     buffer[0] = buffer[0] << offset;
279 
280     if((offset + height) > 8) {
281         offset2 = ((offset+height)-8);
282         buffer[1] = buffer[1] - (0xFF << (offset2));
283     }
284 
285     cursor = posx + (page*LCDWIDTH);
286 
287     for(int i=0; i<width; i++) SSH1106::buff[cursor+i] |= buffer[0];
288 
289     if(buffer[1] != 0xFF && (page+1) < LCDPAGES) {         // only write if line takes up > 1 page & keep inside display area
290         for(int i=0; i<width; i++) SSH1106::buff[cursor+i+LCDWIDTH] |= buffer[1];
291     }
292 }
293 
294 void SSH1106::drawbufferLineVert(char posx, char posy, char height, char width)
295 {
296     char page, pagecount, offset, offset2;
297     int cursor;
298 
299     page = posy/LCDPAGES;
300     pagecount = height/LCDPAGES;
301     offset2 = height - (pagecount*LCDPAGES);
302     cursor = posx + (page*LCDWIDTH); // LCDWIDTH
303 
304     for(int i=0; i<width; i++) SSH1106::buff[cursor+i] |= (0xFF>>offset);
305 
306     for(; pagecount > 1; pagecount--) {
307         page++;
308         cursor += LCDWIDTH;
309         for(int i=0; i<width; i++) SSH1106::buff[cursor+i] |= 0xFF;
310     }
311 
312     cursor += LCDWIDTH;
313     for(int i=0; i<width; i++) SSH1106::buff[cursor+i] |= (0xFF >> offset2);
314 }
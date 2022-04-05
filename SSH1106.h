1  #ifndef SSH1106_H
2  #define SSH1106_H
3  
4  #include "mbed.h"
5  #include "font_4x5.h"
6  #include "font_5x8.h"
7  #include "font_6x6.h"
8  #include "font_6x8.h"
9  #include "font_7x7.h"
10 #include "font_8x8.h"
11 #include "font_8x8_1.h"
12 #include "bold_font.h"
13 #include "font2d_hunter.h"
14 #include "font2d_formplex12.h"
15 
16 #define LCDWIDTH 128
17 #define LCDHEIGHT 64
18 #define LCDPAGES 8
19 
20 class SSH1106
21 {
22 public:
23 
24     // Constructor
25     SSH1106(SPI &spi, DigitalOut &lcd_cs, DigitalOut &cd, DigitalOut &rst);
26 
27     // Initialize LCD
28     void init(void);
29     
30     // Set contrast (0 - 63), initialized to 40
31     void setContrast(char contrast);
32 
33     // Place cursor at position
34     void setCursor(char column, char line);
35 
36     // Clear screen
37     void clear(void);
38 
39     // Write text to LCD where font format is a 2-dimensional array (only 96x8 byte, 8x8 pixel fonts supported)
40     void writeText2d(char column, char page, const char font_address[96][8], const char *text, int size);
41 
42     // Write text to LCD where font format is a 1-dimensional array. >8 pixel fonts are not working yet, will update soon
43     void writeText(char column, char page, const char *font_address, const char *str, const uint8_t size);
44 
45     // Draw a 128x64 pixel bitmap
46     void drawBitmap(const char *data);
47     
48     // Draw a horizontal line, start positions / height / width in pixels
49     void drawLineHor(char posx, char posy, char height, char width);
50     
51     // Draw a vertical line, start positions / height / width in pixels
52     void drawLineVert(char posx, char posy, char height, char width);
53     
54     //-----------------------------------------------------------------------------------------------------------------------------
55     // Functions below are buffered versions; possible to write things on top of each other without clearing pixels (ORs all data).
56     // Use update() to write buffer to LCD.
57     // Clear buffer before writing 1st time (initialize all values)
58     // writetext / drawbitmap will be added soon
59     //-----------------------------------------------------------------------------------------------------------------------------
60     
61     // Clear buffer
62     void clearBuffer(void);
63     
64     // Write buffer to LCD
65     void update(void);
66     
67     // Draw a horizontal line, start positions / height / width in pixels. Buffered version; possible to write things on top of each other
68     // without clearing pixels (ORs all data). Use update() to write buffer to LCD
69     void drawbufferLineHor(char posx, char posy, char height, char width);
70     
71     // Draw a vertical line, start positions / height / width in pixels. Buffered version.
72     void drawbufferLineVert(char posx, char posy, char height, char width);
73 
74 private:
75 
76     SPI         *_lcd;
77     DigitalOut  *_lcd_cs;
78     DigitalOut  *_lcd_cd;
79     DigitalOut   *_lcd_rst;
80     uint8_t     _lcdbuffer[LCDWIDTH*LCDPAGES];
81     char        buff[LCDWIDTH*LCDPAGES];
82 
83 };
84 
85 #endif
86 
#include "LPD8806.h"
#include "SPI.h"
#include <Wire.h>
#include "Adafruit_LSM303.h"

Adafruit_LSM303 lsm;

// Example to control LPD8806-based RGB LED Modules in a strip!
/*****************************************************************************/

// this is for teensyduino support
int dataPin = 12;
int clockPin = 6;
// Set the first variable to the NUMBER of pixels. 32 = 32 pixels in a row
// The LED strips are 32 LEDs per meter but you can extend/cut the strip
LPD8806 strip = LPD8806(32, dataPin, clockPin);


/* Global variables */
int accel[3];  // we'll store the raw acceleration values here
int mag[3];  // raw magnetometer values stored here
float realAccel[3];  // calculated acceleration values here

#define SCALE 2  // accel full-scale, should be 2, 4, or 8
#define X 0
#define Y 1
#define Z 2

void setup() {
  // Start up the LED strip
  strip.begin();

  // Update the strip, to start they are all 'off'
  strip.show();
  
  //LSM stuff;
  Serial.begin(9600);
  
  // Try to initialise and warn if we couldn't detect the chip
  if (!lsm.begin())
  {
    Serial.println("Oops ... unable to initialize the LSM303. Check your wiring!");
    while (1);
  }
}

// function prototypes, do not remove these!
void colorChase(uint32_t c, uint8_t wait);
void colorWipe(uint32_t c, uint8_t wait);
void dither(uint32_t c, uint8_t wait);
void scanner(uint8_t r, uint8_t g, uint8_t b, uint8_t wait);
void wave(uint32_t c, int cycles, uint8_t wait);
void rainbowCycle(uint8_t wait);
uint32_t Wheel(uint16_t WheelPos);
  int i=0,p=0,delta=1;

#define RED 'strip.Color(255,0,0)'

int heading=0,tiltheading=0,pix=0,l=0;


int value_to_color(int v);
int value_to_color(int v)
{
   return(Wheel(abs((int)v %255)));
}

void loopreal(){
  lsm.read();
  
  accel[0]=(int)lsm.accelData.x;
  accel[1]=(int)lsm.accelData.y;
  accel[2]=(int)lsm.accelData.z;
  mag[0]=(int)lsm.magData.x;
  mag[1]=(int)lsm.magData.y;  
  mag[2]=(int)lsm.magData.z;  
  
  for (int i=0; i<3; i++)
    realAccel[i] = (float)accel[i]/980.0 ;// / pow(2, 15) * SCALE;  // calculate real acceleration values, in units of g
  /* print both the level, and tilt-compensated headings below to compare */
  heading=getHeading(mag);
  tiltheading=getTiltHeading(mag, realAccel);
  pix=tiltheading*32/360 ;
  pix=-heading*32/360 ;
  if (pix<0) { pix+=32 ; }
  
  l++;
  if (0==l%25) {
  Serial.print("Accel X: "); Serial.print((int)lsm.accelData.x); Serial.print(" ");
  Serial.print("Y: "); Serial.print((int)lsm.accelData.y);       Serial.print(" ");
  Serial.print("Z: "); Serial.print((int)lsm.accelData.z);     Serial.print(" ");
  Serial.print("Mag X: "); Serial.print((int)lsm.magData.x);     Serial.print(" ");
  Serial.print("Y: "); Serial.print((int)lsm.magData.y);         Serial.print(" ");
  Serial.print("Z: "); Serial.println((int)lsm.magData.z);       Serial.print(" ");
  Serial.print("realAccel X: "); Serial.print(realAccel[0]);     Serial.print(" ");
  Serial.print("Y: "); Serial.print(realAccel[1]);         Serial.print(" ");
  Serial.print("Z: "); Serial.println(realAccel[2]);       Serial.print(" ");
  // see appendix A in app note AN3192 
  float pitch = asin(-realAccel[X]);
  float roll = asin(realAccel[Y]/cos(pitch));
  Serial.print("Pitch: "); Serial.print(pitch);         Serial.print(" ");
  Serial.print("Roll: "); Serial.println(roll);       Serial.print(" ");
  
    Serial.print(heading);  // this only works if the sensor is level
    Serial.print("\t\t");  // print some tabs
    Serial.print(tiltheading);  // see how awesome tilt compensation is?!
    Serial.print("North pixel:");
    Serial.print(pix);
    Serial.print("\n\r");
  }
  
  affiche_cap(pix);

  delay(10);
}

void affiche_cap(int nord)
{
  int sud=nord-(strip.numPixels()/2);
  if( sud<0) { sud=nord+strip.numPixels()/2 ;}
  
  int ouest=nord-(strip.numPixels()/4);
  if (ouest<0)  {nord+(strip.numPixels()/4);}
  
  int est=ouest-(strip.numPixels()/2);
  if (est<0) {est=ouest+(strip.numPixels()/2);}
  
  for (int p=0; p < strip.numPixels(); p++) {
    strip.setPixelColor(p, 0);
  } 
  strip.setPixelColor(pix,strip.Color(255,0,0));
  strip.setPixelColor((pix==0)?strip.numPixels()-1:pix-1,strip.Color(255/8,0,0));
  strip.setPixelColor((pix==strip.numPixels())?0:pix+1,strip.Color(255/8,0,0));
  strip.setPixelColor(pix,strip.Color(255,0,0));
  
  strip.setPixelColor(sud,strip.Color(255,255,0));   
  strip.setPixelColor(est,strip.Color(0,255,0));
  strip.show();
}

void loop () {
  // make a pretty rainbow cycle!
// wave(strip.Color(0,127,0), 4, 20); 
loopreal();
  
}

float getHeading(int * magValue)
{
  // see section 1.2 in app note AN3192
  float heading = 180*atan2(magValue[Y], magValue[X])/PI;  // assume pitch, roll are 0
  
  if (heading <0)
    heading += 360;
  
  return heading;
}

float getTiltHeading(int * magValue, float * accelValue)
{
  // see appendix A in app note AN3192 
  float pitch = asin(-accelValue[X]);
  float roll = asin(accelValue[Y]/cos(pitch));
  
  float xh = magValue[X] * cos(pitch) + magValue[Z] * sin(pitch);
  float yh = magValue[X] * sin(roll) * sin(pitch) + magValue[Y] * cos(roll) - magValue[Z] * sin(roll) * cos(pitch);
  float zh = -magValue[X] * cos(roll) * sin(pitch) + magValue[Y] * sin(roll) + magValue[Z] * cos(roll) * cos(pitch);

  float heading = 180 * atan2(yh, xh)/PI;
  if (yh >= 0)
    return heading;
  else
    return (360 + heading);
}

void looplm() {

   //strip.setPixelColor(0,strip.Color(0,255,0));
  //strip.setPixelColor(1,strip.Color(0,0,254));
  //strip.setPixelColor(2,strip.Color(255,0,0));
  
   for (int p=0; p < strip.numPixels(); p++) {
    strip.setPixelColor(p, 0);
  } 
  if (p>31) delta=-1;
  if (p<1) delta=1;
    strip.setPixelColor(p,Wheel(i++));
   p+=delta; 
    i%=384;
  strip.show();
 delay(250);
  }

void loopstrip() {

  // Send a simple pixel chase in...
  colorChase(strip.Color(127,127,127), 20); // white
  colorChase(strip.Color(127,0,0), 20);     // red
  colorChase(strip.Color(127,127,0), 20);   // yellow
  colorChase(strip.Color(0,127,0), 20);     // green
  colorChase(strip.Color(0,127,127), 20);   // cyan
  colorChase(strip.Color(0,0,127), 20);     // blue
  colorChase(strip.Color(127,0,127), 20);   // magenta

  // Fill the entire strip with...
  colorWipe(strip.Color(127,0,0), 20);      // red
  colorWipe(strip.Color(0, 127,0), 20);     // green
  colorWipe(strip.Color(0,0,127), 20);      // blue
  colorWipe(strip.Color(0,0,0), 20);        // black

  // Color sparkles
  dither(strip.Color(0,127,127), 50);       // cyan, slow
  dither(strip.Color(0,0,0), 15);           // black, fast
  dither(strip.Color(127,0,127), 50);       // magenta, slow
  dither(strip.Color(0,0,0), 15);           // black, fast
  dither(strip.Color(127,127,0), 50);       // yellow, slow
  dither(strip.Color(0,0,0), 15);           // black, fast

  // Back-and-forth lights
  scanner(127,0,0, 30);        // red, slow
  scanner(0,0,127, 15);        // blue, fast

  // Wavy ripple effects
  wave(strip.Color(127,0,0), 4, 20);        // candy cane
  wave(strip.Color(0,0,100), 2, 40);        // icy

  // make a pretty rainbow cycle!
  rainbowCycle(0);  // make it go through the cycle fairly fast

  // Clear strip data before start of next effect
  for (int i=0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, 0);
  }
}

// Cycle through the color wheel, equally spaced around the belt
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for (j=0; j < 384 * 5; j++) {     // 5 cycles of all 384 colors in the wheel
    for (i=0; i < strip.numPixels(); i++) {
      // tricky math! we use each pixel as a fraction of the full 384-color
      // wheel (thats the i / strip.numPixels() part)
      // Then add in j which makes the colors go around per pixel
      // the % 384 is to make the wheel cycle around
      strip.setPixelColor(i, Wheel(((i * 384 / strip.numPixels()) + j) % 384));
    }
//  j+=24;    
    strip.show();   // write all the pixels out
    delay(wait);
  }
}

// fill the dots one after the other with said color
// good for testing purposes
void colorWipe(uint32_t c, uint8_t wait) {
  int i;

  for (i=0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
      delay(wait);
  }
}

// Chase a dot down the strip
// good for testing purposes
void colorChase(uint32_t c, uint8_t wait) {
  int i;

  for (i=0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, 0);  // turn all pixels off
  }

  for (i=0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, c); // set one pixel
      strip.show();              // refresh strip display
      delay(wait);               // hold image for a moment
      strip.setPixelColor(i, 0); // erase pixel (but don't refresh yet)
  }
  strip.show(); // for last erased pixel
}

// An "ordered dither" fills every pixel in a sequence that looks
// sparkly and almost random, but actually follows a specific order.
void dither(uint32_t c, uint8_t wait) {

  // Determine highest bit needed to represent pixel index
  int hiBit = 0;
  int n = strip.numPixels() - 1;
  for(int bit=1; bit < 0x8000; bit <<= 1) {
    if(n & bit) hiBit = bit;
  }

  int bit, reverse;
  for(int i=0; i<(hiBit << 1); i++) {
    // Reverse the bits in i to create ordered dither:
    reverse = 0;
    for(bit=1; bit <= hiBit; bit <<= 1) {
      reverse <<= 1;
      if(i & bit) reverse |= 1;
    }
    strip.setPixelColor(reverse, c);
    strip.show();
    delay(wait);
  }
  delay(250); // Hold image for 1/4 sec
}

// "Larson scanner" = Cylon/KITT bouncing light effect
void scanner(uint8_t r, uint8_t g, uint8_t b, uint8_t wait) {
  int i, j, pos, dir;

  pos = 0;
  dir = 1;

  for(i=0; i<((strip.numPixels()-1) * 8); i++) {
    // Draw 5 pixels centered on pos.  setPixelColor() will clip
    // any pixels off the ends of the strip, no worries there.
    // we'll make the colors dimmer at the edges for a nice pulse
    // look
    strip.setPixelColor(pos - 2, strip.Color(r/4, g/4, b/4));
    strip.setPixelColor(pos - 1, strip.Color(r/2, g/2, b/2));
    strip.setPixelColor(pos, strip.Color(r, g, b));
    strip.setPixelColor(pos + 1, strip.Color(r/2, g/2, b/2));
    strip.setPixelColor(pos + 2, strip.Color(r/4, g/4, b/4));

    strip.show();
    delay(wait);
    // If we wanted to be sneaky we could erase just the tail end
    // pixel, but it's much easier just to erase the whole thing
    // and draw a new one next time.
    for(j=-2; j<= 2; j++) 
        strip.setPixelColor(pos+j, strip.Color(0,0,0));
    // Bounce off ends of strip
    pos += dir;
    if(pos < 0) {
      pos = 1;
      dir = -dir;
    } else if(pos >= strip.numPixels()) {
      pos = strip.numPixels() - 2;
      dir = -dir;
    }
  }
}

// Sine wave effect
#define PI 3.14159265
void wave(uint32_t c, int cycles, uint8_t wait) {
  float y;
  byte  r, g, b, r2, g2, b2;

  // Need to decompose color into its r, g, b elements
  g = (c >> 16) & 0x7f;
  r = (c >>  8) & 0x7f;
  b =  c        & 0x7f; 

  for(int x=0; x<(strip.numPixels()*5); x++)
  {
    for(int i=0; i<strip.numPixels(); i++) {
      y = sin(PI * (float)cycles * (float)(x + i) / (float)strip.numPixels());
      if(y >= 0.0) {
        // Peaks of sine wave are white
        y  = 1.0 - y; // Translate Y to 0.0 (top) to 1.0 (center)
        r2 = 127 - (byte)((float)(127 - r) * y);
        g2 = 127 - (byte)((float)(127 - g) * y);
        b2 = 127 - (byte)((float)(127 - b) * y);
      } else {
        // Troughs of sine wave are black
        y += 1.0; // Translate Y to 0.0 (bottom) to 1.0 (center)
        r2 = (byte)((float)r * y);
        g2 = (byte)((float)g * y);
        b2 = (byte)((float)b * y);
      }
      strip.setPixelColor(i, r2, g2, b2);
    }
    strip.show();
    delay(wait);
  }
}

/* Helper functions */

//Input a value 0 to 384 to get a color value.
//The colours are a transition r - g - b - back to r

uint32_t Wheel(uint16_t WheelPos)
{
  byte r, g, b;
  switch(WheelPos / 128)
  {
    case 0:
      r = 127 - WheelPos % 128; // red down
      g = WheelPos % 128;       // green up
      b = 0;                    // blue off
      break;
    case 1:
      g = 127 - WheelPos % 128; // green down
      b = WheelPos % 128;       // blue up
      r = 0;                    // red off
      break;
    case 2:
      b = 127 - WheelPos % 128; // blue down
      r = WheelPos % 128;       // red up
      g = 0;                    // green off
      break;
  }
  return(strip.Color(r,g,b));
}


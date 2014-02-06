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


#define DEBOUNCE 10 // button debouncer, how many ms to debounce, 5+ ms is usually plenty

// here is where we define the buttons that we'll use. button "1" is the first, button "6" is the 6th, etc
byte buttons[] = {9 , 10}; // the analog 0-5 pins are also known as 14-19
// This handy macro lets us determine how big the array up above is, by checking the size
#define NUMBUTTONS sizeof(buttons)
// we will track if a button is just pressed, just released, or 'currently pressed' 
byte pressed[NUMBUTTONS], justpressed[NUMBUTTONS], justreleased[NUMBUTTONS];

int blue_idx=0;
int red_idx=1;

//int period=250 ; // interval entre updates.
int period=1 ; // interval entre updates.
int mode; // loop swticher 



 

void setup() {
  // inputs
  byte i;
  for (i=0; i< NUMBUTTONS; i++) {
    pinMode(buttons[i], INPUT);   
  }  
  
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
//    while (1);
  }
  mode=0; // loop switcher
}

// function prototypes, do not remove these!
int colorChase(uint32_t c, uint8_t wait);
int colorWipe(uint32_t c, uint8_t wait);
int dither(uint32_t c, uint8_t wait);
int scanner(uint8_t r, uint8_t g, uint8_t b, uint8_t wait);
int wave(uint32_t c, int cycles, uint8_t wait);
int rainbowCycle(uint8_t wait);
uint32_t Wheel(uint16_t WheelPos);
int i=0,p=0,delta=1;
void check_switches();
int bousole();



int heading=0,tiltheading=0,pix=0,l=0;


int value_to_color(int v);
int value_to_color(int v)
{
   return(Wheel(abs((int)v %255)));
}

void check_switches()
{
  static byte previousstate[NUMBUTTONS];
  static byte currentstate[NUMBUTTONS];
  static long lasttime;
  byte index;

  if (millis() < lasttime) {
     // we wrapped around, lets just try again
     lasttime = millis();
  }
  
  if ((lasttime + DEBOUNCE) > millis()) {
    // not enough time has passed to debounce
    return; 
  }
  // ok we have waited DEBOUNCE milliseconds, lets reset the timer
  lasttime = millis();
  
  for (index = 0; index < NUMBUTTONS; index++) {
    justpressed[index] = 0;       // when we start, we clear out the "just" indicators
    justreleased[index] = 0;
     
    currentstate[index] = digitalRead(buttons[index]);   // read the button
    
    /*     
    Serial.print(index, DEC);
    Serial.print(": cstate=");
    Serial.print(currentstate[index], DEC);
    Serial.print(", pstate=");
    Serial.print(previousstate[index], DEC);
    Serial.print(", press=");
    */
    
    if (currentstate[index] == previousstate[index]) {
      if ((pressed[index] == LOW) && (currentstate[index] == LOW)) {
          // just pressed
          justpressed[index] = 1;
      }
      else if ((pressed[index] == HIGH) && (currentstate[index] == HIGH)) {
          // just released
          justreleased[index] = 1;
      }
      pressed[index] = !currentstate[index];  // remember, digital HIGH means NOT pressed
    }
    //Serial.println(pressed[index], DEC);
    previousstate[index] = currentstate[index];   // keep a running tally of the buttons
  }
}


void loop_debug_switch()
{
  while (1){
    check_switches();
    Serial.print("Red:");
    Serial.print(pressed[red_idx]);
    Serial.print(" Blue:");
    Serial.print(pressed[blue_idx]);
    Serial.print("\n");
    delay(250); 
  }
}

// return 0 except if both, return 1 
//int num_modes=4;
int process_switches()
{
  // 1/2% delta
  int inc=1;
  int min=1,max=25000;
  
  // both
  //if (!pressed[red_idx] && !pressed[blue_idx]) { 
  if (!pressed[blue_idx]) {     
    Serial.print("Got Both buttons, mode:");
    delay(DEBOUNCE*100); // sinon, y'en rentre 10-100...
    mode++;
    period=random(8)+1; //1-9
    period=period*period; // 1-81 exponentiel tendance en bas.
  //  if (mode>=num_modes) { mode=0; }
    Serial.print(period);
    Serial.print("\n");
    // reset this otherwise we get here again, pas claire... 
    pressed[red_idx]=pressed[blue_idx]=1 ;
    return 1;
  }
   // faster 
  //Serial.print(period);
  if (!pressed[red_idx]) { 
     period-=inc;   
  //   Serial.print(" is faster\n"); 
  }

  // slower
  if (!pressed[blue_idx]) { 
    period+=inc; 
  //  Serial.print(" is slower\n"); 
  }
  // but there are limits!
  if(period<min) { period = min;}
  if(period>max) { period = max;}
  
  return 0;
}


void loop () {
  
//  check_switches();
//loop_debug_switch();
//  looplm();
  switch(mode) {
    case 0 :
        Move();
//      allColors();
      break;
    case 1: 
      looplm();
      break;
    case 2:
      waveRoutine();
      break;
    case 3:
      rainbowCycle(0);      
      break;
    case 4:
      scannerRoutine();
      break;
    case 5:
      wipeRoutine();
      break;
    case 6: 
      ditherRoutine();
      break;
    case 7 :
      bousole();
      break;
    case 8 :
      colorChaseRoutine();
      break;
    default : 
      mode=0;
      ;;
  } 
}

int delay_n_poll(int p=period)
{

 for (int l=0; l< p ; l++ ) {
   check_switches() ; 
   if (1== process_switches()) {return 1;}
   delay(1);
 } 
   return 0;
}

int looplm() {
  byte r,g,b;
  uint32_t c;
  while (true){
  // black
   for (int p=0; p < strip.numPixels(); p++) {
    strip.setPixelColor(p, 0);
  } 
  // change de bord au boute et nouvelle couleur random
  if (p>31){ i=random(384); delta=-1; }
  if (p<1) { i=random(384) ;delta=1; }
  
  c=Wheel(i);
  strip.setPixelColor(p,c);
  // Need to decompose color into its r, g, b elements
  g = (c >> 16) & 0x7f;
  r = (c >>  8) & 0x7f;
  b =  c        & 0x7f; 
  // less less less less bright
  r=r>>4; b=b>>4 ; g=g>>4;
  
  c=strip.Color(r,g,b);
  strip.setPixelColor(p-1,c);
  strip.setPixelColor(p+1,c);
  p+=delta; 
  i%=384;
  strip.show();
  if (delay_n_poll()) return 1;
  }
  return 0; //impossible 
}

uint8_t myFavoriteColors[][3] = {{200,   0, 200},   // purple
                                 {200,   0,   0},   // red 
                                 {200, 200, 200},   // white
                               };
// don't edit the line below
#define FAVCOLORS sizeof(myFavoriteColors) / 3
 
// mess with this number to adjust TWINklitude :)
// lower number = more sensitive
#define MOVE_THRESHOLD 350

int Move(){
 
  // Take a reading of accellerometer data
  lsm.read();
 // Serial.print("Accel X: "); Serial.print(lsm.accelData.x); Serial.print(" ");
 // Serial.print("Y: "); Serial.print(lsm.accelData.y);       Serial.print(" ");
 // Serial.print("Z: "); Serial.print(lsm.accelData.z);     Serial.print(" ");
 
  // Get the magnitude (length) of the 3 axis vector
  // http://en.wikipedia.org/wiki/Euclidean_vector#Length
  double storedVector = lsm.accelData.x*lsm.accelData.x;
  storedVector += lsm.accelData.y*lsm.accelData.y;
  storedVector += lsm.accelData.z*lsm.accelData.z;
  storedVector = sqrt(storedVector);
  //Serial.print("Len: "); Serial.println(storedVector);
  
  // wait a bit
  if (delay_n_poll(100)) return 1;

//  delay(100);
  
  // get new data!
  lsm.read();
  double newVector = lsm.accelData.x*lsm.accelData.x;
  newVector += lsm.accelData.y*lsm.accelData.y;
  newVector += lsm.accelData.z*lsm.accelData.z;
  newVector = sqrt(newVector);
//  Serial.print("New Len: "); Serial.println(newVector);

  int force=abs(newVector - storedVector);  
  // are we moving 
  if (force > MOVE_THRESHOLD) {
    // we want dividor between 50 - 1000 (sky is the limit-> slow
    // we have period [ 1 - 5-10k ] sky is the limit->slow 
//    int dividor=period; // for now take period strit, could be scaled.
    int dividor=250; // for now take period strit, could be scaled.
    
    // between 1 and 10 maybe, based on ? period?
    // 1- 10k -> 1 - 10 
    int flash_wait=period/1000 ;

    // needed after transition from other state, not at boot...
    for (int p=0; p < strip.numPixels(); p++) {
      strip.setPixelColor(p, 0);
    } 


//    Serial.println("Twinkle! dividor:");    Serial.println(dividor);
    if ( flashRandom(flash_wait, force/dividor) ) {return 1;}  // first number is 'wait' delay, shorter num == shorter twinkle
//    flashRandom(2, 5);  // second number is how many neopixels to simultaneously light up
//    flashRandom(2, 7);
  }
  return 0;
}

  // Need to decompose color into its r, g, b elements
int C_to_g(uint32_t c) { return  ((c >> 16) & 0x7f) ;}
int C_to_r(uint32_t c) { return (c >>  8) & 0x7f; }
int C_to_b(uint32_t c) {return   c        & 0x7f;  }

 
int flashRandom(int wait, uint8_t howmany) {
 
  byte ps[howmany]; // pixels
  uint32_t cos[howmany]; // colors
  byte steps[howmany]; // steps to fade
  byte max_steps=0; 
  if (howmany > strip.numPixels() ) { howmany=strip.numPixels();} 
  
  for(uint16_t i=0; i<howmany; i++) {
    // pick a random favorite color!
//    int c = random(FAVCOLORS);
    uint32_t c = Wheel(random(384));
 
    // get a random pixel from the list
    int j = random(strip.numPixels());
    for (int ii=0; ii<i ; ii++ ) { 
      // if we find this color, pick another and look again at the start
      if ( ps[ii] == j ) { j=random(strip.numPixels()); ii=0 ;} 
    }
    ps[i]=j;
    cos[i]=c;
    steps[i]=random(4)+1 ;// 1 - 5 steps
    if (steps[i] > max_steps ) {max_steps=steps[i];} 
    
  }
  // now we will 'fade' it in x steps
  
  for (int x=0; x < max_steps; x++) {
  
  for(uint16_t i=0; i<howmany; i++) {
    // pick a random favorite color!
    uint32_t c = cos[i];
    int red = C_to_r(c);
    int green = C_to_g(c);
    int blue = C_to_b(c);
//    int s=steps[i]; // This version would make each pixel independant, but need to integrate loop below
    int s=max_steps ; // steps
    //Serial.print("Lighting up "); Serial.println(j); 
    
      int r = red * (x+1); r /= s;
      int g = green * (x+1); g /= s;
      int b = blue * (x+1); b /= s;
      
      strip.setPixelColor(ps[i], strip.Color(r, g, b));
      strip.show();
      if (delay_n_poll(wait)) return 1;
  }
  }
    Serial.println("Max_steps: ");
    Serial.println(max_steps);
    Serial.println("\n");
  // & fade out in 5 steps
//max_steps=random(6)+2; // 2 - 8 steps 
  for (int x=max_steps; x >= 0; x--) {

  for(uint16_t i=0; i<howmany; i++) {

    // pick a random favorite color!
    uint32_t c = cos[i];
    int red = C_to_r(c);
    int green = C_to_g(c);
    int blue = C_to_b(c);
    int s=max_steps ; // this to change.
    
      int r = red * x; r /= s;
      int g = green * x; g /= s;
      int b = blue * x; b /= s;
      
      strip.setPixelColor(ps[i], strip.Color(r, g, b));
      strip.show();
      if (delay_n_poll(wait)) return 1;
    }
  }
  // LEDs will be off when done (they are faded to 0)
  return 0;
}
 

int allColors()
{
  static long cnt=0 ; 
  
//  for (int p=0; p < strip.numPixels(); p++) {
  //  strip.setPixelColor(p, 0);
 // } 
  int colors=3;
  for (int p=0; p < strip.numPixels(); p++) 
  {
//    if(0==p%1){ strip.setPixelColor(p,strip.Color((cnt++)+p,0,0)); }
//    if(0==p%2){strip.setPixelColor(p,strip.Color(0,cnt+p,0));}
//    if(0==p%3){strip.setPixelColor(p,strip.Color(0,0,cnt+p));}
    strip.setPixelColor(p,strip.Color(cnt,cnt,cnt));
  } 
  Serial.println(" Showing : ");
  Serial.println(cnt);
//  Serial.println("Showing : ");
  strip.show();

  if (cnt++>127) {cnt=0;}  

  if (delay_n_poll()) return 1;
  return 0;
}

int colorChaseRoutine() {
// Send a simple pixel chase in...
  uint32_t colors[]= { strip.Color(127,127,127),  // white
                    strip.Color(127,0,0),    // red
                    strip.Color(127,127,0),  // yellow
                    strip.Color(0,127,0),    // green
                    strip.Color(0,127,127),  // cyan                    
                    strip.Color(0,0,127),    // blue
                    strip.Color(127,0,127),  // magenta
                    strip.Color(127,127,127),  // white
                    strip.Color(127,0,0),      // red
                    strip.Color(127,127,0),  // yellow
                    strip.Color(0,127,0),    // green
                    strip.Color(0,127,127),  // cyan
                    strip.Color(0,0,127),      // blue
                    strip.Color(127,0,127)    // magenta
  } ;
  for (int t=0; t<sizeof(colors)/sizeof(uint32_t) ; t++ ){
    if (colorChase(colors[t],20)) {return 1;}
  }
  return 0;
}

#define CANDY_CANE strip.Color(0,0,100)
#define RED strip.Color(127,0,0)
#define CYAN  strip.Color(0,127,127)
#define BLACK strip.Color(0,0,0)
#define GREEN strip.Color(0,127,0)
#define MAGENTA strip.Color(127,0,127)
#define YELLOW  strip.Color(127,127,0)
#define BLUE strip.Color(0,0,127)
#define RANDOM Wheel(random(384))

int waveRoutine() {
   uint32_t colors[]= { CANDY_CANE, YELLOW, GREEN, RED,RANDOM,RANDOM,RANDOM,RANDOM,RANDOM } ;
  for (int t=0; t<sizeof(colors)/sizeof(uint32_t) ; t++ ){
    if (wave(colors[t],2,20)) {return 1;}
  }
  return 0;
}

int scannerRoutine() {
   int r,g,b;
   uint32_t colors[]= { RED, BLUE,RANDOM, RED,RANDOM,RANDOM,RANDOM,RANDOM,RANDOM } ;
  for (int t=0; t<sizeof(colors)/sizeof(uint32_t) ; t++ ){
        g = (colors[t] >> 16) & 0x7f;
        r = (colors[t] >>  8) & 0x7f;
        b =  colors[t]        & 0x7f; 
        if  (scanner(r,g,b,20)) {return 1;}
  }
  return 0;

}


int ditherRoutine() {
   uint32_t colors[]= { BLACK,CYAN , BLACK, MAGENTA,BLACK, YELLOW,BLACK, Wheel(random(384)),BLACK,RANDOM,BLACK,RANDOM,BLACK,RANDOM,BLACK,RANDOM} ;
  for (int t=0; t<sizeof(colors)/sizeof(uint32_t) ; t++ ){
    if (dither(colors[t],20)) {return 1;}
  }
  return 0;
}
int wipeRoutine() {
   uint32_t colors[]= { RED , GREEN, BLUE,BLACK,RANDOM,BLACK,RANDOM,BLACK,RANDOM};
  for (int t=0; t<sizeof(colors)/sizeof(uint32_t) ; t++ ){
    if (colorWipe(colors[t],20)) {return 1;}
  }
  return 0;
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
int rainbowCycle(uint8_t wait) {
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
    if (delay_n_poll()) { return 1; }
  }
  return 0;
}

// fill the dots one after the other with said color
// good for testing purposes
int colorWipe(uint32_t c, uint8_t wait) {
  int i;

  for (i=0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
      if (delay_n_poll()) {return 1;}
  }
  return 0;
}

// Chase a dot down the strip
// good for testing purposes
int colorChase(uint32_t c, uint8_t wait) {
  int i;

  for (i=0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, 0);  // turn all pixels off
  }

  for (i=0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, c); // set one pixel
      strip.show();              // refresh strip display
      //delay(wait);               // hold image for a moment
      if ( delay_n_poll() ) { return 1; } 
      strip.setPixelColor(i, 0); // erase pixel (but don't refresh yet)
  }
  strip.show(); // for last erased pixel
  return 0 ; 
}

// An "ordered dither" fills every pixel in a sequence that looks
// sparkly and almost random, but actually follows a specific order.
int dither(uint32_t c, uint8_t wait) {

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
    if (delay_n_poll()) { return 1;}
  }
  delay(250); // Hold image for 1/4 sec
  return 0;
}

// "Larson scanner" = Cylon/KITT bouncing light effect
int scanner(uint8_t r, uint8_t g, uint8_t b, uint8_t wait) {
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
    if ( delay_n_poll()) { return 1 ;}
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
  return 0 ;
}

// Sine wave effect
#define PI 3.14159265
int wave(uint32_t c, int cycles, uint8_t wait) {
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
    //delay(wait);
    if (delay_n_poll()) { return 1; }
  }
  return 0 ; 
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

int bousole(){
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
  if (0==l%2500) {
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
     Serial.print("mode:");
    Serial.print(mode);
    Serial.print("\n\r");

  }
  
  affiche_cap(pix);

  return delay_n_poll();
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


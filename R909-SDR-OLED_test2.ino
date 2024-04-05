// R909-SDR-OLED radio V3.2 
// 
//  "H:\nob\AIRBAND RECEIVER\9H5BM_5351\R909_PANEL\R909-SDR_1\R909-SDR-OLED_test\R909-SDR-OLED_test2"
//
// 2024.02.04  V3.2
//  AUTO FM 2 inc? ?
//  SCA at firstFM?
//
// 2024.01.14 85%/39%
//  FREQ display delayed 1 step at the special case
//  BAND NG 
//
//
// Issues & progress 2023.10
//  1. 5351 freq tune 8.333    for future study
//  2. Band width change  for future
//
// 
// si5351a ARDUINO LO  Ver3.0 (LCD&KEY, i2c SSD1306 LCD, Timer）
// Rotary switch:INT(D2,D3), Function switch: D4 port, Squelch:A3
// Si5351_ADDR = 0x60, CPU:Arduino PRO mini, LCD: SSD1306:

//Libraries definition
#include <EEPROM.h>
#include "Wire.h"


//////// OLED SSD1306 //// 
// Thanks Adafruit for providing the library of OLED SSD1306
#include <Adafruit_GFX.h>     //Adafruit  GFX https://github.com/adafruit/Adafruit-GFX-Library
#include <Adafruit_SSD1306.h> //Adafruit SSD1306 https://github.com/adafruit/Adafruit_SSD1306
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 64, &Wire);

//////// Si4732 //// https://github.com/pu2clr/SI4735
// Thanks PU2CLR for providing Si4735/32 libraries
#include <SI4735.h>
#define Si4732_ADDR 0x11                 // 
#define RESET_PIN 17
#define FM_STATION_FREQ 8020             // 80.2 MHz - Select FM802
SI4735 rx;

//////// Si5351a ////
// How to set fequency on Si5351a via i2c was based on TJ lab  
// Thanks Dr.Uebo "https://tj-lab.org/2017/03/13/si5351/"
#define Si5351_ADDR 0x60
// Si5351a clock
#define SI5351_CLK0 0
#define SI5351_CLK1 1
#define SI5351_CLK2 2
//initialise object SI5351 pll for 5351JBH library
//Si5351 si5351;         // start the object for the si5351
//#define XT_CAL_F 37000 //Si5351 calibration factor, adjust to get exatcly 10MHz. 
                       //Increasing this value will decreases the frequency and vice versa.

//pins assign
#define REA 2          // D2 2
#define REB 3          // D3 3
#define RESW A0        // Rotary encoder push switch A0 
#define FUNC_SW A2     // SW1:<80, SW2:<250, SW3:<430, SW4:<600  

#define SQLMUTE  4     // digital output SQUELCH MUTE  - MUTE AMP WHEN NO SIGNAL 

#define led_pin A1     // Panel orange LED

unsigned long int startF = 78000000;     // Changed to 78MHz of Japanese FM starting
unsigned long int interfreq =  21400000; // 21.40Mhz
int freq_correction = 0;

//  Rotary encoder handler for arduino. v1.1
//  Copyright 2011 Ben Buxton. Licenced under the GNU GPL Version 3. Contact: bb@cactii.net  
//  http://www.buxtronix.net/2011/10/rotary-encoders-done-properly.html
//  Rotary encoder test debug sketch
//  PWM/3: Rotary A --> D2
//  PWM/2: Rotary B --> D3
//  GND: C
#include <Rotary.h>
#define HALF  // better 
Rotary r = Rotary(REA, REB);

// Assign function number 1-8 for short pushing at FUNCTION mode
// long pushing at 1:FREQ -> 9:FREQSET, 2:STEP-> 10:STEPSET
// 3:MEMORY-> 11:MMEMORYPUT, 4:SCAN->12:SCANAUTO, 
// 5:VOLUME&6:SQUELCH&7:BAND->9:FREQSET

//  !!! change number of function
//
// 12 modes defined 0:NONE,1:FREQ,2:STEP,3:MEMORY,4:SCAN,5:VOLUME,6:SQUELCH,
//  7:BAND,8:FUNCTION,9:FREQSET,10:STEPPUT,11:MEMORYPUT,12:SCANAUTO,
// STATE 1:FR,2:ST,3:ME,4:SC,5:vol,6:squ,7:band,8:func
// STATE+RE -> parameter increase/decrease
#define NONE 0          //
#define FREQ 1
#define STEP 2
#define MEMORY 3
#define SCAN 4
#define VOLUME 5
#define SQUELCH 6
#define BAND 7          // AM/FM SELECTION 
#define FUNCTION 10
#define FREQSET 11      // double push for FREQ set
#define STEPPUT 12      // double push for STEP set
#define MEMORYPUT 13    // double push for memory F set
#define SCANAUTO 14     // double push for Automatic SCAN set
#define BAND_W 8        // band width select
#define F_COR 9         // frequency error correction value inc/dec
#define BAND_WSet 15    // band width set
#define F_CORSet 16     // frequency error correction value set


// EEPROM Address
#define FREQ_AD 0       // EEPROM address for FREQ data as long
#define FSTEP_AD 4      // EEPROM address for FSTEP data as long 
#define VOL_AD 400      // EEPROM ADDRESS for stored volume 
#define SQU_AD 404      // EEPROM address for squelch setting 
#define BAND_AD 406     // EEPROM ADDRESS for band 
#define previous_freqFM_AD 408 // EEPROM ADDRESS for previous_freqFM
#define previous_freqAM_AD 412 // EEPROM ADDRESS for previous_freqAM
#define MCHAN0 8        // MCHAN start ADD, MCHAN0 + 4 * chan  chan: 0- 50 
#define BAND_W_AD 414   // EEPROM ADDRESS for BAND width 
#define F_COR_AD 416    // frequency error correction value

//
#define Time_elapsedRSSI 300  // RSSI check period mS

// Band AM or FM at Si4732
#define AM_FUNCTION 1
#define FM_FUNCTION 0

//---------------------------------------------------------
// functions
  void FUNC_Disp ( char*);              //  cursor(0,23)
  void Step_Disp(void);
  void Freq_Disp(void);
  void Band_setting(void);
  void Vol_Squ_Disp(void);

  void s_meter_Disp(void);
  void clearLCDLine (int, int, int);
  void snr_rssi_Disp(void) ;
  
  void layout(); 
  void drawbargraph(void);

  void startup_text(void);
  void sprintfreq(char*, int, int, int);
  
  void LongToStr(long , char*);
  void Fdds_Space(char *);
  
  void send_frequency(long, int);
  void set_freq(unsigned long);         // TJ lab's routine entry
  void si5351_init(void);
  void cmd_si5351(char , char);
  void setstep();
  
  int function_key(void);
  int mode_define(unsigned char) ;
  void rotary_event(uint8_t);
  
  void TimerCount(void);
  void blink(long, long);
  
  void putFirstSettings(void) ;
  void ShowSettings(void);
  void getSettings(void);

  void freqEEPROMput(int, long*);
  void freqEEPROMget(int, long);  
  
  void setVolume (uint8_t );
  void set21400kHz (void);
  void set802MHz (void);
  void setFMfreq (uint16_t );


  
// variables
boolean if_mode = 1;             //  1：LO mode 21.4MHz above
volatile int mode = FUNCTION;    //  current mode 
// When mode is FUNCTION, mode_temp is candidate to be selected by click. Blinking.
volatile char mode_last = FREQ;  //  last time mode as FREQ
volatile char mode_temp = FREQ;  //  candidate
volatile char memo_ad = 0;       //  select  memory address
unsigned char set_sw = 1;        //  set freq on memory, or step to freq
volatile unsigned long int freq1, freq0, freq_0, freq_last;

volatile unsigned long int previous_freqFM = 80200000;   // FM802 in OSAKA
volatile unsigned long int previous_freqAM = 118100000;  // ITM TOWER
volatile unsigned long int fstep ;
uint8_t fstep_idx=4;                   // 1-6
uint8_t bandAM_idx=0;                  // 0-6
volatile unsigned long int_time = 0 ;
volatile unsigned long time0 = 0;
volatile unsigned long time1 = 0;
volatile long  Last_millis;
int Timer_LED = 1000;
int Timer_mash_i = 500;
int Timer_mash = 0;
int Timer_RESW_i = 50;
int Timer_RESW = -1;
int Timer_scan = -1;
int Timer_DISP_REF = 50;
int Timer_Mem_Write = 500;
int Timer_Mem_Write1 = 500;
volatile boolean Mem_Write1=0;

volatile boolean j, k;                   // led/LCD blinking status
volatile int ADCdata;
volatile char scan_ad = 0, scan_ad_last=0;   //  scanning address counter
volatile char i = 0;
volatile unsigned char re_result = 0;    // Rotary switch active result 0x10:right,0x20:left
volatile unsigned char result;

// 0:no,1:FR,2:ST,3:ME,4:SC,9:vol,10 squelch , 11 band, 12 func
char freqdisp[5] = { 0x46, 0x52, 0x45, 0x51, 0 };        // To display FRQ mode = 1
char stepdisp[5] = {0x53, 0x54, 0x45, 0x50, 0 };         // To display STP mode = 2
char mdisp[6] = {0x4d, 0x45, 0x4d, 0x30, 0x30 ,0};       // To display MEM number as mode = 3
char scandisp[6] = { 0x53, 0x43, 0x4e, 0x30 , 0x30 , 0 };// To display SCA channel mode = 4
char ascandisp[6] = {0x41, 0x53, 0x41, 0x30, 0x30 , 0 }; // To display ASA(Automatic scanning) mode = 14
char voldisp[6] = {0x56, 0x4f, 0x4c, 0x30 ,0x30 , 0};    // To display VOL mode = 5
char squdisp[6] = {0x53, 0x51, 0x55, 0x30 ,0x30 , 0};    // To display SQU SQUELCH LEVEL mode = 6
char banddisp[5] = {0x42, 0x41, 0x4e, 0x44, 0 };         // To display BAND mode = 7
char funcdisp[5] = { 0x46, 0x55, 0x4e, 0x43, 0 };        // To display FUNC mode = 10
char b_wdisp[6] = {0x42, 0x20, 0x57, 0x30 ,0x30 , 0};    // To display band width = 8
char f_erdisp[6] = {"F_COR"};   // To display Si5351a freq error correction  = 9

char spdisp[6] = { 0x20, 0x20, 0x20, 0x20, 0x20, 0 };    // SPACE

char eachdisp[8] = { 0, 0, 0, 0, 0, 0,0,0 };

const char *bandwidthAM[]={"6","4","3","2","1","1.8","2.5"};

boolean RESW_last = 1;
boolean RESW_value_last = 1;
boolean RESW_pending = 0;
int RESW_result_p;
volatile unsigned char SW_result = 0;   // no:0, one click:2, double click:3
volatile unsigned char SW_result0 = 0;
volatile unsigned int sw_value, sw_stat;
int func_sw_value;

int VolumeLevel = 15, last_VolumeLevel = 15;   // 
uint8_t vl_pos, vl_ind;
uint8_t sq_pos, sq_ind;
 
int SquelchLevel = 0, last_SquelchLevel = 0;    // 

boolean BandSelect = false, BandSelect_last= false; // am/fm:false
boolean last_BandSelect = true;     // am:true/fm:false

// LCD SECOND LINE DOUBLE USE
boolean valid = false;
unsigned long previousMillis = 0;
const long interval = 5000;         //5 seconds
unsigned long elapsedRSSI;


// for squelch audio detection
int val = 0;
int timer = 0;
int maxi = 0;
unsigned char s_value;
boolean  s_dot_disp = 0;
unsigned char rssi, snr;
int ascan_hold = 0;

int debug=0,state1=0, state2=0;    // Debug counter

// Rotary SW functions  Interrupt handler
ISR(PCINT2_vect) {
  re_result = r.process();    // DIR_CW re_result==0x10, 
                              // DIR_CCW re_result==0x20
}

// -----------------------------------------------------------
// Power up setting section
void setup() {
// to configure a mode of the pins
  pinMode(led_pin, OUTPUT);             // monitor LED
  digitalWrite( led_pin, 1 );           // LED on
 
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, HIGH);

  pinMode(REA, INPUT_PULLUP);           // Rotary encoder port  
  pinMode(REB, INPUT_PULLUP);           // 
  pinMode(RESW, INPUT_PULLUP);          // Rotary encoder push SW

  pinMode(SQLMUTE, INPUT_PULLUP);       // amp mute when no signal detected

  delay(200);
// i2c starts up and SSD1306 sets up
  Wire.begin();  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(WHITE);

// Opening message
  display.setTextSize(2);
  display.setCursor(0, 25);
  display.print(F("R909SDRV32"));
  display.setCursor(92, 48);
  display.print(F("kpa "));
  display.display();
  delay(200);
  
////////////// 4732 set up section ///////////
  int16_t si4735Addr = rx.getDeviceI2CAddress(RESET_PIN);
  if ( si4735Addr == 0 ) {    // 4732 address check  0x11 expected
    display.setTextSize(2);
    display.setCursor(0, 48);
    display.print(F("No 4732"));
    display.display();
    while (digitalRead(RESW)==1);     // If you start without 4732, push RESW.
  }

  rx.setup(RESET_PIN, AM_FUNCTION);   //  Let start default radio function 
  delay(200);

  rx.setVolume(10);           // once set Range 0-127
  delay(200);
  set21400kHz();              // Set Si4732 as 21.4MHz mother radio

  /////// Switch sence ////////////////////////////////////////////////////////
  SW_result0 = function_key();         // Freq mode set on power start
  if (SW_result0 == 0) if_mode = 1;    // When no action, set IF_mode for LO

  r.begin(true);                       // rotary encoder initialise
  Last_millis = millis();
 
  delay(50);
  
// EEPROM data recover for clock frequency data
// There are FREQ, FSTEP, and MCHAN0-9
  freqEEPROMget( FREQ_AD, freq1);      // FREQ data  recover from EPROM

  EEPROM.get( FSTEP_AD, fstep);        // fstep data from EEPROM Read
  EEPROM.get( VOL_AD, VolumeLevel);    // volume data  recover from EPROM
  EEPROM.get( SQU_AD, SquelchLevel);   // squelch data from EEPROM Read
  EEPROM.get( BAND_AD, BandSelect);    // band data  recover from EPROM
  EEPROM.get( previous_freqFM_AD, previous_freqFM );    // 
  EEPROM.get( previous_freqAM_AD, previous_freqAM );    // 
  EEPROM.get( BAND_W_AD, bandAM_idx);   // band with index recover from EPROM
  EEPROM.get( F_COR_AD,freq_correction);
  
  int d = digitalRead(RESW);           // If RESW is ON at starting up, 
  if (d == 0) {                        // EEPROM initialised
    putFirstSettings();
  }

//  getSettings();
  Freq_Disp();                        // display freq
  freq_0 = freq1 + interfreq;

  si5351_init();                     // initialise the si5351

  if (freq1 < 10000000 || freq1 > 200000000) { //10-200MHz
    freq1 = 118100000;
    putFirstSettings();
    EEPROM.put( FREQ_AD, freq1);      // FREQ data  118.1MHz for EPROM
    freq_0 =  118100000;              // changed 118.1MHz AM
    for ( memo_ad = 0 ; memo_ad < 50; memo_ad++ ) {
      EEPROM.put( FREQ_AD + 8 + memo_ad * 4, freq_0); // FREQ data  118.1MHz for eEPROM
    }
  }

  delay(500);
  if ( fstep == 1000 || fstep == 10000 || fstep == 100000 ||
       fstep == 25000 || fstep == 1000000 || fstep == 10000000) {
  }
  else {
    display.setTextSize(2);
    display.setCursor(0, 0);
    display.print(F("STP Err")) ;
    display.display();
    delay(1000);
    fstep = 100000;
    EEPROM.put( FSTEP_AD, fstep );   // fstep data 100kHz for EEPROM
    EEPROM.put( BAND_W_AD, bandAM_idx=0);   // band with index preset on EPROM
    EEPROM.put( F_COR_AD,0);
    delay(200);
  }
  
  digitalWrite( led_pin,0 );         // off
  
  // Set up INT function for RE
  PCICR |= (1 << PCIE2);
  PCMSK2 |= (1 << PCINT18) | (1 << PCINT19);   sei();
  re_result = 0;

  // volume
  rx.setVolume(VolumeLevel);      // Range 0-63

  // Display initialized data
  Freq_Disp();                    // Display current FREQ

  Vol_Squ_Disp();                 // Display current Volume & Squlch level
  layout();

  if (if_mode == 1) freq_0 = freq1 + interfreq ; // On IF mode FREQ must be 
                                                 // upper than RX FREQ

  if (BandSelect == true) {       // AM mode
    set21400kHz();                // Change to AM 21.4MHz mode
    send_frequency(freq1, 0);     // send it to si5351
  }
  else {                          // FM selected
    setFMfreq(freq1/10000);       // Change to FM mode   
  }


  // debug print out
  /*    Serial.print("j: ");        // LCD blinking status
      Serial.println(j);
      Serial.print("mode: ");
      Serial.println(mode);
      Serial.print("mode_temp: ");
      Serial.println(mode_temp);
      Serial.print("SW_result: ");
      Serial.println(SW_result);
      Serial.print("SW_result0: ");
      Serial.println(SW_result0);
      
      Serial.print("freq1: ");
      Serial.println(freq1);  
       */
       
  // To inform the end of starting up
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print(F("setup"));
  display.display();

  delay(1000);   
// Once erase screen
  display.clearDisplay();
}
// End of setup


//= =========================MAIN===================================
void loop() {
  TimerCount();                 // To manage timer counter
  digitalWrite(led_pin, 0);     // To turn off LED
  /*
      Serial.print("j: ");
      Serial.println(j);
      Serial.print("mode: ");
      Serial.println(mode);
      Serial.print("mode_temp: ");
      Serial.println(mode_temp);
      Serial.print("SW_result: ");
      Serial.println(SW_result);
      Serial.print("SW_result0: ");
      Serial.println(SW_result0);
  */
  // Basic function for timer control led on_off & LCD blink
  blink(200, 400);      // Blinking for monitor LED and FUNC display on OLED

  // Check SW action
  SW_result = function_key();           // Get RESW result(on:50mS push) SW_result == 1
  SW_result0 = mode_define(SW_result);  // Define SW status 0:no, 1:single, 2:double

  //  RE push switch to FUNC mode, push switch again to each mode
  if (SW_result0 == 1) {
    if (mode != FUNCTION)  {    //  Go into FUNC mode
      mode_last = mode;
      mode = FUNCTION;  //  0:no,1:FR,2:ST,3:ME,4:SC,5:vol,6:squelch,7:Band,
                        //  8:band width,9:Frequency correction,
                        //  10:func,11:FSet,12:STSet,13:MWrite,14:AScan,
                        //  15:band w set, 16:F COR set
    }
    else { //  If FUNC, every click goes each FUNC defined
      mode_last = mode;
      mode = mode_temp;
    }
  }
  if ( SW_result0 == 2 ) {      //  select double click
    mode_last = mode_temp;      // double push

// To define mode by RE double push switching **********
// long pushing at 1:FREQ -> 9:FREQSET, 2:STEP-> 10:STEPSET
// 3:MEMORY-> 11:MMEMORYPUT, 4:SCAN->12:SCANAUTO, 
// 5:VOLUME&6:SQUELCH&7:BAND->9:FREQSET
    switch (mode) {
      case FREQ:{
        mode = FREQSET;         // freq, vol, squ and band set
        break;
      }
      case STEP:{
        mode = STEPPUT;         // STP set
        break; 
      }
      case MEMORY:{
        mode = MEMORYPUT;       // Memory set
        break;
      }
      case SCAN:{
        mode = SCANAUTO;        // Automatic scanning
        break;    
      }
      case VOLUME:{
        mode = FREQSET;         // freq, vol, squ and band set
        break;    
      } 
      case SQUELCH:{
        mode = FREQSET;         // freq, vol, squ and band set
        break;    
      }            
      case BAND:{
        mode = FREQSET;         // freq, vol, squ and band set
        break;    
      }     

      case BAND_W:{
        mode = FREQSET;         // freq, vol, squ and band set
        break;    
      } 
      case F_COR:{
        mode = FREQSET;         // freq, vol, squ and band set
        break;    
      }       
      default:
        mode = FUNCTION;        // Go back to FUNC
    }
  }
// end of sw action

// Rotary encoder service
  if (re_result != 0) {         // If CW, re_result is 0x10, if CCW, 0x20.
    rotary_event(re_result);    // To start the concerning process
    re_result = 0;
  }
   
  
// process depended on mode
  switch ( mode ) {
    
    case FUNCTION: {                     // function mode is 12 
        if (j == 0) {                    // boolean j led blink state
          switch ( mode_temp ) {    
            case FREQ: {                 //  FREQ determining mode 1
                sprintf(eachdisp, freqdisp);
                break;
              }
            case STEP: {                 //  STEP select mode 2
                sprintf(eachdisp, stepdisp);     
                break;
              }
            case VOLUME: {               //  volume select mode 9
                sprintf(eachdisp, voldisp);   
                break;
              }
            case SQUELCH: {              //  squelch select mode 10
                sprintf(eachdisp, squdisp);              
                break;
              }
            case BAND: {                 //  band select mode 11
                sprintf(eachdisp, banddisp);                   
                break;
              }
            case MEMORY: {               //  MEMORY ch select mode 3
                sprintf(eachdisp, mdisp);
                break;
              }
            case SCAN: {                 //  SCAN select mode 4
                sprintf(eachdisp, scandisp) ;
                break;
              }
            case BAND_W: {               //  BAND width select mode 13
                sprintf(eachdisp, b_wdisp) ;
                break;
              }
            case F_COR: {                //  VFO error correction value 14
                sprintf(eachdisp, f_erdisp) ;
                break;
              }
              
            // next were added later
            default: sprintf(eachdisp, funcdisp);  //  Function select mode 13
          }
        }    
          if (j == 1) sprintf(eachdisp, funcdisp); //  LCD blink
      }                                  // end of process for FUNCTION
      break;
      
    case FREQ: {                         //  FREQ determining mode 1
        sprintf(eachdisp,  freqdisp ) ;   
        break;
      }
    case STEP: {                         //  STEP select mode 2
        sprintf(eachdisp,  stepdisp ) ;
        break;
      }

    case VOLUME: {                       //  volume select mode 5
        voldisp[3] = 0x30 | (VolumeLevel / 10) ;
        voldisp[4] = 0x30 | (VolumeLevel % 10) ;
        sprintf(eachdisp, voldisp ) ;
        break;
      }
    case SQUELCH: {                      //  squelch select mode 6
        squdisp[3] = 0x30 | (SquelchLevel / 10) ;
        squdisp[4] = 0x30 | (SquelchLevel % 10) ;      
        sprintf(eachdisp, squdisp ) ;
        break;
      }
    // band
    case BAND: {                         //  band select mode 7
        sprintf(eachdisp, banddisp ) ;
        delay(200);
        state2++;   // debug counter
        break;
      }
    case MEMORY: {                       //  MEMORY ch select mode 3
        mdisp[3] = 0x30 | (memo_ad / 10) ;
        mdisp[4] = 0x30 | (memo_ad % 10) ;
        sprintf(eachdisp, mdisp) ;       // 
        break;
      }

    case SCAN: {                         //  Manual step SCAN
      //  fm mode to be added
        scandisp[3] = 0x30 | (scan_ad / 10) ;     //  display scan chan
        scandisp[4] = 0x30 | (scan_ad % 10) ;
        sprintf(eachdisp,  scandisp ) ;        
        freqEEPROMget( MCHAN0 + scan_ad * 4, freq1); // read out MEM frequency
        
        if (freq_last == freq1)  break;

        if (BandSelect != BandSelect_last){
          Band_setting();
          BandSelect_last=BandSelect;
        }
     //   Freq_Disp();
        freq_0 = freq1  ;
        // FREQ is more than IF above IF frequency for RX FREQ
        if (if_mode == 1) freq_0 = freq1  + interfreq  ; 
 
        freq_last = freq1 ;
        if(BandSelect == false) {    // If FM mode, 
          setFMfreq(freq1/10000);    //
        }
        else {
          send_frequency(freq_0, 0); //send it to si5351
          set21400kHz();              // Change to AM 21.4MHz mode
        }
        break;
      }
      case BAND_W: {               //  BAND width select mode 13
        b_wdisp[3] = 0x30 | (bandAM_idx / 10) ;
        b_wdisp[4] = 0x30 | (bandAM_idx % 10) ;        
        sprintf(eachdisp, b_wdisp) ;
        break;
      }
      case F_COR: {                //  VFO error correction value 14
        
        sprintf(eachdisp, "%5d", freq_correction) ;
        break;
      }


    case FREQSET: {                      //  FREQ button long push 
        sprintf(eachdisp, freqdisp) ;    //  freq display
        
        freq0 = freq1;
        if(BandSelect == false) freq0 = -freq1;  // If FM, convert to negative
        EEPROM.put( FREQ_AD, freq0 );    // freq data into EEPROM

        EEPROM.put( VOL_AD, VolumeLevel);    // volume data  recover from EPROM
        EEPROM.put( SQU_AD, SquelchLevel);   // squelch data from EEPROM Read
        EEPROM.put( BAND_AD, BandSelect);    // band data  recover from EPROM
        EEPROM.put( BAND_W_AD, bandAM_idx);   // band with index preset on EPROM
        EEPROM.put( F_COR_AD,freq_correction);
        
        EEPROM.put( previous_freqFM_AD, previous_freqFM );    // 
        EEPROM.put( previous_freqAM_AD, previous_freqAM );    // 

        mode = FREQ;
        break;
      }
    case STEPPUT: {                 //  STEP written 
       sprintf(eachdisp, stepdisp) ; 
       if(Timer_Mem_Write<=0 && Mem_Write1==0){
         sprintf(eachdisp, spdisp) ;    // 
         EEPROM.put( FSTEP_AD, fstep ); // fstep data of 100kHz unit in EEPROM  
         Timer_Mem_Write=800;   
         Mem_Write1=1;     
       }
       if(Timer_Mem_Write>0 && j==1){       
         sprintf(eachdisp, spdisp) ;    // 2nd line 1st character step display
       }
       if(Timer_Mem_Write<=0){
         mode = FREQ;
         Mem_Write1=0;
       }
       break;
      }
    case MEMORYPUT: {                    //  MEMORY write  
        sprintf(eachdisp, mdisp) ;       //  
        
        freq0 = freq1;
        if( BandSelect == false) freq0 = -(freq1);
        EEPROM.put( MCHAN0 + memo_ad * 4, freq0); // Write FREQ on defined EEPROM

        display.clearDisplay(); 
        delay(300); 
        sprintf(eachdisp, spdisp) ;       //  line 2 first column is space
        layout();
        display.clearDisplay();  
        delay(300); 
        sprintf(eachdisp, mdisp) ;        //  line 2 first column is m
        layout();
        display.clearDisplay();  
        mode = FREQ;
        break;
      }
    case SCANAUTO: {                       //  AUTO SCAN mode from M-chan
 //     if (ascan_hold==0) {               // ascan_hold ?
 //       ascan_hold--;
 //       break;
 //     }
 //     else{
        if (Timer_scan <= 0) {             //  If timer up
          Timer_scan = 100;
          ascandisp[3] = 0x30 | (scan_ad / 10) ;   //  display scan chan
          ascandisp[4] = 0x30 | (scan_ad % 10) ;   //  display scan chan
          sprintf(eachdisp, ascandisp ) ;
         
          freqEEPROMget( MCHAN0 + scan_ad * 4, freq1); // read out MEM frequency

          scan_ad++;                        //　Changed 20201013
          if (scan_ad > 49) scan_ad = 0;
                    
          if (freq1 == 118000000){          // 118mhz
            Timer_scan = 0;
            break;                          // If 118MHz, then skip.
          }
          if(BandSelect == false ){         // If FM, skip FM freq
            BandSelect = true;
            Timer_scan = 0;
            break;  
          }  
          
          Freq_Disp();
          
          Band_setting();      

          if (if_mode == 1) freq_0 = freq1 + interfreq  ; // 
          send_frequency(freq_0, 0);        //send it to si5351
          delay(300);
          state2 = s_value;
          s_meter_Disp();                   // Get AGC voltage & change S-meter
          if ( s_value > SquelchLevel ) {
            Timer_scan = 2000 ;             // If squelch off, wait 2 sec
            ascan_hold = 2;
          }
        }
        break;
      }
 //   }
  }
  
  
  // S METER & squelch
  // CHECK TIMER FOR TIMEOUT
  if (( millis() - elapsedRSSI ) > Time_elapsedRSSI ){  // 300ms
    s_meter_Disp();
    elapsedRSSI = millis();
  // Squelch 
    if(SquelchLevel>rssi){
      pinMode (SQLMUTE, INPUT_PULLUP);    // Mute the amp 
      digitalWrite (led_pin, LOW);        //  LED OFF
    }
    else {
      pinMode(SQLMUTE, OUTPUT);
      digitalWrite (SQLMUTE, LOW);        // Activate the amp
      digitalWrite (led_pin, HIGH);       //  LED ON
    }
  }

  // SW1:<80, SW2:<250, SW3:<430, SW4:<600 
  // actual 0, 166, 356, 519
  // This set keeps 2 keys of SW1 & SW2
  func_sw_value = analogRead(FUNC_SW);
  if (func_sw_value < 900) {
    delay(10);
    sw_stat = 1;
    if (func_sw_value>100) sw_stat = 2; 
     
    switch ( sw_stat){
      case 1:
        mode = FREQ;           // freq mode 0
        break; 
      case 2:   
        mode = VOLUME;         // VOLUME mode 5
        break;      
    }


  }

// display
  
  if(Timer_DISP_REF<=0){      // Periodic display timer expired
    
    digitalWrite(led_pin, k++);    
//    FUNC_Disp(funcdisp);

    Freq_Disp();
    snr_rssi_Disp() ;
    Vol_Squ_Disp();
    
    layout();
    display.clearDisplay();    
    Timer_DISP_REF = 500;    // Display every 500mS
  }


}
//end of loop

// ***************************************
// **********************************



//  ************************************
//   FUNCTIONs
//  ************************************

void Band_setting() {             // Am/FM band replace on Si4732 and display the mode
  if( last_BandSelect != BandSelect){  // If band changes
   
    if (BandSelect == true) {     // FM->AM
      set21400kHz();              // Change to AM 21.4MHz mode
    }
    else if (BandSelect == false) {  // AM->FM
      setFMfreq(freq1/10000);     // Change to FM mode      
    }
    last_BandSelect = BandSelect ;
  }
  Freq_Disp();
}

//
void getSettings() {

  freqEEPROMget( FREQ_AD, freq1);   // freq1:FREQ data recover from EPROM 
                                    // freq0:as AM:+, FM:- freq1:absolute
                                    // When freq0 is minus, Bandselect becomes false:FM 
  EEPROM.get( FSTEP_AD, fstep);     // fstep data from EEPROM Read
  EEPROM.get( VOL_AD, VolumeLevel); // volume data  recover from EPROM
  EEPROM.get( SQU_AD, SquelchLevel);// squelch data from EEPROM Read
  EEPROM.get( BAND_AD, BandSelect); // band data  recover from EPROM
  EEPROM.get( F_COR_AD,freq_correction); 


  EEPROM.get( previous_freqFM_AD, previous_freqFM );    // 
  EEPROM.get( previous_freqAM_AD, previous_freqAM );    // 
}

// 
void putFirstSettings() {

  freq0 = freq1;  
  if( BandSelect == false) freq0 = -(freq1);
  EEPROM.put( FREQ_AD, freq0);            // FREQ data  100MHz for EPROM

//  freqEEPROMput(FREQ_AD, freq1);        // Put freq1 + BAND on EEPROM
  EEPROM.put( FSTEP_AD, fstep);           // fstep data from EEPROM Read
  EEPROM.put( VOL_AD, VolumeLevel);       // volume data  recover from EPROM
  EEPROM.put( SQU_AD, SquelchLevel);      // squelch data from EEPROM Read
  EEPROM.put( BAND_AD, BandSelect);       // band data  recover from EPROM
  EEPROM.put( F_COR_AD,freq_correction); 
   
  EEPROM.put( previous_freqFM_AD, previous_freqFM );    // 
  EEPROM.put( previous_freqAM_AD, previous_freqAM );    // 
  
  freq_0 =  118000000;
  
  for ( memo_ad = 0 ; memo_ad < 50; memo_ad++ ) {
    EEPROM.put( FREQ_AD + 8 + memo_ad * 4, freq_0); // FREQ data  AM 118.1MHz for EEPROM
  }
  freq_0 =  118100000;
  EEPROM.put( FREQ_AD + 8 , freq_0); // FREQ data  AM 118.1MHz for EEPROM #0
  
  freq_0 =  -80200000;
  EEPROM.put( FREQ_AD + 8 + 49 * 4, freq_0); // FREQ data  FM 80.2MHz for EEPROM #49
  freq_0 =  -85100000;
  EEPROM.put( FREQ_AD + 8 + 48 * 4, freq_0); // FREQ data  FM 85.1MHz for EEPROM #48
  delay(500);
}

// Long FREQ data changes to 10 digit numerics
void LongToStr(long dat, char *fm) {
  long fmdat;
  int i;
  fmdat = dat;  
  for (i = 0; i < 11; i++) {
    fm[10 - i] = ( fmdat % 10 ) | 0x30;
    fmdat = fmdat / 10;
  }
}

// Reducing preceding zero into space
void  Fdds_Space(char *fm) {
  int i;
  for (i = 0; i < 10; i++) {
    if ( fm[i] == 0x30) {
      fm[i] = 0x20;
    }
    else i = 11;
  }
}

// Function SEL determining  D4 port: RE push SW
// result 0:no key, 1:off-> on
int function_key() {
  result = 0;
  //  SW sensing to avoid chattering
  if (Timer_RESW <= 0) {
    boolean RESW_value = digitalRead(RESW);
    if ((RESW_value == 0) && (RESW_value_last == 1)) {
      Timer_RESW = Timer_RESW_i;
      RESW_value_last = 0;
      result = 1;
    }     if ((RESW_value == 1) && (RESW_value_last == 0)) {
      Timer_RESW = Timer_RESW_i;
      RESW_value_last = 1;
      result = 0;
    }
    RESW_value_last = RESW_value;
  }
  return result ;                 // Switch not settled 
}

// After key result(RESW_click), wait timer_mash. Then decide single/double
// result 0:no key, off-> on pending, 1:single click, 2:double click
int mode_define(unsigned char keyresult) {
  char result = 0;
  // To detect double click  Detect double within a cirtain second
  // After keyresult = 1;
  // waiting double click  (Timer_mash > 0): mash pending
  if ((Timer_mash > 0) && (keyresult == 1)) {  // Double RESW pushed
    RESW_result_p = 2;
  }
  else if ((Timer_mash <= 0) && (keyresult == 1)) { // Firstly RESW pushed
    Timer_mash = Timer_mash_i;             // Mash timer set
    RESW_result_p = 1;            // Once set single click
  }
  // waiting timer end
  else if (( Timer_mash <= 0 ) && (RESW_result_p >= 1)) {
    result = RESW_result_p;  // Stop to wait & report result
    RESW_result_p = 0;
  }
  return result;
}


// Timing managing function: When Time_value <= 0, timer expired
void TimerCount(void) {
  long Current_millis = millis();
  long time_dif = Current_millis - Last_millis;   // Calculate how many millis after last call
  Last_millis = Current_millis;                   // Remember current time
  if (Timer_mash > 0) Timer_mash = Timer_mash - time_dif; // To wait double click timing
  if (Timer_RESW > 0) Timer_RESW = Timer_RESW - time_dif; // To avoid chattering
  if (Timer_LED > 0) Timer_LED = Timer_LED - time_dif;    // To turn on/off for #13 LED
  if (Timer_scan > 0) Timer_scan = Timer_scan - time_dif;
  if (Timer_DISP_REF > 0) Timer_DISP_REF = Timer_DISP_REF - time_dif;    // Display refresh timing
  if (Timer_Mem_Write > 0) Timer_Mem_Write = Timer_Mem_Write - time_dif; // EEPROM put blink timing
}

//  LED on/off & OLED FUNCTION blink:j
void blink(long on_time, long off_time) {  // LED on_off, j effects FUNC on/off
  if ((Timer_LED <= 0) && (j == 1)) {
    digitalWrite(led_pin, 0 );       // Monitor Led off
    j = 0;                           // bit j off  j:LCD blink
    Timer_LED = off_time;
  }
  else if ((Timer_LED <= 0) && (j == 0)) {
    digitalWrite(led_pin, 1 );       // Mon Led on
    j = 1;                           // bit j off  j:LCD blink
    Timer_LED = on_time;
  }
}


void FUNC_Disp ( char* func){        // Display FUNC of #2 size on X:0,Y:25 
  display.setTextSize(2);
  display.setCursor(0, 25);
  display.print(func);
  display.print("TEST");
}


//********* display frequency *********
// Assemble the format of frequency value and display it on the OLED
void Freq_Disp() {              // Format freq data for display & display
  unsigned int m = freq1 / 1000000;
  unsigned int k = (freq1 % 1000000) / 1000;
  unsigned int h = (freq1 % 1000) / 1;

  display.clearDisplay();
  display.setTextSize(2);

  char buffer[15] = "";
  if (m < 1) {                   // In he case of less than MHz, kkk.hhh kHz
    display.setCursor(5, 24); 
    sprintf(buffer, "%003d.%003d", k, h);
  
  }
  else if (m < 100) {            // In the case of more than MHz and less than 100MHz, mm.kkk hhh MHz
    display.setCursor(5, 24); 
    sprintf(buffer, "%2d.%003d,%003d", m, k, h);
  }
  else if (m >= 100){            // In the case of more than 100MHz, mmm.kkk hh MHz
    unsigned int h = (freq1 % 1000) / 10;
    display.setCursor(5, 24); 
    sprintf(buffer, "%3d.%003d,%002d", m, k, h);
   }
  display.print(buffer);
}

// Serial.ino   
// show in serial the eeprom settings
void  ShowSettings() {
/*
  Serial.println("Stored EEPROM Settings");
  Serial.print("\nFreq = ");
  Serial.println(freq1);
  Serial.print("\nFstep = ");
  Serial.println(fstep);
  Serial.print("\nVolume = ");
  Serial.println(VolumeLevel);
  Serial.print("\nSquelch = ");
  Serial.println( SquelchLevel);
  Serial.print("\nBand = ");
  Serial.println(BandSelect);
  Serial.println("----------------------");
  Serial.print("\nfreq_0 = ");
  Serial.println(freq_0);
  Serial.print("\nSfreq1= ");
  Serial.println( freq1);
  Serial.print("\nif_mode = ");
  Serial.println(if_mode);
 */
}


// CHANGE! No change no setting
int32_t last_ffrequency0 = 0;
int32_t last_ffrequency1 = 0;
int32_t last_ffrequency2 = 0;

// calculate and send frequency code to Si5351...
// GITHUB Etherki library 0.01Hz
// Frequencies are indicated in units of 0.01 Hz. Therefore, if you prefer to work 
// in 1 Hz increments in your own code, simply multiply each frequency passed 
// to the library by 100ULL

// Serve 5351
void send_frequency(int32_t ffrequency, int fcontext) {
  switch (fcontext) {        // no use fcontext  only CLK0
    case 0:                                             // Port 1
      if ( ffrequency != last_ffrequency0 ){
        set_freq((uint64_t)ffrequency );                // Set the frequency on the Si5351a.
        last_ffrequency0 = ffrequency;
      }
      break;
    case 1:                                             // Port 2
      if ( ffrequency != last_ffrequency1 ){
        set_freq((uint64_t)ffrequency );
        last_ffrequency1 = ffrequency;
      }
      break;
    case 2:                                             // Port 3
      if ( ffrequency != last_ffrequency2 ){
        set_freq((uint64_t)ffrequency );
        last_ffrequency2 = ffrequency;
      }
      break;
  }
}


//
unsigned long int last_freq = 0;
unsigned long int last_ifclk = 0;
//
void SI_setfreq( unsigned long int freq) {
//  Serial.print("SI5351 Freq: ");
//  Serial.println(freq);
  if( last_freq != freq){
    send_frequency(freq , SI5351_CLK0);
    last_freq = freq;
  }
}

 
// Extract FREQ + BAND from EEPROM. Value is plus for AM and minus for FM
void freqEEPROMget( int addr, long freq){
  EEPROM.get( addr, freq);
  freq1 = abs(freq);          // set freq1 
  if( freq < 0){              // judge BandSelect
    BandSelect = false;
  }
  else BandSelect = true;
} 

void set21400kHz (void){
//  rx.setTuneFrequencyAntennaCapacitor(1);
// Starts default mother radio function
// (AM; from 21.3 to 21.5 MHz; 21.4 MHz; step 1kHz)
  rx.setAM(21300, 21500, 21400, 4);   // mother radio
  rx.setSeekAmLimits(21300, 21500);
  rx.setSeekAmSpacing(4); // spacing 10KHz
  rx.setTuneFrequencyAntennaCapacitor(1); // Set antenna tuning capacitor for SW.
  rx.setAutomaticGainControl(0,37);   // Enable AGC   
  delay(300);
}

void set802MHz (void){            // FM set
//  rx.setTuneFrequencyAntennaCapacitor(0);
// Starts default radio function and band 
// (FM; from 75 to 108 MHz; 80.2 MHz; step 100kHz)
  rx.setFM(7500, 10800, 8020, 10);   // Once FM receive
  rx.setSeekAmRssiThreshold(0);
  rx.setSeekAmSrnThreshold(10);
  rx.setTuneFrequencyAntennaCapacitor(0); // Set antenna tuning capacitor for FM
  rx.setAutomaticGainControl(1,0);   // Disable AGC
  delay(300);
}

void setFMfreq(uint16_t fm_freq){ // FM set
//  rx.setup(RESET_PIN, 0);
//  rx.setTuneFrequencyAntennaCapacitor(0);
// Starts default radio function and band 
// (FM; from 75 to 108 MHz; ? MHz; step 100kHz)
  rx.setFM(7500, 10800, fm_freq, 10);   // Once FM receive
  rx.setSeekAmRssiThreshold(0);
  rx.setSeekAmSrnThreshold(10);

  delay(300);
}  

//---------------------Si5351a section------
////////////////// TJ lab. Si5351a code ////////////////
void set_freq(unsigned long freqs) {
//    freqs [Hz]
//
//    fvco= fxtal*(a+b/c)  ( a:15 -- 90,   b:0 -- 1048575, c:1 -- 1048575 )
//    freq= fvco /(a+b/c)  ( a:4, 6--1800, b:0 -- 1048575, c:1 -- 1048575 )
//
//    P1= 128*a +   floor(128*b/c) - 512
//    P2= 128*b - c*floor(128*b/c)
//    P3= c
//
  
  int k;
  unsigned long M;
  unsigned int R;
  
  if(freqs<1500) freqs=1500; else if(freqs>280000000) freqs=280000000;
  
  if(     freqs> 150000000){M=4; R=0;}
  else if(freqs>=63000000){M=6; R=0;}
  else if(freqs>=27500000){M=14; R=0;}
  else if(freqs>=13000000){M=30; R=0;}
  else if(freqs>= 6500000){M=62; R=0;}
  else if(freqs>= 3000000){M=126; R=0;}
  else if(freqs>= 1500000){M=280; R=0;}  
  else if(freqs>=  700000){M=600; R=0;}
  else if(freqs>=  330000){M=1280; R=0;}
  else if(freqs>=  150000){M=1300; R=1;}
  else if(freqs>=   67000){M=1500; R=2;}
  else if(freqs>=   30300){M=1600; R=3;} 
  else if(freqs>=   14000){M=1800; R=4;}
  else if(freqs>=    7000){M=1800; R=5;} 
  else if(freqs>=    3500){M=1800; R=6;} 
  else{M=1800; R=7;}
  
  freqs*=M;
  freqs<<=R;
  unsigned long F25MHZ = 25000000 + freq_correction;
  unsigned long c=0xFFFFF;  
  unsigned long a=freqs/F25MHZ;
  unsigned long b=(long)((double)(freqs-a*F25MHZ)*(double)c/(double)F25MHZ);
  unsigned long dd=(128*b)/c;
  unsigned long P1=128*a+dd-512;
  unsigned long P2=128*b-c*dd;
  unsigned long P3=c;
  
  
  //Set fvco of PLL_A
    cmd_si5351(26,(P3>>8)&0xFF);        //MSNA_P3[15:8]
    cmd_si5351(27,P3&0xFF);             //MSNA_P3[7:0]
    cmd_si5351(28,(P1>>16)&0x03);       //MSNA_P1[17:16]
    cmd_si5351(29,(P1>>8)&0xFF);        //MSNA_P1[15:8]
    cmd_si5351(30,P1&0xFF);             //MSNA_P1[7:0]
    cmd_si5351(31,(P3>>12)&0xF0|(P2>>16)&0x0F);//MSNA_P3[19:16], MSNA_P2[19:16]
    cmd_si5351(32,(P2>>8)&0xFF);        //MSNA_P2[15:8]
    cmd_si5351(33,P2&0xFF);             //MSNA_P2[7:0]
  
  // Set MS0, MS1
  // a=M, b=0, c=1 ---> P1=128*M-512, P2=0, P3=1  
  if(M==4){
    P1=0;
    cmd_si5351(42,0);                   //MS0_P3[15:8]
    cmd_si5351(43,1);                   //MS0_P3[7:0]
    cmd_si5351(44,0b00001100);          //0,R0_DIV[2:0],MS0_DIVBY4[1:0],MS0_P1[17:16]
    cmd_si5351(45,0);                   //MS0_P1[15:8]
    cmd_si5351(46,0);                   //MS0_P1[7:0]
    cmd_si5351(47,0);                   //MS0_P3[19:16], MS0_P2[19:16]
    cmd_si5351(48,0);                   //MS0_P2[15:8]
    cmd_si5351(49,0);                   //MS0_P2[7:0]
    
    cmd_si5351(50,0);                   //MS1_P3[15:8]
    cmd_si5351(51,1);                   //MS1_P3[7:0]
    cmd_si5351(52,0b00001100);          //0,R1_DIV[2:0],MS1_DIVBY4[1:0],MS1_P1[17:16]
    cmd_si5351(53,0);                   //MS1_P1[15:8]
    cmd_si5351(54,0);                   //MS1_P1[7:0]
    cmd_si5351(55,0);                   //MS1_P3[19:16], MS0_P2[19:16]
    cmd_si5351(56,0);                   //MS1_P2[15:8]
    cmd_si5351(57,0);                   //MS1_P2[7:0] 
   }
   else{
    P1=128*M-512;
    cmd_si5351(42,0);                    //MS0_P3[15:8]
    cmd_si5351(43,1);                    //MS0_P3[7:0]
    cmd_si5351(44,(R<<4)&0x70|(P1>>16)&0x03);//0,R0_DIV[2:0],MS0_DIVBY4[1:0],MS0_P1[17:16]
    cmd_si5351(45,(P1>>8)&0xFF);         //MS0_P1[15:8]
    cmd_si5351(46,P1&0xFF);              //MS0_P1[7:0]
    cmd_si5351(47,0);                    //MS0_P3[19:16], MS0_P2[19:16]
    cmd_si5351(48,0);                    //MS0_P2[15:8]
    cmd_si5351(49,0);                    //MS0_P2[7:0]
    
    cmd_si5351(50,0);                    //MS1_P3[15:8]
    cmd_si5351(51,1);                    //MS1_P3[7:0]
    cmd_si5351(52,(R<<4)&0x70|(P1>>16)&0x03);//0,R1_DIV[2:0],MS1_DIVBY4[1:0],MS1_P1[17:16]
    cmd_si5351(53,(P1>>8)&0xFF);         //MS1_P1[15:8]
    cmd_si5351(54,P1&0xFF);              //MS1_P1[7:0]
    cmd_si5351(55,0);                    //MS1_P3[19:16], MS0_P2[19:16]
    cmd_si5351(56,0);                    //MS1_P2[15:8]
    cmd_si5351(57,0);                    //MS1_P2[7:0]
  }
  cmd_si5351(165,0);
  cmd_si5351(166,M); 
  
  cmd_si5351(177,0xA0);    // Reset PLL_A.  Mが変化（特にM=4からM>=6に変化）するときは必須かも…
}

//レジスタに１バイトデータを書き込む。 
void cmd_si5351(char Reg , char Data) {
   Wire.beginTransmission(Si5351_ADDR); 
   Wire.write(Reg); 
   Wire.write(Data); 
   Wire.endTransmission(); 
}

// Si5351a set up at 118.1+21.4=139.5MHz
void si5351_init(void){
    cmd_si5351(183,0b10010010); // CL=8pF
    cmd_si5351(16,0x80);      // Disable CLK0
    cmd_si5351(17,0x80);      // Disable CLK1
    set_freq(139500000);      // 139.5MHz
    cmd_si5351(177,0xA0);     // Reset PLL_A  
    cmd_si5351(16,0x4F);      // Enable CLK0 (MS0=Integer Mode, Source=PLL_A) 
//    cmd_si5351(17,0x4F);      // Enable CLK1 (MS1=Integer Mode, Source=PLL_A) 
}

//---------------------The end of Si5351a section------

void Vol_Squ_Disp(void){
  display.setTextSize(1);
//Volume indicator pointer
  display.setCursor(0, 47);
  display.print(F("V"));
  display.print(VolumeLevel);  
  display.print(F(" S"));  
  display.print(SquelchLevel);  
}

void snr_rssi_Disp() {
// For display SNR&RSSI 
  display.setTextSize(1);
  display.setCursor(40, 47);
  display.print(F("S"));
//  display.setCursor(61, 48);
  display.print(snr);
  display.print(F(" R"));  
//  display.setCursor(76, 48);
  display.print(rssi);      
}

// to get SNR:0-127,RSSI:0-127 from Si4732
void s_meter_Disp()   {
  char s_disp[11];
  unsigned char i;
  rx.getCurrentReceivedSignalQuality();
  rssi = rx.getCurrentRSSI();           // Get RSSI from Si4732
  snr = rx.getCurrentSNR();
  s_value = map(rssi, 0, 63, 0, 9);    // s max changed

  if(rssi!=0) digitalWrite(led_pin, HIGH);
  else  digitalWrite(led_pin, HIGH);    // if rssi, led ON
  
}

// S-meter bargraph
void drawbargraph() {
  display.setTextSize(1);
  // S-meter displaying   NEED to test
  display.setCursor(0, 57);
  display.print(F("SM"));  
  for ( int xx = s_value; xx >= 0; xx--){ 
    byte s_pos = 15+5*(xx-1);
    display.fillRect(s_pos, 58, 2, 6, WHITE);
  }
}

// To draw lines, texts on the bitmap and screen out
void layout() {
  display.setTextColor(WHITE);

  display.drawLine(0, 20, 127, 20, WHITE);  // Horizontal 1
  display.drawLine(0, 43, 127, 43, WHITE);  // Horizontal 2
  display.drawLine(72, 10, 127, 10, WHITE); // Horizontal 3
  display.drawLine(15, 55, 82, 55, WHITE);  // S meter
  display.drawLine(67, 2, 67 , 17, WHITE);  // Vertical 1
  display.drawLine(87, 48, 87, 63, WHITE);  // Vertical 2  
  
// To display step frequency  
  display.setTextSize(1);
  display.setCursor(70, 0);
  display.print(F("STP"));

  if (fstep_idx == 1) display.print(F(" 10Hz ")); 
  if (fstep_idx == 2) display.print(F(" 1kHz ")); 
  if (fstep_idx == 3) display.print(F("10kHz "));
  if (fstep_idx == 4) display.print(F("100kHz")); 
  if (fstep_idx == 5) display.print(F(" 1MHz "));
  if (fstep_idx == 6) display.print(F("25kHz "));

// To display IF frequency 
  display.setTextSize(1);
  display.setCursor(92, 48);
  display.print(F("IF: "));
  display.print(mode);  

  if(debug){                     // When debugging, display state1, 2
    display.setCursor(91, 57);
    display.print(F("ST"));
    display.print(state1);
    display.print(F(" "));
    display.print(state2);
  }
  else {                         // To display the interfreq
    display.setCursor(92, 57);
    display.print(interfreq);
    display.print(F("k"));
  }
  // To display the unit
  display.setTextSize(1);
  display.setCursor(110, 12);
  if (freq1 < 1000000) display.print(F("kHz"));
  else display.print(F("MHz"));
  
  display.setCursor(70, 12);
  if(BandSelect == true) {      // AM mode
    display.print(F("AM "));   
  }  
  else display.print(F("FM "));  
  if (interfreq == 0) display.print(F("VFO"));
  else display.print(F(" LO")); 

// To display mode & parameter
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print(eachdisp);
//
//
//  bandlist(); 
  drawbargraph();              // SM display
  display.display();
  delay(50);
}

// To convert fstep_idx to fstep value
void  setstep() {                     // fstep 1-6
  switch (fstep_idx) {
    case 1: fstep = 10; break;
    case 2: fstep = 1000; break;
    case 3: fstep = 10000; break;
    case 4: fstep = 100000; break;
    case 5: fstep = 1000000; break;
    case 6: fstep = 25000; break;
  }
}

// 12 modes defined 0:NONE,1:FREQ,2:STEP,3:MEMORY,4:SCAN,5:FREQSET,6:STEPPUT,
//  7:MMEMORYPUT,8:SCANAUTO,9:VOLUME,10:SQUELCH,11:BAND,12:FUNCTION
void rotary_event(uint8_t dir) {         // Rotary encoder Up/Down 
  digitalWrite( led_pin, 1 );            // LED on
  if (mode == FREQ) {                    // FREQ:mode 0 : Up/Down frequency
    if (dir == 0x10){                    // Clockwise
       freq1 = freq1 + fstep;
      
      // limit the frequency according to the band
      if (BandSelect == false) {         // FM mode limit
        if (freq1 > (long) 109000000) {  //  109MHz -> 76MHz
          freq1 = (long)   109000000;    //
        }
      }
      if (BandSelect == true) {          // AM mode limit
        if (freq1 > (long) 136000000) {  //  136MHz -> 118MHz
          freq1 = (long)   136000000;
        }
      }
      freq_0 = freq1;                // Written value for 5351a
      if (if_mode == true) {
        freq_0 = freq1  + interfreq  ;   // on IF mode 21.4MHz upper
      }
      if (BandSelect == true) {      // AM mode
 //       set21400kHz();               // Change to AM 21.4MHz mode
        send_frequency(freq_0, 0);   // send it to si5351
      }
      else {                         // FM mode
        setFMfreq(freq1/10000);      // Change to FM mode 
      }
      
    }
    if (dir == 0x20) {               // counter clockwise
      freq1 = freq1 - fstep;         // freq1:managing FREQ
      if (BandSelect == false) {     // fm mode
        if (freq1 < 76000000) {      // 76MHz->118MHz
          freq1 = 76000000;
        }
      }
      if (BandSelect == true) {      // am mode
        if (freq1 < 118000000) {     // 118MHz->
          freq1 = (long)   118000000;
        }
      }
      freq_0 = freq1;                // actual frequency for 5351a
      if (if_mode == true) {
        freq_0 = freq1  + interfreq  ;   // on IF mode IF-F upper
      }
      if (BandSelect == true) {
  //      set21400kHz();               // Change to AM 21.4MHz mode
        send_frequency(freq_0, 0);   //send it to si5351
      }
      else {
        setFMfreq(freq1/10000);      // Change to FM mode      
      }
    }
    Freq_Disp();
    
            state1=1;
  }
  
  else if (mode == STEP) {           // Up/Down step fstep_idx 1-6
    if (dir == 0x10) {               // Clockwise, increment  fstep_no
      fstep_idx ++;
      if (fstep_idx >= 7) fstep_idx = 1;
    }
    else if (dir == 0x20) {
      fstep_idx --;
      if (fstep_idx < 1) fstep_idx = 6;
    }
    setstep();                       // To effect the step frequency
  }  
  else if (mode == VOLUME) {         // Up/Down volume level 0-63
    if (dir == 0x10){
      rx.volumeUp();                 // Range 0-63
      VolumeLevel = rx.getVolume();
    }
    else if (dir == 0x20) {
      rx.volumeDown();
      VolumeLevel = rx.getVolume();
    }
              state1=2;
  }
  
  else if (mode == SQUELCH) {                  // Up/Down squelch level 0-14

    if (dir == 0x10)  SquelchLevel = SquelchLevel + 1;
    if (SquelchLevel > 63) SquelchLevel = 63;
    if (dir == 0x20) SquelchLevel = SquelchLevel - 1;
    if (SquelchLevel <= 0) SquelchLevel = 0;
  } 
  
  else if (mode == MEMORY) { // clockwise STEP
    if (dir == 0x10) {      
      memo_ad ++ ;
      if (memo_ad > 49)  memo_ad = 0;
    }
    else if (dir == 0x20) {// Counter clockwise STEP　
      memo_ad --;
      if (memo_ad < 0)  memo_ad = 49;
    }
  }
  else if (mode == SCAN) {// clockwise STEP
    if (dir == 0x10){  
      scan_ad ++;
      if (scan_ad > 49)  scan_ad = 0;
    }
    else if (dir == 0x20) {       // Counter clockwise STEP　
      scan_ad --;
      if (scan_ad < 0)  scan_ad = 49;
    }
    
  }
  else if (mode == BAND_W) {         // Up/Down step bandAM_idx 1-6
    if (dir == 0x10) {               // Clockwise, increment  fstep_no
      bandAM_idx ++;
      if (bandAM_idx >= 7) bandAM_idx = 0;
    }
    else if (dir == 0x20) {
      bandAM_idx --;
      if (bandAM_idx < 0) bandAM_idx = 6;
    }
    rx.setBandwidth(bandAM_idx,1);  // To effect the Si4732 AM band width
  }  

  else if (mode == F_COR) {          // Increase/decrease correction value by 1Hz
    if (dir == 0x10) {               // Clockwise, increment  correction value
      freq_correction ++;
      if (freq_correction >= 32768) freq_correction = 32767; // INT +32767 - 0 - -32767
    }
    else if (dir == 0x20) {
      freq_correction --;
      if (freq_correction <= -32768) freq_correction = -32767;
    }
  }  

  
  // Selectable mode 1:FR,2:ST,3:ME,4:SC,5:vol,6:squ,7:band,8:b_w,9:f_cor
  else if (mode == FUNCTION) {    // clockwise VOLUME
    if (dir == 0x10){
      mode_temp ++;
      if (mode_temp > 10) mode_temp = 10; //1-10   
    }
    else if (dir == 0x20) {      // Counter clockwise function
      mode_temp --;
      if (mode_temp < 0) mode_temp = 0; //
    }  
  }
  
// counter clockwise or clockwise band
// changed the BandSelect 
  else if (mode == BAND) { 
    BandSelect = !BandSelect ;  // AM->FM or FM->AM

  
// AM mode selected   
    if( BandSelect == true ){   // Change from FM to AM
      previous_freqFM = freq1;
      freq1 = previous_freqAM; // To recover the last AM freq
      freq_0 = freq1;
      if (if_mode == true) {      
        freq_0 = freq1 + interfreq ;  //  on IF mode 21.4MH upper
      }
      send_frequency(freq_0, 0);  //send it to si5351
    }
// FM mode selected
    else {                      // Change from AM to FM
      previous_freqAM = freq1;  // store the current AM freq
      freq1 = previous_freqFM;
      setFMfreq(freq1/10000);   // Set FM FREQ on Si4732      
    }
    
    Band_setting();             // To change 4732 setting
    Freq_Disp();                // update the frequency according to the band selected
  }
}

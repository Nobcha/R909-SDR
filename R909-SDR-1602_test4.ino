// SNR,RSSI SQU ok 2023.12.18
// R909-SDR-1602 radio V2.2  
// 4 function key added 2023.12.18
//
//
// R909OLED_test1 to be  202310.29 pending
//  1. 5351 freq tune 8.333    for future study
//  2. VOL from FM62429 to Si4732  OK
//  3. S-meter from AGC to Si4732 RSSI
//  4. AM set for 21.4MHz/AM on Si4732  AM mode
//  5. FM tune  If FM call FM tune  F display
//  6. signal band width change  for future
//
// Issues & progress
//  1. 5351 freq tune 8.333    for future study
//  2. Band width change  for future
//
// 
// si5351a ARDUINO LO  Ver3.0 (LCD&KEY, i2c SSD1306 LCD, Timer）
// Rotary switch:INT(D2,D3), Function switch: D4 port, Squelch:A3
// Si5351_ADDR = 0x60, CPU:Arduino PRO mini, LCD: SSD1306:
//           Nextly replace Arduino into PRO MINI and case in
//  LCD display 0123456789abcdef
//  SSD1306     FRQ  100.000MHz
//              ATS00 STP100kHz
//              S**********  IF

// lcd 16x2  with 4 bit address

#define lcdTypeLCD4bit

//Libraries definition
#include <EEPROM.h>
#include <Rotary.h>
#include <si5351JBH.h>
#include "Wire.h"
#include "Adafruit_LiquidCrystal.h"

Adafruit_LiquidCrystal lcd(8, 9, 10, 11, 12, 13);

//initialise object SI5351 pll
Si5351 si5351;         // start the object for the si5351
#define XT_CAL_F 37000 //Si5351 calibration factor, adjust to get exatcly 10MHz. 
                       //Increasing this value will decreases the frequency and vice versa.

//pins assign
#define RESW A0        // Rotary encoder push switch A0 
#define REA 2          // D2 2
#define REB 3          // D3 3

#define BANDRLY  5     // digital output to control relay
#define SQLMUTE  4     // digital output SQUELCH MUTE  - MUTE AMP WHEN NO SIGNAL 
#define led_pin A1     // Panel orange LED
#define lcd_bl  7      // LCD back light
#define FUNC_SW A2     // SW1:<80, SW2:<250, SW3:<430, SW4:<600  

unsigned long int startF = 78000000; // Changed to 78MHz of Japanese FM starting
unsigned long int rxclk =  21400000; // 21.40Mhz

// #define Si5351_ADDR 0x60
#define Si4732_ADDR 0x11   // 

// Assign function number short/long pushing
// 8 modes defined 0:no,1:FR,2:ST,3:ME,4:SC,5:FS,6:STS,7:MS,8:ASC,9:FU
// adding 2 MODES volume and squelch
#define NONE 0          //
#define FREQ 1
#define STEP 2
#define VOLUME 3
#define SQUELCH 4
#define BAND 5          // AM/FM SELECTION 
#define MEMORY 6
#define SCAN 7
#define FREQSET 8       // double push for FREQ set
#define STEPPUT 9       // double push for STEP set
#define MEMORYPUT 10    // double push for memory F set
#define SCANAUTO 11     // double push for Automatic SCAN set
#define FUNCTION 12

// EEPROM Address
#define FREQ_AD 0       // EEPROM address for FREQ data as long
#define FSTEP_AD 4      // EEPROM address for FSTEP data as long 
#define VOL_AD 400      // EEPROM ADDRESS for stored volume 
#define SQU_AD 404      // EEPROM address for squelch setting 
#define BAND_AD 406     // EEPROM ADDRESS for band 
#define previous_freqFM_AD 408 // EEPROM ADDRESS for previous_freqFM
#define previous_freqAM_AD 412 // EEPROM ADDRESS for previous_freqAM
#define MCHAN0 8        // MCHAN start ADD, MCHAN0 + 4 * chan  chan: 0- 50 

//
#define Time_elapsedRSSI 300  // RSSI check period mS

// Band AM or FM on Si4732
#define AM_FUNCTION 1
#define FM_FUNCTION 0

/*
  Rotary encoder handler for arduino. v1.1
  Copyright 2011 Ben Buxton. Licenced under the GNU GPL Version 3. Contact: bb@cactii.net  
  http://www.buxtronix.net/2011/10/rotary-encoders-done-properly.html
  Rotary encoder test debug sketch
  PWM/3: Rotary A --> D2
  PWM/2: Rotary B --> D3
  GND: C
*/
#define HALF  // better 
Rotary r = Rotary(REA, REB);

  // functions
  void LCD_Disp ( char, char, char*);
  void Step_Disp(void);
  void Dsp_Disp(void);
  void LongToStr(long , char*);
  void Fdds_Space(char *);
  //void set_freq(unsigned long);
  void si5351_init(void);
  //void cmd_si5351(char , char);
  int function_key(void);
  int mode_define(unsigned char) ;
  void s_meter_disp(void);
  void TimerCount(void);
  void blink(long, long);
  void putFirstSettings(void) ;
  void ShowSettings(void);
  void getSettings(void);
  void Band_Disp(void);
  void send_frequency(long, int);
  void Vol_Disp(void);
  void Squ_Disp(void);
  void squ_disp(void);
  void Band_disp(void);
  void clearLCDLine (int, int, int);
  void checkVolLimits(void);
  void checkSquLimits(void);
  void checkRX2(void );
  void checkRX(void);
  void updateFreq(long);
  void setVol(void);
  void freqEEPROMput(int, long*);
  void freqEEPROMget(int, long);  
  void setVolume (uint8_t );
  void set21400kHz (void);
  void set802MHz (void);
  void setFMfreq (uint16_t );
  
// variables
volatile boolean if_mode = 1;         //  1：LO mode 10.7MHz above
volatile unsigned char mode = FUNCTION;   //  select mode as FREQ
volatile unsigned char mode_last = FREQ;
volatile unsigned char mode_temp = FREQ;
volatile char memo_ad = 0;            //  select  memory address
unsigned char  set_sw = 1;            //  set freq on memory, or step to freq
volatile unsigned long int freq1, freq0, freq_0, freq_last;

volatile unsigned long int previous_freqFM = 80200000;   // FM802
volatile unsigned long int previous_freqAM = 118100000;  //ITM TOWER
volatile unsigned long int fstep ;
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
volatile boolean j;                   // led/LCD blinking status
volatile int ADCdata;
volatile char scan_ad = 0, scan_ad_last=0;            //  scanning address counter
volatile char i = 0;
volatile unsigned char re_result = 0; // Rotary switch active result 0x10:right,0x20:left
volatile unsigned char result;
char squdisp[4] = {0x53, 0x51, 0x55, 0}; // displays SQU SQUELCH LEVEL
char voldisp[4] = {0x56, 0x4f, 0x4c, 0}; // displays VOL
char mdisp[4] = {0x4d, 0x45, 0x4d, 0};   // displays  MEM
char stepdisp[4] = {0x53, 0x54, 0x50, 0 };  //STP
char spdisp[4] = { 0x20, 0x20, 0x20, 0 };   // SPACE
char scandisp[4] = { 0x53, 0x43, 0x4e, 0 }; //SCN
char freqdisp[4] = { 0x46, 0x52, 0x51, 0 }; //FRQ
char funcdisp[4] = { 0x46, 0x55, 0x20, 0 }; //FU
char ascandisp[4] = { 0x41, 0x53, 0x41, 0 }; // ASA
char banddisp[4] = { 0x42, 0x4e, 0x44, 0 };  // BND


boolean RESW_last = 1;
boolean RESW_value_last = 1;
boolean RESW_pending = 0;
int RESW_result_p;
volatile unsigned char SW_result = 0, SW_result0 = 0; //  no:0, pending:1, one:2, double:3
volatile unsigned int sw_value, sw_stat;

uint8_t VolumeLevel = 15;
uint8_t last_VolumeLevel = 15;   // Added 20221010

int8_t SquelchLevel = 0;
int8_t last_SquelchLevel = 0;   // Added 20221010

boolean BandSelect = false, BandSelect_last= false;      // am/fm:false
boolean last_BandSelect = true;  // am:true/fm:false

// LCD SECOND LINE DOUBLE USE
boolean valid = false;
unsigned long previousMillis = 0;
const long interval = 5000;      //5 seconds
unsigned long elapsedRSSI;


// for squelch audio detection
int val = 0;
int timer = 0;
int maxi = 0;
unsigned char s_value;
boolean  s_dot_disp = 0;
unsigned char rssi, snr;

////////////////// Si4732/////////////////
#include <SI4735.h>

#define RESET_PIN 17
#define FM_STATION_FREQ 8020            // 80.2 MHz - Select FM802

SI4735 rx;

// Rotary SW functions  Interrupt handler
ISR(PCINT2_vect) {
  re_result = r.process();              // DIR_CW=0x10,DIR_CCW=0x20
}

// Power up setting section
void setup() {
  // configure the pins
  pinMode(led_pin, OUTPUT);             // monitor LED
  digitalWrite( led_pin, 1 );           // on
  pinMode(lcd_bl, OUTPUT);              // LCD back light
  digitalWrite( lcd_bl, 1 );            // on
  
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, HIGH);

  pinMode(REA, INPUT_PULLUP);           // Rotary encoder port  
  pinMode(REB, INPUT_PULLUP);           // 
  pinMode(RESW, INPUT_PULLUP);          // Rotary encoder push SW
  //pinMode(BANDRLY, OUTPUT);             // relay band selection
  //digitalWrite (BANDRLY, LOW);          //  
  
  pinMode(SQLMUTE, INPUT_PULLUP);       // amp mute when no signal detected
//    digitalWrite (SQLMUTE, LOW);          // UNMute the amp

  // Initialize 4bit  LCD
  lcd.begin(16, 2);                     // initialise the LCD

  // Start up message for 1602A
  lcd.setCursor( 0, 0);
  lcd.print("R909-SDR-1602   ");
  lcd.setCursor( 4, 1);
  lcd.print("Version 2.00");
  
  delay(2000);
  lcd.clear();
  Serial.begin(9600);
  Serial.println("\nR909-SDR-1602 Radio V2.00");

////////////// 4732 set up section ///////////
  int16_t si4735Addr = rx.getDeviceI2CAddress(RESET_PIN);
  if ( si4735Addr == 0 ) {    // 4732 address check  0x11 expected
    lcd.setCursor( 4, 1);
    lcd.print("Not find Si4732 ");    
    Serial.println("\nCan't find 4732");   
    while (1);
  }


  rx.setup(RESET_PIN, AM_FUNCTION);   //  Starts default radio function 
  delay(500);

  rx.setVolume(10);           // once set Range 0-127
  delay(500);
  set21400kHz();              // Set Si4732 as 21.4MHz mother radio
 
  /////// Switch sence ////////////////////////////////////////////////////////
  SW_result0 = function_key();         // Freq mode set on power start
  if (SW_result0 == 0) if_mode = 1;    // When no action, set IF_mode as 10.7MHz on LO

  r.begin(true);                       // rotary encoder initialise
  Last_millis = millis();

  LCD_Disp ( 0, 0, "Loading ...") ;

  delay(50);
  
  // EEPROM data recover for clock frequency data
  // There are FREQ, FSTEP, and MCHAN0-9
  freqEEPROMget( FREQ_AD, freq1);      // FREQ data  recover from EPROM

  EEPROM.get( FSTEP_AD, fstep);        // fstep data from EEPROM Read
  
 // Newly added 2022.09.16       **********
  EEPROM.get( VOL_AD, VolumeLevel);    // volume data  recover from EPROM
  EEPROM.get( SQU_AD, SquelchLevel);   // squelch data from EEPROM Read
  EEPROM.get( BAND_AD, BandSelect);    // band data  recover from EPROM
//  Newly added 2022.10.11       **********
  EEPROM.get( previous_freqFM_AD, previous_freqFM );    // 
  EEPROM.get( previous_freqAM_AD, previous_freqAM );    // 
   

  int d = digitalRead(RESW);
  if (d == 0) {
    putFirstSettings();
    LCD_Disp ( 0, 0, "Writing Defaults to EEPROM") ;
  }

  getSettings();
  //  setVol(); // set volume with eeprom value
  // ShowSettings();
  // BandSelect = false;   // fm mode
  Dsp_Disp();                        // display freq
  freq_0 = freq1 + rxclk;

  // Serial.print("\nFreq sent to SI5351 = ");
  // Serial.println(freq_0);
  si5351_init();   // initialise the si5351


  if (freq1 < 10000000 || freq1 > 200000000) { //10-200MHz
    LCD_Disp ( 0, 0, "Freq Read Error  ") ;
    Serial.println("\nFreq Read Error");
    Serial.println(freq1);
    freq1 = 118100000;
    putFirstSettings();

    EEPROM.put( FREQ_AD, freq1);      // FREQ data  100MHz for EPROM
    freq_0 =  118100000;              // changed 118.1MHz AM
    for ( memo_ad = 0 ; memo_ad < 50; memo_ad++ ) {
      EEPROM.put( FREQ_AD + 8 + memo_ad * 4, freq_0); // FREQ data  100MHz for eEPROM
    }
  }

//  digitalWrite (BANDRLY, HIGH);          //  
  delay(500);

#ifdef lcdTypeLCD

  //    lcd.PageClear() ;
  lcd.clear();            // Once display off
#endif

  pinMode(lcd_bl, OUTPUT);              // LCD back light
  digitalWrite( lcd_bl, 1 );            // on
  
  delay(500);
  if ( fstep == 1000 || fstep == 10000 || fstep == 100000 ||
       fstep == 25000 || fstep == 1000000 || fstep == 10000000) {
  }
  else {
    LCD_Disp ( 0, 1, "Step Read Error") ;
    Serial.println("\nStep Read Error");
    Serial.println(fstep);
    fstep = 100000;
    EEPROM.put( FSTEP_AD, fstep );   // fstep data 100kHz for EEPROM
    delay(500);
  }

  // Set INT for RE
  PCICR |= (1 << PCIE2);
  PCMSK2 |= (1 << PCINT18) | (1 << PCINT19);   sei();

  // volume
  rx.setVolume(VolumeLevel);      // Range 0-63

  // Display initializing
  Dsp_Disp();                     // Display current FREQ

  Step_Disp();                    // Display current step FREQ

  Band_Disp();                    // Display current Band

  if (if_mode == 1) freq_0 = freq1  + rxclk  ; //  On IF mode FREQ must be 21.4MHz upper tahn RX FREQ

  freq_0 = freq1 + rxclk  ;       // UPDATE the freq

  if (BandSelect == true) {       // AM mode
 //   digitalWrite (BANDRLY, HIGH);
 //   set21400kHz();              // Change to AM 21.4MHz mode
    send_frequency(freq_0, 0);    //send it to si5351
  }
  else {
//    digitalWrite (BANDRLY, LOW);
//    setFMfreq(freq1/10000);     // Change to FM mode  

    Serial.print("FM-F:");
    Serial.print(freq1/1000);    
  }

  delay(500);

  // debug print out
      Serial.print("j: ");        // LCD blinking status
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
       
}
// End of setup


//= =========================MAIN===================================
void loop() {
  TimerCount();
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
  blink(200, 800);            // Turn on 1sec/off 0.5sec by monitor LED and FUNC on LCD on/off

  // Check SW action
  SW_result = function_key();           //  Get RESW result(on:50mS push)
  SW_result0 = mode_define(SW_result);  //  Define SW status 0:no, 1:single, 2:double

  // if (SW_result0 > 1)   Serial.println(SW_result0);    // debug
  //  RE push switch to FUNC mode, push switch again to each mode
  if (SW_result0 == 1) {
    if (mode != FUNCTION)  {    //  Go into FUNC mode
      mode_last = mode;
      mode = FUNCTION;          //0:no,1:FR,2:ST,3:ME,4:SC,5:FSet,6:STSet,7:MWrite,8:AScan,9:vol,10 squelch , 11 band, 12 func
    }
    else { //  If FUNC, every click goes each FUNC defined
      mode_last = mode;
      mode = mode_temp;

    }
  }
  if ( SW_result0 == 2 ) {      //  select double click
    mode_last = mode_temp;      // double push

// To define mode by RE push switching **********
    switch (mode_temp) {
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
      default:
        mode = FUNCTION;        // Go back to FUNC
    }
  }
// RE push ***************************

  //  Each function for each mode
  // clear the line
  // LCD_Disp ( 0, 1, "    ") ;
  int eachdisp;
  switch ( mode ) {
    
    case FUNCTION: {                     // function mode is 12 
        if (j == 0) {                    // boolean j led/LCD state
          switch ( mode_temp ) {    
            case FREQ: {                 //  FREQ determining mode 1
                eachdisp = freqdisp;
                break;
              }
            case STEP: {                 //  STEP select mode 2
                eachdisp = stepdisp;     
                break;
              }
            case VOLUME: {                 //  volume select mode 10
                eachdisp = voldisp;   
                break;
              }
            case SQUELCH: {                //  squelch select mode 11
                eachdisp = squdisp;              
                break;
              }
            case BAND: {                   //  band select mode 12
                eachdisp = banddisp;                   
                break;
              }
            case MEMORY: {                 //  MEMORY ch select mode 3
                eachdisp = mdisp ;
                break;
              }
            case SCAN: {                   //  SCAN select mode 4
                eachdisp = scandisp ;
                break;
              }
            // next were added later
            default: eachdisp =  funcdisp;  //  Function select mode 9
          }
        }    
          if (j == 1) eachdisp =  funcdisp; //  LCD blink
      }
        LCD_Disp ( 0, 1, eachdisp ) ;
        break;
      
    case FREQ: {                         //  FREQ determining mode 1
        LCD_Disp ( 0, 1, freqdisp ) ;
        Step_Disp();
        break;
      }
    case STEP: {                         //  STEP select mode 2
        LCD_Disp ( 0, 1, stepdisp ) ;
        Step_Disp();
        break;
      }

    case VOLUME: {                       //  volume select mode 10
        LCD_Disp ( 0, 1, voldisp ) ;
        Vol_Disp();
        break;
      }
    case SQUELCH: {                      //  squelch select mode 11
        LCD_Disp ( 0, 1, squdisp ) ;
        Squ_Disp();
        break;
      }
    // band
    case BAND: {                         //  band select mode 12
        LCD_Disp ( 0, 1, banddisp ) ;
        Band_Disp();       
        break;
      }

    case MEMORY: {                       //  MEMORY ch select mode 3
        LCD_Disp ( 0, 1, mdisp ) ;
        break;
      }

    case SCAN: {                         //  Manual step SCAN
      //  fm mode to be added
        LCD_Disp ( 0, 1, scandisp ) ;
        scandisp[1] = 0x30 | (scan_ad / 10) ;     //  display scan chan
        scandisp[2] = 0x30 | (scan_ad % 10) ;
        clearLCDLine(6, 1, 4);
        
        freqEEPROMget( MCHAN0 + scan_ad * 4, freq1); // read out MEM frequency
        
        if (freq_last == freq1)  break;

        if (BandSelect != BandSelect_last){
          Band_Disp();
          BandSelect_last=BandSelect;
        }
        Dsp_Disp();
        freq_0 = freq1 + rxclk  ;
        if (if_mode == 1) freq_0 = freq1  + rxclk  ; // FREQ is more than 10.7MH above on RX FREQ
 
        freq_last = freq1 ;
        if(BandSelect == false) {    // If FM mode, 
          setFMfreq(freq1/10000);    //
        }
        else send_frequency(freq_0, 0); //send it to si5351
        break;
      }

    case FREQSET: {                      //  FREQ button long push 
        LCD_Disp ( 0, 1, spdisp) ;       //  2nd line 1st character space display
        delay(500);
        LCD_Disp ( 0, 1, freqdisp) ;     //  2nd line 1st character freq display
        
        freq0 = freq1;
        if(BandSelect == false) freq0 = -freq1;  // If FM convert to negative
        EEPROM.put( FREQ_AD, freq0 );    // freq data into EEPROM

        EEPROM.put( VOL_AD, VolumeLevel);    // volume data  recover from EPROM
        EEPROM.put( SQU_AD, SquelchLevel);   // squelch data from EEPROM Read
        EEPROM.put( BAND_AD, BandSelect);    // band data  recover from EPROM

        EEPROM.put( previous_freqFM_AD, previous_freqFM );    // 
        EEPROM.put( previous_freqAM_AD, previous_freqAM );    // 


 // ***************
   
        mode = FREQ;
        break;
      }
    case STEPPUT: {                      //  STEP written  
        LCD_Disp ( 0, 1, stepdisp) ;     // 2nd line 1st character space display
        EEPROM.put( FSTEP_AD, fstep );   //  fstep data 100kHz for EEPROM
        delay(500);
        LCD_Disp ( 0, 1, spdisp) ;       //  2nd line 1st character space display
        delay(500);
        LCD_Disp ( 0, 1, stepdisp) ;     //  2nd line 1st character step display

        mode = STEP;
        break;
      }
    case MEMORYPUT: {                    //  MEMORY write  
        LCD_Disp ( 0, 1, mdisp) ;        //  line 2 first column is m
        
        freq0 = freq1;
        if( BandSelect == false) freq0 = -(freq1);
        EEPROM.put( MCHAN0 + memo_ad * 4, freq0); // Write FREQ data on defined EEPROM

        delay(500);
        LCD_Disp ( 0, 1, spdisp) ;       //  line 2 first column is space
        delay(500);
        LCD_Disp ( 0, 1, mdisp) ;        //  line 2 first column is m

        mode = MEMORY;
        break;
      }
    case SCANAUTO: {                       //  AUTO SCAN mode from M-chan
        if (Timer_scan <= 0) {             //  If timer up
          Timer_scan = 100;
          ascandisp[1] = 0x30 | (scan_ad / 10) ;   //  display scan chan
          ascandisp[2] = 0x30 | (scan_ad % 10) ;   //  display scan chan
          LCD_Disp ( 0, 1, ascandisp ) ;
          clearLCDLine(6, 1, 4);
         
          freqEEPROMget( MCHAN0 + scan_ad * 4, freq1); // read out MEM frequency

          scan_ad++;                        //　Changed 20201013
          if (scan_ad > 49) scan_ad = 0;
                    
          if (freq1 == 118000000){           // 118mhz
            Timer_scan = 0;
            break;                          // If 118MHz, then skip.
          }
          if(BandSelect == false ){         // If FM, skip FM freq
            BandSelect = true;
            Timer_scan = 0;
            break;  
          }  
          
          Dsp_Disp();
          Band_Disp();      

          if (if_mode == 1) freq_0 = freq1 + rxclk  ; //  IFモード時は10.7MH上位を発振
          send_frequency(freq_0, 0);        //send it to si5351
          delay(300);
          
          s_meter_disp();                      // Get AGC voltage & change S-meter
          if ( s_value > SquelchLevel ) Timer_scan = 2000 ; // If squelch off, wait 1 sec
        }
        break;
      }
  }
  // re_result was set in ISR
  if (SW_result0 == 1) re_result = 0; // If SW pending, depress RE function

  if (re_result  )  { // Left 0x20 ? Right 0x10?

    // clean the band area
    clearLCDLine (3, 1, 6);
    Band_Disp();
    
    // mode= 0:no,1:FR,2:ST,3:ME,4:SC,5:FSet,6:STSet,7:MWrite,8:AScan,9:FU
    if ((mode == FREQ) && (re_result == 0x10 ) ) {  // clockwise FREQ
      freq1 = freq1 + fstep;
      
      // limit the frequency according to the band
      if (BandSelect == false) { // fm mode
        if (freq1 > (long) 109000000) {         //  109MHz -> 76MHz
          freq1 = (long)   109000000; //
          Dsp_Disp();
        }
      }
      if (BandSelect == true) { // am mode
        if (freq1 > (long) 136000000) {         //  136MHz -> 118MHz
          freq1 = (long)   136000000;
          Dsp_Disp();
        }
      }

      freq_0 = freq1;
      if (if_mode == 1) {
        freq_0 = freq1  + rxclk  ; // on IF mode 10.7MH upper
      }
      //SI_setfreq(freq_0);

      //  Serial.print("freq_0 :");
      // Serial.println(freq_0);
      Dsp_Disp();
      Step_Disp();
      freq_0 = freq1 + rxclk  ;

      if (BandSelect == true) {
        set21400kHz();             // Change to AM 21.4MHz mode
        send_frequency(freq_0, 0); //send it to si5351
      }
      else {
         setFMfreq(freq1/10000);     // Change to FM mode 
//           Serial.print("FM-F:");
//        Serial.print(freq1/1000);     
      }
    }
    else if ((mode == FREQ) && (re_result == 0x20) ) { //counter clockwise FREQ
      freq1 = freq1 - fstep;
      
      if (BandSelect == false) {  // fm mode
        if (freq1 < 76000000) {           // 76MHz->118MHz
          freq1 = 76000000;

          Dsp_Disp();
          Step_Disp();
        }
      }
      if (BandSelect == true) {     // am mode
        if (freq1 < 118000000) {    // 118MHz->
          freq1 = (long)   118000000;

          Dsp_Disp();
          Step_Disp();
        }
      }
      freq_0 = freq1  + rxclk  ;    //  on IF mode 10.7MH upper
      //SI_setfreq(freq_0);
      if (BandSelect == true) {
         set21400kHz();             // Change to AM 21.4MHz mode
        send_frequency(freq_0, 0);  //send it to si5351
      }
      else {
        setFMfreq(freq1/10000);     // Change to FM mode      
//        Serial.print("FM-F:");
//        Serial.print(freq1/1000);
      }
      Dsp_Disp();
      Step_Disp();
    }
    else if ((mode == STEP) && (re_result == 0x10) ) { // clockwise STEP
      if (fstep == 25000)  fstep = 100;   // 25000-->100
      fstep = fstep * 10;
      if (fstep > 10000000)  fstep = 100; // 100-->1000
      if (fstep == 100)  fstep = 25000;   // 100-->25000
      Step_Disp();
    }
    else if ((mode == STEP) && (re_result == 0x20 )) { // counter clockwise STEP　
      if (fstep == 25000)  fstep = 100;   // 25000-->100
      fstep = fstep / 10;
      if (fstep < 100) fstep = 10000000;  //  100-->1000
      if (fstep == 100)  fstep = 25000;   // 100-->25000
      Step_Disp();
    }
    else if ((mode == MEMORY) && (re_result == 0x10 )) { // clockwise STEP
      memo_ad ++ ;
      if (memo_ad > 49)  memo_ad = 0;
      mdisp[1] = 0x30 | (memo_ad / 10) ;
      mdisp[2] = 0x30 | (memo_ad % 10) ;
      LCD_Disp ( 0, 1, mdisp) ;           // #13 LED on/off in turn
    }
    else if ((mode == MEMORY) && (re_result == 0x20 )) {// Counter clockwise STEP　
      memo_ad --;
      if (memo_ad < 0)  memo_ad = 49;
      mdisp[1] = 0x30 | (memo_ad / 10) ;
      mdisp[2] = 0x30 | (memo_ad % 10) ;
      LCD_Disp ( 0, 1, mdisp) ;
    }
    else if ((mode == SCAN) && (re_result == 0x10 )) {// clockwise STEP
      scan_ad ++;
      if (scan_ad > 49)  scan_ad = 0;
      scandisp[1] = 0x30 | (scan_ad / 10) ;
      scandisp[2] = 0x30 | (scan_ad % 10) ;
      clearLCDLine(6, 1, 4);  
      LCD_Disp ( 0, 1, scandisp) ;
    }
    else if ((mode == SCAN) && (re_result == 0x20 )) {// Counter clockwise STEP　
      scan_ad --;
      if (scan_ad < 0)  scan_ad = 49;
      scandisp[1] = 0x30 | (scan_ad / 10) ;
      scandisp[2] = 0x30 | (scan_ad % 10) ;
      clearLCDLine(6, 1, 4);
      LCD_Disp ( 0, 1, scandisp) ;
    }
       
  //volume 0 is minimum 63 is max  VolumeLevel
    else if ((mode == VOLUME) && (re_result == 0x10 )) { // counter clockwise VOLUME　
      rx.volumeUp();      // Range 0-63
      VolumeLevel = rx.getVolume();
      Vol_Disp();         // show volume level on display        
    }
    else if ((mode == VOLUME) && (re_result == 0x20 )) { //  clockwise VOLUME　
      rx.volumeDown();
      VolumeLevel =  rx.getVolume();
      Vol_Disp();           // show volume level on display
    }
    
  //squelch 0 is minimum 63 is max
    else if ((mode == SQUELCH) && (re_result == 0x20 )) { // counter clockwise VOLUME　
      //  LCD_Disp ( 2, 3, "SQU:") ;
      SquelchLevel =  SquelchLevel - 1;
      checkSquLimits();
      Squ_Disp();  // show squelch level on display

    }
    else if ((mode == SQUELCH) && (re_result == 0x10 )) { //  clockwise VOLUME　
      // LCD_Disp ( 2, 3, "SQU:") ;
      SquelchLevel =  SquelchLevel + 1;
      checkSquLimits();                  // #1380
      Squ_Disp();  // show squelch level on display
    }
    else if ((mode == BAND) && (re_result )) { 
 
      // counter clockwise or clockwise band
      // changed the BandSelect 
      BandSelect = !BandSelect ;  // AM->FM or FM->AM
      
 // AM mode selected   
      if( BandSelect == true ){   // Change from FM to AM
        previous_freqFM = freq1;
        
        if ( previous_freqAM != freq1){
          freq1 = 118000000;

        }
        else {
          freq1 = previous_freqAM; // store the current AM freq
        }
      }
  // FM mode selected
      else {                      // Change from AM to FM
        previous_freqAM = freq1;  // store the current AM freq
        
        if ( previous_freqFM != freq1){
          freq1 = (long)76000000;
       
        }
        else {
          freq1 = previous_freqFM;
        }
      }
      Dsp_Disp();             // Update the display
      
      freq_0 = freq1  + rxclk  ;  //  on IF mode 10.7MH upper
      //SI_setfreq(freq_0);
      send_frequency(freq_0, 0);  //send it to si5351
      Band_Disp();                // show BAND on display
      Dsp_Disp();                 //update the frequency according to the band selected
    }
       
    else if ((mode == FUNCTION) && (re_result == 0x10 )) {// clockwise VOLUME
      mode_temp ++;
      if (mode_temp > 7) mode_temp = 7; //5
      // clearLCDLine(0, 1, 15);
    }
    else if ((mode == FUNCTION) && (re_result == 0x20 )) {// Counter clockwise function
      mode_temp --;
      if (mode_temp < 1) mode_temp = 1; //4
      // clearLCDLine(0, 1, 15);
    }
  }
  re_result = 0;
  
  // S METER & squelch
  // CHECK TIMER FOR TIMEOUT
  if (( millis() - elapsedRSSI ) > Time_elapsedRSSI ){
    s_meter_disp();
    elapsedRSSI = millis();
    
  // Squelch 
    if(SquelchLevel>rssi){
      pinMode (SQLMUTE, INPUT_PULLUP);    // Mute the amp  STILL POP NOISE
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
  if (analogRead(FUNC_SW) < 800) {
    delay(50);
    sw_value = analogRead(FUNC_SW);
    sw_stat = map(sw_value, 0, 450, 1, 4); 

    /*
      Serial.print("F-SW ADC:");
      Serial.print(sw_value);  
      Serial.print(" F-SW:");
      Serial.println(sw_stat); 
    */
     
    switch ( sw_stat){
      case 1:
        mode = FREQ;           // freq mode
        break; 
      case 2:   
        mode = STEP;           // STEP mode
        break;      
      case 3: 
        mode = VOLUME;         // VOLUME mode
        break;
      case 4:   
        mode = SQUELCH;        // SQUELCH mode
        break;   
    }

    digitalWrite(led_pin, 1);
    delay(150);
  }

}
//end of loop

// ***************************************
// **********************************



//  ************************************
//   FUNCTIONs
//  ************************************

// Band.ino  2022/08/18
void Band_Disp() {
  if( last_BandSelect != BandSelect){
   
// clearLCDLine(10,0,5);
  
    if (BandSelect == true) {
    // LCD_Disp ( 13, 0, "AM");
    //switch on the band relay
//      digitalWrite (BANDRLY, HIGH);
      set21400kHz();             // Change to AM 21.4MHz mode
    //freq1 = 118000000;

    }
    else if (BandSelect == false) {
     // LCD_Disp ( 13, 0, "FM");
      //switch off the relay
 //     digitalWrite (BANDRLY, LOW);
      setFMfreq(freq1/10000);     // Change to FM mode      

    }
    last_BandSelect = BandSelect ;
    delay(100);                     // Wait relay operating time
  }
// Pad the "s" spaces from line/col "clearLCDLine(col, line, s);"  
//  clearLCDLine(6, 1, 4);  
  Dsp_Disp();
  
}


// display.ino   2022/08/14  ****************************************
//#define lcdTypeOled
//#define lcdTypeLCD
//#define lcdTypeLCD4bit

// EEprom.ino   **************************************
// eeprom functions  2022/07/19

void getSettings() {

  freqEEPROMget( FREQ_AD, freq1);       // FREQ data  recover from EPROM
  EEPROM.get( FSTEP_AD, fstep);      // fstep data from EEPROM Read
  EEPROM.get( VOL_AD, VolumeLevel);       // volume data  recover from EPROM
  EEPROM.get( SQU_AD, SquelchLevel);      // squelch data from EEPROM Read
  EEPROM.get( BAND_AD, BandSelect);       // band data  recover from EPROM

  EEPROM.get( previous_freqFM_AD, previous_freqFM );    // 
  EEPROM.get( previous_freqAM_AD, previous_freqAM );    // 
}

void putFirstSettings() {

  freq0 = freq1;  
  if( BandSelect == false) freq0 = -(freq1);
  EEPROM.put( FREQ_AD, freq0);          // FREQ data  100MHz for EPROM

//  freqEEPROMput(FREQ_AD, freq1);          // Put freq1 + BAND on EEPROM
  EEPROM.put( FSTEP_AD, fstep);           // fstep data from EEPROM Read
  EEPROM.put( VOL_AD, VolumeLevel);       // volume data  recover from EPROM
  EEPROM.put( SQU_AD, SquelchLevel);      // squelch data from EEPROM Read
  EEPROM.put( BAND_AD, BandSelect);       // band data  recover from EPROM
//  Newly added 2022.10.11       **********
  EEPROM.put( previous_freqFM_AD, previous_freqFM );    // 
  EEPROM.put( previous_freqAM_AD, previous_freqAM );    // 
  
  freq_0 =  118000000;
  
  for ( memo_ad = 0 ; memo_ad < 50; memo_ad++ ) {
    EEPROM.put( FREQ_AD + 8 + memo_ad * 4, freq_0); // FREQ data  AM 118.1MHz for EEPROM
  }
  freq_0 =  118100000;
  EEPROM.put( FREQ_AD + 8 , freq_0); // FREQ data  FM 118.1MHz for EEPROM #0
  
  freq_0 =  -80200000;
  EEPROM.put( FREQ_AD + 8 + 49 * 4, freq_0); // FREQ data  FM 80.2MHz for EEPROM #49
  freq_0 =  -85100000;
  EEPROM.put( FREQ_AD + 8 + 48 * 4, freq_0); // FREQ data  FM 85.1MHz for EEPROM #48
  delay(500);
}

// Function.ino   2022/08/18 **********************************
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
  return result ;                 // Switch not settled 20201002
}

// After key result(RESW_click), wait timer_mash. Then decide single/double
// result 0:no key, 1:off-> on pending, 2:single click, 3:double click
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
  if (Timer_LED > 0) Timer_LED = Timer_LED - time_dif;  // To turn on/off for #13 LED
  if (Timer_scan > 0) Timer_scan = Timer_scan - time_dif;
}

//  LED#13 on/off & LCD blink:j
void blink(long on_time, long off_time) {  // #13 LED monit on_off, j effects FUNC on/off
  if ((Timer_LED <= 0) && (j == 1)) {
//    digitalWrite(led_pin, 0 );     // Mon Led on/off
    j = 0;                           // bit j off  j:LCD blink
    Timer_LED = off_time;
    //Serial.println("LED off");
  }
  else if ((Timer_LED <= 0) && (j == 0)) {
//    digitalWrite(led_pin, 1 );     // Mon Led on/off
    j = 1;                           // bit j off  j:LCD blink
    Timer_LED = on_time;
   // Serial.println("LED on");
  }
}

//  Lcd.ino   2022/08/19 **********************************
// Pad the "s" spaces from line/col "clearLCDLine(col, line, s);"
void clearLCDLine(int col, int line, int s)
{
  lcd.setCursor(col, line);
  for (int n = 0; n < s; n++) // 16 indicates symbols in line. For 2x16 LCD write - 16
  {
    lcd.print(" ");
  }
}

// ***************** LCD_Disp *********************
void LCD_Disp ( char column, char line, char* charbuf) {

#ifdef lcdTypeOled
  u8g2.drawStr(column * 8, (line * 11 + 9), charbuf); // column=column, raw bit = line * 11 + 9
  u8g2.sendBuffer();
#endif

#ifdef lcdTypeLCD
  lcd.setCursor( column, line);
  lcd.print( charbuf );
#endif

#ifdef lcdTypeLCD4bit
  lcd.setCursor( column, line);
  lcd.print( charbuf );
#endif
}


//***** fstep displaying ***** 1kHz,10kHz,25kHz,100kHz,1MHz,10MHz
void Step_Disp() {                           // display fstep on LCD
  if (fstep == 1000)  {
    LCD_Disp ( 3, 1, "1kHz  ") ; //stp 1khz

    //  Serial.println("Step 1kHz   ");
  }
  else if (fstep == 10000) {
    LCD_Disp ( 3, 1, "10kHz  ") ;
    //  Serial.println("Step 10kHz   ");
  }
  else if (fstep == 25000) {
    LCD_Disp ( 3, 1, "25kHz  ") ;
    // Serial.println("Step 25kHz   ");
  }
  else if (fstep == 100000) {
    LCD_Disp ( 3, 1, "100kHz ") ;
    // Serial.println("Step 100kHz   ");
  }
  else if (fstep == 1000000) {
    LCD_Disp ( 3, 1, "1MHz  ") ;
    // Serial.println("Step 1MHz   ");
  }
  else {
    LCD_Disp ( 3, 1, "10MHz ") ;
    // Serial.println("Step 10MHz   ");
  }
  delay(50);
}


//********* display frequency *********
void Dsp_Disp() {              // Format freq data for display & display
  char fdds[16];
  char fdds1[17];
  char fdxs[3] = {'F', 'A', 'M'};

  LongToStr(freq1, fdds);     // convert Long data 16 columns
  Fdds_Space(fdds);            // change leading "0" to space

  fdds1[16] =   '\0';
  fdds1[15] =  32;
  fdds1[14] = 'z';
  fdds1[13] =  'H';
  fdds1[12] =  'M';
  fdds1[11] = fdds[7];
  fdds1[10] = fdds[6];
  fdds1[9] = fdds[5];
  fdds1[8] = '.';
  fdds1[7] = fdds[4];
  fdds1[6] = fdds[3];
  fdds1[5] = fdds[2];
  fdds1[4] = 32;           // space 20h
  fdds1[3] = 32;
  fdds1[2] = 32;
  if (BandSelect == true){ // AM 
    fdds1[0] = fdxs[1];
  }
  if (BandSelect == false){ // FM 
    fdds1[0] = fdxs[0];
  }
  fdds1[1] = fdxs[2];
//fdds1[0] = fdxs[0];

  LCD_Disp ( 0, 0, fdds1) ;

}

//
// Limits.ino   2022/08/15 ***********************************

void  checkSquLimits() {
  if ( SquelchLevel < 0)   SquelchLevel = 0;
  if ( SquelchLevel > 32)  SquelchLevel = 32; //
}

// Serial.ino   2022/07/29  **************************
//show in serial the eeprom settings
void  ShowSettings() {

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
 
}


// Si5351a.ino   2022/08/19 ****************************************
//using library

// CHANGE! No change no setting
int32_t last_ffrequency0 = 0;
int32_t last_ffrequency1 = 0;
int32_t last_ffrequency2 = 0;

// calculate and send frequency code to Si5351...
void send_frequency(int32_t ffrequency, int fcontext) {
  switch (fcontext) {
    case 0:                                             // Port 1
      if ( ffrequency != last_ffrequency0 ){
        si5351.set_freq((uint64_t)ffrequency * 100, SI5351_CLK0);
        last_ffrequency0 = ffrequency;
      }
      break;
    case 1:                                             // Port 2
      if ( ffrequency != last_ffrequency1 ){
        si5351.set_freq((uint64_t)ffrequency * 100, SI5351_CLK1);
        last_ffrequency1 = ffrequency;
      }
      break;
    case 2:                                             // Port 3
      if ( ffrequency != last_ffrequency2 ){
        si5351.set_freq((uint64_t)ffrequency * 100, SI5351_CLK2);
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
  Serial.print("SI5351 Freq: ");
  Serial.println(freq);
  if( last_freq != freq){
    si5351.set_freq(freq , SI5351_CLK0);
    last_freq = freq;
  }
//  if( last_ifclk != ifclk){
//    si5351.set_freq(ifclk , SI5351_CLK1);
//    last_ifclk = ifclk;
//  }
}


void si5351_init() {
  // Initialize the Si5351
  si5351.init(SI5351_CRYSTAL_LOAD_8PF , 0 , 0);


  // Set CLK0 to output vfo frequency with a fixed PLL frequency
  si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);

  // Initialize each ports frequency
  si5351.set_freq(startF, SI5351_CLK0);
//  si5351.set_freq(ifclk, SI5351_CLK1);  // clk1: 2nd OSC

  //switch off clk1
//  si5351.output_enable(SI5351_CLK1, 1);

  // Set each ports power
  si5351.output_enable(SI5351_CLK0, 1);
  si5351.output_driver(SI5351_CLK0, 1);
  si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_2MA);

  // in case phase needs to be shifted
 si5351.set_phase(SI5351_CLK2, 50); //90 DEGREE SHIFT
 si5351.pll_reset(SI5351_PLLA); // RESET THE PLL

// enable the output and driver strength 
//  si5351.output_enable(SI5351_CLK1, 1);
//  si5351.output_driver(SI5351_CLK1, 1);
//  si5351.drive_strength(SI5351_CLK1, SI5351_DRIVE_8MA);

}

//update the pll frequency
void updateFreq(long fr){

  send_frequency(fr,0);
}

// Smeter.ino  2022/08/19  ****************************
// Changed 2023/12/03 by JA3KPA  Si4732 SNR& RSSI
 
void s_meter_disp() {  

 /////////////////////////////////////////////////////////////
 ///////////////// 4732 //  2023.11.10  RSSI 0-127,snr 0-127
  char s_disp[11];
  unsigned char i;
  rx.getCurrentReceivedSignalQuality();
  rssi = rx.getCurrentRSSI();           // Get RSSI from Si4732
  snr = rx.getCurrentSNR();
  s_value = map(rssi, 0, 127, 0, 9);    // s max changed

  if(rssi!=0) digitalWrite(led_pin, HIGH);
  else  digitalWrite(led_pin, HIGH);    // if rssi, led ON
  
  if(s_dot_disp==1){
    s_disp[0] = 0x53;                     // first letter S
    for ( i = 0; i < 5; i++) {
      if (s_value > i) {                  // i=0 s_value=1 set 1 S_MAX=8
        s_disp[i + 1] = 0x23;             // 0x23; #
      } 
      else s_disp[i + 1] = 0x20;
    }
    s_disp[9] = 0x0;
    s_disp[10] = 0x0;
  }
  else{
    s_disp[0] = 0x53;  // S
    s_disp[3] = 0x52;  // R
    s_disp[1] = 0x30 | snr/10;
    s_disp[2] = 0x30 | snr%10;
    s_disp[4] = 0x30 | rssi/10;
    s_disp[5] = 0x30 | rssi%10;    
  }
  LCD_Disp ( 10, 1, s_disp) ;
}


//squelch display
void Squ_Disp() {
  char c[3];
  String(SquelchLevel).toCharArray(c, 3);
  clearLCDLine(4, 1, 5);  
  LCD_Disp ( 3, 1, c) ;
}

void Vol_Disp(void){
  char b[3];
  String(VolumeLevel).toCharArray(b, 3);
  clearLCDLine(4, 1, 5);
  LCD_Disp ( 3, 1, b) ;
}
 
// Extract FREQ + BAND from EEPROM
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
// Starts default mother radio function(AM; from 21.3 to 21.5 MHz; 21.4 MHz; step 1kHz)
  rx.setAM(21300, 21500, 21400, 4);   // mother radio
  rx.setSeekAmLimits(21300, 21500);
  rx.setSeekAmSpacing(4); // spacing 10KHz

  delay(300);
}

void set802MHz (void){            // FM set
//  rx.setTuneFrequencyAntennaCapacitor(0);
// Starts default radio function and band (FM; from 75 to 108 MHz; 80.2 MHz; step 100kHz)
  rx.setFM(7500, 10800, 8020, 10);   // Once FM receive
  rx.setSeekAmRssiThreshold(0);
  rx.setSeekAmSrnThreshold(10);

  delay(300);
}

void setFMfreq(uint16_t fm_freq){ // FM set
//  rx.setup(RESET_PIN, 0);
//  rx.setTuneFrequencyAntennaCapacitor(0);
// Starts default radio function and band (FM; from 75 to 108 MHz; ? MHz; step 100kHz)
  rx.setFM(7500, 10800, fm_freq, 10);   // Once FM receive
  rx.setSeekAmRssiThreshold(0);
  rx.setSeekAmSrnThreshold(10);

  delay(300);
}  

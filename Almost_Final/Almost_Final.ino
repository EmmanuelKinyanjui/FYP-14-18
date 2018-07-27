#include <Servo.h>
#include <Wire.h>
#include "rgb_lcd.h"
#include "U8glib.h"



///************************VARIABLES****************************************/////
///ServoOne connected
const int servoOne = 26;
const int servoTwo =  5; 
const int servoThree = 6;

///Conveyor Motor pins
const int motorPin1 =25;
const int motorPin2 =27;
const int enablePin=4;

////linear actuator////
const int motorPin3 =31;
const int motorPin4 =33;
const int enablePin2=29;

//Interrupt Pins
const int inductiveSensor = 3;
const int binActivator = 18;
const int capSensor =2;

//limitSwitch
const int crusherLS = 51;

//LDR
const int glass_ldr = A8;
const int plastic_ldr = A9;

//Crusher status
bool crusherHasRun;

////counter lcd///
rgb_lcd lcd;
const int colorR = 0;
const int colorG = 0;
const int colorB = 200;

////gLCD char bitmap
const uint8_t charBitmap[][8] = {
   { 0xc, 0x12, 0x12, 0xc, 0, 0, 0, 0 },
   { 0x6, 0x9, 0x9, 0x6, 0, 0, 0, 0 },
   { 0x0, 0x6, 0x9, 0x9, 0x6, 0, 0, 0x0 },
   { 0x0, 0xc, 0x12, 0x12, 0xc, 0, 0, 0x0 },
   { 0x0, 0x0, 0xc, 0x12, 0x12, 0xc, 0, 0x0 },
   { 0x0, 0x0, 0x6, 0x9, 0x9, 0x6, 0, 0x0 },
   { 0x0, 0x0, 0x0, 0x6, 0x9, 0x9, 0x6, 0x0 },
   { 0x0, 0x0, 0x0, 0xc, 0x12, 0x12, 0xc, 0x0 }
   
};

//gLCD rook bitmap
const uint8_t rook_bitmap[] U8G_PROGMEM = {
0x00, // 00000000
0x55, // 01010101
0x7f, // 01111111
0x3e, // 00111110
0x3e, // 00111110
0x3e, // 00111110
0x3e, // 00111110
0x7f // 01111111
};

//counter variables
int metal_counter=0;
int glass_counter=0;
int plastic_counter=0;


///Object Initialization
Servo myServoOne; 
Servo myServoTwo;
Servo myServoThree;
U8GLIB_ST7920_128X64_1X u8g(39, 41, 43); //pin connection(E,r/w,rs)

///***************************BASE FUNCTIONS*****************************/
//initialize the code when activated
void setup() {
  interrupts();
  attachInterrupt(digitalPinToInterrupt(inductiveSensor), inductiveFunc ,FALLING);
 // attachInterrupt(digitalPinToInterrupt(binActivator), crusherActivate ,HIGH);
  attachInterrupt(digitalPinToInterrupt(capSensor), capacitiveFunc ,FALLING);

/// Geared motor///
  pinMode(motorPin1,OUTPUT);
  pinMode(motorPin2,OUTPUT);

//ldr setup

  pinMode(plastic_ldr,INPUT);
  pinMode(glass_ldr,INPUT);
    
///linear motor///
  pinMode(motorPin3,OUTPUT);
  pinMode(motorPin4,OUTPUT);

  //servo////
   myServoOne.attach(servoOne);
   myServoOne.write(90); //up position
   myServoTwo.attach(servoTwo);
   myServoTwo.write(0); //up position
   myServoThree.attach(servoThree);
   myServoThree.write(0); //up position
   delay(2000);
   myServoThree.write(75); //up position

  //serial configurations
  Serial.begin(9600);

   //glcd configurations
   //assign default color value
    if ( u8g.getMode() == U8G_MODE_R3G3B2 ) {
      u8g.setColorIndex(255);     // white
    }
    else if ( u8g.getMode() == U8G_MODE_GRAY2BIT ) {
      u8g.setColorIndex(3);         // max intensity
    }
    else if ( u8g.getMode() == U8G_MODE_BW ) {
      u8g.setColorIndex(1);         // pixel on
    }
    else if ( u8g.getMode() == U8G_MODE_HICOLOR ) {
      u8g.setHiColorByRGB(255,255,255);}
  
    pinMode(8, OUTPUT);

    ///counter lcd configuration////
   lcd.begin(16, 2);
    
   lcd.setRGB(colorR, colorG, colorB);
   int charBitmapSize = (sizeof(charBitmap ) / sizeof (charBitmap[0]));

   lcd.begin(16,2);               // initialize the lcd 

   for ( int i = 0; i < charBitmapSize; i++ )
   {
      lcd.createChar ( i, (uint8_t *)charBitmap[i] );
   }
  
//  //glcd start message///
   welcome_message();////first message 
   clear_screen();////clear screen
   introduction_message(); ///intro message 
   clear_screen();////clear screen
   counter_start_message(); // start counter from zero

    //start the conveyor motor and retract the linear motor
    motor_run();
    linear_motor_retract();
}


//run the code forever
void loop() {

///check the plastic LDR value
if(!check_plastic_ldr())
  {//if it is too low, means the item is not transparent hence open the path
    Serial.println("I am here");
    myServoThree.write(0);
    delay(3000);
    myServoThree.write(75);
  }

//check if the LDR has been blocked by an opaque material
if(metal_counter%3==0&&crusherHasRun == false)
  {
    linear_actuator_message();
    linear_motor_activate();
  }
 // //check the state of the crusher limit switch
 if(digitalRead(crusherLS) == HIGH)
  {
    linear_motor_retract();
    delay(7000);
    linear_motor_stop();
  }

idle_message();
counter_autoscroll_display();
}



///***************************USER FUNCTIONS**********************************////
//motor activate
void motor_run(){
  analogWrite(enablePin,255);
  digitalWrite( motorPin1,HIGH);
  digitalWrite( motorPin2,LOW);
}
//stop the conveyor
void motor_stop(){
  analogWrite(enablePin,0);
  digitalWrite( motorPin1,HIGH);
  digitalWrite( motorPin2,HIGH);
  
}



//linear motor activate
void linear_motor_activate(){
    delay(2000);
    analogWrite(enablePin2,255);
    digitalWrite( motorPin3,HIGH);
    digitalWrite( motorPin4,LOW);
}
//retract the linear motor
void linear_motor_retract(){
  digitalWrite(enablePin2,HIGH);
  digitalWrite(motorPin4,HIGH);
  digitalWrite(motorPin3,LOW);
}
//retract the linear motor
void linear_motor_stop(){
  digitalWrite(enablePin2,LOW);
  digitalWrite( motorPin4,HIGH);
  digitalWrite( motorPin3,HIGH);
}


//counter start message
void counter_start_message(){


lcd.home ();
   // Do a little animation by writing to the same location
  
     for ( int a = 0; a < 5; a++ )
     {
   for ( int i = 0; i < 2; i++ )
   {
    lcd.setCursor ( 0, 0 );
  
      for ( int j = 0; j < 16; j++ )
      {
         lcd.print (char(random(7)));
      }
      lcd.setCursor ( 0, 1);
      delay (200); 
   }
}
lcd.setCursor ( 1, 1);
lcd.print("Garbage Counter");
delay (2000);
lcd.clear();
}
//counter auto-scroll///
void counter_autoscroll_display(){
lcd.setCursor(1, 1);
    // Print a message to the LCD.
    lcd.print("Metal:");
    lcd.print(metal_counter);
     lcd.print(" ");
    lcd.print("Glass:");
    lcd.print(glass_counter);
    lcd.print(" ");
    lcd.print("Plastic:");
    lcd.print(plastic_counter);
    lcd.print(" ");
    //delay(1000);
    for (int positionCounter = 0; positionCounter < 10; positionCounter++) {
        // scroll one position left:
        lcd.scrollDisplayLeft();
        // wait a bit:
       
    }
     //delay(200);
}
/////glcd functions//////
void welcome_message(){
 
  u8g.firstPage();  
  do {
   u8g.setColorIndex(1);
   u8g.setFont(u8g_font_gdb12);
  //u8g.setFont(u8g_font_osb21);
   
   u8g.drawStr( 0, 35, "Welcome");
   
  } while( u8g.nextPage() ); 
   // If its too fast, you could add a delay
  delay(3000);
  }    
void introduction_message(void) {
    u8g.firstPage();  
  do {
u8g.setFont(u8g_font_unifont);
  //u8g.setFont(u8g_font_osb29);
  u8g.drawFrame(0,0,128,63);
  u8g.drawStr( 4, 12, "Garbage Sorting");
  u8g.drawStr( 52, 25, "and");
   u8g.drawStr( 25, 35, "Separation");
   u8g.drawStr( 40, 55, "FYP14-18");
  } while( u8g.nextPage() );
  delay(3000);
}
void metal_message(){
 
  u8g.firstPage();  
  do {
//u8g.setFont(u8g_font_gdb12);
  u8g.setFont(u8g_font_fub11);
   u8g.drawBitmapP( 60, 10, 1, 8, rook_bitmap);
   u8g.drawStr( 0, 35, "Metal detected");
   u8g.setFont(u8g_font_courB10);
   u8g.setFontPosTop();
   u8g.drawStr(0, 45, "(Blockade ON)"); // actual display position is (0,24)
  u8g.drawHLine(5, 45+12, 110); // All other procedures are also affected
  } while( u8g.nextPage() ); 
   // If its too fast, you could add a delay
  
  }
void glass_message(){
 
  u8g.firstPage();  
  do {
u8g.setFont(u8g_font_fub11);
   u8g.drawBitmapP( 60, 10, 1, 8, rook_bitmap);
   u8g.drawStr( 0, 35, "Glass detected");
   u8g.setFont(u8g_font_courB10);
   u8g.setFontPosTop();
   u8g.drawStr(0, 45, "(Blockade ON)"); // actual display position is (0,24)
  u8g.drawHLine(5, 45+12, 110); // All other procedures are also affected
  } while( u8g.nextPage() ); 
   // If its too fast, you could add a delay
  
  }
void plastic_message(){
 
  u8g.firstPage();  
  do {
u8g.setFont(u8g_font_fub11);
   u8g.drawBitmapP( 60, 10, 1, 8, rook_bitmap);
   u8g.drawStr( 0, 35, "Plastic detected");
   u8g.setFont(u8g_font_courB10);
   u8g.setFontPosTop();
   u8g.drawStr(0, 45, "(Blockade ON)"); // actual display position is (0,24)
  u8g.drawHLine(5, 45+12, 110); // All other procedures are also affected
  } while( u8g.nextPage() ); 
   // If its too fast, you could add a delay
  
  }
void linear_actuator_message() {

  int a;
 for(a>0;a<15;a++){
  
 u8g.firstPage();  
  do {
   u8g.setFont(u8g_font_6x10); 
 u8g.drawStr( 10,10,"METAL LIMIT REACHED");
//  u8g.drawBox(5,10,20,10);
//  u8g.drawBox(10+a,15,30,7);
  u8g.drawStr( 0, 30, "Linear Actuator ON");
  u8g.drawFrame(5,10+30,40,20);
  u8g.drawFrame(10+a,15+30,60,10); 



  } while( u8g.nextPage() ); 
}
  
}
void idle_message(){

 u8g.setColorIndex(1);
 u8g.setFont(u8g_font_6x10);
  u8g.setFontRefHeightExtendedText();
  u8g.setDefaultForegroundColor();
  u8g.setFontPosTop();
  int a;
 for(a>0;a<25;a++){
 u8g.firstPage();  
  do {

 u8g.drawStr( 0, 0, "Material Detection");
  u8g.drawLine(7+a, 10, 40, 55);
  u8g.drawLine(7+a*2, 10, 60, 55);
  u8g.drawLine(7+a*3, 10, 80, 55);
  u8g.drawLine(7+a*4, 10, 100, 55);
  } while( u8g.nextPage() ); 
}
  
  }
void clear_screen(){
u8g.firstPage();  
    do {
    } while( u8g.nextPage() );
   delay(5);
}


///********************************LDR FUNCTION*************************************///
bool check_glass_ldr(){
//read the value of the LDR
int ldr_value = analogRead(glass_ldr);
Serial.print("Glass LDR:"); 
Serial.println(ldr_value);
sei();
delay(500);
if(ldr_value>50 && ldr_value<120)
  {
    return true;
  }
 else
 {
  return false;
  }
}
bool check_plastic_ldr(){
//read the value of the LDR
int ldr_value = digitalRead(plastic_ldr);
Serial.print("Plastic LDR:"); 
Serial.println(ldr_value);
if(ldr_value == LOW)
  {
    return false;
  }
 else
 {
  return true;
  }
}
///********************************INTERRUPT FUNCTIONS*************************************
//function call after an interrupt is raised on the inductive sensor
void inductiveFunc(){
  metal_message();
  metal_counter++;
  myServoOne.write(0);
  sei();
  delay(2000);
  myServoOne.write(90);
  cli();
  crusherHasRun = false;
  
}
//function call after an interrupt is raised on the cap sensor
void capacitiveFunc(){
  motor_stop();
  
  if(check_glass_ldr())
  {
    glass_counter++;
    glass_message();
    sei();
    delay(500);
  myServoTwo.write(90);
  sei();
  delay(2000);
  motor_run();
  sei();
  delay(1000);
  myServoTwo.write(0);
 }
  cli();
  motor_run();
  

}



////function call to activate crusher
//void crusherActivate(){
//Serial.println("Crusher Activating");
//sei();
//  delay(500);
//linear_actuator_message();
//linear_motor_activate();
//cli();
//}





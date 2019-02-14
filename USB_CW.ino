#include <Keyboard.h>
#include <Mouse.h>

#define BUZZER 1
#define BUTTON 14
#define CW 0
#define PLUG 2
#define WPM 18

int mouse_mode = 1;
int buzzer_on = 0;
int mouse_down;
int mouse_init;
int mode_change = 0;
int word_length = 0;
int new_word = 0;

int dot_length;
int word_space;
int letter_space;
unsigned long last_time = millis();
unsigned long start_silence, end_silence;

byte morse_buffer[33];
int  mbp = 0;

int timepoint()
{
   unsigned long t = millis();
   int td = (int) (t - last_time);
   last_time = t;
   return td;
}

void push(char c)
{
   if (mbp < 31) 
   {
      morse_buffer[mbp] = c;
      mbp++; 
   }
}

void type(char c)
{
   Keyboard.print(c);
   if ((c == ' ') || (c == '\n')) new_word = 1;
   else if (new_word) 
   {
      word_length = 0;
      new_word = 0;
   }

   word_length++;
}

void mistake()
{
   int i;
   for (i=0; i<word_length; i++) Keyboard.print((char) 0x8);
   word_length = 0;
}

void translate()
{
   Serial.println((char *) morse_buffer);
   if (mbp == 1)
   {
      if (morse_buffer[0] == '-') type('t');
      else type('e');
   }
   else if (mbp == 2)
   {
      if ((morse_buffer[0] == '-') && (morse_buffer[1] == '-')) type('m');
      else if ((morse_buffer[0] == '-') && (morse_buffer[1] == '*')) type('n');
      else if ((morse_buffer[0] == '*') && (morse_buffer[1] == '-')) type('a');
      else type('i');
   }
   else if (mbp == 3)
   {
      if ((morse_buffer[0] == '-') && (morse_buffer[1] == '-') && (morse_buffer[2] == '-')) type('o');
      else if ((morse_buffer[0] == '-') && (morse_buffer[1] == '-') && (morse_buffer[2] == '*')) type('g');
      else if ((morse_buffer[0] == '-') && (morse_buffer[1] == '*') && (morse_buffer[2] == '-')) type('k');
      else if ((morse_buffer[0] == '-') && (morse_buffer[1] == '*') && (morse_buffer[2] == '*')) type('d');
      else if ((morse_buffer[0] == '*') && (morse_buffer[1] == '-') && (morse_buffer[2] == '-')) type('w');
      else if ((morse_buffer[0] == '*') && (morse_buffer[1] == '-') && (morse_buffer[2] == '*')) type('r');
      else if ((morse_buffer[0] == '*') && (morse_buffer[1] == '*') && (morse_buffer[2] == '-')) type('u');
      else type('s');
   }
   else if (mbp == 4)
   {
       if (morse_buffer[0] == '-')
       {
           if ((morse_buffer[1] == '*') && (morse_buffer[2] == '*') && (morse_buffer[3] == '*')) type('b');
           else if ((morse_buffer[1] == '*') && (morse_buffer[2] == '-') && (morse_buffer[3] == '*')) type('c');
           else if ((morse_buffer[1] == '-') && (morse_buffer[2] == '*') && (morse_buffer[3] == '-')) type('q');
           else if ((morse_buffer[1] == '*') && (morse_buffer[2] == '*') && (morse_buffer[3] == '-')) type('x');
           else if ((morse_buffer[1] == '*') && (morse_buffer[2] == '-') && (morse_buffer[3] == '-')) type('y');
           else type('z');
       }
       else
       {
           if ((morse_buffer[1] == '*') && (morse_buffer[2] == '-') && (morse_buffer[3] == '*')) type('f');
           else if ((morse_buffer[1] == '*') && (morse_buffer[2] == '*') && (morse_buffer[3] == '*')) type('h');
           else if ((morse_buffer[1] == '-') && (morse_buffer[2] == '-') && (morse_buffer[3] == '-')) type('j');
           else if ((morse_buffer[1] == '-') && (morse_buffer[2] == '*') && (morse_buffer[3] == '-')) type('\n');
           else if ((morse_buffer[1] == '-') && (morse_buffer[2] == '*') && (morse_buffer[3] == '*')) type('l');
           else if ((morse_buffer[1] == '-') && (morse_buffer[2] == '-') && (morse_buffer[3] == '*')) type('p');
           else type('v');
       }
   }
   else if (mbp == 8) mistake();
   
   mbp = 0;
   memset(morse_buffer, 0, 32);
}

void setup() 
{
  mouse_down = 0;
  mouse_init = 0;
  pinMode(CW,  INPUT_PULLUP);
  pinMode(PLUG,  INPUT_PULLUP);
  pinMode(BUTTON,  INPUT_PULLUP);
  pinMode(BUZZER,  OUTPUT);
  Serial.begin(9600);

  // Calculate constants
  dot_length = 1200 / WPM;
  word_space = dot_length * 10;
  letter_space = dot_length * 3;
  Serial.println("USB/CW started.");
  Serial.println(dot_length);

  memset(morse_buffer, 0, 33);

  // Initialise
  Mouse.begin();
  Keyboard.begin();
  mouse_init = 1;
}

void loop() 
{
   if ((!mouse_init) && (digitalRead(PLUG) == HIGH)) 
   {
       Serial.println("Key plugged in.");
       Mouse.begin();
       Keyboard.begin();
       mouse_init = 1;
   }

   if (mouse_init) 
   {
      if (digitalRead(PLUG) == LOW)
      {
          Serial.println("Unplugged."); 
          Mouse.end();
          Keyboard.end();
          mouse_init = 0;
      }
      else
      {
         // Key down
         if (digitalRead(CW) == LOW) 
         {
            if (!mouse_down)
            {
               int gap = timepoint();  

               if (mouse_mode) Mouse.press();
               else 
               {
                   if ((gap > word_space) && (!new_word)) type(' ');
                   if (gap > letter_space) 
                   { 
                      if (mbp > 0) translate();
                   }
               } 
               if (buzzer_on) digitalWrite(BUZZER, HIGH);
               mouse_down = 1;
            }
         }
         // Key up
         else if (mouse_down)
         {
            int keyed = timepoint();

            if (mouse_mode) Mouse.release();
            else if (keyed < (dot_length * 2)) 
            {
               push('*');
            }
            else
            { 
               push('-');
            }
            digitalWrite(BUZZER, LOW);
            mouse_down = 0;
         }
         // time out
         if ((mbp > 0) && (!mouse_down) && ((millis() - last_time) > letter_space)) 
         {
            translate();
         }
      }
   }

   // Button press
   if (digitalRead(BUTTON) == LOW)
   {
      delay(30);
      if ((digitalRead(BUTTON) == LOW)  && (!mode_change))
      {        
          mode_change = 1;
          if ((mouse_mode) && (!buzzer_on)) buzzer_on = 1;
          else if ((mouse_mode) && (buzzer_on)) 
          {
             mouse_mode = 0;
             buzzer_on = 0;
          }
          else if ((!mouse_mode) && (!buzzer_on)) buzzer_on = 1; 
          else 
          {
             mouse_mode = 1;
             buzzer_on = 0;  
          }
          if (buzzer_on) Serial.println("Buzzer on.");
          else Serial.println("Buzzer off.");
          digitalWrite(BUZZER, HIGH);
          delay(50);
          digitalWrite(BUZZER, LOW);

          if (digitalRead(CW) == LOW) Serial.println("key down.");
          else Serial.println("key up.");

          if (digitalRead(PLUG) == LOW) Serial.println("plug in.");
          else Serial.println("plug out.");
      }
   }
   else mode_change = 0;
   
   delay(5);
}

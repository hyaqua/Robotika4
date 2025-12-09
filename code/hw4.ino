#include <LiquidCrystal.h>
#include <ServoTimer2.h>

const int pot_pins[] = {A5, A4, A3, A2};
int pot_vals[] = {0,0,0,0};

const int lightres_pin = A0;
int lightres_val = 0;
unsigned long last_time_add = 0;
unsigned long wait_time = 600; // wait for second if more than that has passed then add another 20 mins

const int btn_pins[] = {2,3};
volatile bool btn_pressed[] = {false, false};
volatile unsigned long btn_last_int_time[] = {0L, 0L};
unsigned long debounce_time = 200;

const int piezo_pin = 6;

const int servo_pin = 5;
const int servo_angles[] = {0, 90};
int servo_pos = 0; // open or closed
int servo_pwm[] = {750,2250};
ServoTimer2 servo;

const int rs = 8, en = 9, d4 = 10, d5= 11, d6 = 12, d7 = 13;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
char line1[17]; // For keeping track of text on screen
char line2[17];
bool update_text = true;

int major_mode = 0; // Unlocked, Locked
int minor_mode = 0; // Used to know what screen to show and what values to expect

unsigned long time = 0; // How much time is left until unlock in seconds (can have around 100 years)

int pin_or_math = 0; // 0 - pin, 1- math

char question_str[17];
int difficulty = 0; // difficulty slider, 0-2, 0 easy, 1 medium 2 hard
int answer = 0; // four numbers, used by both pin and math
int pot_guess = 0;

const int max_tries = 5;
int tries = 0;
bool correct = false;
bool debug = true;

int days,hours,minutes,seconds;
char pin[5];

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(piezo_pin, OUTPUT);
  pinMode(servo_pin, OUTPUT);
  cli(); // stop interrupts
  TCCR1A = 0; // set entire TCCR1A register to 0
  TCCR1B = 0; // same for TCCR1B
  TCNT1  = 0; // initialize counter value to 0
  // set compare match register for 1 Hz increments
  OCR1A = 62499; // = 16000000 / (256 * 1) - 1 (must be <65536)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS12, CS11 and CS10 bits for 256 prescaler
  TCCR1B |= (1 << CS12) | (0 << CS11) | (0 << CS10);
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  sei(); // allow interrupts

  Serial.begin(9600);  // for printing curr status;

  attachInterrupt(digitalPinToInterrupt(btn_pins[0]), btn1_interrupt, RISING);
  attachInterrupt(digitalPinToInterrupt(btn_pins[1]), btn2_interrupt, RISING);

  lcd.begin(16, 2);
  reset_lines();

  servo.attach(servo_pin);
  randomSeed(analogRead(A1));
}

ISR(TIMER1_COMPA_vect){
   // Goes off every 1s
   if(time > 0L && major_mode == 1){
    time--;
   }
   debug = true;
}

void loop() {
  // Run stuff here:

  if(debug){
    Serial.print("MajorMode:");
    Serial.print(major_mode);
    Serial.print("\tMinorMode:");
    Serial.println(minor_mode);
    Serial.print("pot_vals: ");
    Serial.print(pot_vals[0]);
    Serial.print("\t");
    Serial.print(pot_vals[1]);
    Serial.print("\t");
    Serial.print(pot_vals[2]);
    Serial.print("\t");
    Serial.print(pot_vals[3]);
    Serial.println("\t");

    Serial.print("Time:");
    Serial.println(time);
    debug=false;
  }

  update_screen();
  get_pots();
  // Do main loop things here:
  switch(major_mode){
    case 0:
      // Unlocked screen
      switch (minor_mode){
        case 0:
          reset_lines();
          //               0123456789ABCDEF
          put_text(line1, "Press btn1/2", 0);
          put_text(line2, "To lock the box", 0);
          update_text = true;
          minor_mode++;
          break;
        case 1:
          // wait for buttons to proceed
          if (btn_pressed[0] || btn_pressed[1]){
            btn_pressed[0] = false;
            btn_pressed[1] = false;
            minor_mode++;
          }
          break;
        case 2:
          reset_lines();
          put_text(line1, "Rotate dials to ", 0);
          put_text(line2, "enter time", 0);
          update_text = true;
          minor_mode++;
          break;
        case 3:
          if (btn_pressed[0] || btn_pressed[1]){
            btn_pressed[0] = false;
            btn_pressed[1] = false;
            minor_mode++;
            reset_lines();
            put_text(line1, "  DD:HH:MM:SS", 0);
            update_text = true;
          }
          break;
        case 4:
          // entering time:
          {
          days = map(pot_vals[0], 0, 1023, 0, 99);
          hours = map(pot_vals[1], 0, 1023, 0, 23);
          minutes = map(pot_vals[2], 0, 1023, 0, 59);
          seconds = map(pot_vals[3], 0, 1023, 0, 59);

          char time_text[12];
          sprintf(time_text, "%02d:%02d:%02d:%02d", days, hours, minutes, seconds);
          put_text(line2, time_text, 2);
          update_text = true;

          if (btn_pressed[0] || btn_pressed[1]){
            time = (days * 86400L) + (hours * 3600L) + (minutes * 60L) + seconds;
            btn_pressed[0] = false;
            btn_pressed[1] = false;
            minor_mode++;
          }
          break;
          }
        case 5:
          reset_lines();
          put_text(line1, "btn1: set pin",0);
          put_text(line2, "btn2:math problem",0);
          update_text = true;
          minor_mode++;
          break;
        case 6:
          if(btn_pressed[0]){       // pin
            pin_or_math = 0;
            btn_pressed[0] = false;
            btn_pressed[1] = false;
            minor_mode++;
          }
          else if(btn_pressed[1]){  // math
            pin_or_math = 1;
            btn_pressed[0] = false;
            btn_pressed[1] = false;
            minor_mode = 10;
          }
          break;
        case 7:
          reset_lines();
          pin_or_math = 0;
          put_text(line1,"Enter pin:",0);
          minor_mode++;
          break;
        case 8:
          pin[0] = '0' + map(pot_vals[0], 0, 1023, 0, 9);
          pin[1] = '0' + map(pot_vals[1], 0, 1023, 0, 9);
          pin[2] = '0' + map(pot_vals[2], 0, 1023, 0, 9);
          pin[3] = '0' + map(pot_vals[3], 0, 1023, 0, 9);
          put_text(line2, pin, 0);
          update_text = true;

          if (btn_pressed[0] || btn_pressed[1]){
            btn_pressed[0] = false;
            btn_pressed[1] = false;
            answer = (pin[0]-'0' * 1000) + (pin[1]-'0' * 100) + (pin[2]-'0' * 10) + (pin[3]-'0');
            minor_mode++;
          }
          break;
        case 9:
          // lock screen
          minor_mode = 0; // major_mode 1 handles locking and unlocking so we dont use the function ourselves
          major_mode = 1;
          break;
        case 10: // Math mode selection
          // ask difficulty, then generate a question based on it
          reset_lines();
          put_text(line1, "btn1:change diff",0);
          put_text(line2, "btn2:save choice",0);

          update_text = true;
          minor_mode++;
          break;
        case 11:
          if(btn_pressed[0] || btn_pressed[1]){
            btn_pressed[0] = false;
            btn_pressed[1] = false;
            minor_mode++;
            difficulty = 0;
            reset_lines();
            put_text(line1, "difficulty:",0);
            put_text(line2, "EASY   ", 0);
            update_text=true;
          }
          break;
        case 12:
          if(btn_pressed[0]){
            btn_pressed[0] = false;
            btn_pressed[1] = false;
            difficulty++;
            update_text = true;
          }
          if(difficulty == 0) put_text(line2, "EASY   ", 0);
          if(difficulty == 1) put_text(line2, "MEDIUM ", 0);
          if(difficulty == 2) put_text(line2, "HARD   ", 0);

          if(btn_pressed[1]){
            btn_pressed[0] = false;
            btn_pressed[1] = false;
            minor_mode++;
            reset_lines();
            question_gen(difficulty);
            put_text(line1, "question:", 0);
            put_text(line2, question_str, 0);
            update_text = true;
          }
          break;
        case 13: // we generate the question and then show it to the user
          if(btn_pressed[0] || btn_pressed[1]){
            btn_pressed[0] = false;
            btn_pressed[1] = false;
            reset_lines();
            put_text(line1, "Answer:", 0);
            itoa(answer, pin, 10); // convert to string
            put_text(line2, pin, 0);
            update_text = true;
            minor_mode++;
          }
          break;
        case 14: // Showing answer, wait for button to switch to locked mode
          if(btn_pressed[0] || btn_pressed[1]){
            btn_pressed[0] = false;
            btn_pressed[1] = false;
            reset_lines();
            minor_mode = 0;
            major_mode = 1;
          }
      }
      break;
    case 1:
      {
      lightres_val = analogRead(lightres_pin); // get light value to check if tried to open, if the user did, then add another 20 mins


      if(lightres_val > 800){
        unsigned long open_time = millis();
        if(open_time - last_time_add > wait_time){
          time += 20*60UL;
          last_time_add = open_time; // if last time it was opened was more than 0.5s ago then add another 20min and reset time
        }
      }

      if(time == 0 || correct){
        unlock();
        major_mode = 0;
        minor_mode = 0;
        update_text = true;
        reset_lines();
        return;
      }

      if(pin_or_math == 0){ // Pin mode
        switch(minor_mode){
          case 0: // lock device
            lock();
            reset_lines();
            put_text(line1, "Time left",0);
            minor_mode++;
            break;
          case 1: // show time, wait for button
            days = (int) (time/86400UL);
            hours = (int) ((time%86400UL)/3600UL);
            minutes = (int) ((time%3600) / 60);
            seconds = (int) (time%60UL);
            sprintf(line2, "%02d:%02d:%02d:%02d", days,hours,minutes,seconds);
            update_text=true;
            if(btn_pressed[0] || btn_pressed[1]){
              btn_pressed[0] = false;
              btn_pressed[1] = false;
              reset_lines();
              //              0123456789ABCDEF
              put_text(line1,"Enter pin:", 0);
              minor_mode++;
            }
            break;
          case 2: // Show pin entry, wrong guess adds 10 mins to timer!
            if(tries > max_tries){
              btn_pressed[0] = false;
              btn_pressed[1] = false;
              minor_mode = 0;
              return;
            }
            days = (int) (time/86400UL);
            hours = (int) ((time%86400UL)/3600UL);
            minutes = (int) ((time%3600) / 60);
            seconds = (int) (time%60UL);
            sprintf(line2, "%02d:%02d:%02d:%02d", days,hours,minutes,seconds);
            pin[0] = '0' + map(pot_vals[0], 0, 1023, 0, 9);
            pin[1] = '0' + map(pot_vals[1], 0, 1023, 0, 9);
            pin[2] = '0' + map(pot_vals[2], 0, 1023, 0, 9);
            pin[3] = '0' + map(pot_vals[3], 0, 1023, 0, 9);
            put_text(line1, pin, 11);
            update_text = true;

            if(btn_pressed[0]){ // try guess
              btn_pressed[0] = false;
              btn_pressed[1] = false;
              if(tries < max_tries){
                int guess = (pin[0]-'0' * 1000) + (pin[1]-'0' * 100) + (pin[2]-'0' * 10) + (pin[3]-'0');
                if(answer != guess){
                  time += 20*60UL;
                }
                tries+=1;
              } else{
                minor_mode = 1;
              }
              btn_pressed[0] = false;
              btn_pressed[1] = false;
            }
            if(btn_pressed[1]){ // exit
              btn_pressed[0] = false;
              btn_pressed[1] = false;
              minor_mode = 1;
              reset_lines();
              put_text(line1, "Time left",0);
              update_text = true;
            }
            break;
        }
      }
      if(pin_or_math == 1){
        switch(minor_mode){
          case 0: // lock device
            lock();
            reset_lines();
            put_text(line1, "Time left",0);
            minor_mode++;
            break;
          case 1:
            days = (int) (time/86400UL);
            hours = (int) ((time%86400UL)/3600UL);
            minutes = (int) ((time%3600) / 60);
            seconds = (int) (time%60UL);
            sprintf(line2, "%02d:%02d:%02d:%02d", days,hours,minutes,seconds);
            update_text=true;
            if(btn_pressed[0] || btn_pressed[1]){
              btn_pressed[0] = false;
              btn_pressed[1] = false;
              reset_lines();
              //              0123456789ABCDEF
              put_text(line1,question_str, 0);
              minor_mode++;
            }
            break;
          case 2: // Showing question, able to enter answer
            if(tries > max_tries){
              btn_pressed[0] = false;
              btn_pressed[1] = false;
              minor_mode = 0;
              return;
            }
            pin[0] = '0' + map(pot_vals[0], 0, 1023, 0, 9);
            pin[1] = '0' + map(pot_vals[1], 0, 1023, 0, 9);
            pin[2] = '0' + map(pot_vals[2], 0, 1023, 0, 9);
            pin[3] = '0' + map(pot_vals[3], 0, 1023, 0, 9);
            put_text(line2,"Guess:",0);
            put_text(line2, pin, 7);
            update_text = true;

            if(btn_pressed[0]){ // try guess
              btn_pressed[0] = false;
              btn_pressed[1] = false;
              if(tries < max_tries){
                int guess = map(pot_vals[0], 0, 1023, 0, 9)*1000 + map(pot_vals[1], 0, 1023, 0, 9)*100 + map(pot_vals[2], 0, 1023, 0, 9)*10 + map(pot_vals[3], 0, 1023, 0, 9);
                Serial.print("Guess:");
                Serial.print(guess);
                Serial.print("\t Answer:");
                Serial.println(answer);
                if(answer != guess){
                  time += 20*60UL;
                } else {
                  correct = true;
                }
                tries+=1;
              } else{
                minor_mode = 1;
              }
              btn_pressed[0] = false;
              btn_pressed[1] = false;
            }
            if(btn_pressed[1]){ // exit
              btn_pressed[0] = false;
              btn_pressed[1] = false;
              minor_mode = 1;
              reset_lines();
              put_text(line1, "Time left",0);
              update_text = true;
            }
            break;
            break;

        }
      }
      // Locked screen
      break;
      }
  }
}

void lock(){
  servo_pos = 1;
  servo.write(map(servo_angles[servo_pos], 0, 360, servo_pwm[0], servo_pwm[1]));
}
void unlock(){
  servo_pos = 1;
  servo.write(map(servo_angles[servo_pos], 0, 360, servo_pwm[0], servo_pwm[1]));
}

void get_pots() {  // Get values from potenciometers to an array
  pot_vals[0] = analogRead(pot_pins[0]);
  pot_vals[1] = analogRead(pot_pins[1]);
  pot_vals[2] = analogRead(pot_pins[2]);
  pot_vals[3] = analogRead(pot_pins[3]);
}

void update_screen(){
  if(update_text){
    lcd.setCursor(0, 0);
    lcd.print(line1);
    lcd.setCursor(0, 1);
    lcd.print(line2);
    update_text = false;
  }
}

void reset_lines() {
  put_text(line1, "                ", 0);
  put_text(line2, "                ", 0);
}

void put_text(char *dest, const char *src, int offset) {
  // we're only using this for line1 and line2 so destination length will always be 16
  int dest_len = 16;
  if (offset < 0 || offset >= dest_len) return;  // if offset is too small or too large do nothing;
  // putting before src_len to not waste cycles :d
  int src_len = strlen(src);

  int copy_len = min(src_len, dest_len - offset);  // check if we can copy the whole text, or just a part of it;

  update_text = true;

  strncpy(dest + offset, src, copy_len);
}


void question_gen(int tier) {
  long ans = -1;
  long a, b, c, d, tempVal;
  int type;

  while (ans < 0 || ans >= 10000) {
    if (tier == 1) {  // 1st level (easy)
      type = random(0, 4);
      switch (type) {
        case 0: // A + B - C
          a = random(10, 200); b = random(10, 200); c = random(10, 200);
          ans = a + b - c;
          snprintf(question_str, 17, "%ld+%ld-%ld", a, b, c);
          break;
        case 1: // A - B + C
          a = random(50, 200); b = random(10, 50); c = random(10, 100);

          ans = a - b + c;
          snprintf(question_str, 17, "%ld-%ld+%ld", a, b, c);
          break;

        case 2: // A * B
          a = random(2, 15); b = random(2, 15);

          ans = a * b;
          snprintf(question_str, 17, "%ld*%ld", a, b);
          break;

        case 3: // A / B
          b = random(2, 15);
          tempVal = random(2, 50);
          a = b * tempVal;
          ans = tempVal;
          snprintf(question_str, 17, "%ld/%ld", a, b);
          break;
      }
    } else if (tier == 2) { // second level (medium)
      type = random(0, 4);
      switch (type) {
        case 0: // A * B - C
          a = random(10, 20); b = random(10, 20); c = random(10, 100);

          ans = (a * b) - c;
          snprintf(question_str, 17, "%ld*%ld-%ld", a, b, c);
          break;

        case 1: // A + B * C
          a = random(10, 100); b = random(2, 15); c = random(2, 15);

          ans = a + (b * c);
          snprintf(question_str, 17, "%ld+%ld*%ld", a, b, c);
          break;

        case 2: // (A + B) * C
          a = random(1, 20); b = random(1, 20); c = random(2, 10);

          ans = (a + b) * c;
          snprintf(question_str, 17, "(%ld+%ld)*%ld", a, b, c);
          break;

        case 3: // A / B + C
          b = random(2, 12);
          tempVal = random(5, 50);
          a = b * tempVal;
          c = random(10, 100);

          ans = tempVal + c;
          snprintf(question_str, 17, "%ld/%ld+%ld", a, b, c);
          break;
      }
    }
    else {  // 3rd level (highest)
      type = random(0, 4);
      switch (type) {
        case 0: // A*B - C*D
          a = random(15, 50); b = random(15, 50);
          c = random(2, 15); d = random(2, 15);

          ans = (a * b) - (c * d);
          snprintf(question_str, 17, "%ld*%ld-%ld*%ld", a, b, c, d);
          break;

        case 1: // A*B + C*D
          a = random(2, 20); b = random(2, 20);
          c = random(2, 20); d = random(2, 20);

          ans = (a * b) + (c * d);
          snprintf(question_str, 17, "%ld*%ld+%ld*%ld", a, b, c, d);
          break;

        case 2: // (A + B) / C
          c = random(2, 15);
          tempVal = random(10, 500);
          long totalTop = c * tempVal;
          a = random(1, totalTop - 1);
          b = totalTop - a;

          ans = tempVal;
          snprintf(question_str, 17, "(%ld+%ld)/%ld", a, b, c);
          break;

        case 3: // A * B / C 
          c = random(2, 10);
          tempVal = random(1, 20);
          a = c * tempVal;
          b = random(2, 20);

          ans = (a * b) / c;
          snprintf(question_str, 17, "%ld*%ld/%ld", a, b, c);
          break;
      }
    }
  }
  answer = ans;
}


void btn1_interrupt() {
  unsigned long curr_time = millis();
  if (curr_time - btn_last_int_time[0] > debounce_time) {
    btn_pressed[0] = true;
  }
  btn_last_int_time[0] = curr_time;
}

void btn2_interrupt() {
  unsigned long curr_time = millis();
  if (curr_time - btn_last_int_time[1] > debounce_time) {
    btn_pressed[1] = true;
  }
  btn_last_int_time[1] = curr_time;
}
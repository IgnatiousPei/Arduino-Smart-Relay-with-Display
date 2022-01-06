/* 
*Author: Ashween Ignatious Peiris
*Last modified: 26-Dec-2021

*Overview: This program performs voltage and time based relay switching, while providing the option to reset the time to a desired value
*          in the case of battery backup failure. Extensive dynamic menus allow the user to view and edit the data in a structured and 
*          familiar manner. 

MIT License

Copyright (c) 2022 Ashween Ignatious Peiris

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Bounce2.h>
#include <RTClib.h>
#include <SPI.h>
#include <TimeLib.h>
#include <TimeAlarms.h>
#include <EEPROM.h>
#include <string.h>
#include <Arduino.h>

/* Input pin setup for the buttons*/
const int IN_up_btn_pin = 3;
const int IN_down_btn_pin = 2;
const int IN_left_btn_pin = 4;
const int IN_right_btn_pin = 5;
const int IN_sel_btn_pin = 7;
const int IN_back_btn_pin = 6;

/* Pin setup for the I2C LCD display */
// Setting the address of the LCD diplay as 0x27
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);
// the number of the LED pin so it can be dimmed through PWM
const int OUT_led_pin = 11;

/* Pin setup for the voltage measurement */
int IN_voltage_pin = A0;

/* Pin setup for the relay output */
int OUT_relay_pin = 8;

/* VARIABLE DECLARATIONS */

// Sleeping time (set in milliseconds)
const int T_SLEEP = 8500;

// time alarm state variables
int view_time_alarm_state = 0;
int set_time_alarm_state = 0;
int reset_time_alarm_state = 0;
int temp_time_alarm_num = 0;

// voltage alarm state variables
int view_volt_alarm_state = 0;
int set_volt_alarm_state = 0;
int reset_volt_alarm_state = 0;

// set datetime state variables
int view_datetime_state = 0;
int set_datetime_state = 0;

/* 


? START TIME ALARM VARIABLES
*/
// temporary variables for the time on and timeoff (used inside functions and then cleared)
int time_on_temp[] = {0, 0, 0, 0, 0};
int time_off_temp[] = {0, 0, 0, 0, 0};
int *ptimeon = &time_on_temp[0];
int *ptimeoff = &time_off_temp[0];

// temporary time variables shown on screen when setting and resetting (used inside functions and then cleared)
String time_on_temp_s = "0000";
String time_off_temp_s = "0000";

// persistent string time variables (stored in "RAM")
String ON_times_s[10] = {"0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000"};
String OFF_times_s[10] = {"0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000"};
bool active_alarms[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// EEPROM variables stored elsewhere
/* 
? END TIME ALARM VARIABLES


*/

/* 


? START VOLTAGE ALARM VARIABLES
*/
// temporary integer voltages (used inside functions and then cleared)
int volt_on_temp[] = {0, 0, 0, 0};
int volt_off_temp[] = {0, 0, 0, 0};

// pointers to the integer voltages (used inside functions and then cleared)
int *pvolton = &volt_on_temp[0];
int *pvoltoff = &volt_off_temp[0];

// temporary string voltage variables (used inside functions and then cleared)
String volt_on_temp_s = "00.0";
String volt_off_temp_s = "00.0";

// persistent string voltage variables (stored in "RAM")
String ON_volt_s = "00.0";
String OFF_volt_s = "00.0";

// displayed integer voltage variables (stored in "RAM")
int ON_volt = 0;
int OFF_volt = 0;

// persistant voltage alarm flag (stored in "RAM")
bool volt_active = 0;

// measured volatge value (stored in "RAM")
int voltage = 0;

// EEPROM variables stored elsewhere
/* 
? END VOLTAGE ALARM VARIABLES


*/

/* 


? START DATETIME VARIABLES
*/
// datetime temporary variable values
int date_time_temp[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int *pdatetime_temp = &date_time_temp[0];

uint16_t newyear = 0;
uint8_t newmonth = 0;
uint8_t newday = 0;
uint8_t newhour = 0;
uint8_t newminute = 0;
uint8_t newsec = 0;

/* 
? END DATETIME VARIABLES


*/

/* 


? START EEPROM VARIABLES
*/
// the EEPROM addresses for the stored value arrays
int ee_on_address = 0;
int ee_off_address = 60;
int ee_set_address = 120;

int ee_volts_on_address = 140;
int ee_volts_off_address = 150;
int ee_volts_set_address = 160;

int read_eeprom_st = 0;

char ee_on[10][5] = {"0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000"};
char ee_off[10][5] = {"0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000"};
bool ee_active[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

int ee_volts_on[] = {0, 0, 0, 0};
int ee_volts_off[] = {0, 0, 0, 0};
bool ee_volts_active = 0;
/* 
? END EEPROM VARIABLES


*/

/* 


? START LCD VARIABLES
*/
// cursor position
int cursorPos = 0;
int *ptrCurPos = &cursorPos;
/* 
? END LCD VARIABLES


*/

/* 


? START BUTTON INPUT DEFINITIONS
*/
Bounce up = Bounce();
Bounce dn = Bounce();
Bounce lt = Bounce();
Bounce rt = Bounce();
Bounce ok = Bounce();
Bounce bc = Bounce();
/* 
? END BUTTON INPUT DEFINITIONS


*/

/* 


? START RTC DECLARATION
*/
// Setting up the RTC for use
RTC_DS1307 rtc;
int prev_sec = 0;
int al_num = 0;
/* 
? END RTC DECLARATIONS


*/
int state = 0;
int need_clean = 0;

/* FUNCTION DECLARATIONS */

/* 



????????????????????????????

? UTILITY FUNCTIONS USED

????????????????????????????
*/

//+ Handles button presses and maps it to state transitions
int handle_button_inputs(int up_st, int dn_st, int lt_st, int rt_st, int ok_st, int bc_st, int curr_st)
{
  int next_state = curr_st;

  if (up.rose())
  {
    next_state = up_st;
  }
  else if (dn.rose())
  {
    next_state = dn_st;
  }
  else if (lt.rose())
  {
    next_state = lt_st;
  }
  else if (rt.rose())
  {
    next_state = rt_st;
  }
  else if (ok.rose())
  {
    next_state = ok_st;
  }
  else if (bc.rose())
  {
    next_state = bc_st;
  }

  return next_state;
}

//+ Reset voltage temporary variables
void reset_temp_volt_variables()
{
  int length_of_volt_temps = sizeof(volt_on_temp) / sizeof(volt_on_temp[0]);

  volt_off_temp_s = "00.0";
  volt_on_temp_s = "00.0";

  for (int i = 0; i < length_of_volt_temps; i++)
  {
    volt_on_temp[i] = 0;
    volt_off_temp[i] = 0;
  }
}

//+ Reset time temporary variables
void reset_temp_time_variables()
{
  int length_of_time_temps = sizeof(time_on_temp) / sizeof(time_on_temp[0]);

  time_off_temp_s = "0000";
  time_on_temp_s = "0000";

  for (int i = 0; i < length_of_time_temps; i++)
  {
    time_on_temp[i] = 0;
    time_off_temp[i] = 0;
  }
}

void reset_temp_datetime_variables()
{
  for (int i = 0; i < 16; i++)
  {
    date_time_temp[i] = 0;
  }
}

//+ Handles the entry of time values
int handle_time_entry(int cursorPos, int *ptime)
{
  /* Time is entered as an arry of 4 integers in the 24 hour format
   as HHMM where HH is the hour and MM is the minute
   
   e.g. 13:45 (1:45 pm) is represented as 1345
    */

  // int currentDigit = time[cursorPos];
  int currentDigit = *(ptime + cursorPos);
  // if the UP switch is pressed
  if (up.rose())
  {
    // switching based on the digit of the value
    switch (cursorPos)
    {
    case 0:
      if (currentDigit < 1)
      {
        currentDigit += 1;
      }
      // cap the second digit within 24 hrs
      // else if (currentDigit < 2 && time[1] <= 3)
      else if (currentDigit < 2 && *(ptime + 1) <= 3)
      {
        currentDigit += 1;
      }
      break;
    case 1:
      if (*(ptime + 0) < 2)
      {
        if (currentDigit < 9)
        {
          currentDigit += 1;
        }
      }
      else if (currentDigit < 3)
      {
        currentDigit += 1;
      }
      break;
    case 2:
      if (currentDigit < 5)
      {
        currentDigit += 1;
      }
      break;
    case 3:
      if (currentDigit < 9)
      {
        currentDigit += 1;
      }
      break;
    default:
      break;
    }
    *(ptime + cursorPos) = currentDigit;
    lcd.setCursor(cursorPos, 1);
    lcd.print(String(currentDigit));
    lcd.setCursor(cursorPos, 1);
    lcd.cursor();
  }
  else if (dn.rose())
  {
    if (currentDigit > 0)
    {
      currentDigit -= 1;
    }
    *(ptime + cursorPos) = currentDigit;
    lcd.setCursor(cursorPos, 1);
    lcd.print(String(currentDigit));
    lcd.setCursor(cursorPos, 1);
    lcd.cursor();
  }
  else if (lt.rose())
  {
    if (cursorPos > 0)
    {
      cursorPos -= 1;
    }
    lcd.setCursor(cursorPos, 1);
    lcd.cursor();
  }
  else if (rt.rose())
  {
    if (cursorPos < 3)
    {
      cursorPos += 1;
    }
    lcd.setCursor(cursorPos, 1);
    lcd.cursor();
  }
  else if (ok.rose())
  {
    // ! this code is not reached
    cursorPos = 0;
    lcd.setCursor(cursorPos, 1);
  }
  return cursorPos;
}

//+ Resets the time arrays at a given index and stores the result in EEPROM and RAM
void reset_time(int index)
{
  String zero = "0000";
  // Setting the live variables
  ON_times_s[index] = zero;
  OFF_times_s[index] = zero;
  active_alarms[index] = false;

  // Setting the EEPROM stored variables
  zero.toCharArray(ee_on[index], 5);
  zero.toCharArray(ee_off[index], 5);
  ee_active[index] = false;

  // Pushing to EEPROM
  EEPROM.put(ee_on_address, ee_on);
  EEPROM.put(ee_off_address, ee_off);
  EEPROM.put(ee_set_address, ee_active);

  reset_temp_time_variables();
}

//+ Handles the entry of voltage values
int handle_volt_entry(int cursorPos, int *pvolt)
{
  /* Voltage is entered as an array of 4 integers in the format
    as XX.Y where XX is the voltage and Y is the hundreds of millivolts

   e.g. 11.5 (11.5V) is represented as 11.5

   NOTE: Only the 0,1,and 3 indices are used and the index 2 is left blank intentionally. 
   
  */

  int periodPos = 2;
  int maxPos = 3;
  int minPos = 0;
  int currentDigit = *(pvolt + cursorPos);

  if (up.rose())
  {
    if (currentDigit < 9 && cursorPos != periodPos)
    {

      currentDigit += 1;
      *(pvolt + cursorPos) = currentDigit;
      lcd.setCursor(cursorPos, 1);
      lcd.print(String(currentDigit));
      lcd.setCursor(cursorPos, 1);
      lcd.cursor();
    }
  }
  if (dn.rose())
  {
    if (currentDigit > 0 && cursorPos != periodPos)
    {
      currentDigit -= 1;
      *(pvolt + cursorPos) = currentDigit;
      lcd.setCursor(cursorPos, 1);
      lcd.print(String(currentDigit));
      lcd.setCursor(cursorPos, 1);
      lcd.cursor();
    }
  }
  if (lt.rose())
  {
    if (cursorPos > minPos)
    {
      cursorPos -= 1;

      // this is done so that the cursor never reaches period index
      if (cursorPos == periodPos)
      {
        cursorPos -= 1;
      }
    }
  }
  if (rt.rose())
  {
    if (cursorPos < maxPos)
    {
      cursorPos += 1;

      // this is done so that the cursor never reaches period index
      if (cursorPos == periodPos)
      {
        cursorPos += 1;
      }
    }
  }
  if (ok.rose())
  {
    cursorPos = 0;
    lcd.setCursor(cursorPos, 1);
    lcd.noCursor();
  }
  return cursorPos;
}

//+ Handles the entry of date time values
int handle_new_date_time_entry(int cursorPos, int *pnewdatetime)
{
  /* New datetime is entered as an array of 16 integers in the format
    as YYYY0MM0DD0HH0MM

   

  // Note: the indices at position 4,7,10 and 13 are left blank intentionally   
  */

  int maxPos = 15;
  int minPos = 0;
  int currentDigit = *(pnewdatetime + cursorPos);

  if (up.rose())
  {
    if (currentDigit < 9 && (cursorPos != 4 || cursorPos != 7 || cursorPos != 10 || cursorPos != 13))
    {
      currentDigit += 1;
      *(pnewdatetime + cursorPos) = currentDigit;
      lcd.setCursor(cursorPos, 1);
      lcd.print(String(currentDigit));
      lcd.setCursor(cursorPos, 1);
      lcd.cursor();
    }
  }
  if (dn.rose())
  {
    if (currentDigit > 0 && (cursorPos != 4 || cursorPos != 7 || cursorPos != 10 || cursorPos != 13))
    {
      currentDigit -= 1;
      *(pnewdatetime + cursorPos) = currentDigit;
      lcd.setCursor(cursorPos, 1);
      lcd.print(String(currentDigit));
      lcd.setCursor(cursorPos, 1);
      lcd.cursor();
    }
  }
  if (lt.rose())
  {
    if (cursorPos > minPos)
    {
      cursorPos -= 1;

      // this is done so that the cursor never reaches period index
      if (cursorPos == 4 || cursorPos == 7 || cursorPos == 10 || cursorPos == 13)
      {
        cursorPos -= 1;
      }
    }
  }
  if (rt.rose())
  {
    if (cursorPos < maxPos)
    {
      cursorPos += 1;

      // this is done so that the cursor never reaches period index
      if (cursorPos == 4 || cursorPos == 7 || cursorPos == 10 || cursorPos == 13)
      {
        cursorPos += 1;
      }
    }
  }
  if (ok.rose())
  {
    cursorPos = 0;
    lcd.setCursor(cursorPos, 1);
    lcd.noCursor();
  }
  return cursorPos;
}

//+ This function returns the measured voltage
int measure_voltage()
{

  int measured_value = 0;
  int sum_of_samples = 0;
  int valid_samples = 0;
  int average_measured_value = 0;

  float calculated_voltage = 0;
  int formatted_voltage = 0;

  float R_in = 3430.0;
  float R_big = 8980.0;

  // averaging the sampled voltages to smooth the sampling
  for (int i = 0; i < 5; i++)
  {
    // read the value from pin
    analogRead(IN_voltage_pin);

    measured_value = analogRead(IN_voltage_pin);

    if (measured_value != 0)
    {
      sum_of_samples += measured_value;
      valid_samples++;
    }
    delayMicroseconds(200);
  }

  // calculating the average sampled value
  average_measured_value = sum_of_samples / valid_samples;

  // calculates the voltage from the measured analog value
  // 5 is for the voltage reference of the arduino and 1023 is the 10-bit sample range
  calculated_voltage = (average_measured_value * 5 * (R_big + R_in)) / (R_in * 1023);

  // multiplies the calculated voltage by 1000 to get the first 3 digits
  formatted_voltage = int(calculated_voltage * 10);

  /* Serial.print("Average measured value:");
  //Serial.println(average_measured_value);

  //Serial.print("Valid :");
  //Serial.println(valid_samples);

  //Serial.print("Calculated voltage:");
  //Serial.println(calculated_voltage);
  //Serial.print("Returned voltage:");
  //Serial.println(formatted_voltage);
  Serial.println("\n\n\n\n\n\n\n"); */

  return formatted_voltage;
}

//+ This function handles whats shown onscreen
// View this code alongside the handle_states function for clarity
void update_menu(int state)
{
  switch (state)
  {

  case 0: // IDLE STATE
    break;

  case 1: // HOME -> TIME_ALARMS_MENU
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F(">Time alarms"));
    lcd.setCursor(0, 1);
    lcd.print(F(" Voltage alarm"));
    analogWrite(OUT_led_pin, 127); // Brighten the display
    break;

  case 2: // HOME -> VOLT_ALARM_MENU
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F(" Time alarms"));
    lcd.setCursor(0, 1);
    lcd.print(F(">Voltage alarm"));
    analogWrite(OUT_led_pin, 127); // Brighten the display
    break;

  case 3: // TIME_ALARMS_MENU -> VIEW_TIME_ALARMS_MENU
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F(">View timer(s)"));
    lcd.setCursor(0, 1);
    lcd.print(F(" Set timer"));
    break;

  case 4: // VOLT_ALARM_MENU -> VIEW_VOLT_ALARM_MENU
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F(">View volt alarm"));
    lcd.setCursor(0, 1);
    lcd.print(F(" Set volt alarm"));
    break;

  case 5: // TIME_ALARMS_MENU -> SET_TIME_ALARMS_MENU
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F(">Set timer"));
    lcd.setCursor(0, 1);
    lcd.print(F(" Delete timer"));
    break;

  case 6: // TIME_ALARMS_MENU -> RESET_TIME_ALARMS_MENU
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F(" Set timer"));
    lcd.setCursor(0, 1);
    lcd.print(F(">Delete timer"));
    break;

  case 7: // TIME_ALARMS_MENU -> VIEW_TIME_ALARMS_MENU -> VIEW_TIME_ALARMS_PROGRAM

    break;

  case 8: // TIME_ALARMS_MENU -> SET_TIME_ALARMS_MENU -> SET_TIME_ALARMS_PROGRAM

    break;

  case 9: // TIME_ALARMS_MENU -> RESET_TIME_ALARMS_MENU -> RESET_TIME_ALARMS_PROGRAM

    break;

  case 10: // VOLT_ALARM_MENU -> SET_VOLT_ALARM_MENU
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F(" View volt alarm"));
    lcd.setCursor(0, 1);
    lcd.print(F(">Set volt alarm"));
    break;

  case 11: // VOLT_ALARM_MENU -> VIEW_VOLT_ALARM_MENU -> VIEW_VOLTAGE_ALARM_PROGRAM
    break;
  case 12: // VOLT_ALARM_MENU -> SET_VOLT_ALARM_MENU -> SET_VOLTAGE_ALARM_PROGRAM
    break;

  case 13: // SET_TIME_MENU
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F(" Voltage alarm"));
    lcd.setCursor(0, 1);
    lcd.print(F(">Set/view time"));
    analogWrite(OUT_led_pin, 127); // Brighten the display
    break;

  case 14: // SET_TIME_MENU -> VIEW_DATETIME_MENU
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F(">View datetime"));
    lcd.setCursor(0, 1);
    lcd.print(F(" Set datetime"));
    break;

  case 15: // SET_TIME_MENU -> SET_DATETIME_MENU
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F(" View datetime"));
    lcd.setCursor(0, 1);
    lcd.print(F(">Set datetime"));
    break;

  default:
    break;
  }
}

/* 



????????????????????????????????????

?FUNCTIONS THAT DISPLAY TO THE USER

????????????????????????????????????
*/

//+ Show the idle screen which is dimmed and has the current time and voltage
void show_idle_screen()
{
  /* 
todo: have 2 seperate states where 1st does init and the 2nd does 
todo: the showing time; to free up CPU time
 */
  //* Setting the global states to their defaults

  if (need_clean)
  {
    view_time_alarm_state = 0;
    set_time_alarm_state = 0;
    reset_time_alarm_state = 0;
    temp_time_alarm_num = 0;

    view_volt_alarm_state = 0;
    set_volt_alarm_state = 0;
    reset_volt_alarm_state = 0;

    view_datetime_state = 0;
    set_datetime_state = 0;

    reset_temp_volt_variables();
    reset_temp_time_variables();
    reset_temp_datetime_variables();

    // reset the alarm number
    al_num = 0;
    lcd.clear();
    lcd.noCursor();

    need_clean = 0;
  }

  DateTime now = rtc.now();
  if (prev_sec != now.second())
  {
    if (now.second() == 0)
    {
      lcd.clear();
    }
    prev_sec = now.second();
    lcd.setCursor(0, 0);
    lcd.print(F(" Time :"));
    lcd.print(F(" "));
    lcd.print(now.hour());
    lcd.print(':');
    lcd.print(now.minute());
    lcd.print(':');
    lcd.print(now.second());
    lcd.print(F("  "));

    voltage = measure_voltage();
    lcd.setCursor(0, 1);
    lcd.print(F(" Voltage :"));
    lcd.print((voltage / 10.0));
    lcd.print(F("V"));
  }
}

// * Time alarms section
//+ Displays and allows the scrolling through all the alarms
void view_time_alarms()
{
  //setting flag to show variables are in memory that need cleaning
  need_clean = 1;

  // only runs once when called from reset
  if (view_time_alarm_state == 0)
  {
    al_num = 0;
    lcd.clear();

    view_time_alarm_state = 1;
  }

  // "View alarms" option selected
  // The first alarm is shown
  if (view_time_alarm_state == 1)
  {

    if (rt.rose() && al_num < int((sizeof(ON_times_s)) / (sizeof(ON_times_s[0])) - 1))
    {
      al_num += 1;

      lcd.clear();
    }
    else if (lt.rose() && al_num > 0)
    {
      al_num -= 1;

      lcd.clear();
    }

    lcd.setCursor(0, 0);
    lcd.print(F("ON:"));
    lcd.print(ON_times_s[al_num]);
    lcd.print(F(" "));
    lcd.print(F("OFF:"));
    lcd.print(OFF_times_s[al_num]);
    lcd.setCursor(0, 1);
    lcd.print(F("<-("));
    lcd.print(al_num + 1);
    lcd.print(F(")->"));
  }
}

//+ Set a value into the times arrays
void set_time_alarm()
{
  //setting flag to show variables are in memory that need cleaning
  need_clean = 1;

  // only runs once when called from reset
  if (set_time_alarm_state == 0)
  {
    al_num = 0;

    for (size_t i = 0; i < ((sizeof(time_on_temp)) / (sizeof(time_on_temp[0])) - 1); i++)
    {
      time_on_temp[i] = 0;
      time_off_temp[i] = 0;
    }

    // resetting the temporary time variables
    time_on_temp_s = "0000";
    time_off_temp_s = "0000";

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Select alarm: "));
    lcd.setCursor(0, 1);
    lcd.print(F("Alarm "));
    lcd.print(al_num + 1);

    set_time_alarm_state = 4;
  }

  // This is the select alarm screen
  if (set_time_alarm_state == 4)
  {
    // increment/decrement the selected alarm number
    if (rt.rose() && al_num < int((sizeof(ON_times_s)) / (sizeof(ON_times_s[0])) - 1))
    {
      al_num += 1;
      lcd.clear();

      lcd.setCursor(0, 0);
      lcd.print(F("Select alarm: "));
      lcd.setCursor(0, 1);
      lcd.print(F("Alarm "));
      lcd.print(al_num + 1);
    }
    else if (lt.rose() && al_num > 0)
    {
      al_num -= 1;
      lcd.clear();

      lcd.setCursor(0, 0);
      lcd.print(F("Select alarm: "));
      lcd.setCursor(0, 1);
      lcd.print(F("Alarm "));
      lcd.print(al_num + 1);
    }
    else if (ok.rose())
    {
      temp_time_alarm_num = al_num;
      set_time_alarm_state = 5;
    }
  }

  // Debouncing the OK button
  if (set_time_alarm_state == 5)
  {
    if (ok.fell())
    {
      lcd.clear();

      set_time_alarm_state = 6;
    }
  }

  // This is the edit ON alarm screen
  if (set_time_alarm_state == 6)
  {
    lcd.setCursor(0, 0);
    lcd.print(F("Edit "));
    lcd.print(temp_time_alarm_num + 1);
    lcd.print(F(" ON time"));
    lcd.setCursor(0, 1);
    lcd.print(ON_times_s[temp_time_alarm_num]);

    // if a button is pressed then display the cursor and go to another state
    if (up.rose() || dn.rose() || lt.rose() || rt.rose())
    {
      lcd.setCursor(cursorPos, 1);
      lcd.cursor();

      int temp_time = ON_times_s[temp_time_alarm_num].toInt();
      //Serial.println(temp_time);

      int temp_d1 = temp_time / 1000;
      time_on_temp[0] = temp_d1;

      int temp_d2 = (temp_time - temp_d1 * 1000) / 100;
      time_on_temp[1] = temp_d2;

      int temp_d3 = (temp_time - temp_d1 * 1000 - temp_d2 * 100) / 10;
      time_on_temp[2] = temp_d3;

      int temp_d4 = (temp_time - temp_d1 * 1000 - temp_d2 * 100) % 10;
      time_on_temp[3] = temp_d4;

      set_time_alarm_state = 7;
    }

    // this is if the time is already the correct time
    else if (ok.rose())
    {
      time_on_temp_s = ON_times_s[temp_time_alarm_num];
      set_time_alarm_state = 8;
    }
  }

  // The time entry part
  if (set_time_alarm_state == 7)
  {

    lcd.setCursor(cursorPos, 1);
    if (up.rose() || dn.rose() || lt.rose() || rt.rose())
    {
      cursorPos = handle_time_entry(cursorPos, ptimeon);
    }
    else if (ok.rose())
    {
      // Saving the chosen time to a temporary variable
      time_on_temp_s = String(*(ptimeon)) + String(*(ptimeon + 1)) + String(*(ptimeon + 2)) + String(*(ptimeon + 3));
      set_time_alarm_state = 8;
    }
  }

  // Debouncing the OK button
  if (set_time_alarm_state == 8)
  {
    if (ok.fell())
    {
      lcd.clear();
      lcd.noCursor();
      cursorPos = 0;
      set_time_alarm_state = 9;
    }
  }

  // This is the edit OFF alarm screen
  if (set_time_alarm_state == 9)
  {
    lcd.setCursor(0, 0);
    lcd.print(F("Edit "));
    lcd.print(temp_time_alarm_num + 1);
    lcd.print(F(" OFF time"));
    lcd.setCursor(0, 1);
    lcd.print(OFF_times_s[temp_time_alarm_num]);

    if (up.rose() || dn.rose() || lt.rose() || rt.rose())
    {
      lcd.setCursor(cursorPos, 1);
      lcd.cursor();

      int temp_time = OFF_times_s[temp_time_alarm_num].toInt();
      //Serial.println(temp_time);

      int temp_d1 = temp_time / 1000;
      time_off_temp[0] = temp_d1;

      int temp_d2 = (temp_time - temp_d1 * 1000) / 100;
      time_off_temp[1] = temp_d2;

      int temp_d3 = (temp_time - temp_d1 * 1000 - temp_d2 * 100) / 10;
      time_off_temp[2] = temp_d3;

      int temp_d4 = (temp_time - temp_d1 * 1000 - temp_d2 * 100) % 10;
      time_off_temp[3] = temp_d4;

      set_time_alarm_state = 10;
    }

    // this is if the time is already the correct time
    else if (ok.rose())
    {
      time_off_temp_s = OFF_times_s[temp_time_alarm_num];
      set_time_alarm_state = 11;
    }
  }

  // The time entry part
  if (set_time_alarm_state == 10)
  {

    lcd.setCursor(cursorPos, 1);
    if (up.rose() || dn.rose() || lt.rose() || rt.rose())
    {
      cursorPos = handle_time_entry(cursorPos, ptimeoff);
    }
    else if (ok.rose())
    {
      // Saving the chosen time to a temporary variable
      time_off_temp_s = String(*(ptimeoff)) + String(*(ptimeoff + 1)) + String(*(ptimeoff + 2)) + String(*(ptimeoff + 3));
      set_time_alarm_state = 11;
    }
  }

  // Debouncing the OK button
  if (set_time_alarm_state == 11)
  {
    if (ok.fell())
    {
      lcd.clear();
      lcd.noCursor();
      cursorPos = 0;
      set_time_alarm_state = 12;
    }
  }

  // Confirmation screen for the times
  if (set_time_alarm_state == 12)
  {
    lcd.setCursor(0, 0);
    lcd.print(F("ON "));
    lcd.print(time_on_temp_s);
    lcd.print(F(" OFF "));
    lcd.print(time_off_temp_s);
    lcd.setCursor(0, 1);
    lcd.print(F(" Confirm?"));

    // saving the times to memory if ok
    if (ok.rose())
    {
      // Setting the live variables
      ON_times_s[temp_time_alarm_num] = time_on_temp_s;
      OFF_times_s[temp_time_alarm_num] = time_off_temp_s;
      active_alarms[temp_time_alarm_num] = true;

      // Setting the EEPROM stored variables
      time_on_temp_s.toCharArray(ee_on[temp_time_alarm_num], 5);
      time_off_temp_s.toCharArray(ee_off[temp_time_alarm_num], 5);
      ee_active[temp_time_alarm_num] = true;

      //! Saving the time to EEPROM
      // Pushing to EEPROM
      EEPROM.put(ee_on_address, ee_on);
      EEPROM.put(ee_off_address, ee_off);
      EEPROM.put(ee_set_address, ee_active);

      //Serial.print("Saved ");
      //Serial.print(time_on_temp_s);
      //Serial.print(" to EEPROM\n");

      //Serial.print("Saved ");
      //Serial.print(time_off_temp_s);
      //Serial.print(" to EEPROM\n");

      //Serial.print("Saved alarm ");
      //Serial.print(temp_time_alarm_num + 1);
      //Serial.print(" state to EEPROM\n");

      reset_temp_time_variables();
      // switching to the next state
      set_time_alarm_state = 13;
    }
  }

  // Debouncing the OK button
  if (set_time_alarm_state == 13)
  {
    if (ok.fell())
    {
      lcd.clear();
      lcd.noCursor();
      cursorPos = 0;
      lcd.setCursor(0, 0);
      lcd.print("Alarm saved");
      lcd.setCursor(0, 1);
      lcd.print("Going to idle");
    }
  }
}

//+ Resets a time alarm and deactivates it from the active time alarms array in EEPROM and RAM
void reset_time_alarm()
{
  //setting flag to show variables are in memory that need cleaning
  need_clean = 1;
  // only runs once when called from reset
  if (reset_time_alarm_state == 0)
  {
    al_num = 0;
    lcd.clear();

    reset_time_alarm_state = 1;
  }

  // "View alarms" option selected
  // The first alarm is shown
  if (reset_time_alarm_state == 1)
  {

    if (rt.rose() && al_num < int((sizeof(ON_times_s)) / (sizeof(ON_times_s[0])) - 1))
    {
      al_num += 1;

      lcd.clear();
    }
    else if (lt.rose() && al_num > 0)
    {
      al_num -= 1;

      lcd.clear();
    }
    else if (ok.rose())
    {
      lcd.clear();
      lcd.setCursor(0, 0);

      reset_time_alarm_state = 2;
    }

    lcd.setCursor(0, 0);
    lcd.print(F("ON:"));
    lcd.print(ON_times_s[al_num]);
    lcd.print(F(" "));
    lcd.print(F("OFF:"));
    lcd.print(OFF_times_s[al_num]);
    lcd.setCursor(0, 1);
    lcd.print(F("<-("));
    lcd.print(al_num + 1);
    lcd.print(F(")->"));
  }
  if (reset_time_alarm_state == 2)
  {
    lcd.setCursor(0, 0);
    lcd.print("Del alarm ");
    lcd.print(al_num + 1);
    lcd.print("?");
    lcd.setCursor(0, 1);
    lcd.print("Press OK to Del");

    if (ok.rose())
    {
      lcd.clear();

      reset_time_alarm_state = 3;

      // handle the delete of the time alarm
      reset_time(al_num);
    }
  }
  if (reset_time_alarm_state == 3)
  {
    if (ok.fell())
    {
      lcd.setCursor(0, 0);
      lcd.print("Alarm ");
      lcd.print(al_num + 1);
      lcd.print(" del");
      lcd.setCursor(0, 1);
      lcd.print("Going to idle");
    }
  }
}

//+ Checks whether a time alarm is triggered and handles the output
//needs to be delayed after running to prevent flickering of relay
void handle_time_alarms()
{

  // defining the time
  DateTime now = rtc.now();

  // dividing the size of the whole array by the size of an element to find the
  // number of elements
  int numTimers = sizeof(active_alarms) / sizeof(active_alarms[0]);

  if (now.second() == 0)
  {
    for (int i = 0; i < numTimers; i++)
    {
      // checking if the alarm is set before proceeding
      if (active_alarms[i])
      {
        // time is of the format 24 hr format
        int now_int = (now.hour() * 100) + (now.minute());

        // checking the ON time alarms
        if (ON_times_s[i].toInt() == now_int)
        {
          // SWITCH ON THE RELAY
          digitalWrite(OUT_relay_pin, HIGH);

          //Serial.print("Alarm ");
          //Serial.print(i + 1);
          //Serial.print(" activated\n");
        }
        else if (OFF_times_s[i].toInt() == now_int)
        {

          // SWITCH OFF THE RELAY
          digitalWrite(OUT_relay_pin, LOW);

          //Serial.print("Alarm ");
          //Serial.print(i + 1);
          //Serial.print(" deactivated\n");
        }
      }
    }
  }
}

// * Voltage alarms section
//+ Display the voltages at which the relay will be triggered ON and OFF
void view_volt_alarm()
{
  //setting flag to show variables are in memory that need cleaning
  need_clean = 1;

  // Clears the display on entry and prepares the display
  if (view_volt_alarm_state == 0)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.noCursor();
    view_volt_alarm_state = 1;
  }

  //shows the voltage parameters
  if (view_volt_alarm_state == 1)
  {
    lcd.setCursor(0, 0);
    lcd.print(F("ON "));
    lcd.print(ON_volt_s);
    lcd.print(F(" OFF "));
    lcd.print(OFF_volt_s);
    lcd.setCursor(0, 1);
    lcd.print(F("Voltage now:"));
    lcd.print(voltage / 10.0);

    if (bc.rose())
    {
      //TODO: return to the earlier menu
    }
  }
}

//+ Sets a value for the voltage variable
void set_voltage_alarm()
{
  //setting flag to show variables are in memory that need cleaning
  need_clean = 1;

  // only runs once when called from reset
  if (set_volt_alarm_state == 0)
  {
    lcd.clear();
    set_volt_alarm_state = 1;
  }

  // Showing the voltage alarm menu
  if (set_volt_alarm_state == 1)
  {
    lcd.setCursor(0, 0);
    lcd.print(F("Press OK to continue"));

    // clearing the temporary integer voltage values
    for (size_t i = 0; i < ((sizeof(volt_on_temp)) / (sizeof(volt_on_temp[0])) - 1); i++)
    {
      volt_on_temp[i] = 0;
      volt_off_temp[i] = 0;
    }

    // resetting the temporary voltage variables
    volt_on_temp_s = "00.0";
    volt_off_temp_s = "00.0";

    // state to show that the menu is displayed
    if (ok.rose())
    {
      set_volt_alarm_state = 2;
    }
  }

  // Debouncing the OK button
  if (set_volt_alarm_state == 2)
  {
    if (ok.fell())
    {
      lcd.clear();
      set_volt_alarm_state = 6;
    }
  }

  // This is the edit ON alarm screen
  if (set_volt_alarm_state == 6)
  {
    lcd.setCursor(0, 0);
    lcd.print(F("Edit  ON volt"));
    lcd.setCursor(0, 1);
    lcd.print(ON_volt_s);

    volt_on_temp[0] = ON_volt_s.substring(0, 1).toInt();
    volt_on_temp[1] = ON_volt_s.substring(1, 2).toInt();
    volt_on_temp[3] = ON_volt_s.substring(3).toInt();

    // if a button is pressed then display the cursor and go to another state
    if (up.rose() || dn.rose() || lt.rose() || rt.rose())
    {
      lcd.setCursor(cursorPos, 1);
      lcd.cursor();

      set_volt_alarm_state = 7;
    }

    // this is if the voltage is already the correct voltage
    else if (ok.rose())
    {
      volt_on_temp_s = String(volt_on_temp[0]) + String(volt_on_temp[1]) + String(".") + String(volt_on_temp[3]);
      set_volt_alarm_state = 8;
    }
    else if (bc.rose())
    {
      lcd.clear();

      set_volt_alarm_state = 14;
    }
  }

  // The voltage entry part
  if (set_volt_alarm_state == 7)
  {
    lcd.setCursor(cursorPos, 1);

    if (up.rose() || dn.rose() || lt.rose() || rt.rose())
    {
      cursorPos = handle_volt_entry(cursorPos, pvolton);
    }
    else if (ok.rose())
    {
      // Saving the chosen voltage to a temporary variable
      volt_on_temp_s = String(volt_on_temp[0]) + String(volt_on_temp[1]) + String(".") + String(volt_on_temp[3]);
      set_volt_alarm_state = 8;
    }
    else if (bc.rose())
    {
      lcd.clear();

      set_volt_alarm_state = 14;
    }
  }

  // Debouncing the OK button
  if (set_volt_alarm_state == 8)
  {
    if (ok.fell())
    {
      lcd.clear();
      lcd.noCursor();
      cursorPos = 0;
      set_volt_alarm_state = 9;
    }
  }

  // This is the edit OFF alarm screen
  if (set_volt_alarm_state == 9)
  {
    lcd.setCursor(0, 0);
    lcd.print(F("Edit OFF volt"));
    lcd.setCursor(0, 1);
    lcd.print(OFF_volt_s);

    volt_off_temp[0] = OFF_volt_s.substring(0, 1).toInt();
    volt_off_temp[1] = OFF_volt_s.substring(1, 2).toInt();
    volt_off_temp[3] = OFF_volt_s.substring(3).toInt();

    if (up.rose() || dn.rose() || lt.rose() || rt.rose())
    {
      lcd.setCursor(cursorPos, 1);
      lcd.cursor();

      set_volt_alarm_state = 10;
    }

    // this is if the time is already the correct time
    else if (ok.rose())
    {
      volt_off_temp_s = String(volt_off_temp[0]) + String(volt_off_temp[1]) + String(".") + String(volt_off_temp[3]);

      set_volt_alarm_state = 11;
    }
    else if (bc.rose())
    {
      lcd.clear();

      set_volt_alarm_state = 14;
    }
  }

  // The time entry part
  if (set_volt_alarm_state == 10)
  {

    lcd.setCursor(cursorPos, 1);
    if (up.rose() || dn.rose() || lt.rose() || rt.rose())
    {
      cursorPos = handle_volt_entry(cursorPos, pvoltoff);
    }
    else if (ok.rose())
    {
      // Saving the chosen time to a temporary variable
      volt_off_temp_s = String(volt_off_temp[0]) + String(volt_off_temp[1]) + String(".") + String(volt_off_temp[3]);
      set_volt_alarm_state = 11;
    }
    else if (bc.rose())
    {
      lcd.clear();

      set_volt_alarm_state = 14;
    }
  }

  // Debouncing the OK button
  if (set_volt_alarm_state == 11)
  {
    if (ok.fell())
    {
      lcd.clear();
      lcd.noCursor();
      cursorPos = 0;
      set_volt_alarm_state = 12;
    }
  }

  // Confirmation screen for the voltages
  if (set_volt_alarm_state == 12)
  {
    lcd.setCursor(0, 0);
    lcd.print(F("ON "));
    lcd.print(volt_on_temp_s);
    lcd.print(F(" OFF "));
    lcd.print(volt_off_temp_s);
    lcd.setCursor(0, 1);
    lcd.print(F(" Confirm?"));

    // saving the times to memory if ok
    if (ok.rose())
    {

      // Setting the live variables
      ON_volt_s = volt_on_temp_s;
      OFF_volt_s = volt_off_temp_s;

      // ON_volt = volt_on_temp_s.toInt() * 10;
      // OFF_volt = volt_off_temp_s.toInt() * 10;
      ON_volt = int(volt_on_temp_s.toFloat() * 10);

      OFF_volt = int(volt_off_temp_s.toFloat() * 10);

      //Serial.println(ON_volt);
      //Serial.println(OFF_volt);

      // Setting the local persistant stored variables
      for (int i; i < 4; i++)
      {
        ee_volts_on[i] = volt_on_temp[i];
        ee_volts_off[i] = volt_off_temp[i];
      }

      ee_volts_active = true;

      //! Saving the time to EEPROM
      // Pushing to EEPROM

      EEPROM.put(ee_volts_on_address, ee_volts_on);
      EEPROM.put(ee_volts_off_address, ee_volts_off);
      EEPROM.put(ee_volts_set_address, ee_volts_active);

      //Serial.print("Saved ");
      //Serial.print(volt_on_temp_s);
      //Serial.print(" to EEPROM\n");

      //Serial.print("Saved ");
      //Serial.print(volt_off_temp_s);
      //Serial.print(" to EEPROM\n");

      //Serial.println("Saved voltage alarm to EEPROM");

      reset_temp_volt_variables();

      // switching to the next state
      set_volt_alarm_state = 13;
    }
  }

  // Debouncing the OK button
  if (set_volt_alarm_state == 13)
  {

    if (ok.fell())
    {
      lcd.clear();
      lcd.noCursor();
      cursorPos = 0;
      lcd.setCursor(0, 0);
      lcd.print("New values set");
      lcd.setCursor(0, 1);
      lcd.print("Going to idle");
    }
  }

  // Debouncing the OK button
  if (set_volt_alarm_state == 14)
  {
    if (bc.fell())
    {
      lcd.setCursor(0, 0);
      lcd.print("No values set");
      lcd.setCursor(0, 1);
      lcd.print("Going to idle");
      reset_temp_volt_variables();
    }
  }
}

//+ Checks whether a voltage alarm is triggered and handles the output
//needs to be delayed after running to prevent flickering of relay
void handle_volt_alarm(int volt_measured)
{
  //(to charge the battery))
  if (volt_measured <= ON_volt)
  {
    //Close the relay contacts to charge the battery
    digitalWrite(OUT_relay_pin, HIGH);
  }
  //(battery has finished charging)
  else if (volt_measured >= OFF_volt)
  {
    //Open the relay contacts to stop charging the battery
    digitalWrite(OUT_relay_pin, LOW);
  }
  else if (ON_volt < volt_measured && volt_measured < OFF_volt)
  {
    // Close the relay to charge the battery
    // do nothing
  }
  // Serial.println(ON_volt);
  // Serial.println(OFF_volt);
}

// * Datetime section
//+ Shows the current date and time
void view_datetime()
{
  //setting flag to show variables are in memory that need cleaning
  need_clean = 1;

  if (view_datetime_state == 0)
  {
    lcd.clear();
    view_datetime_state = 1;
  }
  if (view_datetime_state == 1)
  {
    DateTime now = rtc.now();

    lcd.setCursor(0, 0);
    lcd.print(F("Date:"));
    lcd.print(now.day());
    lcd.print(F("/"));
    lcd.print(now.month());
    lcd.print(F("/"));
    lcd.print(now.year());

    lcd.setCursor(0, 1);
    lcd.print(F("Time:"));
    lcd.print(now.hour());
    lcd.print(F(":"));
    lcd.print(now.minute());
    lcd.print(F(":"));
    lcd.print(now.second());
  }
}

//+ Allows the user to set the date and time
void set_datetime()
{
  // Setting flag to show variables are in memory that need cleaning
  need_clean = 1;

  // Clears the screen on entry
  if (set_datetime_state == 0)
  {
    lcd.clear();
    set_datetime_state = 1;
  }

  // Write the current date to LCD and then switch to input
  if (set_datetime_state == 1)
  {
    // obtains the time as of running
    DateTime now = rtc.now();
    int year = now.year();
    int month = now.month();
    int day = now.day();
    int hour = now.hour();
    int minute = now.minute();

    // Displays user prompt
    lcd.setCursor(0, 0);
    lcd.print(F("Set DATE & TIME"));

    // Prints the date in the format DD/MM/YYYY
    lcd.setCursor(0, 1);

    //the year will always be printed as in the array (2000-2099)
    lcd.print(year);

    //
    lcd.print(F("/"));

    //printing the month as in the array
    if (month < 10)
    {
      lcd.print("0");
      lcd.print(month);
    }
    else
    {
      lcd.print(month);
    }

    //
    lcd.print(F("/"));

    //printing the day as in the array
    if (day < 10)
    {
      lcd.print("0");
      lcd.print(day);
    }
    else
    {
      lcd.print(day);
    }

    //
    lcd.print(F(" "));

    //printing the hours as in the array
    if (hour < 10)
    {
      lcd.print("0");
      lcd.print(hour);
    }
    else
    {
      lcd.print(hour);
    }

    //
    lcd.print(F(":"));

    //printing the minutes as in the array
    if (minute < 10)
    {
      lcd.print("0");
      lcd.print(minute);
    }
    else
    {
      lcd.print(minute);
    }

    // Linking the temporary variables to the displayed values
    //year
    date_time_temp[0] = year / 1000;
    date_time_temp[1] = (year / 100) - date_time_temp[0] * 10;
    date_time_temp[2] = (year / 10) - date_time_temp[0] * 100 - date_time_temp[1] * 10;
    date_time_temp[3] = year % 10;
    //month
    date_time_temp[5] = month / 10;
    date_time_temp[6] = month % 10;
    //day
    date_time_temp[8] = day / 10;
    date_time_temp[9] = day % 10;
    //hour
    date_time_temp[11] = hour / 10;
    date_time_temp[12] = hour % 10;
    //minute
    date_time_temp[14] = minute / 10;
    date_time_temp[15] = minute % 10;

    // Sets the cursor to the bottom row and switches on the cursor
    lcd.setCursor(0, 1);
    lcd.cursor();

    // Immediately switch state to listen to input
    set_datetime_state = 2;
  }

  //
  if (set_datetime_state == 2)
  {
    lcd.setCursor(cursorPos, 1);

    if (up.rose() || dn.rose() || lt.rose() || rt.rose())
    {

      cursorPos = handle_new_date_time_entry(cursorPos, pdatetime_temp);
    }
    else if (ok.rose())
    {
      newyear = date_time_temp[0] * 1000 + date_time_temp[1] * 100 + date_time_temp[2] * 10 + date_time_temp[3];
      newmonth = date_time_temp[5] * 10 + date_time_temp[6];
      newday = date_time_temp[8] * 10 + date_time_temp[9];
      newhour = date_time_temp[11] * 10 + date_time_temp[12];
      newminute = date_time_temp[14] * 10 + date_time_temp[15];

      /* Serial.print("New year:");
      Serial.println(newyear);
      Serial.print("New month:");
      Serial.println(newmonth);
      Serial.print("New day:");
      Serial.println(newday);
      Serial.print("New hour:");
      Serial.println(newhour);
      Serial.print("New minute:");
      Serial.println(newminute); */

      DateTime newDate = DateTime(newyear, newmonth, newday, newhour, newminute);
      if (newDate.isValid())
      {
        rtc.adjust(newDate);
        set_datetime_state = 3;
      }
      else
      {
        // switch to error state
        set_datetime_state = 5;
      }
    }
    else if (bc.rose())
    {
      state = 3;
      return;
    }
  }
  if (set_datetime_state == 3)
  {
    lcd.clear();
    lcd.noCursor();
    cursorPos = 0;
    lcd.setCursor(0, 0);
    lcd.print(F("New time set"));
    lcd.setCursor(0, 1);
    lcd.print(F("Going to idle"));
    cursorPos = 0;

    set_datetime_state = 4;
  }
  // empty state to stay in until it goes to idle
  if (set_datetime_state == 4)
  {
  }

  if (set_datetime_state == 5)
  {
    if (ok.fell())
    {
      lcd.clear();
      lcd.noCursor();
      cursorPos = 0;
      lcd.setCursor(0, 0);
      lcd.print(F("Wrong datetime"));
      lcd.setCursor(0, 1);
      lcd.print(F("Press OK "));
      cursorPos = 0;
      set_datetime_state = 6;
    }
  }

  if (set_datetime_state == 6)
  {

    if (ok.rose())
    {
      set_datetime_state = 0;
    }
  }
}

// * The main loop program
//+ THE MAIN PROGRAM: A finite state machine that handles states and transitions
// View this code alongside the update_menu function for clarity
int handle_states(int curr)
{

  int next_state = curr;

  switch (curr)
  {

  // IDLE STATE
  case 0:

    // shows the idle screen
    show_idle_screen();

    // listens to an input and switcHes state accordingly
    next_state = handle_button_inputs(1, 1, 1, 1, 1, 1, curr);

    if (next_state != curr)
      update_menu(next_state);

    break;

    // HOME -> TIME_ALARMS_MENU
  case 1:
    //next_state = handle_inputs_to_states(2, 2, curr, curr, 3, curr);
    next_state = handle_button_inputs(curr, 2, curr, curr, 3, curr, curr);

    if (next_state != curr)
      update_menu(next_state);

    break;

  // HOME -> VOLT_ALARM_MENU
  case 2:
    //next_state = handle_inputs_to_states(1, 1, curr, curr, 4, curr);
    next_state = handle_button_inputs(1, 13, curr, curr, 4, curr, curr);

    if (next_state != curr)
      update_menu(next_state);

    break;

    // TIME_ALARMS_MENU -> VIEW_TIME_ALARMS_MENU
  case 3:
    //next_state = handle_inputs_to_states(curr, 5, curr, curr, 7, curr);
    next_state = handle_button_inputs(curr, 5, curr, curr, 7, 1, curr);

    if (next_state != curr)
      update_menu(next_state);

    break;

  // VOLT_ALARM_MENU -> VIEW_VOLT_ALARM_MENU
  case 4:

    next_state = handle_button_inputs(curr, 10, curr, curr, 11, 2, curr);

    if (next_state != curr)
      update_menu(next_state);

    break;

  // TIME_ALARMS_MENU -> SET_TIME_ALARMS_MENU
  case 5:

    //next_state = handle_inputs_to_states(3, 6, curr, curr, 8, curr);
    next_state = handle_button_inputs(3, 6, curr, curr, 8, 1, curr);

    if (next_state != curr)
      update_menu(next_state);

    break;

  // TIME_ALARMS_MENU -> RESET_TIME_ALARMS_MENU
  case 6:

    //next_state = handle_inputs_to_states(5, curr, curr, curr, curr, curr);
    next_state = handle_button_inputs(5, curr, curr, curr, 9, 1, curr);

    if (next_state != curr)
      update_menu(next_state);

    break;

  // TIME_ALARMS_MENU -> VIEW_TIME_ALARMS_MENU -> VIEW_TIME_ALARMS_PROGRAM
  case 7:
    view_time_alarms();

    if (next_state != curr)
      update_menu(next_state);

    break;

  // TIME_ALARMS_MENU -> SET_TIME_ALARMS_MENU -> SET_TIME_ALARMS_PROGRAM
  case 8:

    set_time_alarm();

    if (next_state != curr)
      update_menu(next_state);

    break;

  // TIME_ALARMS_MENU -> RESET_TIME_ALARMS_MENU -> RESET_TIME_ALARMS_PROGRAM
  case 9:

    reset_time_alarm();

    if (next_state != curr)
      update_menu(next_state);

    break;

  // VOLT_ALARM_MENU -> SET_VOLT_ALARM_MENU
  case 10:

    next_state = handle_button_inputs(4, curr, curr, curr, 12, 2, curr);

    if (next_state != curr)
      update_menu(next_state);

    break;

  // VOLT_ALARM_MENU -> VIEW_VOLT_ALARM_MENU -> VIEW_VOLTAGE_ALARM_PROGRAM
  case 11:

    view_volt_alarm();

    if (next_state != curr)
      update_menu(next_state);

    break;
  // VOLT_ALARM_MENU -> SET_VOLT_ALARM_MENU -> SET_VOLTAGE_ALARM_PROGRAM
  case 12:

    set_voltage_alarm();

    if (next_state != curr)
      update_menu(next_state);

    break;
  // SET_NEW_TIME_MENU
  case 13:
    next_state = handle_button_inputs(2, curr, curr, curr, 14, 2, curr);

    if (next_state != curr)
      update_menu(next_state);

    break;

  // SET_TIME_MENU -> VIEW_DATETIME_MENU
  case 14:
    next_state = handle_button_inputs(curr, 15, curr, curr, 16, 13, curr);

    if (next_state != curr)
      update_menu(next_state);

    break;

    // SET_TIME_MENU -> SET_DATETIME_MENU
  case 15:
    next_state = handle_button_inputs(14, curr, curr, curr, 17, 13, curr);

    if (next_state != curr)
      update_menu(next_state);

    break;
  case 16:

    // run the view_datetime function
    view_datetime();

    break;
  case 17:

    //run the set_datetime function
    set_datetime();

    break;

  default:
    break;
  }

  return next_state;
}

/*




?????????????????????????

?Arduino code starts here

?????????????????????????
*/
void setup()
{

  // put your setup code here, to run once:
  Serial.begin(9600);

  lcd.begin(16, 2);
  lcd.clear();

  // Dimming the LCD display
  analogWrite(OUT_led_pin, 10);

  // Defining the RELAY pin
  pinMode(OUT_relay_pin, OUTPUT);

  pinMode(IN_voltage_pin, INPUT);

  // Attaching the debounce objects to their pins.
  up.attach(IN_up_btn_pin, INPUT);
  dn.attach(IN_down_btn_pin, INPUT);
  lt.attach(IN_left_btn_pin, INPUT);
  rt.attach(IN_right_btn_pin, INPUT);
  ok.attach(IN_sel_btn_pin, INPUT);
  bc.attach(IN_back_btn_pin, INPUT);

  // Setting up the intervals for the debouncing.
  up.interval(50);
  dn.interval(50);
  lt.interval(50);
  rt.interval(50);
  ok.interval(50);
  bc.interval(50);

  /*   // only comment these out when initialising a device
  EEPROM.put(ee_on_address, ee_on);
  EEPROM.put(ee_off_address, ee_off);
  EEPROM.put(ee_set_address, ee_active);

  EEPROM.put(ee_volts_on_address, ee_volts_on);
  EEPROM.put(ee_volts_off_address, ee_volts_off);
  EEPROM.put(ee_volts_active, ee_volts_active);
  */

  // reading time eeprom values during startup
  char read_ee_on[10][5];
  char read_ee_off[10][5];
  bool read_active[10];

  EEPROM.get(ee_on_address, read_ee_on);
  EEPROM.get(ee_off_address, read_ee_off);
  EEPROM.get(ee_set_address, read_active);

  for (size_t i = 0; i < 10; i++)
  {
    ON_times_s[i] = String(read_ee_on[i]);
    OFF_times_s[i] = String(read_ee_off[i]);
    active_alarms[i] = read_active[i];
  }

  // reading voltage eeprom values during startup

  int read_ee_volts_on[4];
  int read_ee_volts_off[4];
  bool read_volts_active;

  EEPROM.get(ee_volts_on_address, read_ee_volts_on);
  EEPROM.get(ee_volts_off_address, read_ee_volts_off);
  EEPROM.get(ee_volts_set_address, read_volts_active);

  ON_volt_s = String(read_ee_volts_on[0]) + String(read_ee_volts_on[1]) + String(".") + String(read_ee_volts_on[3]);
  OFF_volt_s = String(read_ee_volts_off[0]) + String(read_ee_volts_off[1]) + String(".") + String(read_ee_volts_off[3]);
  ON_volt = read_ee_volts_on[0] * 100 + read_ee_volts_on[1] * 10 + read_ee_volts_on[3];
  OFF_volt = read_ee_volts_off[0] * 100 + read_ee_volts_off[1] * 10 + read_ee_volts_off[3];
  volt_active = read_volts_active;

  if (!rtc.begin())
  {
    Serial.println(F("Couldn't find RTC"));
    Serial.flush();
    abort();
  }

  if (!rtc.isrunning())
  {
    Serial.println(F("RTC is NOT running, let's set the time!"));
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}

void loop()
{
  DateTime now = rtc.now();

  // Button states are refreshed
  up.update();
  dn.update();
  lt.update();
  rt.update();
  ok.update();
  bc.update();

  // checks whether time alarms have been activated every minute
  if (now.second() == 0)
  {

    handle_time_alarms();

    // delay to prevent flickering of relay
    delay(250);
  }
  // checks the voltage every 5 seconds
  else if (now.second() % 5 == 0)
  {
    // store the measured voltage into the global voltage variable
    voltage = measure_voltage();

    // use the global voltage to decide what to do to the relay
    handle_volt_alarm(voltage);

    // delayed to prevent flickering of relay
    delay(100);
  }

  bool go_to_sleep = ((up.currentDuration() > T_SLEEP) && (dn.currentDuration() > T_SLEEP) && (lt.currentDuration() > T_SLEEP) && (rt.currentDuration() > T_SLEEP) && (ok.currentDuration() > T_SLEEP) && (bc.currentDuration() > T_SLEEP));

  // Switches the arduino to the low power state if no button inputs have changed for T_SLEEP milliseconds
  if (go_to_sleep)
  {
    // Dims the display
    analogWrite(OUT_led_pin, 10);

    // Shows the idle screen
    state = 0;
  }
  else
  {
    // Brigtens the display
    analogWrite(OUT_led_pin, 127);
  }

  state = handle_states(state);
}

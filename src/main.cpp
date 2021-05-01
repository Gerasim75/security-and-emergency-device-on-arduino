/////////////////////////// Security and emergency GSM alarm system with SMS-controlled relays ////////////////////////////////

#include  <SoftwareSerial.h>                                
#include  <EEPROM.h> 
#include  <CyberLib.h> 
#include  "sensor.h"
#include  "function.h"

//#define _SS_MAX_RX_BUFF 128                             // adjust the RX buffer size SoftwareSerial
                        
#define ZONE_1 8          // input                 
#define ZONE_2 9          // input                     
#define ZONE_3 10         // input                          
#define ZONE_4 11         // input fire hazard
#define ZONE_5 12         // input gas leak
#define ZONE_6 19         // input water leak
#define KEYBOARD_PIN 18   // input keyboard 
#define VOLT_PIN 17       // input electrical network
#define BUZZER_PIN 6      // on the board
#define BUTTON_PIN 16     // on the board                                    
                   
SoftwareSerial SIM800(14, 15);                                  // RX, TX (pins for connecting the SIM module) 

String operator_code = "";                                      // Variable of the sim operator code
String _response     =  "";                                     // Variable for storing the SIM module response
String phones[5] = {"","","","",""};                            // Admin phone number
String temp = "";
int AT_counter = 1;
int balance_send;
unsigned long timer_ring = millis();                            // Initialize the timer for the incoming call
unsigned long timer_balance = millis();                         // Initialize the timer to send notifications about the balance on the SIM card
unsigned long timer_alarm_time = millis();                      // Initialize the siren timer
unsigned long timer_delay = millis();                           // Trigger delay timer
unsigned long last_update  =  millis();                         // Last update time      
unsigned long update_period =  20000;                           // Check every N seconds
unsigned long time_alarm  =  90000;                             // Working hours of the siren
unsigned int timer_delay_off;                                   // Alarm delay time in milliseconds
unsigned int timer_delay_on;                                    // Arming delay time in milliseconds
unsigned long time_balance = 86400000;                          // Balance check periodicity time in milliseconds (day 86400000 ms)
long counter_errors = 0;                                        // Iteration counter in case of an incoming SMS processing error
unsigned long alarm_timer = 0;
bool flag_zone1 = false;
bool flag_delay_zone1 = false;
bool flag_keyboard = false;
bool flag_balance = false;
bool flag_loop = false;
bool flag_call = false;                                         // Incoming call flag  
bool flag_volt = false;                                         // Flag for the absence of voltage in the electrical network
bool flag_tel = false;                                          // Outgoing call flag
bool mode = false;                                              // Arming mode on/off
bool flag_alarm = false;                                        // Call counter about workings
bool flag_timer_sms = false;                                    // Flag for processing incoming SMS messages
bool hasmsg  =  false;                                          // Flag for messages to be deleted
bool flag_alarm_timer = 0;
byte macros_id = 0;                                             // Macro storage variable
bool macros_1[6] = {1,1,1,1,1,1};                               // All zones are active
bool macros_2[6] = {1,1,1,0,0,0};                               // Only security zones are active
bool macros_3[6] = {0,0,0,1,1,1};                               // Only emergency zones are active
bool macros_4[6] = {1,0,0,1,1,1};                               // Emergency zones and one security zone are active
bool macros_5[6] = {1,1,1,1,0,0};                               // Security zones and one emergency zone are active (fire)
byte triggered[6] = {9,9,9,9,9,9};                              // Array of addresses of triggered sensors
byte counter_triggered = 0;                                     // Counter of sensor workings
byte counter_admins = 5;                                        // Number of administrators
Sensor zone[6];                                                 // Array of objects Sensor

///////////////////////////////////////// Primary controller settings function /////////////////////////////////////////

void setup() 
{
  D2_Out;
  D3_Out;
  D4_Out;
  D5_Out;
  D6_Out;
  D7_Out;
  D8_In;
  D9_In;
  D10_In;
  D11_In;
  D12_In;
  D13_Out;
  D16_In;
  D17_In;
  D18_In;
  D19_In;

  zone[0].pin_ = ZONE_1;
  zone[1].pin_ = ZONE_2;
  zone[2].pin_ = ZONE_3;
  zone[3].pin_ = ZONE_4;
  zone[4].pin_ = ZONE_5;
  zone[5].pin_ = ZONE_6;

  Serial.begin(9600);                                  // Speed of data exchange with the computer
  SIM800.begin(9600);                                  // Speed of data exchange with the SIM module
  
  InitialZones();
  InitialEeprom();
  
  balance_send = ((String)temp).toInt();               ////////////////////////////////////////////////////////////////
  temp = F("Systema v5.3 Status OFF. Phones - ");      //
  for(int i = 0; i < counter_admins; i ++ )            //
  {                                                    //
    if(phones[i][0] == '+')                            //
    {                                                  //
      temp += phones[i];                               //
      if(phones[i + 1][0] == '+')                      //
      {                                                //
        temp += F(", ");                               //
      }                                                //      
    }                                                  //   Forming a message about the controller reboot
  }                                                    //
  temp += F(". ");                                     //
  temp += F("Time ON ");                               //
  temp += timer_delay_on/1000;                         //
  temp += F(". Time OFF ");                            //  
  temp += timer_delay_off/1000;                        //
  temp += F(". Min balance ");                         //
  temp += balance_send;                                //
  temp += F(".Code ");                                 //
  temp += operator_code;                               //
  temp += F(".");                                      ////////////////////////////////////////////////////////////////

  TestModem();                                         // We check the performance of the SIM module
  InitialModem();                                      // Initializing the SIM module                                       
  delay(1000);
  SendSMS(temp);                                       // We send an SMS notification about the controller restart to the administrator
  tone(BUZZER_PIN, 1915);                              // We signal the speaker about the passage of the stage of loading the controller
  delay(1000);
  noTone(6);
  last_update  =  millis();                            // Reset the timer    
}

/////////////////////////////////////////////// Main cyclic function ////////////////////////////////////////////

void loop() 
{ 
  flag_loop = true;
  GetKeyboard();                                      // We are waiting for data from the keyboard
  GetBalanceSim();                                    // Check the balance on the SIM card
  SetDingDong();                                      // Light and sound indication of incoming call
  GetNewSMS();                                        // We receive incoming SMS messages
  
  if (SIM800.available())   {                         // If the modem sent something...
    _response  =  WaitResponse();                     // We get a response from the modem for analysis
    _response.trim();                                 // Removing extra spaces at the beginning and end
    if (_response.indexOf("+CMTI:") > -1)             // If you received a message about sending SMS
    {    
     flag_timer_sms = true;      
    }
    if (_response.startsWith("RING"))                // If there is an incoming call
    { 
      GetIncomingCall();                             // Processing it
    }  
    else                                             // If not
    {
      flag_call = false;                             // Omit the input flag of the incoming call
    }
  }
  if(Serial.available())                             // Waiting for the Serial command
  {                         
    SIM800.write(Serial.read());                     // And send the received command to the modem
  }
  if(SIM800.available())                             // Waiting for a command from the modem
  {                         
    Serial.write(SIM800.read());                     // And send the received command to Serial
  }
  if(!mode)                                          // If disarmed (off mode)
  { 
    flag_delay_zone1 = false;
    flag_zone1 = false;
    flag_tel = false;  
    flag_alarm_timer = false; 
    flag_alarm = false;                              // We omit the flag of messages about the trigger
    counter_triggered = 0;
    for(int i = 0; i < 6; i ++ )
    {
      zone[i].alarm_ = false;
      zone[i].send_alarm_ = false;
      triggered[i] = 9;
    }            
    D2_Low;                                          // Turning off the alarm                 
    D13_Low;                                         // Turn off the LED
    noTone(6);                                       // Turning off the siren
    //Serial.println(F("!mode"));                      // If necessary we send a message to the port monitor 
  }
  else                                               // If it is on guard (on mode)
  {
    if((alarm_timer + time_alarm) < millis())
    {
      D2_Low;                                        // If the time of the siren is out, turn it off
    }
//    Serial.println(F("mode"));                       // If necessary we send a message to the port monitor              
    D13_High;                                        // Turn on the LED
    GetSensors();                                    // We look at the sensors
    AlarmMessages();                                 // We send the necessary notifications
  }
  GetVoltage();                                      // We control the power supply of the system
}


/////////////////////////////////////////// Call notification function ////////////////////////////////////////////

void Call(String & num)
{
  String comand = F("ATD");
  SendATCommand("AT+COLP=0", true);                // Response waiting mode
  //SendATCommand("AT + VTD = 4", true);             // The duration of the tone signals for AT + VTD. Parameter value 1..255
  comand = comand + num;                           // Putting together a command
  comand = comand + ";";                           // Adding a character to the end of the command
  SendATCommand(comand, true);                     // Sending a call command
  //delay(3000);                                     // Pause until the call is reset
  //SendATCommand("ATH0", true);                     // Resetting the call 
}

//////////////////////////////////////// Function of sending a command to the SIM module /////////////////////////////////////

String SendATCommand(String cmd, bool waiting) 
{
  String _resp  =  "";                                            // Variable for storing the result
  SIM800.println(cmd);                                            // Sending a command to the module
  if (waiting)                                                    // If you need to wait for a response
  {                                                 
    _resp  =  WaitResponse();                                     // We are waiting for the response to be sent
    // If Echo Mode is disabled (ATE0), then these 3 lines can be commented out
    if (_resp.startsWith(cmd))                                    
    {                                  
      _resp  =  _resp.substring(_resp.indexOf("\r", cmd.length()) + 2); // Removing the duplicate command from the response
    }
  }
  return _resp;                                                   // Returning the result. Empty if there is a problem
}

//////////////////////////////////////// The function of waiting for a response from the SIM module ///////////////////////////////////////////////////////

String WaitResponse()                                             // Function for waiting for a response and returning the received result
{                                          
  String _resp  =  "";                                            // Variable for storing the result
  unsigned long _timeout  =  millis() + 10000;                    // Variable for tracking the timeout (10 seconds)
  while (!SIM800.available() && millis() < _timeout)  {};         // We are waiting for a response for 10 seconds, 
                                                                  // if a response has arrived or a timeout has occurred, then
  if (SIM800.available())                                         // If there is something to read
  {                                       
    _resp  =  SIM800.readString();                                // Read and remember
  }
  return _resp;                                                   // Returning the result. Empty if there is a problem
}

///////////////////////////////////////////// Sensor monitoring function //////////////////////////////////////////////////////////////

void GetSensors()
{
  for(int i = 0; i < 6; i ++ )                           // We pass through the sensors
  {
    if(zone[i].ReadPin())                                // If there is a trigger
    {
      flag_alarm = true;                                 // We raise the flag of the working out
      triggered[counter_triggered] = zone[i].adress_;    // We write the address of the zone in the array of workings
      counter_triggered ++ ;                             // Increasing the address counter
    }
  }
  if(triggered[0] == 0 && !flag_zone1)                   // If zone 1 was triggered first and this is the first trigger
  {
    timer_delay = millis();                              // Starting the delay timer
    flag_zone1 = true;                                   // We raise the flag of working out in zone 1
  }
}

/////////////////////////////////////////////// Notification function //////////////////////////////////////////////////////////////////////
                                                                 
void AlarmMessages()
{
  if(flag_alarm)
  {
    for(int i = 0; i < counter_triggered; i ++ )
    {
      if(zone[triggered[i]].alarm_ == true && zone[triggered[i]].send_alarm_ == false) // If the sensor failure flag is raised and the notification was not sent
      {
        if(i == 0 && triggered[i] == 0)                                          // If the trigger is in zone 1
        {
          if(millis() - timer_delay > timer_delay_off)                           // If the timer is triggered
          {     
            ActivateRelay(zone[triggered[i]].pin_alarm_1_, zone[triggered[i]].pin_alarm_2_); // Activating the alarm relay
            SendSMS(zone[triggered[i]].message_alarm_);                          // Sending an SMS notification
            if(!flag_tel)                                                        // If you haven't called the administrator yet
            {
              flag_tel = true;                                                   // Raising the call flag  
              Call(phones[0]);                                                   // We call the administrator
            }
            zone[triggered[i]].send_alarm_ = true;                               // Raising the flag of the sent notification
            flag_delay_zone1 = true;                                             // We raise the flag of the delay of working out
            break;                 
          }
        }
        else
        {
          if(triggered[i] == 1 && flag_zone1 && !flag_delay_zone1) break;
          ActivateRelay(zone[triggered[i]].pin_alarm_1_, zone[triggered[i]].pin_alarm_2_);   // Activating the alarm relay
          SendSMS(zone[triggered[i]].message_alarm_);                                 // Sending an SMS notification
          if(!flag_tel)                                                               // If you haven't called the administrator yet
          {
            flag_tel = true;                                                          // Raising the call flag 
            Call(phones[0]);                                                          // We call the administrator
          }
          zone[triggered[i]].send_alarm_ = true;                                      // Raising the flag of the sent notification
          break;                                                                      // Exiting the loop
        }
      }
    }
  }
}
     
///////////////////////////////////////////////// SMS parsing function /////////////////////////////////////////////////

void ParseSMS(String & msg)                                     // SMS Parsing
{ 
                                  
  String msgheader   =  "";
  String msgbody     =  "";
  String msgphone    =  "";

  msg  =  msg.substring(msg.indexOf("+CMGR: "));
  msgheader  =  msg.substring(0, msg.indexOf("\r"));            // We pull out the phone
  msgbody  =  msg.substring(msgheader.length() + 2);
  msgbody  =  msgbody.substring(0, msgbody.lastIndexOf("OK"));  // Pulling out the SMS text
  msgbody.trim();
  int firstIndex  =  msgheader.indexOf("\",\"") + 3;
  int secondIndex  =  msgheader.indexOf("\",\"", firstIndex);
  msgphone  =  msgheader.substring(firstIndex, secondIndex);

  if (msgphone == phones[0] || msgphone == phones[1] || msgphone == phones[2]
   || msgphone == phones[3] || msgphone == phones[4])           // If the admin's phone number, then
  {     
    SetLedState(msgbody, msgphone);                             // Execute the command
  }
}
 
///////////////////////////////////////// The function of reading the command code from the SMS ///////////////////////////////////////////

void SetLedState(String & result, String & msgphone)   
{
  bool correct  =  false;                         // To optimize the code, the command correctness variable
  if (result.length()  ==  1)                     // If the message contains a single digit
  {
    int ledState  =  ((String)result[0]).toInt(); // We get the first digit of the command-the state (0 - off, 1-on)
    if (ledState  >=  0 && ledState  <=  5) 
    {                                             // If everything is fine, execute the command
      if (ledState == 1) 
      {
        EEPROM.write(106, 0);                     // We write it in memory  
        macros_id = 0;
        InitialMacros();                          // Initialize the macro
        SendSMS(F("mode 1 ON"));                  // We send an SMS with the result to the administrator 
        mode = true;       
      }
      if(ledState == 2) 
      {
        EEPROM.write(106, 1);                     // We write it in memory
        macros_id = 1;
        InitialMacros();                          // Initialize the macro
        SendSMS(F("mode 2 ON"));                  // We send an SMS with the result to the administrator 
        mode = true;              
      }
      if(ledState == 3)
      {
        EEPROM.write(106, 2);                    // We write it in memory
        macros_id = 2;
        InitialMacros();                         // Initialize the macro   
        SendSMS(F("mode 3 ON"));                 // We send an SMS with the result to the administrator 
        mode = true;            
      }
      if(ledState == 4)
      { 
        EEPROM.write(106, 3);                    // We write it in memory
        macros_id = 3;
        InitialMacros();                         // Initialize the macro
        SendSMS(F("mode 4 ON"));                 // We send an SMS with the result to the administrator    
        mode = true;               
      }
      if(ledState == 5) 
      {
        EEPROM.write(106, 4);                    // We write it in memory
        macros_id = 4;
        InitialMacros();                         // Initialize the macro  
        SendSMS(F("mode 5 ON"));                 // We send an SMS with the result to the administrator
        mode = true;
      }
      if(ledState == 0) 
      {
        mode = false;                            // We're disarming you
        SendSMS(F("mode OFF"));                  // We send an SMS with the result to the administrator
      }     
      tone(BUZZER_PIN, 1915);                    // We signal the speaker to change the security mode
      delay(1000);
      noTone(6); 
      correct  =  true;                          // Command correctness flag
    }
    if (!correct) 
    {
      SendSMS(F("Incorrect command!"));          // We send an SMS with the result to the administrator
    }
  }
  else
    {
      if(result.length()  ==  13 && msgphone == phones[0])  // If the message contains a phone number and the sender is admin 1
      {
        bool flag_number = false;
        for(int k = 1; k < counter_admins; k ++ )       // We go through the list of numbers
        {
          if(phones[k] == result)                       // If the number is in position k
           {
            flag_number = true;                         // Raising the number availability flag
            switch(k)                                   // We look for it in the EEPROM memory and delete it
            {
              case 1:
                    for(int i = 20; i < 33; i ++ )
                      {
                        EEPROM.write(i, (byte)' ');     // Overwrite the admin number with 2 spaces                       
                      }
                      phones[k] = " ";                  // Clearing the variable
                      tone(BUZZER_PIN, 1915);           // We signal with the speaker about the deletion of the backup number
                      delay(1000);
                      noTone(6);
                      SendSMS(F("Number 2 deleted!"));
                      break;
              case 2:
                    for(int i = 40; i < 53; i ++ )
                      {
                        EEPROM.write(i, (byte)' ');     // Overwrite the admin number with 3 spaces
                      }
                      phones[k] = " ";                  // Clearing the variable
                      tone(BUZZER_PIN, 1915);           // We signal with the speaker about the deletion of the backup number
                      delay(1000);
                      noTone(6);
                      SendSMS(F("Number 3 deleted!"));
                      break;
              case 3:
                    for(int i = 60; i < 73; i ++ )
                     {
                        EEPROM.write(i, (byte)' ');     // Overwrite the admin number with 4 spaces
                     }
                     phones[k] = " ";                   // Clearing the variable
                     tone(BUZZER_PIN, 1915);            // We signal with the speaker about the deletion of the backup number
                     delay(1000);
                     noTone(6);
                     SendSMS(F("Number 4 deleted!"));
                     break;
              case 4:
                    for(int i = 80; i < 93; i ++ )
                    {
                        EEPROM.write(i, (byte)' ');     // Overwrite the admin number with 5 spaces
                    }
                    phones[k] = " ";                    // Clearing the variable
                    tone(BUZZER_PIN, 1915);             // We signal with the speaker about the deletion of the backup number
                    delay(1000); 
                    noTone(6);
                    SendSMS(F("Number 5 deleted!"));
                    break;
              default:                                        // If something went wrong
              SendSMS(F("Number not deleted!"));  
            }
          }
        }
        if(flag_number)
        {
          result = " ";
          return;
        }
        for(int k = 1; k < counter_admins;k ++ )              // We go through the list of numbers
        {
          if (phones[k][0] != '+' && result[0] == '+' && !flag_number) // If there is a space in position k, the number is valid and has not yet been recorded
          {
            switch(k)
            {
              case 1:
                    for(int i = 20, j = 0; i < 33; i ++ , j ++ )
                      {
                        EEPROM.write(i, (byte)result[j]);     // We write down the admin number in memory 2                       
                      }
                      phones[k] = result;                     // Write the admin number to the variable 2
                      tone(BUZZER_PIN, 1915);                 // We signal with the speaker that the backup number is accepted
                      delay(1000);
                      noTone(6);
                      SendSMS(F("Number 2 accepted!")); 
                      flag_number = true;                     // Raising the number entry flag
                      break;
              case 2:
                    for(int i = 40, j = 0; i < 53; i ++ , j ++ )
                      {
                        EEPROM.write(i, (byte)result[j]);     // We write down the admin number in memory 3
                      }
                      phones[k] = result;                     // Write the admin number to the variable 3
                      tone(BUZZER_PIN, 1915);                 // We signal with the speaker that the backup number is accepted
                      delay(1000);
                      noTone(6);
                      SendSMS(F("Number 3 accepted!"));
                      flag_number = true;                     // Raising the number entry flag
                      break;
              case 3:
                    for(int i = 60, j = 0; i < 73; i ++ , j ++ )
                     {
                        EEPROM.write(i, (byte)result[j]);     // We write down the admin number in memory 4
                     }
                     phones[k] = result;                      // Write the admin number to the variable 4
                     tone(BUZZER_PIN, 1915);                  // We signal with the speaker that the backup number is accepted
                     delay(1000);
                     noTone(6);
                     SendSMS(F("Number 4 accepted!"));
                     flag_number = true;                      // Raising the number entry flag
                     break;
              case 4:
                    for(int i = 80, j = 0; i < 93; i ++ , j ++ )
                    {
                        EEPROM.write(i, (byte)result[j]);     // We write down the admin number in memory 5
                    }
                    phones[k] = result;                       // Write the admin number to the variable 5
                    tone(BUZZER_PIN, 1915);                   // We signal with the speaker that the backup number is accepted
                    delay(1000);
                    noTone(6);
                    SendSMS(F("Number 5 accepted!"));
                    flag_number = true;                       // Raising the number entry flag
                    break;
              default:
              SendSMS(F("Number not accepted!"));  
            }
          }                 
        }                            
      }
      else
        {
            if(result.length()  ==  5)                   // If the message contains a 5-digit command
            {
              String command = "";                       // Declaring a variable for writing commands
              String item = "";                          // Declaring a variable for writing values
              command += result[0];                      // Getting the command
              command += result[1];              
              item += result[3];                         // Getting the value
              item += result[4]; 
              if(command == "On")                        // If the command " power-on delay"
              {
                  for(int i = 100, j = 0; i < 102; i ++ , j ++ )
                  {
                    EEPROM.write(i, (byte)item[j]);      // Write the delay time to the EEPROM memory           
                  }
                  timer_delay_on = item.toInt() * 1000;  // Writing the delay time to a variable
                  item += F(" seс on");                  // Creating a notification
                  SendSMS(item);                         // Sending a notification
              }
              if(command == "Of")                        // If the command "shutdown delay"
              {
                  for(int i = 102, j = 0; i < 104; i ++ , j ++ )
                  {
                    EEPROM.write(i, (byte)item[j]);      // Write the delay time to the EEPROM memory           
                  }
                  timer_delay_off = item.toInt() * 1000; // Writing the delay time to a variable
                  item += F(" seс of");                  // Creating a notification
                  SendSMS(item);                         // Sending a notification
              } 
              
              if(command == "Bl")                        // If the command "minimum balance for notification"
              {
                  for(int i = 104, j = 0; i < 106; i ++ , j ++ )
                  {
                    EEPROM.write(i, (byte)item[j]);      // Write the minimum balance for the notification to the EEPROM memory           
                  }
                  balance_send = item.toInt();           // Write the minimum balance for the notification to the variable
                  item += F(" grn bl");                  // Creating a notification
                  SendSMS(item);                         // Sending a notification
              }

             if(result[0] == '*' && result[4] == '#')         // If the command "balance verification code" 
             {
              operator_code = "";                             // Clearing the balance check code storage variable
                  for(int i = 107, j = 0; i < 112; i ++ , j ++ )
                  {
                    EEPROM.write(i, (byte)result[j]);         // Write the balance check code to the EEPROM memory
                    operator_code += result[j];               // Write the balance check code to the variable           
                  }  
                  SendSMS(operator_code);                     // Sending a notification      
             }
            }
             else
            {
              if(result.length()  ==  2)                      // If the message contains a 2-digit command
              {
                String command = "";                          // Declaring a variable for writing commands
                command += result[0];                         // Getting the value in the variable   
                command += result[1];
                if(command == "10")                           // If the command turn off the relay 1
                {
                  D5_Low;                                     // Turning off the relay 2
                  SendSMS(F("Relay 1 OFF"));                  // We send a notification about switching off the relay 1
                }
                if(command == "11")                           // If the command enable relay 1
                {
                  D5_High;                                    // Turning on the relay 1
                  SendSMS(F("Relay 1 ON"));                   // We send a notification about switching on the relay 1
                }                
                if(command == "21")                           // If the command enable relay 2
                {   
                  D7_High;                                    // Turning on the relay 2
                  SendSMS(F("Relay 2 ON"));                   // We send a notification about switching on the relay 2
                }
                if(command == "20")                           // If the command turn off the relay 2
                {    
                  D7_Low;                                     // Turning off the relay 2
                  SendSMS(F("Relay 2 OFF"));                  // We send a notification about switching off the relay 2
                }
              }
            }
         }
      }

}

//////////////////////////////////////////// The function of sending SMS messages ///////////////////////////////////////////

void SendSMS(String message)
{
  for(int i = 0; i < counter_admins; i ++ )
  {
    if(phones[i][0] == '+')
    {
      SendATCommand("AT+CMGS=\"" + phones[i] + "\"", true);          // Switching to text message input mode
      SendATCommand(message + "\r\n" + (String)((char)26), true);    // After the text, we send a line break and Ctrl + Z
      delay(2500);
    }
  } 
}

///////////////////////////////////////// Function for getting the SIM card BALANCE in the menu /////////////////////////////

float GetBalans(String & code) 
{
bool flag = true;
_response  =  SendATCommand("AT+CUSD=1,\"" + code + "\"", true);   // Sending a USSD balance request
 while(flag)                                              // Running a non-ending loop to wait for a response to the request
 {
  if (SIM800.available())                                 // If the modem sent something
  {
    _response  =  WaitResponse();                         // We get a response from the modem for analysis 
    _response.trim();                                     // Removing extra spaces at the beginning and end
    if (_response.startsWith("+CUSD:"))                   // If you received a notification about the USSD response (account balance)
    {   
      if (_response.indexOf("\"")  >  -1)                 // If the response contains quotation marks, it means that there is a message (a guard against "empty" USSD responses)                                                          
      {       
        String msgBalance  =  _response.substring(_response.indexOf("\"") + 1, 50);  // We get the text directly
        msgBalance  =  msgBalance.substring(0, msgBalance.indexOf("\""));
        float balance  =  GetFloatFromString(msgBalance);                            // Extracting information about the balance     
        flag = false;                                                                // Exit the loop to the main menu
        return balance;
      }
    }
    else
    {
      return 0;
    }
   }
  }
 return 0;
}
///////////////////////// Function for EXTRACTING DIGITS from a message - for parsing the balance from a USSD request ////////////////

float GetFloatFromString(String & str) {            
  bool flag  =  false;
  String result  =  "";
  str.replace(",", ".");                                   // If a comma is used as the decimal separator, we change it to a dot
  for (unsigned int i  =  0; i < str.length(); i ++ ) 
  {
    if (isDigit(str[i]) || (str[i]  ==  (char)46 && flag))   
    {                                                      // If a group of digits begins (at the same time, we do not pay attention to a point without digits)
      if (result  ==  "" && i  >  0 && (String)str[i - 1]  ==  "-") 
      {                                                    // We must not forget that the balance can be negative
        result += "-";                                     // Adding a sign at the beginning
      }
      result += str[i];                                    // We begin to collect them together
      if (!flag) flag  =  true;                            // We set a flag that indicates that the number assembly has started
    }
    else  
    {                                                      // If the numbers are over and the flag indicates that the build has already been completed
      if (str[i] != (char)32) 
      {                                                    // If the order of the number is separated by a space, we ignore it, otherwise
        if (flag) break;                                   // We believe that all
      }
    }
  }
  return result.toFloat();                                 // Returning the received number
}

/////////////////////////////////////////////// SIM balance notification function ////////////////////////////////////////////////////////////////
                                                                                   
void GetBalanceSim()
{
// if((millis()-timer_balance) > 20000)                          // Frequency of checking the balance
// if((millis()-timer_balance) > time_balance || !flag_balance)  // The frequency of checking the balance (if the timer or the first check is triggered)
if((millis() - timer_balance) > time_balance)
  {
    String message  =  F("Balance of sim  =  ");
    float balance = GetBalans(operator_code);              // Extracting the balance
    delay(3000);
    if(balance <= balance_send)                            // Balance threshold for notification
    {
      message += (String)balance;                          // Adding the balance to the message
      SendSMS(message);                                    // We send a notification about the balance to the administrator
    }
    timer_balance = millis();                              // Resetting the counter
    flag_balance = true;                                   // We raise the flag of the performed balance check
  }
}

void InitialZones()////////////////////////////// Zone initialization function ////////////////////////////////////////////////////
  {
///////////////////////// 0  No alarm    
///////////////////////// 3  Alarm relay output                               
///////////////////////// 1  Gas outlet valve                                  
///////////////////////// 2  Water outlet valve                          

    for(int i = 0; i < 6; i ++ )
    {
      zone[i].adress_ = i;                                    // Initializing the zone address
      switch (i)                                              // Initialize alarm pins and trigger notifications
      {
      case 0:                                                 // For zone 1 (security)
       strcpy(zone[i].message_alarm_,"Triggered to zone 1!"); // We form a notification about the trigger
       zone[i].pin_alarm_1_ = 3;                              // When triggered, we activate the alarm relay
       break;
      case 1:                                                 // For zone 2 (security)
       strcpy(zone[i].message_alarm_,"Triggered to zone 2!"); // We form a notification about the trigger
       zone[i].pin_alarm_1_ = 3;                              // When triggered, we activate the alarm relay
       break;
      case 2:                                                 // For zone 3 (security)
       strcpy(zone[i].message_alarm_,"Triggered to zone 3!"); // We form a notification about the trigger 
       zone[i].pin_alarm_1_ = 3;                              // When triggered, we activate the alarm relay
       break;
      case 3:                                                 // For zone 4 (emergency "Fire")
       strcpy(zone[i].message_alarm_,"Wildfire!!!");          // We form a notification about the trigger 
       zone[i].pin_alarm_1_ = 3;                              // When triggered, we activate the alarm relay
       zone[i].pin_alarm_2_ = 1;                              // When triggered, we activate the gas valve shut-off relay
       break;
      case 4:                                                 // For zone 5 (emergency "Gas")
       strcpy(zone[i].message_alarm_,"Gas leakage!!!");       // We form a notification about the trigger
       zone[i].pin_alarm_1_ = 3;                              // When triggered, we activate the alarm relay
       zone[i].pin_alarm_2_ = 1;                              // When triggered, we activate the gas valve shut-off relay
       break;
      case 5:                                                 // For zone 6 (emergency "Water")
       strcpy(zone[i].message_alarm_,"Water leak!");          // We form a notification about the trigger 
       zone[i].pin_alarm_1_ = 3;                              // When triggered, we activate the alarm relay
       zone[i].pin_alarm_2_ = 2;                              // When triggered, we activate the water valve shut-off relay
       break;
      }
    }
    InitialMacros();
  }

///////////////////////////////////////// Macro initialization function /////////////////////////////////////////////

  void InitialMacros()      
  {
    switch (macros_id)
    {
     case 0:
      for(int i = 0; i < 6; i ++ )
      {
        zone[i].status_ = macros_1[i];   // All zones are active
      }
      break;
     case 1:
      for(int i = 0; i < 6; i ++ )
      {
        zone[i].status_ = macros_2[i];   // Only emergency zones are active        
      }
      break;
     case 2:
      for(int i = 0; i < 6; i ++ )
      {
        zone[i].status_ = macros_3[i];   // Only security zones are active       
      }
      break;
     case 3:
      for(int i = 0; i < 6; i ++ )
      {
        zone[i].status_ = macros_4[i];   // Emergency zones and two security zones are active      
      }
      break;  
    default:
      for(int i = 0; i < 6; i ++ )
      {
        zone[i].status_ = macros_1[i];   // All zones are active    
      }
      break;  
    }  
      
  }

////////////////////////////////////////////// 220 V mains voltage notification function ////////////////////////////////////////////////////

void GetVoltage()
{
 if(digitalRead(VOLT_PIN) == HIGH)                      // If the voltage sensor is triggered (no voltage, battery operation)
 {  
    if(!flag_volt)                                      // If this is the first trigger
    {
      flag_volt = true;                                 // Raising the flag from the sky
      SendSMS(F("Battery operation!"));                 // We send a notification about the work from the battery to the administrator  
    }
 }
 else                                                   // If the sensor is at rest (there is a voltage, work from the 220V network)
 {
    if(flag_volt)                                       // If there was a trigger before that
    {
      flag_volt = false;                                // We lower the flag from the site
      SendSMS(F("Operation from 220 volts!"));          // Sending a recovery notification 
                                                        // works from the 220 V network to the administrator
    }
    return;                                             // We go to the main function
 }
}
/////////////////////////////////////////// The function of checking the availability of the SIM module //////////////////////////////

void TestModem()
{
  do {              // When the power is turned on the MC will load before the SIM module so we are waiting for the download and the response to the command
    _response  =  SendATCommand("AT", true);                  // Sent AT to configure the data exchange rate
    _response.trim();                                         // Removing whitespace characters at the beginning and end
    if(AT_counter == 3)
    {
      tone(BUZZER_PIN, 1915);                                 // We signal with a sound about the lack of communication with the SIM module
      delay(150);
      noTone(6);
      delay(20);
      tone(BUZZER_PIN, 1915);                  
      delay(150);
      noTone(6);
      delay(20);
      tone(BUZZER_PIN, 1915);                  
      delay(150);
      noTone(6);
      delay(1000);
      AT_counter = 0;
      if(flag_loop) break;
    }
    if(_response != "OK")AT_counter ++ ;   
  } while (_response != "OK");                                    // Do not let it continue until the module returns ОК
}

//////////////////////////////////// SIM module initialization function ///////////////////////////////////////

void InitialModem()
{  
  SendATCommand("AT+CMGDA=\"DEL ALL\"", true);                    // We delete all SMS messages, so as not to clog the memory
  delay(1000);
                                                                  // Modem setup commands on each startup
//SendATCommand("AT + DDET = 1", true);                           // Enable DTMF
  SendATCommand("AT+CLIP=1", true);                               // Enable АОН
  delay(1000);
  SendATCommand("AT+CMGF=1;&W", true);                            // Enabling SMS text mode and immediately save the value (AT&W)!
  delay(1000);
}

///////////////////////////////////////// Function for receiving new SMS messages ///////////////////////////////////////

void GetNewSMS()
  { 
   if (millis() - last_update > update_period || flag_timer_sms)  // If it's time to check for new messages
   { 
    do {
      _response  =  SendATCommand("AT+CMGL=\"REC UNREAD\"", true);// Sending a request to read unread messages
      delay(1000);
      if (_response.indexOf("+CMGL: ")  >  -1) 
      {                                                           // If there is at least one, we get its index
        int msgIndex  =  _response.substring(_response.indexOf("+CMGL: ") + 7, 
        _response.indexOf("\"REC UNREAD\"", _response.indexOf("+CMGL: "))).toInt();
        int i  =  0;                                              // Declaring the attempt counter
        counter_errors = 0;
        do 
        {
          i ++ ;                                                  // Increasing the counter
          Serial.println (i);
          _response  =  SendATCommand("AT+CMGR=" + (String)msgIndex + ",1", true);  // Trying to get the SMS text by the index
          _response.trim();                                       // Removing spaces at the beginning/end
          if (_response.endsWith("OK")) 
          {                                                       // If the answer ends with " OK"
            if (!hasmsg) hasmsg  =  true;                         // We set the flag for the presence of messages to delete
            SendATCommand("AT+CMGR=" + (String)msgIndex, true);   // Making the message read
            SendATCommand("\n", true);                            // Reinsurance - output of a new line
            ParseSMS(_response);                                  // Sending the message text for processing
            flag_timer_sms = false;
            break;                                                // Exit from do{}
          }
          else 
          {                                                       // If the message does not end with OK
            SendATCommand("\n", true);                            // We send a new line and try again
          }
        } while (i < 10);
        break;
      }
      else 
      {
        counter_errors ++ ;
        last_update  =  millis();                                 // Reset the timer 
        if (hasmsg || counter_errors > 10) 
        {
          SendATCommand("AT+CMGDA=\"DEL ALL\"", true);            // Deleting all messages
          hasmsg  =  false;         
        }
        flag_timer_sms = false; 
        break;
      }
    } while (1);   
   }
  }

/////////////////////////////////////// Keyboard operation function /////////////////////////////////////////////////

void GetKeyboard()
{
  if(digitalRead(KEYBOARD_PIN) == LOW)                              // If the signal from the keyboard is lost
  {
    delay(1500);
    if(digitalRead(KEYBOARD_PIN) == HIGH)                           // If the signal appears after 1.5 seconds, the signal is valid (mode change)
    { 
      if(!flag_keyboard)                                            // The cliff flag is not raised
      {
        if(!mode)                                                   // If we put it on guard
        {
          delay(timer_delay_on);                                    // We are waiting for the specified time before the production
          mode = !mode;                                             // Switching the mode to the opposite
          SendSMS(F("mode ON"));                                    // We send an SMS with the result to the administrator
        }
        else                                                        // If we disarm it
        {
          mode = !mode;                                             // Switching the mode to the opposite
          SendSMS(F("mode OFF"));                                   // We send an SMS with the result to the administrator
        }
      }
      else                                                          // If the cliff flag is raised
      {
        flag_keyboard = false;                                      // Lowering the cliff flag
        SendSMS(F("Keyboard ON"));                                  // We send an SMS about the keyboard connection       
      }
    }
    else                                                            // If the signal does not appear after 1.5 seconds
    {
      if(!flag_keyboard)                                            // If the cliff flag is not raised                   
      {
        flag_keyboard = true;                                       // Raising the cliff flag
        SendSMS(F("Keyboard OFF"));                                 // We send an SMS about the absence of the keyboard
      }
    }
  }
}

///////////////////////////////////////// Relay activation function ////////////////////////////////////////////////////////

void ActivateRelay(byte Alarm1, byte Alarm2)
{
      switch (Alarm1)                          // If an alarm is connected to the sensor 1
      {
        case 0:                                // No alarm
          break;
        case 1:
                       
        D3_High;                               // Gas cut-off valve
          break;
        case 2:
          D4_High;                             // Water cut-off valve
          break;
        case 3:
          D2_High;                             // Alarm
          if(!flag_alarm_timer)
            {                    
              alarm_timer = millis();
              flag_alarm_timer = true;
            }
          break;
      }
      switch (Alarm2)                          // If an alarm is connected to the sensor 2  
      {
        case 0:                                // No alarm
          break;
        case 1:
          D3_High;                             // Gas cut-off valve
          break;
        case 2:
          D4_High;                             // Water cut-off valve
          break;
        case 3:
          D2_High;                             // Alarm
          if(!flag_alarm_timer)
            {                    
              alarm_timer = millis();
              flag_alarm_timer = true;
            }
          break;
      }
}

///////////////////////////////// Non-volatile memory initialization function ///////////////////////////////////////

void InitialEeprom()
{
  for(int i = 0; i < 13; i ++ )
   {
      phones[0] += (char)EEPROM.read(i);                        // We get the admin number from the EEPROM
   }
  for(int i = 20; i < 33; i ++ )
   {
      phones[1] += (char)EEPROM.read(i);                        // We get the admin number from the EEPROM 2
   }
   for(int i = 40; i < 53; i ++ )
   {
      phones[2] += (char)EEPROM.read(i);                        // We get the admin number from the EEPROM 3
   }
   for(int i = 60; i < 73; i ++ )
   {
      phones[3] += (char)EEPROM.read(i);                        // We get the admin number from the EEPROM 4
   }
   for(int i = 80; i < 93; i ++ )
   {
      phones[4] += (char)EEPROM.read(i);                        // We get the admin number from the EEPROM 5
   }
   for(int i = 100; i < 102; i ++ )
   {
      temp += (char)EEPROM.read(i);                             // We get from the EEPROM the delay value is on
   }
   timer_delay_on = ((String)temp).toInt()*1000;                // Initialize the digital variable of the power-on delay
   temp = "";                                                   // Resetting the string variable of the power-on delay
   for(int i = 102; i < 104; i ++ )
   {
      temp += (char)EEPROM.read(i);                             // We get from the EEPROM the delay value is off
   }
   timer_delay_off = ((String)temp).toInt()*1000;               // Initialize the digital variable of the shutdown delay
   temp = "";                                                   // Reset the shutdown delay string variable to zero
  for(int i = 104; i < 106; i ++ )
  {
      temp += (char)EEPROM.read(i);                             // We get the balance from the EEPROM
  }
   macros_id = EEPROM.read(106);                                // Getting the current macro from the EEPROM

  for(int i = 107, j = 0; i < 112; i ++ , j ++ )
  {
    operator_code += (char)EEPROM.read(i);                      // We get the operator code from the EEPROM 
  }
}

////////////////////////////////////// Incoming call indication function /////////////////////////////////////////

void SetDingDong()     
{  
  if((millis() - timer_ring < 3000))                       
  {
    D13_Inv;                                        // Switching the LED
    tone(BUZZER_PIN, 1915);                         // Turn on the squeaker
    delay(50);                                      // Pause
    D13_Inv;                                        // Switching the LED
    noTone(6);                                      // Turn off the squeaker
    delay(50);                                      // Pause
  }
}

//////////////////////////////////////////// Function for processing incoming calls ///////////////////////////////////

void GetIncomingCall()    
{
  bool flag_admin = false;
  flag_call = true;                                     // Raising the call flag
  int phone_index  =  _response.indexOf("+CLIP: \"");   // Is there any information about determining the number, if so, then phone_index > -1
  String inner_phone  =  "";                            // Variable for storing a specific number
  if (phone_index  >=  0)                               // If the information was found
  {
    phone_index += 8;                                                                       // Parse the string and 
    inner_phone  =  _response.substring(phone_index, _response.indexOf("\"", phone_index)); // get the number    
  } 
  for(int i = 0; i < counter_admins; i ++ )
  {
    if(inner_phone == phones[i])
    {
      flag_admin = true;
      break;
    }
  }
  if(mode)                                         // If the security mode is enabled
  {
    if(flag_admin)                                 // If the admin calls
    {
      mode = !mode;                                // Change the security mode
      SendATCommand("ATH", true);                  // Reset call
      SendSMS(F("mode OFF"));                      // We send an SMS with the result to the administrator
      return;                                      // And exit 
    }
    else                                           // It's not the admin who's calling
    {
      SendATCommand("ATH", true);                  // Reset call
      return;                                      // And exit
    }
  }
  else
  {
    if(flag_admin)                                 // If the admin calls
    {
      mode = !mode;                                // Change the security mode
      SendATCommand("ATH", true);                  // Reset call
      SendSMS(F("mode ON"));                       // We send an SMS with the result to the administrator
      return;      
    } 
    timer_ring = millis();                         // Turning on the timer to visualize the call                                                                           
    if (inner_phone.length()  >=  7 && digitalRead(BUTTON_PIN) == HIGH)  // If the number length is more than 6 digits and the button on the board is pressed
    {
      phones[0] = inner_phone;
      for(int i = 0; i < 13; i ++ )
      {
        EEPROM.write(i, (byte)phones[0][i]);      // Write the number to the EEPROM memory
      }      
      SendATCommand("ATH", true);                 // Resetting the call
      delay(3000);          
      tone(BUZZER_PIN, 1915);                     // We signal with the speaker about the acceptance of the administrator's number
      delay(1000);
      noTone(6);  
      SendSMS(F("Administrator number accepted!"));  // We send an SMS with the result to the administrator
    }
  }
}

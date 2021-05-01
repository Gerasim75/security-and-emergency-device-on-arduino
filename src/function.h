#ifndef FUNCTION_H
#define FUNCTION_H

/////////////////////////////// Прототипы функций ////////////////////////////////////////

void Call(String & num);
String SendATCommand(String cmd, bool waiting);
String WaitResponse();
void GetSensors();
void AlarmMessages();
void ParseSMS(String & msg);
void SetLedState(String & result, String & msgphone);
void SendSMS(String message);
float GetBalans(String & code);
float GetFloatFromString(String & str);
void GetBalanceSim();
void InitialZones();
void InitialMacros();
void GetVoltage();
void TestModem();
void InitialModem();
void GetNewSMS();
void GetKeyboard();
void ActivateRelay(byte Alarm1, byte Alarm2);
void InitialEeprom();
void SetDingDong();
void GetIncomingCall();

#endif
#ifndef FUNCTION_H
#define FUNCTION_H
 
/////////////////////////////// Прототипы функций ////////////////////////////////////////

void Call(String & num, byte adress);
String SendATCommand(String cmd, bool waiting);
String WaitResponse();
void GetSensors();
void AlarmMessages();
void ParseSMS(String & msg);
void SetLedState(String & result, String & msgphone);
void SetControlledRelay(String & result);
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
void sendSMSinPDU(String message);
void getPDUPack(String *phone, String *message, String *result, int *PDUlen);
String getDAfield(String *phone, bool fullnum);
String UCS2ToString(String s);
unsigned char HexSymbolToChar(char c);
String StringToUCS2(String s);
unsigned int getCharSize(unsigned char b);
unsigned int symbolToUInt(const String& bytes);
String byteToHexString(byte i);
 
#endif
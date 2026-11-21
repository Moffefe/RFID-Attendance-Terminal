/*
TaskUpdateRTC - klasa ktorej zadaniem jest aktualizacja zegara RTC raz na jakis czas
*/
#ifndef TaskUpdateRTC_h
#define TaskUpdateRTC_h

#define DEBUG_TASKUPDATERTC 1
#if DEBUG_TASKUPDATERTC
#define DEBUG_TASKUPDATERTC_ID "13_"

//#include <SerialListener.h>
//#define DEBUG_TASKUPDATERTC_LOG(...) serialListener.Log(__VA_ARGS__)
#define DEBUG_TASKUPDATERTC_LOG(...) Serial1.println(__VA_ARGS__)
#endif

#include <stdint.h>
#include <SemaphoreManager.h>
#include <UpdateManager.h>
#include <MessageReceiver.h>
#include <MessageUsers.h>
#include <TimeKeeper.h>
#include <DataStore.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <RTClib.h>
#include <TimeLib.h>


class TaskUpdateRTC : public UpdateManagerUser, public MessageReceiver
{
public:
	TaskUpdateRTC(MessageReceiver*, RTC_DS3231*);
	void Update();
	bool TakeMessage(int, void*, void**);

private:
	enum State
	{
		idle,				// Czekamy na nastepna aktualizacje
		getTimeZone,			// Pobieramy informacje o strefie czasowej (w ktorej znajduje sie glowny serwer)
		getUTC,						// Pobieramy czas z serwera NTP
		offline							// Aktualizujemy czas za pomoca modulu RTC (aktualizacja online zawiodla)
	};

	struct UpdateInfo
	{
		unsigned long time_1;
		uint8_t data_1;
	};



	MessageReceiver* user;
	RTC_DS3231* rtc;
	uint8_t* buffer;
	uint8_t bufferSize;
	WiFiUDP udp;
	State currentState;
	UpdateInfo updateInfo;
	State nextState;
	uint8_t nextStateInit;



	void SetState(State, uint8_t = 0);
	void SwitchState();
	void IdleProcedure();
	void GetTimeZoneProcedure();
	void GetUTCProcedure();
	void OfflineProcedure();
};

#endif
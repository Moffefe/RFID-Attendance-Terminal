/*
TaskSync - klasa ktorej zadaniem jest synchronizacja danych
		   na rzecz klasy DataStore
*/
#ifndef TaskSync_h
#define TaskSync_h

#define DEBUG_TASKSYNC 0
#if DEBUG_TASKSYNC
#define DEBUG_TASKSYNC_ID "10_"

//#include <SerialListener.h>
//#define DEBUG_TASKSYNC_LOG(...) serialListener.Log(__VA_ARGS__)
#define DEBUG_TASKSYNC_LOG(...) Serial1.println(__VA_ARGS__)
#endif

#include <stdint.h>
#include <SemaphoreManager.h>
#include <MessageReceiver.h>
#include <MessageUsers.h>
#include <TimeKeeper.h>
#include <DataStore.h>
#include <asyncHTTPrequest.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <Display145617.h>


class TaskSync : public UpdateManagerUser, public MessageReceiver
{
public:
	TaskSync(Display145617* = nullptr);
	void Update();
	bool TakeMessage(int, void*, void**);

private:
	enum State
	{
		idle,
		connect,		// Rozpoczynamy polaczenie z serwerem
		pullTerminal,		// Sciagamy rekordy do tabeli 'terminal'
		pullEmployee,			// Sciagamy rekordy do tabeli 'employee'
		pullEventType,				// Sciagamy rekordy do tabeli 'eventType'
		pushEvent,						// Wysylamy rekordy z tabeli 'event'
		finish								// Informujemy serwer, ze proces aktualizacji zakonczyl sie pomyslnie
	};

	struct UpdateInfo
	{
		uint8_t data_1;
	};



	uint32_t recordIndex;		// Ile rekordow (w aktualnie synchronizowanej tablicy) zostalo juz przeslanych (indeks nastepnego rekordu ktory bedzie przesylany)
	uint8_t tablesToSync;			// Jakie tabele musza zostac zsynchronizowane (bit 0 - terminal, bit 1 - employee, bit 2 - eventType)
	Display145617* display;				// Podczas synchronizacji tabeli 'employee' pojawi sie napis na wyswietlaczu
	asyncHTTPrequest* request;				// Pozwala na asynchroniczna prace z protokolem HTTP
	DataStore::Event::Record* eventRecords;		// Uzywane w stanie pushEvent do przetrzymywania rekordow, ktore zostaly wyslane do serwera (maksymalnie 10)
	uint8_t eventRecordsCount;						// Uzywane w stanie pushEvent, mowi ile dokladnie rekordow zostalo wyslanych do serwera
	State currentState;
	UpdateInfo updateInfo;
	State nextState;
	uint8_t nextStateInit;



	void SetState(State, uint8_t = 0);
	void SwitchState();
	void IdleProcedure();
	void ConnectProcedure();
	void PullTerminalProcedure();
	void PullEmployeeProcedure();
	void PullEventTypeProcedure();
	void PushEventProcedure();
	void FinishProcedure();
	void GetServerAddress(String&);
};

#endif
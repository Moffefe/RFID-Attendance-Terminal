/*
EmployeeManager - klasa ktorej zadaniem jest obsluzenie pracownika
				  korzystajacego z terminala
*/
#ifndef EmployeeManager_h
#define EmployeeManager_h

#define DEBUG_EMPLOYEEMANAGER 1
#if DEBUG_EMPLOYEEMANAGER
#define DEBUG_EMPLOYEEMANAGER_ID "14_"

//#include <SerialListener.h>
//#define DEBUG_EMPLOYEEMANAGER_LOG(...) serialListener.Log(__VA_ARGS__)
#define DEBUG_EMPLOYEEMANAGER_LOG(...) Serial1.println(__VA_ARGS__)
#endif

#include <stdint.h>
#include <SemaphoreManager.h>
#include <MessageReceiver.h>
#include <MessageUsers.h>
#include <Display145617.h>
#include <DataStore.h>
#include <SerialListener.h>
#include <SerialUsers.h>
#include <Keypad145617Buttons.h>
#include <Hash.h>
#include <ESP8266WiFi.h>


class EmployeeManager : public UpdateManagerUser, public MessageReceiver
{
public:
	EmployeeManager(MessageReceiver*, Display145617*);
	void Update();
	bool TakeMessage(int, void*, void**);

private:
	enum State
	{
		idle,					// Nie obslugujemy w tym momencie zadnego pracownika
		authenticateRFID,			// Pracownik skanuje swoj tag RFID
		authenticateNumpad,				// Pracownik wprowadza swoj kod
		chooseEventType,					// Pracownik wybiera rodzaj zdarzenia RCP
		success,								// Wyswietlamy okno powodzenia
		failure,									// Wyswietlamy okno niepowodzenia
		tagScan,										// Skanujemy tag RFID
		status												// Wyswietlamy status terminala
	};

	struct UpdateInfo
	{
		unsigned long time_1;
		uint8_t data_1;
	};



	MessageReceiver* mainController;
	Display145617* display;
	String passCode;			//
	String passCodeHash;		// Do przechowywania danych
	String tagCodeHash;			// aktualnie obslugiwanego pracownika
	uint32_t employeeId;		//
	uint32_t eventTypeId;		//
	DataStore::EventType::Record* eventTypeRecords;	// Potrzebne do utworzenia ekranu z wyborem
	uint8_t eventTypeCount;							// zdarzenia RCP
	uint8_t selectedEventType;						//
	State currentState;
	UpdateInfo updateInfo;
	State nextState;
	uint8_t nextStateInit;



	void SetState(State, uint8_t = 0);
	void SwitchState();
	void IdleProcedure();
	void AuthenticateRFIDProcedure();
	void authenticateNumpadProcedure();
	void ChooseEventTypeProcedure();
	void SuccessProcedure();
	void FailureProcedure();
	void TagScanProcedure();
	void StatusProcedure();
	bool GetCharacterFromButtonCode(uint16_t, char&);
	void CreatePasswordInput(String&);
};

#endif
/*
MainController2 - glowny kontroler w projekcie.
				  Zarzadza procesem uruchamiania, oraz steruje innymi klasami
				  na ATmega328P
*/
#ifndef MainController2_h
#define MainController2_h

#define DEBUG_MAINCONTROLLER2 1
#if DEBUG_MAINCONTROLLER2
#define DEBUG_MAINCONTROLLER2_ID "15_"

//#include <SerialListener.h>
//#define DEBUG_MAINCONTROLLER2_LOG(...) serialListener.Log(__VA_ARGS__)
#define DEBUG_MAINCONTROLLER2_LOG(...) Serial.println(__VA_ARGS__)
#endif

#include <stdint.h>
#include <UpdateManager.h>
#include <SerialListener.h>
#include <SerialUsers.h>
#include <Keypad145617.h>
#include <RFID145617.h>


class MainController2 : public UpdateManagerUser
{
public:
	MainController2();
	void Update();

private:
	enum State
	{
		wait,
		initSerialListener,
		initKeypad,
		initSPI,
		initRFID
	};

	struct UpdateInfo
	{
		unsigned long time_1;
		uint8_t data_1;
	};



	Keypad145617* keypad;
	RFID145617* rfid;
	State currentState;
	UpdateInfo updateInfo;
	State nextState;
	uint8_t nextStateInit;



	void SetState(State, uint8_t = 0);
	void SwitchState();
	void WaitProcedure();
	void InitSerialListenerProcedure();
	void InitKeypadProcedure();
	void InitSPIProcedure();
	void InitRFIDProcedure();
};

#endif
/*
MainController145617 - glowny kontroler w projekcie.
					   Zarzadza procesem uruchamiania, oraz steruje innymi klasami
*/
#ifndef MainController145617_h
#define MainController145617_h

#define DEBUG_MAINCONTROLLER145617 1
#if DEBUG_MAINCONTROLLER145617
#define DEBUG_MAINCONTROLLER145617_ID "12_"

//#include <SerialListener.h>
//#define DEBUG_MAINCONTROLLER145617_LOG(...) serialListener.Log(__VA_ARGS__)
#define DEBUG_MAINCONTROLLER145617_LOG(...) Serial1.println(__VA_ARGS__)
#endif

#include <stdint.h>
#include <UpdateManager.h>
#include <MessageReceiver.h>
#include <MessageUsers.h>
#include <TimeKeeper.h>
#include <Display145617.h>
#include <DataStore.h>
#include <TaskConnect.h>
#include <TaskUpdateRTC.h>
#include <SerialListener.h>
#include <TaskSync.h>
#include <WebServer145617.h>
#include <EmployeeManager.h>


class MainController145617 : public UpdateManagerUser, public MessageReceiver
{
public:
	MainController145617();
	void Update();
	bool TakeMessage(int, void*, void**);

private:
	enum State
	{
		wait,
		initTimeKeeper,
		initDisplay,
		initSdFat,
		memoryCleanup,
		initDataStore,
		initTaskConnect,
		setTime,
		initSerialListener,
		initTaskSync,
		initWebServer,
		initEmployeeManager,
		restart,
		halt
	};

	struct UpdateInfo
	{
		unsigned long time_1;
		uint8_t data_1;
	};



	Display145617 display;
	SdFat sdFat;
	RTC_DS3231 rtc;
	State currentState;
	UpdateInfo updateInfo;
	State nextState;
	uint8_t nextStateInit;



	void SetState(State, uint8_t = 0);
	void SwitchState();
	void WaitProcedure();
	void InitTimeKeeperProcedure();
	void InitDisplayProcedure();
	void InitSdFatProcedure();
	void MemoryCleanupProcedure();
	void InitDataStoreProcedure();
	void InitTaskConnectProcedure();
	void SetTimeProcedure();
	void InitSerialListenerProcedure();
	void InitTaskSyncProcedure();
	void InitWebServerProcedure();
	void InitEmployeeManagerProcedure();
	void RestartProcedure();
	void HaltProcedure();
	void CloseMassMemory();
};

#endif
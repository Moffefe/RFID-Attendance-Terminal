/*
TaskConnect - klasa ktorej zadaniem jest polaczenie z siecia WiFi
*/
#ifndef TaskConnect_h
#define TaskConnect_h

#define DEBUG_TASKCONNECT 1
#if DEBUG_TASKCONNECT
#define DEBUG_TASKCONNECT_ID "11_"

//#include <SerialListener.h>
//#define DEBUG_TASKCONNECT_LOG(...) serialListener.Log(__VA_ARGS__)
#define DEBUG_TASKCONNECT_LOG(...) Serial1.println(__VA_ARGS__)
#endif

#include <stdint.h>
#include <UpdateManager.h>
#include <MessageReceiver.h>
#include <MessageUsers.h>
#include <TimeKeeper.h>
#include <ESP8266WiFi.h>
#include <DataStore.h>


class TaskConnect : public UpdateManagerUser
{
public:
	TaskConnect(MessageReceiver*);
	void Update();

private:
	enum State
	{
		disconnect,
		connectAp,
		createAp,
		end
	};

	struct UpdateInfo
	{
		unsigned long time_1;
		uint8_t data_1;
	};



	MessageReceiver* user;
	State currentState;
	UpdateInfo updateInfo;
	State nextState;
	uint8_t nextStateInit;



	void SetState(State, uint8_t = 0);
	void SwitchState();
	void DisconnectProcedure();
	void ConnectApProcedure();
	void CreateApProcedure();
	void EndProcedure();
};

#endif
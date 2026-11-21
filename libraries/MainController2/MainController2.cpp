#include "Arduino.h"
#include "MainController2.h"



MainController2::MainController2()
{
	keypad = NULL;
	rfid = NULL;
	SetState(State::wait);
}

void MainController2::Update()
{
	SwitchState();

	switch (currentState)
	{
		case State::wait:
			WaitProcedure();
			break;

		case State::initSerialListener:
			InitSerialListenerProcedure();
			break;

		case State::initKeypad:
			InitKeypadProcedure();
			break;

		case State::initSPI:
			InitSPIProcedure();
			break;

		case State::initRFID:
			InitRFIDProcedure();
			break;
	}
}

void MainController2::SetState(State newState, uint8_t initializationData)
{
	nextState = newState;
	nextStateInit = bit(sizeof(initializationData) * 8 - 1) | initializationData;
	updateManager.Attach(this);

	#if DEBUG_MAINCONTROLLER2
	String log;

	switch (nextState)
	{
		case State::wait:
			log += F(DEBUG_MAINCONTROLLER2_ID "51");
			break;

		case State::initSerialListener:
			log += F(DEBUG_MAINCONTROLLER2_ID "52");
			break;

		case State::initKeypad:
			log += F(DEBUG_MAINCONTROLLER2_ID "53");
			break;

		case State::initSPI:
			log += F(DEBUG_MAINCONTROLLER2_ID "54");
			break;

		case State::initRFID:
			log += F(DEBUG_MAINCONTROLLER2_ID "55");
			break;
	}

	DEBUG_MAINCONTROLLER2_LOG(log);
	#endif
}

void MainController2::SwitchState()
{
	if (nextStateInit)
	{
		currentState = nextState;
		updateInfo.data_1 = nextStateInit;

		nextStateInit = 0;
	}
}

void MainController2::WaitProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		updateInfo.time_1 = millis() + 1000;

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}

	if ((long)(millis() - updateInfo.time_1) >= 0)
		SetState(State::initSerialListener);
}

void MainController2::InitSerialListenerProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		serialListener.begin();
		updateInfo.time_1 = millis() + 100;

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}

	if ((long)(millis() - updateInfo.time_1) >= 0)
		SetState(State::initKeypad);
}

void MainController2::InitKeypadProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		Keypad145617::Configuration configuration;
		configuration.user = SERIAL_EMPLOYEEMANAGER;
		configuration.groupNumber = 4;
		configuration.sensorNumber = 4;
		configuration.groupPins = new uint8_t[4];
		configuration.groupPins[0] = A5;
		configuration.groupPins[1] = A4;
		configuration.groupPins[2] = A3;
		configuration.groupPins[3] = A2;
		configuration.sensorPins = new uint8_t[4];
		configuration.sensorPins[0] = 4;
		configuration.sensorPins[1] = 5;
		configuration.sensorPins[2] = 6;
		configuration.sensorPins[3] = 7;
		configuration.scanDuration = 50;
		keypad = new Keypad145617(&configuration);
		keypad->begin();
		delete[] configuration.groupPins;
		delete[] configuration.sensorPins;
		updateInfo.time_1 = millis() + 100;

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}

	if ((long)(millis() - updateInfo.time_1) >= 0)
		SetState(State::initSPI);
}

void MainController2::InitSPIProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		SPI.begin();
		updateInfo.time_1 = millis() + 100;

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}

	if ((long)(millis() - updateInfo.time_1) >= 0)
		SetState(State::initRFID);
}

void MainController2::InitRFIDProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		RFID145617::Configuration configuration;
		configuration.user = SERIAL_EMPLOYEEMANAGER;
		configuration.scanDuration = 200;
		configuration.rstPin = 8;
		configuration.ssPin = 9;
		rfid = new RFID145617(&configuration);
		rfid->begin();
		updateManager.Detach(this);

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}
}

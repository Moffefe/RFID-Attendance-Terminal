#include "Arduino.h"
#include "TaskConnect.h"



TaskConnect::TaskConnect(MessageReceiver* user) :
	user(user)
{
	SetState(State::disconnect);
}

void TaskConnect::Update()
{
	SwitchState();

	switch (currentState)
	{
		case State::disconnect:
			DisconnectProcedure();
			break;

		case State::connectAp:
			ConnectApProcedure();
			break;

		case State::createAp:
			CreateApProcedure();
			break;

		case State::end:
			EndProcedure();
			break;
	}
}

void TaskConnect::SetState(State newState, uint8_t initializationData)
{
	nextState = newState;
	nextStateInit = bit(sizeof(initializationData) * 8 - 1) | initializationData;
	updateManager.Attach(this);

	#if DEBUG_TASKCONNECT
	String log;

	switch (nextState)
	{
		case State::disconnect:
			log += F(DEBUG_TASKCONNECT_ID "51");
			break;

		case State::connectAp:
			log += F(DEBUG_TASKCONNECT_ID "52");
			break;

		case State::createAp:
			log += F(DEBUG_TASKCONNECT_ID "53");
			break;

		case State::end:
			log += F(DEBUG_TASKCONNECT_ID "54");
			break;
	}

	DEBUG_TASKCONNECT_LOG(log);
	#endif
}

void TaskConnect::SwitchState()
{
	if (nextStateInit)
	{
		currentState = nextState;
		updateInfo.data_1 = nextStateInit;

		nextStateInit = 0;
	}
}

void TaskConnect::DisconnectProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		WiFi.mode(WIFI_OFF);
		updateInfo.time_1 = millis() + 2000;

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}

	if ((long)(millis() - updateInfo.time_1) >= 0)
	{
		if (dataStore.terminalRecord.ssid.length() > 0 && dataStore.terminalRecord.password.length() > 0)
			SetState(State::connectAp);
		else
			SetState(State::createAp);
	}
}

void TaskConnect::ConnectApProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		WiFi.mode(WIFI_STA);
		if (dataStore.terminalRecord.useStaticIp
			&&
			dataStore.terminalRecord.staticIp.length()
			&&
			dataStore.terminalRecord.staticGateway.length()
			&&
			dataStore.terminalRecord.staticSubnet.length()
			&&
			dataStore.terminalRecord.staticDNS.length()
			)
		{
			IPAddress ip(dataStore.terminalRecord.staticIp[0], dataStore.terminalRecord.staticIp[1], dataStore.terminalRecord.staticIp[2], dataStore.terminalRecord.staticIp[3]);
			IPAddress gateway(dataStore.terminalRecord.staticGateway[0], dataStore.terminalRecord.staticGateway[1], dataStore.terminalRecord.staticGateway[2], dataStore.terminalRecord.staticGateway[3]);
			IPAddress subnet(dataStore.terminalRecord.staticSubnet[0], dataStore.terminalRecord.staticSubnet[1], dataStore.terminalRecord.staticSubnet[2], dataStore.terminalRecord.staticSubnet[3]);
			IPAddress dns(dataStore.terminalRecord.staticDNS[0], dataStore.terminalRecord.staticDNS[1], dataStore.terminalRecord.staticDNS[2], dataStore.terminalRecord.staticDNS[3]);
			WiFi.config(ip, gateway, subnet, dns);
		}
		WiFi.begin(dataStore.terminalRecord.ssid.c_str(), dataStore.terminalRecord.password.c_str());
		updateInfo.data_1 = 60;
		updateInfo.time_1 = millis() + 500;

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}

	if ((long)(millis() - updateInfo.time_1) >= 0)
	{
		if (WiFi.status() == WL_CONNECTED)
		{
			#if DEBUG_TASKCONNECT							//
			String log;										//
			log += F(DEBUG_TASKCONNECT_ID "201\x1F");		//
			log += WiFi.SSID();								// Udalo sie polaczyc z AP
			log += '\x1F';									//
			log += WiFi.localIP().toString();				//
			DEBUG_TASKCONNECT_LOG(log);						//
			#endif											//

			uint8_t* additionalData = new uint8_t;
			*additionalData = 1;
			user->TakeMessage(MESSAGE_TASK_CONNECT_END, this, (void**)additionalData);
			SetState(State::end);
		}
		else
		{
			--updateInfo.data_1;
			if (updateInfo.data_1)
			{
				updateInfo.time_1 = millis() + 500;
			}
			else
			{
				#if DEBUG_TASKCONNECT					//
				String log;								//
				log += F(DEBUG_TASKCONNECT_ID "202");	// Nie udalo sie polaczyc z AP
				DEBUG_TASKCONNECT_LOG(log);				//
				#endif									//

				uint8_t* additionalData = new uint8_t;
				*additionalData = 2;
				user->TakeMessage(MESSAGE_TASK_CONNECT_END, this, (void**)additionalData);
				SetState(State::end);
			}
		}
	}
}

void TaskConnect::CreateApProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		String tempString_1;
		tempString_1 += F("RFID_SETUP_AP");
		WiFi.mode(WIFI_AP_STA);
		WiFi.softAP(tempString_1.c_str(), "SET_AP_PASSWORD", dataStore.terminalRecord.apChannel);
		updateInfo.time_1 = millis() + 2000;

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}

	if ((long)(millis() - updateInfo.time_1) >= 0)
	{
		#if DEBUG_TASKCONNECT							//
		String log;										//
		log += F(DEBUG_TASKCONNECT_ID "301\x1F");		// Utworzono wlasne AP
		log += WiFi.channel();							//
		DEBUG_TASKCONNECT_LOG(log);						//
		#endif											//

		uint8_t* additionalData = new uint8_t;
		*additionalData = 0;
		user->TakeMessage(MESSAGE_TASK_CONNECT_END, this, (void**)additionalData);
		SetState(State::end);
	}
}

void TaskConnect::EndProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		updateManager.Detach(this);
		timeKeeper.RequestDelete(millis() + 100, this);
		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}
}

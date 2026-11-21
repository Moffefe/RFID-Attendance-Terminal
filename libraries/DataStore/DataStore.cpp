#include "Arduino.h"
#include "DataStore.h"



DataStore dataStore;



DataStore::DataStore()
{
	synchronizing = false;
}

void DataStore::begin()
{
	if (semaphoreManager.TryAcquire(SemaphoreManager::Operation::SD_mainLoopWrite))
	{
		PrepareMassMemory();

		terminal = new Terminal();
		employee = new Employee();
		eventType = new EventType();
		event = new Event();

		if (terminal->RecordCount())
		{
			terminal->GetFromTableAll(&terminalRecord, 1);
		}
		else
		{
			terminalRecord.id = terminal->GetNextFreePK();

			terminalRecord.name = String();
			terminalRecord.areaName = String();
			terminalRecord.passageName = String();

			terminalRecord.ssid += F("YOUR_WIFI_SSID");
			terminalRecord.password += F("YOUR_WIFI_PASSWORD");
			terminalRecord.useStaticIp = 0;
			terminalRecord.staticIp = String();
			terminalRecord.staticGateway = String();
			terminalRecord.staticSubnet = String();
			terminalRecord.staticDNS = String();
			terminalRecord.connectRetries = 3;
			terminalRecord.connectRetriesCurrent = 0;
			terminalRecord.apChannel = 6;
			terminalRecord.username += F("SET_LOCAL_USER");
			terminalRecord.userPassword += F("SET_LOCAL_PW");

			terminalRecord.privateKey = String();
			terminalRecord.publicKey += F("SET_TERMINAL_KEY");
			terminalRecord.serverAddress = String();
			terminalRecord.serverDomain += F("example.invalid");
			terminalRecord.serverPublicKey += F("SET_SERVER_KEY");
			terminalRecord.syncInterval = 5000;

			terminalRecord.authenticateMethod = 0;

			terminalRecord.timeZoneOffset = 0;
			terminalRecord.timeZoneChange = UINT32_MAX;
			terminalRecord.timeZoneOffsetNext = 0;

			terminal->PutInTable(NULL, NULL, &terminalRecord);
		}

		semaphoreManager.Release(SemaphoreManager::Operation::SD_mainLoopWrite);
	}
}

bool DataStore::TakeMessage(int message, void* sender, void** additionalData)
{
	switch (message)
	{
		case MESSAGE_TASK_SYNC_BEGIN:
		{
			synchronizing = true;
			return true;
		}

		case MESSAGE_TASK_SYNC_END:
		{
			synchronizing = false;
			return true;
		}
	}

	return false;
}

bool DataStore::GetSynchronizing()
{
	return synchronizing;
}

// Jej zadaniem jest przygotowanie wszystkich
// folderow potrzebnych do dzialania tej klasy
void DataStore::PrepareMassMemory()
{
	File file;

	if (!file.open("/DataStore", O_READ))
		file.mkdir(File::cwd(), "/DataStore");
	file.close();
}
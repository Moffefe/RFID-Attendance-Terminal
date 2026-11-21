#include "Arduino.h"
#include "TaskUpdateRTC.h"



TaskUpdateRTC::TaskUpdateRTC(MessageReceiver* user, RTC_DS3231* rtc) :
	user(user),
	rtc(rtc),
	buffer(NULL),
	bufferSize(48)
{
	SetState(State::idle , 1);
}

void TaskUpdateRTC::Update()
{
	SwitchState();

	switch (currentState)
	{
		case State::idle:
			IdleProcedure();
			break;

		case State::getTimeZone:
			GetTimeZoneProcedure();
			break;

		case State::getUTC:
			GetUTCProcedure();
			break;

		case State::offline:
			OfflineProcedure();
			break;
	}
}

bool TaskUpdateRTC::TakeMessage(int message, void* sender, void** additionalData)
{
	switch (message)
	{
		case MESSAGE_TIME_KEEPER:
		{
			switch (*((uint16_t*)additionalData))
			{
				case MESSAGE_TASK_UPDATE_RTC_BEGIN:		//
				{										//
					SetState(State::getTimeZone);		// Rozpoczynamy aktualizacje
					break;								//
				}										//

				case MESSAGE_TASK_UPDATE_RTC_TIME_ZONE_CHANGE:
				{
					time_t unixTime = now() - dataStore.terminalRecord.timeZoneOffset;			// Cofamy/przyspieszamy czas
					time_t localTime = unixTime + dataStore.terminalRecord.timeZoneOffsetNext;	// o zaplanowana wartosc

					setTime(localTime);					// Synchronizujemy czas systemowy (biblioteka Time.h),
					rtc->adjust(DateTime(localTime));	// oraz modul RTC

					#if DEBUG_TASKUPDATERTC
					time_t t = now();
					String log;
					log += F(DEBUG_TASKUPDATERTC_ID "11\x1F");
					log += year(t);
					log += '-';
					log += month(t);
					log += '-';
					log += day(t);
					log += 'T';
					log += hour(t);
					log += ':';
					log += minute(t);
					log += ':';
					log += second(t);
					DEBUG_TASKUPDATERTC_LOG(log);
					#endif

					dataStore.terminalRecord.timeZoneOffset = dataStore.terminalRecord.timeZoneOffsetNext;
					dataStore.terminalRecord.timeZoneChange = UINT32_MAX;
					dataStore.terminalRecord.timeZoneOffsetNext = 0;
					if (semaphoreManager.TryAcquire(SemaphoreManager::Operation::SD_mainLoopWrite))
					{
						dataStore.terminal->RemoveFromTableAll();
						dataStore.terminal->PutInTable(NULL, NULL, &dataStore.terminalRecord);
						semaphoreManager.Release(SemaphoreManager::Operation::SD_mainLoopWrite);
					}

					break;
				}
			}

			delete ((uint16_t*)additionalData);
			return true;
		}
	}

	return false;
}

void TaskUpdateRTC::SetState(State newState, uint8_t initializationData)
{
	nextState = newState;
	nextStateInit = bit(sizeof(initializationData) * 8 - 1) | initializationData;
	updateManager.Attach(this);

	#if DEBUG_TASKUPDATERTC
	String log;

	switch (nextState)
	{
		case State::idle:
			log += F(DEBUG_TASKUPDATERTC_ID "51");
			break;

		case State::getTimeZone:
			log += F(DEBUG_TASKUPDATERTC_ID "52");
			break;

		case State::getUTC:
			log += F(DEBUG_TASKUPDATERTC_ID "53");
			break;

		case State::offline:
			log += F(DEBUG_TASKUPDATERTC_ID "54");
			break;
	}

	DEBUG_TASKUPDATERTC_LOG(log);
	#endif
}

void TaskUpdateRTC::SwitchState()
{
	if (nextStateInit)
	{
		currentState = nextState;
		updateInfo.data_1 = nextStateInit;

		nextStateInit = 0;
	}
}

void TaskUpdateRTC::IdleProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		unsigned long updateTime;

		if (bitRead(updateInfo.data_1, 0))		//
		{										// Gdy bit 0 jest ustawiony, to rozpoczynamy
			updateTime = millis();				// aktualizacje natychmiast
		}										//
		else
		{
			if ( dataStore.terminalRecord.timeZoneChange < (now() + 432000) )											//
			{																											//
				unsigned long changeTime = millis() + ( (dataStore.terminalRecord.timeZoneChange - now()) * 1000 );		//
				uint16_t* subMessage = new uint16_t();																	// Jesli zbliza sie zmiana czasu (mniej niz za 5 dni)
				*subMessage = MESSAGE_TASK_UPDATE_RTC_TIME_ZONE_CHANGE;													// to przygotowujemy sie do jej obsluzenia, oraz
				timeKeeper.RequestMessage(changeTime, this, (void**)subMessage);										// przesuwamy aktualizacje czasu za 6 dni
																														//
				updateTime = millis() + 518400000;																		//
			}																											//
			else									//
			{										// Nastepna aktualizacje czasu
				updateTime = millis() + 345600000;	// planujemy za 4 dni
			}										//
		}

		uint16_t* subMessage = new uint16_t();
		*subMessage = MESSAGE_TASK_UPDATE_RTC_BEGIN;
		timeKeeper.RequestMessage(updateTime, this, (void**)subMessage);

		updateManager.Detach(this);

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}
}

void TaskUpdateRTC::GetTimeZoneProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		if (
			(dataStore.terminalRecord.serverAddress.length() || dataStore.terminalRecord.serverDomain.length())
			&&
			(dataStore.terminalRecord.publicKey.length() && dataStore.terminalRecord.serverPublicKey.length())
			&&
			(WiFi.status() == WL_CONNECTED)
		   )
		{
			String tempString_1;
			String tempString_2;
			bool error = false;

			tempString_1 += F("http://");
			tempString_1 += dataStore.terminalRecord.serverDomain;
			tempString_1 += F("/get-time-zone.php");

			HTTPClient http;
			http.begin(tempString_1);
			tempString_1 = String();
			tempString_1 += F("Content-Type");
			tempString_2 += F("application/x-www-form-urlencoded");
			http.addHeader(tempString_1.c_str(), tempString_2.c_str());

			tempString_1 = String();
			tempString_1 += F("publicKey=");
			tempString_1 += dataStore.terminalRecord.publicKey;

			if (http.POST(tempString_1) == 200)
			{
				StaticJsonBuffer<256> jsonBuffer;
				JsonObject& jsonObject = jsonBuffer.parseObject(http.getString());

				if (jsonObject.success())
				{
					if (jsonObject[F("publicKey")] == dataStore.terminalRecord.serverPublicKey)
					{
						dataStore.terminalRecord.timeZoneOffset = jsonObject.get<int32_t>(F("timeZoneOffset"));
						dataStore.terminalRecord.timeZoneChange = jsonObject.get<uint32_t>(F("timeZoneChange"));
						dataStore.terminalRecord.timeZoneOffsetNext = jsonObject.get<int32_t>(F("timeZoneOffsetNext"));

						if (semaphoreManager.TryAcquire(SemaphoreManager::Operation::SD_mainLoopWrite))
						{
							dataStore.terminal->RemoveFromTableAll();
							dataStore.terminal->PutInTable(NULL, NULL, &dataStore.terminalRecord);
							semaphoreManager.Release(SemaphoreManager::Operation::SD_mainLoopWrite);
						}

						#if DEBUG_TASKUPDATERTC
						String log;
						log += F(DEBUG_TASKUPDATERTC_ID "201\x1F");
						log += dataStore.terminalRecord.timeZoneOffset;
						log += '\x1F';
						log += dataStore.terminalRecord.timeZoneChange;
						log += '\x1F';
						log += dataStore.terminalRecord.timeZoneOffsetNext;
						DEBUG_TASKUPDATERTC_LOG(log);
						#endif
					}
					else										//
					{											//
						#if DEBUG_TASKUPDATERTC					//
						String log;								//
						log += F(DEBUG_TASKUPDATERTC_ID "202");	// Nieprawidlowy klucz serwera
						DEBUG_TASKUPDATERTC_LOG(log);			//
						#endif									//
																//
						error = true;							//
					}											//
				}
				else										//
				{											//
					#if DEBUG_TASKUPDATERTC					//
					String log;								//
					log += F(DEBUG_TASKUPDATERTC_ID "203");	// Blad serwera
					DEBUG_TASKUPDATERTC_LOG(log);			//
					#endif									//
															//
					error = true;							//
				}											//
			}
			else										//
			{											//
				#if DEBUG_TASKUPDATERTC					//
				String log;								//
				log += F(DEBUG_TASKUPDATERTC_ID "204");	// Nieudane polaczenie
				DEBUG_TASKUPDATERTC_LOG(log);			//
				#endif									//
														//
				error = true;							//
			}											//

			http.end();

			if (error)						//
				SetState(State::offline);	// W razie jakiegokolwiek bledu
			else							// przechodzimy do stanu offline
				SetState(State::getUTC);	//
		}
		else
		{
			SetState(State::offline);
		}

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}
}

void TaskUpdateRTC::GetUTCProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		String tempString_1;

		buffer = new uint8_t[bufferSize];	//
		memset(buffer, 0, bufferSize);		//
		buffer[0] = 0b11100011;				//
		buffer[1] = 0;						//
		buffer[2] = 6;						// Inicjujemy 'buffer'
		buffer[3] = 0xEC;					//
		buffer[12] = 49;					//
		buffer[13] = 0x4E;					//
		buffer[14] = 49;					//
		buffer[15] = 52;					//
		
		tempString_1 += F("tempus1.gum.gov.pl");				//
		IPAddress timeServerIP;									//
		WiFi.hostByName(tempString_1.c_str(), timeServerIP);	//
		udp.begin(2390);										// Inicjujemy 'udp'
		udp.beginPacket(timeServerIP, 123);						//
		udp.write(buffer, bufferSize);							//
		udp.endPacket();										//

		updateInfo.time_1 = millis() + 5000;		// Ustawiamy timeout

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}

	int cb = udp.parsePacket();
	if (cb)
	{
		udp.read(buffer, bufferSize);													//
		unsigned long highWord = word(buffer[40], buffer[41]);							// Odczytujemy odpowiedz serwera NTP
		unsigned long lowWord = word(buffer[42], buffer[43]);							// i na jej podstawie otrzymujemy
		unsigned long utcTime = highWord << 16 | lowWord;								// 'utcTime' i 'unixTime'
		unsigned long unixTime = utcTime - 2208988800UL;								//

		unsigned long localTime = unixTime + dataStore.terminalRecord.timeZoneOffset;	// Synchronizujemy czas systemowy (biblioteka Time.h),
		setTime(localTime);																// oraz modul RTC
		rtc->adjust(DateTime(localTime));												//
		
		#if DEBUG_TASKUPDATERTC
		time_t t = now();
		String log;
		log += F(DEBUG_TASKUPDATERTC_ID "301\x1F");
		log += year(t);
		log += '-';
		log += month(t);
		log += '-';
		log += day(t);
		log += 'T';
		log += hour(t);
		log += ':';
		log += minute(t);
		log += ':';
		log += second(t);
		DEBUG_TASKUPDATERTC_LOG(log);
		#endif
		
		udp.stop();			//
		delete[] buffer;	// Sprzatanie
		buffer = NULL;		//
		
		uint8_t* additionalData = new uint8_t;											// Przekazujemy uzytkownikowi, ze udalo sie
		*additionalData = 0;															// zsynchronizowac czas systemowy z serwerem
		user->TakeMessage(MESSAGE_TASK_UPDATE_RTC_END, this, (void**)additionalData);	// NTP

		SetState(State::idle);
	}
	else
	{
		if ((long)(millis() - updateInfo.time_1) >= 0)
		{
			udp.stop();			//
			delete[] buffer;	// Sprzatanie
			buffer = NULL;		//

			SetState(State::offline);
		}
	}
}

void TaskUpdateRTC::OfflineProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		if (rtc->lostPower())																//
		{																					//
			#if DEBUG_TASKUPDATERTC															//
			String log;																		//
			log += F(DEBUG_TASKUPDATERTC_ID "401");											//
			DEBUG_TASKUPDATERTC_LOG(log);													// Nieudalo sie pobrac czasu ani z serwera NTP,
			#endif																			// ani z modulu RTC
																							//
			uint8_t* additionalData = new uint8_t;											//
			*additionalData = 1;															//
			user->TakeMessage(MESSAGE_TASK_UPDATE_RTC_END, this, (void**)additionalData);	//
		}																					//
		else
		{
			setTime(rtc->now().unixtime());		// Synchronizujemy czas systemowy (biblioteka Time.h) za pomoca modulu RTC
		
			#if DEBUG_TASKUPDATERTC
			time_t t = now();
			String log;
			log += F(DEBUG_TASKUPDATERTC_ID "402\x1F");
			log += year(t);
			log += '-';
			log += month(t);
			log += '-';
			log += day(t);
			log += 'T';
			log += hour(t);
			log += ':';
			log += minute(t);
			log += ':';
			log += second(t);
			DEBUG_TASKUPDATERTC_LOG(log);
			#endif
		
			uint8_t* additionalData = new uint8_t;											// Przekazujemy uzytkownikowi, ze udalo sie
			*additionalData = 2;															// zsynchronizowac czas systemowy za pomoca
			user->TakeMessage(MESSAGE_TASK_UPDATE_RTC_END, this, (void**)additionalData);	// modulu RTC
		}

		SetState(State::idle);

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}
}
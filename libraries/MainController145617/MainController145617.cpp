#include "Arduino.h"
#include "MainController145617.h"



MainController145617::MainController145617()
{
	SetState(State::wait);
}

void MainController145617::Update()
{
	SwitchState();

	switch (currentState)
	{
		case State::wait:
			WaitProcedure();
			break;

		case State::initTimeKeeper:
			InitTimeKeeperProcedure();
			break;

		case State::initDisplay:
			InitDisplayProcedure();
			break;

		case State::initSdFat:
			InitSdFatProcedure();
			break;

		case State::memoryCleanup:
			MemoryCleanupProcedure();
			break;

		case State::initDataStore:
			InitDataStoreProcedure();
			break;

		case State::initTaskConnect:
			InitTaskConnectProcedure();
			break;

		case State::setTime:
			SetTimeProcedure();
			break;

		case State::initSerialListener:
			InitSerialListenerProcedure();
			break;

		case State::initTaskSync:
			InitTaskSyncProcedure();
			break;

		case State::initWebServer:
			InitWebServerProcedure();
			break;

		case State::initEmployeeManager:
			InitEmployeeManagerProcedure();
			break;

		case State::restart:
			RestartProcedure();
			break;

		case State::halt:
			HaltProcedure();
			break;
	}
}

bool MainController145617::TakeMessage(int message, void* sender, void** additionalData)
{
	switch (message)
	{
		case MESSAGE_TIME_KEEPER:
		{
			switch (*((uint16_t*)additionalData))
			{
				case MESSAGE_CHECK_WIFI_CONNECTION:													//
				{																					//
					if (WiFi.status() == WL_CONNECTED)												//
					{																				//
						uint16_t* subMessage = new uint16_t();										//
						*subMessage = MESSAGE_CHECK_WIFI_CONNECTION;								//
						timeKeeper.RequestMessage(millis() + 5000, this, (void**)subMessage);		//
					}																				//
					else																			// Co kilka sekund sprawdzamy polaczenie z AP,
					{																				// aby moc zareagowac na ewentualne rozlaczenie.
						if(currentState != State::restart && currentState != State::halt)			//
						{																			// Ze wzgledu, ze stany restart i halt wylaczaja Wi-Fi
							#if DEBUG_MAINCONTROLLER145617											// nastepowal falszywy alarm, aby go uniknac sprawdzamy
							String log;																// aktualny stan
							log += F(DEBUG_MAINCONTROLLER145617_ID "11");							//
							DEBUG_MAINCONTROLLER145617_LOG(log);									//
							#endif																	//
																									//
							SetState(State::restart);												//
						}																			//
					}																				//
																									//
					break;																			//
				}																					//
			}

			delete ((uint16_t*)additionalData);
			return true;
		}

		case MESSAGE_TASK_CONNECT_END:
		{
			if (*((uint8_t*)additionalData) == 2)
			{
				++dataStore.terminalRecord.connectRetriesCurrent;					// Odnotowujemy nieudana probe polaczenia

				if (dataStore.terminalRecord.connectRetriesCurrent > dataStore.terminalRecord.connectRetries)	//
				{																								//
					#if DEBUG_MAINCONTROLLER145617																//
					String log;																					//
					log += F(DEBUG_MAINCONTROLLER145617_ID "21\x1F");											// Jezeli osiagnelismy limit nieudanych prob,
					log += dataStore.terminalRecord.connectRetriesCurrent;										// to kasujemy dane dostepowe (przechodzimy w tryb wlasnego AP)
					DEBUG_MAINCONTROLLER145617_LOG(log);														//
					#endif																						//
																												//
					dataStore.terminalRecord.ssid = String();													//
					dataStore.terminalRecord.password = String();												//
					dataStore.terminalRecord.connectRetriesCurrent = 0;											//
				}																								//

				SetState(State::restart);
			}
			else
			{
				if (*((uint8_t*)additionalData) == 1)										//
				{																			//
					uint16_t* subMessage = new uint16_t();									// W razie podlaczenia do zewnetrznego AP,
					*subMessage = MESSAGE_CHECK_WIFI_CONNECTION;							// rozpoczynamy monitorowanie tego polaczenia
					timeKeeper.RequestMessage(millis() + 5000, this, (void**)subMessage);	//
				}																			//

				dataStore.terminalRecord.connectRetriesCurrent = 0;
				SetState(State::setTime);
			}

			if (semaphoreManager.TryAcquire(SemaphoreManager::Operation::SD_mainLoopWrite))	//
			{																				//
				dataStore.terminal->RemoveFromTableAll();									// Aktualizujemy dane
				dataStore.terminal->PutInTable(NULL, NULL, &dataStore.terminalRecord);		// w pamieci masowej
				semaphoreManager.Release(SemaphoreManager::Operation::SD_mainLoopWrite);	//
			}																				//

			delete ((uint8_t*)additionalData);
			return true;
		}

		case MESSAGE_TASK_UPDATE_RTC_END:
		{
			if (*((uint8_t*)additionalData) == 1)
			{
				SetState(State::restart);
			}
			else
			{
				if (currentState == State::setTime)
					SetState(State::initSerialListener);
			}

			delete ((uint8_t*)additionalData);
			return true;
		}

		case MESSAGE_RESTART:
		{
			SetState(State::restart);
			return true;
		}

		case MESSAGE_HALT:
		{
			SetState(State::halt);
			return true;
		}
	}

	return false;
}

void MainController145617::SetState(State newState, uint8_t initializationData)
{
	nextState = newState;
	nextStateInit = bit(sizeof(initializationData) * 8 - 1) | initializationData;
	updateManager.Attach(this);

	#if DEBUG_MAINCONTROLLER145617
	String log;

	switch (nextState)
	{
		case State::wait:
			log += F(DEBUG_MAINCONTROLLER145617_ID "51");
			break;

		case State::initTimeKeeper:
			log += F(DEBUG_MAINCONTROLLER145617_ID "52");
			break;

		case State::initDisplay:
			log += F(DEBUG_MAINCONTROLLER145617_ID "53");
			break;

		case State::initSdFat:
			log += F(DEBUG_MAINCONTROLLER145617_ID "54");
			break;

		case State::memoryCleanup:
			log += F(DEBUG_MAINCONTROLLER145617_ID "55");
			break;

		case State::initDataStore:
			log += F(DEBUG_MAINCONTROLLER145617_ID "56");
			break;

		case State::initTaskConnect:
			log += F(DEBUG_MAINCONTROLLER145617_ID "57");
			break;

		case State::setTime:
			log += F(DEBUG_MAINCONTROLLER145617_ID "58");
			break;

		case State::initSerialListener:
			log += F(DEBUG_MAINCONTROLLER145617_ID "59");
			break;

		case State::initTaskSync:
			log += F(DEBUG_MAINCONTROLLER145617_ID "60");
			break;

		case State::initWebServer:
			log += F(DEBUG_MAINCONTROLLER145617_ID "61");
			break;

		case State::initEmployeeManager:
			log += F(DEBUG_MAINCONTROLLER145617_ID "62");
			break;

		case State::restart:
			log += F(DEBUG_MAINCONTROLLER145617_ID "63");
			break;

		case State::halt:
			log += F(DEBUG_MAINCONTROLLER145617_ID "64");
			break;
	}

	DEBUG_MAINCONTROLLER145617_LOG(log);
	#endif
}

void MainController145617::SwitchState()
{
	if (nextStateInit)
	{
		currentState = nextState;
		updateInfo.data_1 = nextStateInit;

		nextStateInit = 0;
	}
}

void MainController145617::WaitProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		updateInfo.time_1 = millis() + 5000;

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}

	if ((long)(millis() - updateInfo.time_1) >= 0)
		SetState(State::initTimeKeeper);
}

void MainController145617::InitTimeKeeperProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		timeKeeper.begin();
		updateInfo.time_1 = millis() + 100;

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}

	if ((long)(millis() - updateInfo.time_1) >= 0)
		SetState(State::initDisplay);
}

void MainController145617::InitDisplayProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		display.begin();
		display.text_1 += F("Inicjacja");
		display.text_2 += F("wyswietlacza");
		display.SetState(Display145617::State::centeredText);
		updateInfo.time_1 = millis() + 700;

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}

	if ((long)(millis() - updateInfo.time_1) >= 0)
		SetState(State::initSdFat);
}

void MainController145617::InitSdFatProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		display.text_1 = String();
		display.text_2 = String();
		display.text_1 += F("Inicjacja");
		display.text_2 += F("pamieci masowej");
		display.SetState(Display145617::State::centeredText);

		if (!sdFat.begin(16, SPI_HALF_SPEED))
		{
			#if DEBUG_MAINCONTROLLER145617
			String log;
			log += F(DEBUG_MAINCONTROLLER145617_ID "201");
			DEBUG_MAINCONTROLLER145617_LOG(log);
			#endif

			SetState(State::restart);
		}

		updateInfo.time_1 = millis() + 700;

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}

	if ((long)(millis() - updateInfo.time_1) >= 0)
		SetState(State::memoryCleanup);
}

void MainController145617::MemoryCleanupProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		display.text_1 = String();
		display.text_2 = String();
		display.text_1 += F("Oczyszczanie");
		display.text_2 += F("pamieci masowej");
		display.SetState(Display145617::State::centeredText);

		String tempString_1;
		tempString_1 += F("/FormatFlag.txt");

		if (sdFat.exists(tempString_1.c_str()))
		{
			#if DEBUG_MAINCONTROLLER145617
			String log;
			log += F(DEBUG_MAINCONTROLLER145617_ID "301");
			DEBUG_MAINCONTROLLER145617_LOG(log);
			#endif

			sdFat.wipe();

			if (!sdFat.begin(16, SPI_HALF_SPEED))
			{
				#if DEBUG_MAINCONTROLLER145617
				String log;
				log += F(DEBUG_MAINCONTROLLER145617_ID "201");
				DEBUG_MAINCONTROLLER145617_LOG(log);
				#endif

				SetState(State::restart);
			}
		}

		updateInfo.time_1 = millis() + 700;

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}

	if ((long)(millis() - updateInfo.time_1) >= 0)
		SetState(State::initDataStore);
}

void MainController145617::InitDataStoreProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		display.text_1 = String();
		display.text_2 = String();
		display.text_1 += F("Inicjacja");
		display.text_2 += F("bazy danych");
		display.SetState(Display145617::State::centeredText);
		dataStore.begin();
		updateInfo.time_1 = millis() + 700;

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}

	if ((long)(millis() - updateInfo.time_1) >= 0)
		SetState(State::initTaskConnect);
}

void MainController145617::InitTaskConnectProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		display.text_1 = String();
		display.text_2 = String();
		display.text_1 += F("Laczenie z");
		display.text_2 += F("siecia");
		display.SetState(Display145617::State::centeredText);
		new TaskConnect(this);
		updateManager.Detach(this);

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}
}

void MainController145617::SetTimeProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		if (!rtc.begin())
		{
			#if DEBUG_MAINCONTROLLER145617
			String log;
			log += F(DEBUG_MAINCONTROLLER145617_ID "501");
			DEBUG_MAINCONTROLLER145617_LOG(log);
			#endif

			SetState(State::restart);
		}
		else
		{
			display.text_1 = String();
			display.text_2 = String();
			display.text_1 += F("Synchronizacja");
			display.text_2 += F("zegara RTC");
			display.SetState(Display145617::State::centeredText);
			new TaskUpdateRTC(this, &rtc);
			updateManager.Detach(this);
		}

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}
}

void MainController145617::InitSerialListenerProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		display.text_1 = String();
		display.text_2 = String();
		display.text_1 += F("Inicjacja");
		display.text_2 += F("portu UART");
		display.SetState(Display145617::State::centeredText);
		serialListener.begin();
		serialListener.PrepareMassMemory();
		if (semaphoreManager.TryAcquire(SemaphoreManager::Operation::SD_mainLoopWritePresync))
			serialListener.StartCollectingLogs();
	
		updateInfo.time_1 = millis() + 700;
	
		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}

	if ((long)(millis() - updateInfo.time_1) >= 0)
		SetState(State::initTaskSync);
}

void MainController145617::InitTaskSyncProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		display.text_1 = String();
		display.text_2 = String();
		display.text_1 += F("Inicjacja");
		display.text_2 += F("synchronizacji");
		display.SetState(Display145617::State::centeredText);
		new TaskSync(&display);

		updateInfo.time_1 = millis() + 700;

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}

	if ((long)(millis() - updateInfo.time_1) >= 0)
		SetState(State::initWebServer);
}

void MainController145617::InitWebServerProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		display.text_1 = String();
		display.text_2 = String();
		display.text_1 += F("Inicjacja");
		display.text_2 += F("serwera HTTP");
		display.SetState(Display145617::State::centeredText);
		WebServer145617* webServer = new WebServer145617(this , sdFat);
		webServer->begin();

		updateInfo.time_1 = millis() + 700;

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}

	if ((long)(millis() - updateInfo.time_1) >= 0)
		SetState(State::initEmployeeManager);
}

void MainController145617::InitEmployeeManagerProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		new EmployeeManager(this, &display);
		updateManager.Detach(this);

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}
}

// Wylacz Wi-Fi,
// zamknij wszystkie pliki,
// zablokuj pamiec masowa i restartuj
void MainController145617::RestartProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		display.text_1 = String();
		display.text_2 = String();
		display.text_1 += F("Restartowanie");
		display.SetState(Display145617::State::centeredText);
		WiFi.mode(WIFI_OFF);
		updateInfo.time_1 = millis() + 700;

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}

	if ((long)(millis() - updateInfo.time_1) >= 0)
	{
		CloseMassMemory();

		if (semaphoreManager.TryAcquire(SemaphoreManager::Operation::SD_unmount))
			ESP.restart();
	}
}

// Wylacz Wi-Fi,
// zamknij wszystkie pliki,
// zablokuj pamiec masowa i czekaj
void MainController145617::HaltProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		display.text_1 = String();
		display.text_2 = String();
		display.text_1 += F("Wstrzymanie");
		display.SetState(Display145617::State::centeredText);
		WiFi.mode(WIFI_OFF);
		updateInfo.time_1 = millis() + 700;

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}

	if ((long)(millis() - updateInfo.time_1) >= 0)
	{
		CloseMassMemory();

		if (semaphoreManager.TryAcquire(SemaphoreManager::Operation::SD_unmount))
		{
			display.text_1 = String();
			display.text_2 = String();
			display.text_1 += F("Wstrzymanie");
			display.text_2 += F("(odlacz zasilanie)");
			display.SetState(Display145617::State::centeredText);

			updateManager.Detach(this);
		}
	}
}

void MainController145617::CloseMassMemory()
{
	if (serialListener.GetStateLogger() != serialListener.collectStopped)
	{
		serialListener.StopCollectingLogs();
		semaphoreManager.Release(SemaphoreManager::Operation::SD_mainLoopWritePresync);
	}
}

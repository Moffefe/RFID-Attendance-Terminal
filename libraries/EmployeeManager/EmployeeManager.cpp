#include "Arduino.h"
#include "EmployeeManager.h"



EmployeeManager::EmployeeManager(MessageReceiver* mainController, Display145617* display)
{
	EmployeeManager::mainController = mainController;
	EmployeeManager::display = display;
	EmployeeManager::eventTypeRecords = NULL;
	serialListener.Attach(SERIAL_EMPLOYEEMANAGER, this);
	SetState(State::idle);
}

void EmployeeManager::Update()
{
	SwitchState();

	switch (currentState)
	{
		case State::idle:
			IdleProcedure();
			break;

		case State::authenticateRFID:
			AuthenticateRFIDProcedure();
			break;

		case State::authenticateNumpad:
			authenticateNumpadProcedure();
			break;

		case State::chooseEventType:
			ChooseEventTypeProcedure();
			break;

		case State::success:
			SuccessProcedure();
			break;

		case State::failure:
			FailureProcedure();
			break;

		case State::tagScan:
			TagScanProcedure();
			break;

		case State::status:
			StatusProcedure();
			break;
	}
}

bool EmployeeManager::TakeMessage(int message, void* sender, void** additionalData)
{
	switch (message)
	{
		case MESSAGE_SERIAL_HAVE_DATA:
		{
			switch (*((uint16_t*)additionalData))
			{
				case MESSAGE_KEYPAD_145617_BUTTON_ACTION:
				{
					uint16_t buttonCode = ((uint16_t*)additionalData)[1];
					uint8_t buttonState = ((uint8_t*)additionalData)[4];
					char c;

					if (GetCharacterFromButtonCode(buttonCode, c) && buttonState)
					{
						switch (currentState)
						{
							case State::idle:
							{
								switch (c)
								{
									case '#' :						//
									{								//
										SetState(State::tagScan);	// Uzytkownik chce zeskanowac tag RFID
										break;						//
									}								//

									case '*':						//
									{								//
										SetState(State::status);	// Uzytkownik chce poznac status urzadzenia
										break;						//
									}								//

									case 'D':													//
									{															//
										mainController->TakeMessage(MESSAGE_HALT, this, NULL);	// Uzytkownik chce wylaczyc urzadzenie
										break;													//
									}															//

									case 'C':														//
									{																//
										mainController->TakeMessage(MESSAGE_RESTART, this, NULL);	// Uzytkownik chce zrestartowac urzadzenie
										break;														//
									}																//

									default :								//
									{										//
										if (isDigit(c))						//
											passCode += c;					// Uzytkownik chce rozpoczac autoryzacje
																			//
										SetState(State::authenticateRFID);	//
										break;								//
									}										//
								}

								break;
							}

							case State::authenticateNumpad:
							{
								if (isDigit(c))								//
								{											// Zbieramy kolejne cyfry do 'passCode'.
									passCode += c;							// Dodatkowo kazda kolejna cyfra odracza
									updateInfo.time_1 = millis() + 5000;	// anulowanie procesu autoryzacji
								}											//

								if (passCode.length() == 4)				//
								{										//
									passCodeHash = sha1(passCode);		// Zebralismy juz caly kod
									SetState(State::chooseEventType);	//
								}										//
								else															//
								{																//
									display->text_1 = String();									//
									display->text_2 = String();									// Nie mamy jeszcze calego kodu
									display->text_1 += F("Podaj kod");							//
									CreatePasswordInput(display->text_2);						//
									display->SetState(Display145617::State::centeredText);		//
								}																//

								break;
							}

							case State::chooseEventType:
							{
								if (bitRead(updateInfo.data_1, 0))
								{
									switch (c)
									{
										case 'A':
										{
											eventTypeId = eventTypeRecords[selectedEventType].id;

											if (semaphoreManager.TryAcquire(SemaphoreManager::Operation::SD_mainLoopWrite))
											{
												DataStore::Event::Record eventRecord;
												eventRecord.id = dataStore.event->GetNextFreePK();
												eventRecord.time = now() - dataStore.terminalRecord.timeZoneOffset;
												eventRecord.employeeId = employeeId;
												eventRecord.eventTypeId = eventTypeId;
												dataStore.event->PutInTable(NULL, NULL, &eventRecord);

												display->text_1 = String();
												display->text_2 = String();
												display->text_1 += F("Zdarzenie zostalo");
												display->text_2 += F("zapisane");
												display->SetState(Display145617::State::centeredText);
												SetState(State::success, 5);

												semaphoreManager.Release(SemaphoreManager::Operation::SD_mainLoopWrite);
											}
											else
											{
												#if DEBUG_EMPLOYEEMANAGER
												String log;
												log += F(DEBUG_EMPLOYEEMANAGER_ID "21");
												DEBUG_EMPLOYEEMANAGER_LOG(log);
												#endif

												display->text_1 = String();
												display->text_2 = String();
												display->text_1 += F("Blad");
												display->text_2 += F("Pamiec zablokowana");
												display->SetState(Display145617::State::centeredText);
												SetState(State::failure, 5);
											}

											break;
										}

										case 'C':
										{
											display->text_1 = String();
											display->text_2 = String();
											display->text_1 += F("Anulowanie");
											display->SetState(Display145617::State::centeredText);
											SetState(State::failure, 2);
											break;
										}

										case '4':
										{
											if (selectedEventType)
												--selectedEventType;

											display->text_1 = String();
											display->text_2 = String();
											display->text_1 += eventTypeRecords[selectedEventType].name;
											display->text_2 += eventTypeRecords[selectedEventType].comment;
											display->SetState(Display145617::State::horizontalMenu);

											updateInfo.time_1 = millis() + 5000;
											break;
										}

										case '6':
										{
											if (selectedEventType < (eventTypeCount - 1))
												++selectedEventType;

											display->text_1 = String();
											display->text_2 = String();
											display->text_1 += eventTypeRecords[selectedEventType].name;
											display->text_2 += eventTypeRecords[selectedEventType].comment;
											display->SetState(Display145617::State::horizontalMenu);

											updateInfo.time_1 = millis() + 5000;
											break;
										}
									}
								}
								else
								{
									display->text_1 = String();											//
									display->text_2 = String();											// Wyswietlamy menu wyboru
									display->text_1 += eventTypeRecords[selectedEventType].name;		// zdarzenia RCP
									display->text_2 += eventTypeRecords[selectedEventType].comment;		//
									display->SetState(Display145617::State::horizontalMenu);			//

									bitSet(updateInfo.data_1, 0);
									updateInfo.time_1 = millis() + 5000;
								}

								break;
							}

							case State::success:
							case State::failure:
							case State::tagScan:
							{
								SetState(State::idle);
								break;
							}

							case State::status:
							{
								updateInfo.time_1 = millis();
								break;
							}
						}
					}

					break;
				}

				case MESSAGE_RFID_145617_PICC_ACTION:
				{
					String tagCodeString;								//
					tagCodeString += ((uint8_t*)additionalData)[2];		//
					tagCodeString += '-';								//
					tagCodeString += ((uint8_t*)additionalData)[3];		// 4 bajty UID zmieniamy do postaci:
					tagCodeString += '-';								// 213-112-2-34
					tagCodeString += ((uint8_t*)additionalData)[4];		//
					tagCodeString += '-';								//
					tagCodeString += ((uint8_t*)additionalData)[5];		//

					if (currentState == State::tagScan)								//
					{																//
						display->text_1 = String();									//
						display->text_2 = String();									// Wyswietl wartosc zeskanowanego UID
						display->text_1 += F("UID to:");							// i po chwili wroc do stanu idle
						display->text_2 += tagCodeString;							//
						display->SetState(Display145617::State::centeredText);		//
						SetState(State::success, 6);								//
					}																//
					else
					{
						if (currentState == State::idle || currentState == State::authenticateRFID)	//
						{																			// Wypelniamy 'tagCodeHash' i kontynuujemy
							tagCodeHash = sha1(tagCodeString);										// proces autoryzacji
							SetState(State::authenticateNumpad);									//
						}																			//
					}

					break;
				}
			}

			return true;
		}
	}

	return false;
}

void EmployeeManager::SetState(State newState, uint8_t initializationData)
{
	nextState = newState;
	nextStateInit = bit(sizeof(initializationData) * 8 - 1) | initializationData;
	updateManager.Attach(this);

	#if DEBUG_EMPLOYEEMANAGER
	String log;

	switch (nextState)
	{
		case State::idle:
			log += F(DEBUG_EMPLOYEEMANAGER_ID "51");
			break;

		case State::authenticateRFID:
			log += F(DEBUG_EMPLOYEEMANAGER_ID "52");
			break;

		case State::authenticateNumpad:
			log += F(DEBUG_EMPLOYEEMANAGER_ID "53");
			break;

		case State::chooseEventType:
			log += F(DEBUG_EMPLOYEEMANAGER_ID "54");
			break;

		case State::success:
			log += F(DEBUG_EMPLOYEEMANAGER_ID "55");
			break;

		case State::failure:
			log += F(DEBUG_EMPLOYEEMANAGER_ID "56");
			break;

		case State::tagScan:
			log += F(DEBUG_EMPLOYEEMANAGER_ID "57");
			break;

		case State::status:
			log += F(DEBUG_EMPLOYEEMANAGER_ID "58");
			break;
	}

	DEBUG_EMPLOYEEMANAGER_LOG(log);
	#endif
}

void EmployeeManager::SwitchState()
{
	if (nextStateInit)
	{
		currentState = nextState;
		updateInfo.data_1 = nextStateInit;

		nextStateInit = 0;
	}
}

void EmployeeManager::IdleProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		if (!dataStore.GetSynchronizing())									//
		{																	//
			display->text_1 = String();										// Wyswietlamy domyslny
			display->text_1 += F("Gotowy");									// ekran
			display->SetState(Display145617::State::bigTimeWithText);		//
		}																	//

		passCode = String();			//
		passCodeHash = String();		//
		tagCodeHash = String();			//
		employeeId = -1;				//
		eventTypeId = -1;				//
										// Te pola wymagaja kazdorazowego doprowadzenia
		if (eventTypeRecords)			// do stanu wyjsciowego
		{								//
			delete[] eventTypeRecords;	// 
			eventTypeRecords = NULL;	//
		}								//
		eventTypeCount = 0;				//
		selectedEventType = 0;			//

		updateManager.Detach(this);

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}
}

void EmployeeManager::AuthenticateRFIDProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		if (!dataStore.GetSynchronizing())
		{
			if (dataStore.terminalRecord.authenticateMethod == 2 || tagCodeHash.length())
			{
				SetState(State::authenticateNumpad);
			}
			else
			{
				display->text_1 = String();
				display->text_2 = String();
				display->text_1 += F("Zeskanuj tag RFID");
				display->SetState(Display145617::State::centeredText);
				passCode = String();
				updateInfo.time_1 = millis() + 5000;
			}
		}
		else							//
		{								// Podczas synchronizacji nie mozemy
			SetState(State::idle);		// obslugiwac pracownikow
		}								//

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}

	if ((long)(millis() - updateInfo.time_1) >= 0)
	{
		if (!dataStore.GetSynchronizing())								//
		{																//
			display->text_1 = String();									//
			display->text_2 = String();									// Przez chwile wyswietlamy "Anulowanie"
			display->text_1 += F("Anulowanie");							// i wracamy do idle
			display->SetState(Display145617::State::centeredText);		//
			SetState(State::failure, 2);								//
		}																//
		else								//
		{									// Nie mozemy w tym momencie uzyc wyswietlacza,
			SetState(State::idle);			// wiec bezposrednio wracamy do idle
		}									//
	}
}

void EmployeeManager::authenticateNumpadProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		if (!dataStore.GetSynchronizing())
		{
			if (dataStore.terminalRecord.authenticateMethod == 1)
			{
				SetState(State::chooseEventType);
			}
			else
			{
				display->text_1 = String();
				display->text_2 = String();
				display->text_1 += F("Podaj kod");
				CreatePasswordInput(display->text_2);
				display->SetState(Display145617::State::centeredText);
				updateInfo.time_1 = millis() + 5000;
			}
		}
		else						//
		{							// Podczas synchronizacji nie mozemy
			SetState(State::idle);	// obslugiwac pracownikow
		}							//

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}

	if ((long)(millis() - updateInfo.time_1) >= 0)
	{
		if (!dataStore.GetSynchronizing())								//
		{																//
			display->text_1 = String();									//
			display->text_2 = String();									// Przez chwile wyswietlamy "Anulowanie"
			display->text_1 += F("Anulowanie");							// i wracamy do idle
			display->SetState(Display145617::State::centeredText);		//
			SetState(State::failure, 2);								//
		}																//
		else								//
		{									// Nie mozemy w tym momencie uzyc wyswietlacza,
			SetState(State::idle);			// wiec bezposrednio wracamy do idle
		}									//
	}
}

// Pracownik wybiera rodzaj zdarzenia RCP
// Bit 0 - gdy jest nie ustawiony to znaczy,
//         ze aktualnie jest wyswietlane okno
//         "Autoryzacja...". Gdy jest ustawiony,
//         wyswietlane jest menu wyboru zdarzenia RCP
void EmployeeManager::ChooseEventTypeProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		if (!dataStore.GetSynchronizing())
		{
			Ivanseidel::LinkedList<uint8_t> fieldIndexes;															//
			Ivanseidel::LinkedList<uint8_t*> fieldValues;															//
			DataStore::Employee::Record employeeRecord;																//
			uint32_t recordFound = 0;																				//
			switch (dataStore.terminalRecord.authenticateMethod)													//
			{																										//
				case 0:																								//
					fieldIndexes.add(DataStore::Employee::Field::passcodeHash);										//
					fieldIndexes.add(DataStore::Employee::Field::tagCodeHash);										//
					fieldValues.add((uint8_t*)passCodeHash.c_str());												//
					fieldValues.add((uint8_t*)tagCodeHash.c_str());													//
					break;																							//
																													//
				case 1:																								// Wyciagamy z tabeli 'employee' rekord pracownika
					fieldIndexes.add(DataStore::Employee::Field::tagCodeHash);										//
					fieldValues.add((uint8_t*)tagCodeHash.c_str());													//
					break;																							//
																													//
				case 2:																								//
					fieldIndexes.add(DataStore::Employee::Field::passcodeHash);										//
					fieldValues.add((uint8_t*)passCodeHash.c_str());												//
					break;																							//
			}																										//
																													//
			if (semaphoreManager.TryAcquire(SemaphoreManager::Operation::SD_mainLoopRead))							//
			{																										//
				recordFound = dataStore.employee->GetFromTable(&fieldIndexes, &fieldValues, &employeeRecord, 1);	//
				semaphoreManager.Release(SemaphoreManager::Operation::SD_mainLoopRead);								//
			}																										//
			else												//
			{													//
				#if DEBUG_EMPLOYEEMANAGER						//
				String log;										// Nie udalo sie zajac semafora
				log += F(DEBUG_EMPLOYEEMANAGER_ID "401");		//
				DEBUG_EMPLOYEEMANAGER_LOG(log);					//
				#endif											//
			}													//

			if (recordFound)
			{
				bool dayValid = false;																				//
				bool hourValid = false;																				//
																													//
				int8_t weekdayBit = (int8_t)weekday() - 2;															//
				if (weekdayBit < 0)																					//
					weekdayBit += 7;																				//
																													//
				if (bitRead(employeeRecord.accessDays, weekdayBit))													//
					dayValid = true;																				//
																													// Sprawdzamy czy dany pracownik ma w tym momencie
				if (employeeRecord.accessFromHour < employeeRecord.accessToHour)									// dostep do strefy chronionej przez ten terminal
				{																									//
					if ( (hour() > employeeRecord.accessFromHour) && (hour() < employeeRecord.accessToHour) )		//
						hourValid = true;																			//
				}																									//
				else																								//
				{																									//
					if ( (hour() > employeeRecord.accessFromHour) || (hour() < employeeRecord.accessToHour) )		//
						hourValid = true;																			//
				}																									//

				if (dayValid && hourValid)
				{
					employeeId = employeeRecord.id;

					if (semaphoreManager.TryAcquire(SemaphoreManager::Operation::SD_mainLoopRead))
					{
						eventTypeCount = dataStore.eventType->RecordCount();
						eventTypeRecords = new DataStore::EventType::Record[eventTypeCount];
						dataStore.eventType->GetFromTableAll(eventTypeRecords, eventTypeCount);
						selectedEventType = 0;

						display->text_1 = String();
						display->text_2 = String();
						display->text_1 += F("Autoryzacja");
						display->text_2 += employeeRecord.name;
						display->text_2 += ' ';
						display->text_2 += employeeRecord.surname;
						display->SetState(Display145617::State::centeredText);

						bitClear(updateInfo.data_1, 0);
						updateInfo.time_1 = millis() + 3000;

						semaphoreManager.Release(SemaphoreManager::Operation::SD_mainLoopRead);
					}
					else															//
					{																//
						#if DEBUG_EMPLOYEEMANAGER									//
						String log;													//
						log += F(DEBUG_EMPLOYEEMANAGER_ID "402");					//
						DEBUG_EMPLOYEEMANAGER_LOG(log);								//
						#endif														// Nie udalo sie zajac semafora
																					//
						display->text_1 = String();									//
						display->text_2 = String();									//
						display->text_1 += F("Blad");								//
						display->text_2 += F("Pamiec zablokowana");					//
						display->SetState(Display145617::State::centeredText);		//
						SetState(State::failure, 5);								//
					}																//
				}
				else
				{
					display->text_1 = String();
					display->text_2 = String();
					display->text_1 += F("Brak dostepu");
					display->text_2 += F("w tej chwili");
					display->SetState(Display145617::State::centeredText);
					SetState(State::failure, 4);
				}

			}
			else
			{
				display->text_1 = String();
				display->text_2 = String();
				display->text_1 += F("Nie rozpoznano");
				display->SetState(Display145617::State::centeredText);
				SetState(State::failure, 4);
			}
		}
		else						//
		{							// Podczas synchronizacji nie mozemy
			SetState(State::idle);	// obslugiwac pracownikow
		}							//

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}

	if ((long)(millis() - updateInfo.time_1) >= 0)
	{
		if (!dataStore.GetSynchronizing())
		{
			if (bitRead(updateInfo.data_1, 0))							//
			{															//
				display->text_1 = String();								//
				display->text_2 = String();								// Przez chwile wyswietlamy "Anulowanie"
				display->text_1 += F("Anulowanie");						// i wracamy do idle
				display->SetState(Display145617::State::centeredText);	//
				SetState(State::failure, 2);							//
			}															//
			else
			{
				display->text_1 = String();											//
				display->text_2 = String();											// Wyswietlamy menu wyboru
				display->text_1 += eventTypeRecords[selectedEventType].name;		// zdarzenia RCP
				display->text_2 += eventTypeRecords[selectedEventType].comment;		//
				display->SetState(Display145617::State::horizontalMenu);			//

				bitSet(updateInfo.data_1, 0);
				updateInfo.time_1 = millis() + 5000;
			}
		}
		else								//
		{									// Nie mozemy w tym momencie uzyc wyswietlacza,
			SetState(State::idle);			// wiec bezposrednio wracamy do idle
		}									//
	}
}

// Zadaniem tego stanu jest przetrzymanie przez
// okreslony czas jakiejs informacji na wyswietlaczu
// i powrot do stanu idle
void EmployeeManager::SuccessProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		unsigned long duration = (updateInfo.data_1 & B00001111);
		duration *= 1000;
		updateInfo.time_1 = millis() + duration;

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}

	if ((long)(millis() - updateInfo.time_1) >= 0)
		SetState(State::idle);
}

// Zadaniem tego stanu jest przetrzymanie przez
// okreslony czas jakiejs informacji na wyswietlaczu
// i powrot do stanu idle
void EmployeeManager::FailureProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		unsigned long duration = (updateInfo.data_1 & B00001111);
		duration *= 1000;
		updateInfo.time_1 = millis() + duration;

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}

	if ((long)(millis() - updateInfo.time_1) >= 0)
		SetState(State::idle);
}

void EmployeeManager::TagScanProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		if (!dataStore.GetSynchronizing())
		{
			display->text_1 = String();
			display->text_2 = String();
			display->text_1 += F("Zeskanuj tag RFID");
			display->SetState(Display145617::State::centeredText);
			passCode = String();
			updateInfo.time_1 = millis() + 5000;
		}
		else						//
		{							// Podczas synchronizacji nie mozemy
			SetState(State::idle);	// obslugiwac pracownikow
		}							//

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}

	if ((long)(millis() - updateInfo.time_1) >= 0)
	{
		if (!dataStore.GetSynchronizing())								//
		{																//
			display->text_1 = String();									//
			display->text_2 = String();									// Przez chwile wyswietlamy "Anulowanie"
			display->text_1 += F("Anulowanie");							// i wracamy do idle
			display->SetState(Display145617::State::centeredText);		//
			SetState(State::failure, 2);								//
		}																//
		else								//
		{									// Nie mozemy w tym momencie uzyc wyswietlacza,
			SetState(State::idle);			// wiec bezposrednio wracamy do idle
		}									//
	}
}

void EmployeeManager::StatusProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		if (!dataStore.GetSynchronizing())
		{
			updateInfo.data_1 = 0;
			display->text_1 = String();
			display->text_2 = String();
			display->text_3 = String();
			display->text_4 = String();
			display->text_1 += F("Czas pracy:");
			display->text_2 += (uint32_t)floor( (millis() / 1000) / 3600 );
			display->text_2 += F("h ");
			display->text_2 += (uint32_t)floor( (millis() / 1000) % 3600 / 60 );
			display->text_2 += F("m");
			display->text_3 += F("Wolna pamiec RAM:");
			display->text_4 += ESP.getFreeHeap();
			display->text_4 += F(" bajtow");
			display->SetState(Display145617::State::text);
			updateInfo.time_1 = millis() + 5000;
		}
		else						//
		{							// Podczas synchronizacji nie mozemy
			SetState(State::idle);	// obslugiwac pracownikow
		}							//

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}

	if ((long)(millis() - updateInfo.time_1) >= 0)
	{
		switch (updateInfo.data_1)
		{
			case 0:
			{
				updateInfo.data_1 = 1;
				display->text_1 = String();
				display->text_2 = String();
				display->text_3 = String();
				display->text_4 = String();
				display->text_1 += F("Stan sieci:");
				display->text_2 += (WiFi.status() == WL_CONNECTED) ? F("Podlaczony") : F("Nie podlaczony");
				display->text_3 += F("IP lok. serwera:");
				display->text_4 += (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
				display->SetState(Display145617::State::text);
				updateInfo.time_1 = millis() + 5000;
				break;
			}

			case 1:
			{
				updateInfo.data_1 = 2;

				if (WiFi.status() == WL_CONNECTED)
				{
					display->text_1 = String();
					display->text_2 = String();
					display->text_3 = String();
					display->text_4 = String();
					display->text_1 += F("SSID:");
					display->text_2 += dataStore.terminalRecord.ssid;
					display->text_3 += F("RSSI:");
					display->text_4 += WiFi.RSSI();
					display->text_4 += F(" dBm");
					display->SetState(Display145617::State::text);
					updateInfo.time_1 = millis() + 5000;
				}
				else
				{
					updateInfo.time_1 = millis();
				}

				break;
			}

			case 2:
			{
				updateInfo.data_1 = 3;
				display->text_1 = String();
				display->text_2 = String();
				display->text_3 = String();
				display->text_4 = String();
				display->text_1 += F("Nazwa terminala:");
				display->text_2 += dataStore.terminalRecord.name;
				display->text_3 += F("Nazwa strefy:");
				display->text_4 += dataStore.terminalRecord.areaName;
				display->SetState(Display145617::State::text);
				updateInfo.time_1 = millis() + 5000;
				break;
			}

			case 3:
			{
				updateInfo.data_1 = 4;
				display->text_1 = String();
				display->text_2 = String();
				display->text_3 = String();
				display->text_4 = String();
				display->text_1 += F("Nazwa przejscia:");
				display->text_2 += dataStore.terminalRecord.passageName;
				display->text_3 += F("Liczba pracow.:");
				display->text_4 += dataStore.employee->RecordCount();
				display->text_4 += F(" osob");
				display->SetState(Display145617::State::text);
				updateInfo.time_1 = millis() + 5000;
				break;
			}

			case 4:
			{
				SetState(State::idle);
				break;
			}
		}
	}
}

bool EmployeeManager::GetCharacterFromButtonCode(uint16_t buttonCode, char& result)
{
	switch (buttonCode)
	{
		case 0:
			result = '1';
			break;

		case 1:
			result = '2';
			break;

		case 2:
			result = '3';
			break;

		case 3:
			result = 'A';
			break;

		case 4:
			result = '4';
			break;

		case 5:
			result = '5';
			break;

		case 6:
			result = '6';
			break;

		case 7:
			result = 'B';
			break;

		case 8:
			result = '7';
			break;

		case 9:
			result = '8';
			break;

		case 10:
			result = '9';
			break;

		case 11:
			result = 'C';
			break;

		case 12:
			result = '*';
			break;

		case 13:
			result = '0';
			break;

		case 14:
			result = '#';
			break;

		case 15:
			result = 'D';
			break;

		default:
			return false;
	}

	return true;
}

void EmployeeManager::CreatePasswordInput(String& result)
{
	uint8_t remainCount = (passCode.length() > 4) ? 0 : 4 - passCode.length();
	result += passCode;

	for (uint8_t i = 0; i < remainCount; ++i)
		result += '_';
}

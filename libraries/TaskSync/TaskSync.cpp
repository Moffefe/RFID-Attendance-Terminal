#include "Arduino.h"
#include "TaskSync.h"



TaskSync::TaskSync(Display145617* display)
{
	recordIndex = 0;
	tablesToSync = 0;
	TaskSync::display = display;
	request = new asyncHTTPrequest();
	request->setTimeout(5);
	eventRecords = new DataStore::Event::Record[10];
	SetState(State::idle, 1);
}

void TaskSync::Update()
{
	SwitchState();

	switch (currentState)
	{
		case State::idle:
			IdleProcedure();
			break;

		case State::connect:
			ConnectProcedure();
			break;

		case State::pullTerminal:
			PullTerminalProcedure();
			break;

		case State::pullEmployee:
			PullEmployeeProcedure();
			break;

		case State::pullEventType:
			PullEventTypeProcedure();
			break;

		case State::pushEvent:
			PushEventProcedure();
			break;

		case State::finish:
			FinishProcedure();
			break;
	}
}

bool TaskSync::TakeMessage(int message, void* sender, void** additionalData)
{
	switch (message)
	{
		case MESSAGE_TIME_KEEPER:
		{
			switch (*((uint16_t*)additionalData))
			{
				case MESSAGE_TASK_SYNC_BEGIN:
				{
					if (																											//
						(dataStore.terminalRecord.serverAddress.length() || dataStore.terminalRecord.serverDomain.length())			//
						&&																											//
						(dataStore.terminalRecord.publicKey.length() && dataStore.terminalRecord.serverPublicKey.length())			//
						&&																											// Rozpoczynamy proces synchronizacji
						(WiFi.status() == WL_CONNECTED)																				//
					   )																											//
					{																												//
						SetState(State::connect);																					//
					}																												//
					else																												//
					{																													//
						uint16_t* subMessage = new uint16_t();																			// Planujemy nastepna synchronizacje
						*subMessage = MESSAGE_TASK_SYNC_BEGIN;																			//
						timeKeeper.RequestMessage(millis() + dataStore.terminalRecord.syncInterval, this, (void**)subMessage);			//
					}																													//

					break;
				}
			}

			delete ((uint16_t*)additionalData);
			return true;
		}
	}

	return false;
}

void TaskSync::SetState(State newState, uint8_t initializationData)
{
	nextState = newState;
	nextStateInit = bit(sizeof(initializationData) * 8 - 1) | initializationData;
	updateManager.Attach(this);

	#if DEBUG_TASKSYNC
	String log;

	switch (nextState)
	{
		case State::idle:
			log += F(DEBUG_TASKSYNC_ID "51");
			break;

		case State::connect:
			log += F(DEBUG_TASKSYNC_ID "52");
			break;

		case State::pullTerminal:
			log += F(DEBUG_TASKSYNC_ID "53");
			break;

		case State::pullEmployee:
			log += F(DEBUG_TASKSYNC_ID "54");
			break;

		case State::pullEventType:
			log += F(DEBUG_TASKSYNC_ID "55");
			break;

		case State::pushEvent:
			log += F(DEBUG_TASKSYNC_ID "56");
			break;

		case State::finish:
			log += F(DEBUG_TASKSYNC_ID "57");
			break;
	}

	DEBUG_TASKSYNC_LOG(log);
	#endif
}

void TaskSync::SwitchState()
{
	if (nextStateInit)
	{
		currentState = nextState;
		updateInfo.data_1 = nextStateInit;

		nextStateInit = 0;
	}
}

void TaskSync::IdleProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		unsigned long syncTime;												//
																			//
		if (bitRead(updateInfo.data_1, 0))									// Ustalamy kiedy ma nastapic
			syncTime = millis() + 5000;										// nastepna synchronizacja
		else																//
			syncTime = millis() + dataStore.terminalRecord.syncInterval;	//

		uint16_t* subMessage = new uint16_t();								//
		*subMessage = MESSAGE_TASK_SYNC_BEGIN;								// Planujemy nastepna synchronizacje
		timeKeeper.RequestMessage(syncTime, this, (void**)subMessage);		//

		updateManager.Detach(this);

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}
}

void TaskSync::ConnectProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		String tempString_1;
		String tempString_2;

		tempString_1 += F("http://");								//
		tempString_1 += dataStore.terminalRecord.serverDomain;		//
		tempString_1 += F("/connect-terminal.php");					// Laczenie
		tempString_2 += F("POST");									//
		request->open(tempString_2.c_str(), tempString_1.c_str());	//

		tempString_1 = String();												//
		tempString_1 += F("Content-Type");										//
		tempString_2 = String();												// Naglowki
		tempString_2 += F("application/x-www-form-urlencoded");					//
		request->setReqHeader(tempString_1.c_str(), tempString_2.c_str());		//

		tempString_1 = String();								//
		tempString_1 += F("publicKey=");						// Cialo
		tempString_1 += dataStore.terminalRecord.publicKey;		//
		request->send(tempString_1);							//

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}

	if (request->readyState() == 4)
	{
		bool error = false;

		if (request->responseHTTPcode() == 200)
		{
			StaticJsonBuffer<256> jsonBuffer;
			JsonObject& jsonObject = jsonBuffer.parseObject(request->responseText());

			if (jsonObject.success())
			{
				if (jsonObject[F("publicKey")] == dataStore.terminalRecord.serverPublicKey)
				{
					if (jsonObject[F("terminal")] == true)		//
						bitSet(tablesToSync, 0);				//
					else										//
						bitClear(tablesToSync, 0);				//
																//
					if (jsonObject[F("employee")] == true)		//
						bitSet(tablesToSync, 1);				// Ustalamy jakie tabele wymagaja synchronizacji
					else										//
						bitClear(tablesToSync, 1);				//
																//
					if (jsonObject[F("eventType")] == true)		//
						bitSet(tablesToSync, 2);				//
					else										//
						bitClear(tablesToSync, 2);				//

					#if DEBUG_TASKSYNC
					String log;
					log += F(DEBUG_TASKSYNC_ID "201\x1F");
					log += tablesToSync;
					DEBUG_TASKSYNC_LOG(log);
					#endif
				}
				else									//
				{										//
					#if DEBUG_TASKSYNC					//
					String log;							//
					log += F(DEBUG_TASKSYNC_ID "202");	// Nieprawidlowy klucz serwera
					DEBUG_TASKSYNC_LOG(log);			//
					#endif								//
														//
					error = true;						//
				}										//
			}
			else									//
			{										//
				#if DEBUG_TASKSYNC					//
				String log;							//
				log += F(DEBUG_TASKSYNC_ID "203");	// Blad serwera
				DEBUG_TASKSYNC_LOG(log);			//
				#endif								//
													//
				error = true;						//
			}										//
		}
		else									//
		{										//
			#if DEBUG_TASKSYNC					//
			String log;							//
			log += F(DEBUG_TASKSYNC_ID "204");	// Nieudane polaczenie
			DEBUG_TASKSYNC_LOG(log);			//
			#endif								//
												//
			error = true;						//
		}										//

		if (error)							//
		{									//
			tablesToSync = 0;				//
			SetState(State::idle);			// W razie jakiegokolwiek bledu przerywamy
		}									// proces synchronizacji
		else								//
		{									//
			SetState(State::pullTerminal);	//
		}									//
	}
}

void TaskSync::PullTerminalProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		if (bitRead(tablesToSync, 0))
		{
			recordIndex = 0;
			String tempString_1;
			String tempString_2;

			tempString_1 += F("http://");								//
			tempString_1 += dataStore.terminalRecord.serverDomain;		//
			tempString_1 += F("/pull-terminal.php");					// Laczenie
			tempString_2 += F("POST");									//
			request->open(tempString_2.c_str(), tempString_1.c_str());	//

			tempString_1 = String();												//
			tempString_1 += F("Content-Type");										//
			tempString_2 = String();												// Naglowki
			tempString_2 += F("application/x-www-form-urlencoded");					//
			request->setReqHeader(tempString_1.c_str(), tempString_2.c_str());		//

			tempString_1 = String();								//
			tempString_1 += F("publicKey=");						//
			tempString_1 += dataStore.terminalRecord.publicKey;		// Cialo
			tempString_1 += F("&recordIndex=");						//
			tempString_1 += recordIndex;							//
			request->send(tempString_1);							//
		}
		else								//
		{									// Jesli nie potrzeba przeprowadzac synchronizacji
			SetState(State::pullEmployee);	// tabeli 'terminal' to zmieniamy stan
		}									//
	
		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}

	if (request->readyState() == 4)
	{
		bool error = false;

		if (request->responseHTTPcode() == 200)
		{
			StaticJsonBuffer<256> jsonBuffer;
			JsonObject& jsonObject = jsonBuffer.parseObject(request->responseText());
		
			if (jsonObject.success())
			{
				if (jsonObject[F("publicKey")] == dataStore.terminalRecord.serverPublicKey)
				{
					dataStore.terminalRecord.name = jsonObject.get<String>(F("name"));
					dataStore.terminalRecord.areaName = jsonObject.get<String>(F("areaName"));
					dataStore.terminalRecord.passageName = jsonObject.get<String>(F("passageName"));
		
					if (jsonObject[F("authenticateMethod")] == F("RFID"))
					{
						dataStore.terminalRecord.authenticateMethod = 1;
					}
					else
					{
						if (jsonObject[F("authenticateMethod")] == F("numpad"))
							dataStore.terminalRecord.authenticateMethod = 2;
						else
							dataStore.terminalRecord.authenticateMethod = 0;
					}
		
					if (semaphoreManager.TryAcquire(SemaphoreManager::Operation::SD_mainLoopWrite))
					{
						dataStore.terminal->RemoveFromTableAll();
						dataStore.terminal->PutInTable(NULL, NULL, &dataStore.terminalRecord);
						semaphoreManager.Release(SemaphoreManager::Operation::SD_mainLoopWrite);
					}
		
					bitClear(tablesToSync, 0);
					SetState(State::pullEmployee);
				}
				else									//
				{										//
					#if DEBUG_TASKSYNC					//
					String log;							//
					log += F(DEBUG_TASKSYNC_ID "301");	// Nieprawidlowy klucz serwera
					DEBUG_TASKSYNC_LOG(log);			//
					#endif								//
														//
					error = true;						//
				}										//
			}
			else									//
			{										//
				#if DEBUG_TASKSYNC					//
				String log;							//
				log += F(DEBUG_TASKSYNC_ID "302");	// Blad serwera
				DEBUG_TASKSYNC_LOG(log);			//
				#endif								//
													//
				error = true;						//
			}										//
		}
		else									//
		{										//
			#if DEBUG_TASKSYNC					//
			String log;							//
			log += F(DEBUG_TASKSYNC_ID "303");	// Nieudane polaczenie
			DEBUG_TASKSYNC_LOG(log);			//
			#endif								//
												//
			error = true;						//
		}										//
		
		if (error)					//
		{							// W razie jakiegokolwiek bledu przerywamy
			tablesToSync = 0;		// proces synchronizacji
			SetState(State::idle);	//
		}							//
	}
}

// Sciagamy rekordy do tabeli 'employee'
// Bit 0 - wyslalismy juz zadanie
// Bit 1, 2, 3 - liczba nieudanych (pod rzad) prob
void TaskSync::PullEmployeeProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		if (bitRead(tablesToSync, 1) && semaphoreManager.TryAcquire(SemaphoreManager::Operation::SD_mainLoopWrite))		//
		{																												//
			dataStore.TakeMessage(MESSAGE_TASK_SYNC_BEGIN, this, NULL);													//
			String oldFileName;																							//
			String newFileName;																							//
			oldFileName += F("/DataStore/Employee.bin");																//
			newFileName += F("/DataStore/Employee_backup.bin");															//
			File file;																									// Jesli nie potrzeba przeprowadzac synchronizacji
			file.open(oldFileName.c_str(), O_WRITE);																	// tabeli 'employee' to zmieniamy stan.
			file.rename(File::cwd(), newFileName.c_str());																// Gdy trzeba ja przeprowadzic to robimy kopie
			file.close();																								// zapasowa starej tablicy
			file.open(oldFileName.c_str(), O_WRITE | O_CREAT | O_TRUNC);												//
			file.close();																								//
			semaphoreManager.Release(SemaphoreManager::Operation::SD_mainLoopWrite);									//
																														//
			if (display)																								//
			{																											//
				display->text_1 = String();																				//
				display->text_2 = String();																				//
				display->text_1 += F("Synchronizacja");																	//
				display->SetState(Display145617::State::centeredText);													//
			}																											//
																														//
			recordIndex = 0;																							//
		}																												//
		else																											//
		{																												//
			SetState(State::pullEventType);																				//
		}																												//
	
		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}

	// Bit 0 oznacza, ze wyslalismy juz zadanie
	if (bitRead(updateInfo.data_1, 0))
	{
		if (request->readyState() == 4)
		{
			bool error = false;

			if (request->responseHTTPcode() == 200)
			{
				DynamicJsonBuffer jsonBuffer;
				JsonObject& jsonObject = jsonBuffer.parseObject(request->responseText());
			
				if (jsonObject.success())
				{
					if (jsonObject[F("publicKey")] == dataStore.terminalRecord.serverPublicKey)
					{
						JsonArray& rows = jsonObject[F("rows")];
						uint8_t rowsCount = rows.size();
			
						if (rowsCount)
						{
							if (semaphoreManager.TryAcquire(SemaphoreManager::Operation::SD_mainLoopWrite))
							{
								String tempString_1;
								DataStore::Employee::Record record;
								record.rank = 1;
			
								for (auto value : rows)
								{
									JsonObject& row = value.asObject();
			
									record.id = row.get<uint32_t>(F("id"));							//
									record.name = row.get<String>(F("name"));						//
									record.surname = row.get<String>(F("surname"));					//
									record.passcodeHash = row.get<String>(F("passcodeHash"));		//
									record.tagCodeHash = row.get<String>(F("tagCodeHash"));			//
									record.accessDays = 0;											//
									String accessDays = row.get<String>(F("accessDays"));			//
									tempString_1 = String();										//
									tempString_1 += F("pn");										//
									if (accessDays.indexOf(tempString_1) != -1)						//
										bitSet(record.accessDays, 0);								//
									tempString_1 = String();										//
									tempString_1 += F("wt");										//
									if (accessDays.indexOf(tempString_1) != -1)						//
										bitSet(record.accessDays, 1);								//
									tempString_1 = String();										//
									tempString_1 += F("sr");										//
									if (accessDays.indexOf(tempString_1) != -1)						//
										bitSet(record.accessDays, 2);								// Wypelniamy 'record'
									tempString_1 = String();										//
									tempString_1 += F("cz");										//
									if (accessDays.indexOf(tempString_1) != -1)						//
										bitSet(record.accessDays, 3);								//
									tempString_1 = String();										//
									tempString_1 += F("pt");										//
									if (accessDays.indexOf(tempString_1) != -1)						//
										bitSet(record.accessDays, 4);								//
									tempString_1 = String();										//
									tempString_1 += F("sb");										//
									if (accessDays.indexOf(tempString_1) != -1)						//
										bitSet(record.accessDays, 5);								//
									tempString_1 = String();										//
									tempString_1 += F("nd");										//
									if (accessDays.indexOf(tempString_1) != -1)						//
										bitSet(record.accessDays, 6);								//
									record.accessFromHour = row.get<uint8_t>(F("accessFromHour"));	//
									record.accessToHour = row.get<uint8_t>(F("accessToHour"));		//
			
									dataStore.employee->PutInTable(NULL, NULL, &record);
								}
			
								semaphoreManager.Release(SemaphoreManager::Operation::SD_mainLoopWrite);
							}
			
							recordIndex += rowsCount;			// Przygotowujemy sie do nastepnego zadania
							bitClear(updateInfo.data_1, 0);		//
						}
						else																				//
						{																					//
							if (semaphoreManager.TryAcquire(SemaphoreManager::Operation::SD_mainLoopWrite))	//
							{																				//
								String fileName;															//
								fileName += F("/DataStore/Employee_backup.bin");							//
								File file;																	//
								file.open(fileName.c_str(), O_WRITE);										// Po udanej synchronizacji tabeli 'employee'
								file.remove();																// usuwamy kopie zapasowa i przechodzimy do
								file.close();																// aktualizacji 'eventType'
								semaphoreManager.Release(SemaphoreManager::Operation::SD_mainLoopWrite);	//
							}																				//
																											//
							if (display)																	//
							{																				//
								display->text_1 = String();													//
								display->text_1 += F("Gotowy");												//
								display->SetState(Display145617::State::bigTimeWithText);					//
							}																				//
																											//
							dataStore.TakeMessage(MESSAGE_TASK_SYNC_END, this, NULL);						//
																											//
							bitClear(tablesToSync, 1);														//
							SetState(State::pullEventType);													//
						}																					//
					}
					else									//
					{										//
						#if DEBUG_TASKSYNC					//
						String log;							//
						log += F(DEBUG_TASKSYNC_ID "401");	// Nieprawidlowy klucz serwera
						DEBUG_TASKSYNC_LOG(log);			//
						#endif								//
															//
						error = true;						//
					}										//
				}
				else									//
				{										//
					#if DEBUG_TASKSYNC					//
					String log;							//
					log += F(DEBUG_TASKSYNC_ID "402");	// Blad serwera
					DEBUG_TASKSYNC_LOG(log);			//
					#endif								//
														//
					error = true;						//
				}										//
			}
			else									//
			{										//
				#if DEBUG_TASKSYNC					//
				String log;							//
				log += F(DEBUG_TASKSYNC_ID "403");	// Nieudane polaczenie
				DEBUG_TASKSYNC_LOG(log);			//
				#endif								//
													//
				error = true;						//
			}										//
			
			if (error)
			{
				uint8_t errorsInRow = (updateInfo.data_1 & 14) >> 1;
				if (errorsInRow > 3)																//
				{																					//
					if (semaphoreManager.TryAcquire(SemaphoreManager::Operation::SD_mainLoopWrite))	//
					{																				//
						String oldFileName;															//
						String newFileName;															//
						oldFileName += F("/DataStore/Employee_backup.bin");							//
						newFileName += F("/DataStore/Employee.bin");								//
						File file;																	//
						file.open(newFileName.c_str(), O_WRITE);									//
						file.remove();																//
						file.close();																//
						file.open(oldFileName.c_str(), O_WRITE);									//
						file.rename(File::cwd(), newFileName.c_str());								// Wystapilo zbyt duzo bledow zadania, przerywamy
						file.close();																// proces synchronizacji i przywracamy tabele 'employee'
						semaphoreManager.Release(SemaphoreManager::Operation::SD_mainLoopWrite);	//
					}																				//
																									//
					if (display)																	//
					{																				//
						display->text_1 = String();													//
						display->text_1 += F("Blad Synchr.");										//
						display->SetState(Display145617::State::bigTimeWithText);					//
					}																				//
																									//
					dataStore.TakeMessage(MESSAGE_TASK_SYNC_END, this, NULL);						//
					tablesToSync = 0;																//
					SetState(State::idle);															//
																									//
					#if DEBUG_TASKSYNC																//
					String log;																		//
					log += F(DEBUG_TASKSYNC_ID "404");												//
					DEBUG_TASKSYNC_LOG(log);														//
					#endif																			//
				}																					//
				else														//
				{															//
					++errorsInRow;											//
					errorsInRow = errorsInRow << 1;							// Odnotowujemy wystapienie bledu
					updateInfo.data_1 = updateInfo.data_1 & ~14;			// i przygotowujemy sie do ponowienia zadania
					updateInfo.data_1 = updateInfo.data_1 | errorsInRow;	//
																			//
					bitClear(updateInfo.data_1, 0);							//
				}															//
			}
			else												//
			{													// zadanie powiodlo sie, wiec resetujemy licznik bledow
				updateInfo.data_1 = updateInfo.data_1 & ~14;	// dlatego, ze on liczy bledy pod rzad, a nie wszystkie
			}													//
		}
	}
	else
	{
		String tempString_1;
		String tempString_2;

		tempString_1 += F("http://");								//
		tempString_1 += dataStore.terminalRecord.serverDomain;		//
		tempString_1 += F("/pull-employee.php");					// Laczenie
		tempString_2 += F("POST");									//
		request->open(tempString_2.c_str(), tempString_1.c_str());	//

		tempString_1 = String();												//
		tempString_1 += F("Content-Type");										//
		tempString_2 = String();												// Naglowki
		tempString_2 += F("application/x-www-form-urlencoded");					//
		request->setReqHeader(tempString_1.c_str(), tempString_2.c_str());		//

		tempString_1 = String();								//
		tempString_1 += F("publicKey=");						//
		tempString_1 += dataStore.terminalRecord.publicKey;		// Cialo
		tempString_1 += F("&recordIndex=");						//
		tempString_1 += recordIndex;							//
		request->send(tempString_1);							//

		bitSet(updateInfo.data_1, 0);		// Odznaczamy wyslanie zadania
	}
}

void TaskSync::PullEventTypeProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		if (bitRead(tablesToSync, 2))
		{
			recordIndex = 0;
			String tempString_1;
			String tempString_2;

			tempString_1 += F("http://");								//
			tempString_1 += dataStore.terminalRecord.serverDomain;		//
			tempString_1 += F("/pull-eventType.php");					// Laczenie
			tempString_2 += F("POST");									//
			request->open(tempString_2.c_str(), tempString_1.c_str());	//

			tempString_1 = String();												//
			tempString_1 += F("Content-Type");										//
			tempString_2 = String();												// Naglowki
			tempString_2 += F("application/x-www-form-urlencoded");					//
			request->setReqHeader(tempString_1.c_str(), tempString_2.c_str());		//

			tempString_1 = String();								//
			tempString_1 += F("publicKey=");						//
			tempString_1 += dataStore.terminalRecord.publicKey;		// Cialo
			tempString_1 += F("&recordIndex=");						//
			tempString_1 += recordIndex;							//
			request->send(tempString_1);							//
		}
		else								//
		{									// Jesli nie potrzeba przeprowadzac synchronizacji
			SetState(State::pushEvent);		// tabeli 'eventType' to zmieniamy stan
		}									//
	
		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}

	if (request->readyState() == 4)
	{
		bool error = false;

		if (request->responseHTTPcode() == 200)
		{
			DynamicJsonBuffer jsonBuffer;
			JsonObject& jsonObject = jsonBuffer.parseObject(request->responseText());
		
			if (jsonObject.success())
			{
				if (jsonObject[F("publicKey")] == dataStore.terminalRecord.serverPublicKey)
				{
					if (semaphoreManager.TryAcquire(SemaphoreManager::Operation::SD_mainLoopWrite))
					{
						JsonArray& rows = jsonObject[F("rows")];
						dataStore.eventType->RemoveFromTableAll();
						DataStore::EventType::Record record;
		
						for (auto value : rows)
						{
							JsonObject& row = value.asObject();
		
							record.id = row.get<uint32_t>(F("id"));
							record.name = row.get<String>(F("name"));
							record.comment = row.get<String>(F("comment"));
		
							dataStore.eventType->PutInTable(NULL, NULL, &record);
						}
		
						semaphoreManager.Release(SemaphoreManager::Operation::SD_mainLoopWrite);
					}
		
					bitClear(tablesToSync, 2);
					SetState(State::pushEvent);
				}
				else									//
				{										//
					#if DEBUG_TASKSYNC					//
					String log;							//
					log += F(DEBUG_TASKSYNC_ID "501");	// Nieprawidlowy klucz serwera
					DEBUG_TASKSYNC_LOG(log);			//
					#endif								//
														//
					error = true;						//
				}										//
			}
			else									//
			{										//
				#if DEBUG_TASKSYNC					//
				String log;							//
				log += F(DEBUG_TASKSYNC_ID "502");	// Blad serwera
				DEBUG_TASKSYNC_LOG(log);			//
				#endif								//
													//
				error = true;						//
			}										//
		}
		else									//
		{										//
			#if DEBUG_TASKSYNC					//
			String log;							//
			log += F(DEBUG_TASKSYNC_ID "503");	// Nieudane polaczenie
			DEBUG_TASKSYNC_LOG(log);			//
			#endif								//
												//
			error = true;						//
		}										//
		
		if (error)						//
		{								// W razie jakiegokolwiek bledu przerywamy
			tablesToSync = 0;			// proces synchronizacji
			SetState(State::idle);		//
		}								//
	}
}

void TaskSync::PushEventProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		recordIndex = 0;	// Tak naprawde nie jest ona uzywana w tym stanie
	
		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}

	// Bit 0 oznacza, ze wyslalismy juz zadanie
	if (bitRead(updateInfo.data_1, 0))
	{
		if (request->readyState() == 4)
		{
			bool error = false;

			if (request->responseHTTPcode() == 200)
			{
				if (request->responseText() == F("success\n1"))											//
				{																						//
					if (semaphoreManager.TryAcquire(SemaphoreManager::Operation::SD_mainLoopWrite))		//
					{																					//
						Ivanseidel::LinkedList<uint8_t> fieldIndexes;									//
						Ivanseidel::LinkedList<uint8_t*> fieldValues;									//
						fieldIndexes.add(DataStore::Event::Field::id);									//
																										// Wyslanie rekordow do serwera sie powiodlo,
						for (uint8_t i = 0; i < eventRecordsCount; ++i)									// usuwamy wiec te rekordy z tabeli lokalnej (tabela 'event')
						{																				// i przygotowujemy sie do nastepnego zadania
							fieldValues.clear();														//
							fieldValues.add((uint8_t*)(&eventRecords[i].id));							//
							dataStore.event->RemoveFromTable(&fieldIndexes, &fieldValues);				//
						}																				//
																										//
						semaphoreManager.Release(SemaphoreManager::Operation::SD_mainLoopWrite);		//
					}																					//
																										//
					bitClear(updateInfo.data_1, 0);														//
				}																						//
				else									//
				{										//
					#if DEBUG_TASKSYNC					//
					String log;							//
					log += F(DEBUG_TASKSYNC_ID "601");	// Blad serwera
					DEBUG_TASKSYNC_LOG(log);			//
					#endif								//
														//
					error = true;						//
				}										//
			}
			else									//
			{										//
				#if DEBUG_TASKSYNC					//
				String log;							//
				log += F(DEBUG_TASKSYNC_ID "602");	// Nieudane polaczenie
				DEBUG_TASKSYNC_LOG(log);			//
				#endif								//
													//
				error = true;						//
			}										//
	
			if (error)					//
			{							// W razie jakiegokolwiek bledu przerywamy
				tablesToSync = 0;		// proces synchronizacji
				SetState(State::idle);	//
			}							//
		}
	}
	else
	{
		if (semaphoreManager.TryAcquire(SemaphoreManager::Operation::SD_mainLoopRead))		//
		{																					//
			eventRecordsCount = dataStore.event->GetFromTableAll(eventRecords, 10);			// Wyciagamy po 10 wierszy z tabeli 'event'
			semaphoreManager.Release(SemaphoreManager::Operation::SD_mainLoopRead);			//
		}																					//

		if (eventRecordsCount)
		{
			StaticJsonBuffer<1024> jsonBuffer;							//
			JsonArray& events = jsonBuffer.createArray();				//
			for (uint8_t i = 0; i < eventRecordsCount; ++i)				//
			{															//
				JsonObject& event = jsonBuffer.createObject();			// Przelewamy dane z tabeli 'eventRecords'
				event[F("time")] = eventRecords[i].time;				// do tabeli 'events' (ktora jest tabela JSON)
				event[F("employeeId")] = eventRecords[i].employeeId;	//
				event[F("eventTypeId")] = eventRecords[i].eventTypeId;	//
				events.add(event);										//
			}															//

			String tempString_1;
			String tempString_2;

			tempString_1 += F("http://");								//
			tempString_1 += dataStore.terminalRecord.serverDomain;		//
			tempString_1 += F("/push-event.php");						// Laczenie
			tempString_2 += F("POST");									//
			request->open(tempString_2.c_str(), tempString_1.c_str());	//

			tempString_1 = String();												//
			tempString_1 += F("Content-Type");										//
			tempString_2 = String();												// Naglowki
			tempString_2 += F("application/x-www-form-urlencoded");					//
			request->setReqHeader(tempString_1.c_str(), tempString_2.c_str());		//

			tempString_1 = String();								//
			tempString_1 += F("publicKey=");						//
			tempString_1 += dataStore.terminalRecord.publicKey;		//
			tempString_1 += F("&events=");							// Cialo
			tempString_2 = String();								//
			events.printTo(tempString_2);							//
			tempString_1 += tempString_2;							//
			request->send(tempString_1);							//

			bitSet(updateInfo.data_1, 0);		// Odznaczamy wyslanie zadania
		}
		else							//
		{								// W tabeli 'event' nie ma nic
			SetState(State::finish);	// wiecej do wyslania
		}								//
	}
}

void TaskSync::FinishProcedure()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		String tempString_1;
		String tempString_2;

		tempString_1 += F("http://");								//
		tempString_1 += dataStore.terminalRecord.serverDomain;		//
		tempString_1 += F("/synchronize-finish.php");				// Laczenie
		tempString_2 += F("POST");									//
		request->open(tempString_2.c_str(), tempString_1.c_str());	//

		tempString_1 = String();												//
		tempString_1 += F("Content-Type");										//
		tempString_2 = String();												// Naglowki
		tempString_2 += F("application/x-www-form-urlencoded");					//
		request->setReqHeader(tempString_1.c_str(), tempString_2.c_str());		//

		tempString_1 = String();								//
		tempString_1 += F("publicKey=");						// Cialo
		tempString_1 += dataStore.terminalRecord.publicKey;		//
		request->send(tempString_1);							//
	
		SetState(State::idle);
	
		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}
}

void TaskSync::GetServerAddress(String& result)
{
	result += String((uint8_t)dataStore.terminalRecord.serverAddress[0]);
	result += '.';
	result += String((uint8_t)dataStore.terminalRecord.serverAddress[1]);
	result += '.';
	result += String((uint8_t)dataStore.terminalRecord.serverAddress[2]);
	result += '.';
	result += String((uint8_t)dataStore.terminalRecord.serverAddress[3]);
}
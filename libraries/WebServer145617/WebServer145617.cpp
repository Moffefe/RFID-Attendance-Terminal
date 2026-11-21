#include "Arduino.h"
#include "WebServer145617.h"



uint16_t WebServer145617::DownloadRequest::nextFreeKey = 0;
String WebServer145617::User::nextFreeUUID;


WebServer145617::WebServer145617(MessageReceiver* mainController, SdFat& sdFat) :
	mainController(mainController),
	sdFat(sdFat)
{
	pServer = NULL;
	pUser = NULL;
	interruptData = NULL;
}

void WebServer145617::begin()
{
	if (semaphoreManager.TryAcquire(SemaphoreManager::Operation::SD_mainLoopWrite))
	{
		PrepareMassMemory();
		updateInfo.data_1 = bit(sizeof(updateInfo.data_1) * 8 - 1);
		updateManager.Attach(this);
		semaphoreManager.Release(SemaphoreManager::Operation::SD_mainLoopWrite);
	}
}

void WebServer145617::Update()
{
	if (bitRead(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1))
	{
		CreateServer();

		bitClear(updateInfo.data_1, sizeof(updateInfo.data_1) * 8 - 1);
		return;
	}

	if (pUser)											//
	{													//
		if ((long)(millis() - pUser->kickTime) >= 0)	//
		{												//
			delete pUser;								//
			pUser = NULL;								//
														// Jesli zalogowany uzytkownik jest nieaktywny
			#if DEBUG_WEBSERVER145617					// od jakiegos czasu, to go wylogowujemy
			String log;									//
			log += F(DEBUG_WEBSERVER145617_ID "11");	//
			DEBUG_WEBSERVER145617_LOG(log);				//
			#endif										//
		}												//
	}													//
}

bool WebServer145617::TakeMessage(int message, void* sender, void** additionalData)
{
	switch (message)
	{
		case MESSAGE_TIME_KEEPER:
		{
			switch (*((uint16_t*)additionalData))
			{
				case MESSAGE_FORMAT:
				{
					String tempString_1;

					if (semaphoreManager.TryAcquire(SemaphoreManager::Operation::SD_mainLoopWrite))
					{
						File file;
						tempString_1 += F("/FormatFlag.txt");
						file.open(tempString_1.c_str(), O_WRITE | O_CREAT | O_TRUNC);
						file.write('a');
						file.close();

						#if DEBUG_WEBSERVER145617
						String log;
						log += F(DEBUG_WEBSERVER145617_ID "21");
						DEBUG_WEBSERVER145617_LOG(log);
						#endif

						semaphoreManager.Release(SemaphoreManager::Operation::SD_mainLoopWrite);
					}

					break;
				}

				case MESSAGE_CHANGE_SETTINGS:
				{
					if (interruptData)
					{
						dataStore.terminalRecord = *((DataStore::Terminal::Record*)interruptData);

						if (semaphoreManager.TryAcquire(SemaphoreManager::Operation::SD_mainLoopWrite))
						{
							dataStore.terminal->RemoveFromTableAll();
							dataStore.terminal->PutInTable(NULL, NULL, &dataStore.terminalRecord);

							#if DEBUG_WEBSERVER145617
							String log;
							log += F(DEBUG_WEBSERVER145617_ID "22");
							DEBUG_WEBSERVER145617_LOG(log);
							#endif

							semaphoreManager.Release(SemaphoreManager::Operation::SD_mainLoopWrite);
						}

						delete ((DataStore::Terminal::Record*)interruptData);
						interruptData = NULL;
					}

					break;
				}

				case MESSAGE_GENERATE_USER_UUID:
				{
					User::GenerateUUID();
					break;
				}
			}

			delete ((uint16_t*)additionalData);
			return true;
		}
	}

	return false;
}

void WebServer145617::CallbackNotFound(AsyncWebServerRequest* request)
{
	if (semaphoreManager.TryAcquire(SemaphoreManager::Operation::SD_interruptRead))
	{
		String tempString_1;
		String tempString_2;
		bool compressed = false;
		bool canBeCached = false;
		String contentType;
		File file;
		String fileName;
	
		tempString_1 += F("/Assets/");
		tempString_2 += F("/");
		if (request->url().startsWith(tempString_1.c_str()) && !request->url().endsWith(tempString_2.c_str()))
		{
			fileName = request->url();
			if (sdFat.exists(fileName.c_str()))					//
			{													//
				file.open(fileName.c_str(), O_READ);			// Posiadamy dokladnie taki plik o jaki prosi klient
				canBeCached = true;								//
				contentType = GetContentType(request->url());	//
			}													//
			else
			{
				fileName = request->url();
				fileName += F(".gz");
				if (sdFat.exists(fileName.c_str()))							//
				{															//
					file.open(fileName.c_str(), O_READ);					// Posiadamy plik o ktory prosi klient,
					compressed = true;										// ale w wersji skompresowanej
					canBeCached = true;										//
					contentType = GetContentType(request->url());			//
				}															//
			}
		}
		else
		{
			tempString_1 = String();
			tempString_1 += F("/Logs/");
			if ((request->url().startsWith(tempString_1.c_str())) && !request->url().endsWith(tempString_2.c_str()) && (serialListener.GetStateLogger() == serialListener.collectStopped) && AuthenticateUser(request))		//
			{																																																				//
				fileName = request->url();																																													//
				if (sdFat.exists(fileName.c_str()))																																											//
				{																																																			// Dostep do folderu z logami jest mozliwy
					file.open(fileName.c_str(), O_READ);																																									// tylko gdy zbieranie logow jest zatrzymane
					contentType = GetContentType(request->url());																																							//
				}																																																			//
			}																																																				//
			else																				//
			{																					//
				fileName += F("/Htdocs");														//
				fileName += request->url();														//
																								//
				if (fileName.endsWith(tempString_2.c_str()))									//
				{																				//
					fileName += F("index.html");												//
				}																				//
				else																			//
				{																				//
					tempString_1 = String();													//
					tempString_1 += F(".html");													//
					if (!fileName.endsWith(tempString_1.c_str()))								//
						fileName += tempString_1;												//
				}																				// Zakladamy, ze klient prosi o plik .html (strona)
																								//
				if (sdFat.exists(fileName.c_str()))												//
				{																				//
					file.open(fileName.c_str(), O_READ);										//
					contentType = GetContentType(fileName);										//
					contentType += F("; charset=UTF-8");										//
				}																				//
				else																			//
				{																				//
					fileName += F(".gz");														//
					if (sdFat.exists(fileName.c_str()))											//
					{																			//
						file.open(fileName.c_str(), O_READ);									//
						compressed = true;														//
						contentType = GetContentType(fileName);									//
						contentType += F("; charset=UTF-8");									//
					}																			//
				}																				//
			}																					//
		}
	
		if (file)
		{
			#if DEBUG_WEBSERVER145617						//
			String log;										//
			log += F(DEBUG_WEBSERVER145617_ID "101\x1F");	//
			log += fileName;								//
			log += '\x1F';									// Udalo sie znalezc plik o ktory prosi klient,
			log += file.size();								// rozpoczynamy wysylanie
			log += '\x1F';									//
			log += contentType;								//
			DEBUG_WEBSERVER145617_LOG(log);					//
			#endif											//
	
			DownloadRequest* pDownloadRequest = new DownloadRequest(fileName);
			downloadRequests.add(pDownloadRequest);
			request->onDisconnect(std::bind(&WebServer145617::CallbackNotFoundDisconnect, this, pDownloadRequest->key));
			AsyncWebServerResponse* response = request->beginResponse(contentType, file.size(), std::bind(&WebServer145617::CallbackNotFoundFiller, this, pDownloadRequest->key, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	
			if (compressed)
			{
				tempString_1 = String();
				tempString_2 = String();
				tempString_1 += F("Content-Encoding");
				tempString_2 += F("gzip");
				response->addHeader(tempString_1.c_str(), tempString_2.c_str());
			}
	
			if (canBeCached)
			{
				tempString_1 = String();
				tempString_2 = String();
				tempString_1 += F("Cache-Control");
				tempString_2 += F("public, max-age=86400");
				response->addHeader(tempString_1.c_str(), tempString_2.c_str());
			}
	
			request->send(response);
			file.close();
			return;
		}
		else
		{
			if (!downloadRequests.size())
				semaphoreManager.Release(SemaphoreManager::Operation::SD_interruptRead);
		}
	}

	request->send(404);								//
													//
	#if DEBUG_WEBSERVER145617						//
	String log;										// W powyzszych instrukcjach nie udalo sie znalezc pliku o ktory prosi klient,
	log += F(DEBUG_WEBSERVER145617_ID "102\x1F");	// a wiec zwracamy 404
	log += request->url();							//
	DEBUG_WEBSERVER145617_LOG(log);					//
	#endif											//
}

size_t WebServer145617::CallbackNotFoundFiller(uint16_t downloadRequestKey, uint8_t* buffer, size_t maxLen, size_t index)
{
	size_t bytesToSend = 0;
	if (semaphoreManager.TryAcquire(SemaphoreManager::Operation::SD_interruptRead))
	{
		uint8_t downloadRequestIndex = 0;
		File file;
	
		for (; downloadRequestIndex < downloadRequests.size(); ++downloadRequestIndex)				//
		{																							//
			if (downloadRequests.get(downloadRequestIndex)->key == downloadRequestKey)				// Lista 'downloadRequests' przechowuje wszystkie aktualnie majace miejsce
			{																						// akcje wysylania plikow do klientow. Na podstawie 'downloadRequestKey' mozemy
				file.open(downloadRequests.get(downloadRequestIndex)->fileName.c_str(), O_READ);	// ustalic na rzecz ktorej z tych akcji zostala wywolana ta metoda
				break;																				//
			}																						//
		}																							//
	
		if (file)
		{
			if (index < file.size())
			{
				file.seekSet(index);
	
				if (file.available() >= maxLen)
				{
					bytesToSend = maxLen;
					file.read(buffer, bytesToSend);
				}
				else
				{
					bytesToSend = file.available();
					file.read(buffer, bytesToSend);
				}
	
				if (index + bytesToSend >= file.size())
				{
					#if DEBUG_WEBSERVER145617			
					String log;
					log += F(DEBUG_WEBSERVER145617_ID "121\x1F");
					log += downloadRequests.get(downloadRequestIndex)->fileName;
					DEBUG_WEBSERVER145617_LOG(log);
					#endif
	
					delete downloadRequests.get(downloadRequestIndex);
					downloadRequests.remove(downloadRequestIndex);
	
					if (!downloadRequests.size())
						semaphoreManager.Release(SemaphoreManager::Operation::SD_interruptRead);
				}
			}
	
			file.close();
		}
		else
		{
			if (downloadRequestIndex < downloadRequests.size())
			{
				delete downloadRequests.get(downloadRequestIndex);
				downloadRequests.remove(downloadRequestIndex);

				if (!downloadRequests.size())
					semaphoreManager.Release(SemaphoreManager::Operation::SD_interruptRead);
			}
	
			#if DEBUG_WEBSERVER145617			
			String log;
			log += F(DEBUG_WEBSERVER145617_ID "122");
			DEBUG_WEBSERVER145617_LOG(log);
			#endif
		}
	}
	
	return bytesToSend;
}

void WebServer145617::CallbackNotFoundDisconnect(uint16_t downloadRequestKey)
{
	for (uint8_t i = 0; i < downloadRequests.size(); ++i)
	{
		if (downloadRequests.get(i)->key == downloadRequestKey)
		{
			#if DEBUG_WEBSERVER145617			
			String log;
			log += F(DEBUG_WEBSERVER145617_ID "141");
			DEBUG_WEBSERVER145617_LOG(log);
			#endif
	
			delete downloadRequests.get(i);
			downloadRequests.remove(i);

			if (!downloadRequests.size() && semaphoreManager.TryAcquire(SemaphoreManager::Operation::SD_interruptRead))
				semaphoreManager.Release(SemaphoreManager::Operation::SD_interruptRead);

			break;
		}
	}
}

void WebServer145617::CallbackLogin(AsyncWebServerRequest* request)
{
	if (AuthenticateUser(request))					//
	{												//
		String additionalData = "1";				// uzytkownik jest juz zalogowany
		ErrorResponse(request, &additionalData);	//
		return;										//
	}												//

	String tempString_1;
	String tempString_2;
	tempString_1 += F("username");
	tempString_2 += F("password");

	if (request->hasArg(tempString_1.c_str()) && request->hasArg(tempString_2.c_str()))
	{
		if ( (request->arg(tempString_1.c_str()) == dataStore.terminalRecord.username) && (request->arg(tempString_2.c_str()) == dataStore.terminalRecord.userPassword) )
		{
			if (pUser)															//
				delete pUser;													//
			pUser = new User();													// Wyrzucamy poprzednio zalogowanego uzytkownika
			uint16_t* subMessage = new uint16_t();								// i logujemy nowego
			*subMessage = MESSAGE_GENERATE_USER_UUID;							//
			timeKeeper.RequestMessage(millis(), this, (void**)subMessage);		//

			String uuidCookie;							// Dla nowo-zalogowanego uzytkownika przygotowujemy ciasteczko,
			uuidCookie += F("PROJEKTISRCPUUID=");		// dzieki czemu bedziemy mogli go rozpoznac
			uuidCookie += pUser->UUID;					//
			tempString_1 = String();																						//
			tempString_2 = String();																						//
			tempString_1 += F("text/plain");																				//
			tempString_2 += F("Success\n1");																				//
			AsyncWebServerResponse* response = request->beginResponse(200, tempString_1.c_str(), tempString_2.c_str());		//
			tempString_1 = String();																						// Wysylamy mu wiadomosc o powodzeniu logowania,
			tempString_2 = String();																						// wraz z ciasteczkiem
			tempString_1 += F("Cache-Control");																				//
			tempString_2 += F("no-cache");																					//
			response->addHeader(tempString_1.c_str(), tempString_2.c_str());												//
			tempString_1 = String();																						//
			tempString_1 += F("Set-Cookie");																				//
			response->addHeader(tempString_1.c_str(), uuidCookie);															//
			request->send(response);																						//
			return;
		}
	}

	String additionalData = "2";				// Nie udalo sie zalogowac
	ErrorResponse(request, &additionalData);	//
}

void WebServer145617::CallbackLogout(AsyncWebServerRequest* request)
{
	if (!EnterRestrictedResource(request))		// Jesli uzytkownik jest niezalogowany to
		return;									// zwracamy blad "1"

	delete pUser;
	pUser = NULL;

	SuccessResponse(request);
}

void WebServer145617::CallbackFormat(AsyncWebServerRequest* request)
{
	if (!EnterRestrictedResource(request))		// Jesli uzytkownik jest niezalogowany to
		return;									// zwracamy blad "1"

	uint16_t* subMessage = new uint16_t();
	*subMessage = MESSAGE_FORMAT;
	timeKeeper.RequestMessage(millis(), this, (void**)subMessage);

	SuccessResponse(request);
}

void WebServer145617::CallbackShutdown(AsyncWebServerRequest* request)
{
	if (!EnterRestrictedResource(request))		// Jesli uzytkownik jest niezalogowany to
		return;									// zwracamy blad "1"

	mainController->TakeMessage(MESSAGE_HALT, this, NULL);

	SuccessResponse(request);
}

void WebServer145617::CallbackRestart(AsyncWebServerRequest* request)
{
	if (!EnterRestrictedResource(request))		// Jesli uzytkownik jest niezalogowany to
		return;									// zwracamy blad "1"

	mainController->TakeMessage(MESSAGE_RESTART, this, NULL);

	SuccessResponse(request);
}

void WebServer145617::CallbackGet(AsyncWebServerRequest* request)
{
	if (!EnterRestrictedResource(request))		// Jesli uzytkownik jest niezalogowany to
		return;									// zwracamy blad "1"

	String additionalData;
	additionalData += dataStore.terminalRecord.name;
	additionalData += '\n';
	additionalData += dataStore.terminalRecord.areaName;
	additionalData += '\n';
	additionalData += dataStore.terminalRecord.passageName;
	additionalData += '\n';
	additionalData += dataStore.terminalRecord.ssid;
	additionalData += '\n';
	additionalData += dataStore.terminalRecord.useStaticIp;
	additionalData += '\n';
	additionalData += dataStore.terminalRecord.staticIp;
	additionalData += '\n';
	additionalData += dataStore.terminalRecord.staticGateway;
	additionalData += '\n';
	additionalData += dataStore.terminalRecord.staticSubnet;
	additionalData += '\n';
	additionalData += dataStore.terminalRecord.staticDNS;
	additionalData += '\n';
	additionalData += dataStore.terminalRecord.connectRetries;
	additionalData += '\n';
	additionalData += dataStore.terminalRecord.apChannel;
	additionalData += '\n';
	additionalData += dataStore.terminalRecord.publicKey;
	additionalData += '\n';
	additionalData += dataStore.terminalRecord.serverDomain;
	additionalData += '\n';
	additionalData += dataStore.terminalRecord.serverPublicKey;
	additionalData += '\n';
	additionalData += dataStore.terminalRecord.syncInterval;
	additionalData += '\n';
	additionalData += dataStore.terminalRecord.authenticateMethod;
	SuccessResponse(request, &additionalData);
}

void WebServer145617::CallbackSet(AsyncWebServerRequest* request)
{
	if (!EnterRestrictedResource(request))		// Jesli uzytkownik jest niezalogowany to
		return;									// zwracamy blad "1"

	if (request->params() == 14)
	{
		bool badParam = false;

		if (request->getParam(0)->value().length() > 31)	// ssid
			badParam = true;								//

		if (request->getParam(1)->value().length() > 31)	// password
			badParam = true;								//

		if (request->getParam(9)->value().length() > 32)	// publicKey
			badParam = true;								//

		if (request->getParam(10)->value().length() > 31)	// serverDomain
			badParam = true;								//

		if (request->getParam(11)->value().length() > 32)	// serverPublicKey
			badParam = true;								//

		if (!badParam)
		{
			DataStore::Terminal::Record* record = new DataStore::Terminal::Record;
			*record = dataStore.terminalRecord;
			record->ssid = request->getParam(0)->value();
			record->password = request->getParam(1)->value();
			record->useStaticIp = request->getParam(2)->value() == "true" ? true : false;
			//record->staticIp = request->getParam(3);
			//record->staticGateway = request->getParam(4);
			//record->staticSubnet = request->getParam(5);
			//record->staticDNS = request->getParam(6);
			record->connectRetries = request->getParam(7)->value().toInt();
			record->apChannel = request->getParam(8)->value().toInt();
			record->publicKey = request->getParam(9)->value();
			record->serverDomain = request->getParam(10)->value();
			record->serverPublicKey = request->getParam(11)->value();
			record->syncInterval = request->getParam(12)->value().toInt();
			record->authenticateMethod = request->getParam(13)->value().toInt();
			interruptData = (uint8_t*)record;

			uint16_t* subMessage = new uint16_t();
			*subMessage = MESSAGE_CHANGE_SETTINGS;
			timeKeeper.RequestMessage(millis(), this, (void**)subMessage);

			SuccessResponse(request);
		}
		else											//
		{												//
			String additionalData = "2";				// Nieprawidlowe parametry
			ErrorResponse(request, &additionalData);	//
		}												//
	}
	else											//
	{												//
		String additionalData = "3";				// Nieprawidlowa ilosc parametrow
		ErrorResponse(request, &additionalData);	//
	}												//
}

// Sprawdza czy uzytkownik ktory sie
// podlaczyl jest zalogowany
bool WebServer145617::AuthenticateUser(AsyncWebServerRequest* request)
{
	if (pUser)
	{
		String tempString_1;
		tempString_1 += F("Cookie");

		if (request->hasHeader(tempString_1.c_str()))
		{
			String cookie = request->header(tempString_1.c_str());
			tempString_1 = String();
			tempString_1 += F("PROJEKTISRCPUUID=");
			int beginIndex = cookie.indexOf(tempString_1.c_str()) + 17;
			int endIndex = beginIndex + 36;
			String uuid = cookie.substring(beginIndex, endIndex);

			if (uuid == pUser->UUID)
				return true;
		}
	}

	return false;
}

// Ta funkcja musi byc wywolywana
// na poczatku kazdego callbacka powiazanego z zasobem,
// ktory jest dostepny tylko dla admina
bool WebServer145617::EnterRestrictedResource(AsyncWebServerRequest* request)
{
	if (!AuthenticateUser(request))
	{
		String additionalData = "1";
		ErrorResponse(request, &additionalData);
		return false;
	}
	else
	{
		pUser->Renew();
		return true;
	}
}

void WebServer145617::SuccessResponse(AsyncWebServerRequest* request, String* additionalString)
{
	String tempString_1;

	String response;
	response += F("Success");

	if (additionalString)
	{
		response += '\n';
		response += *additionalString;
	}

	tempString_1 += F("text/plain");
	request->send(200, tempString_1.c_str(), response);
}

void WebServer145617::ErrorResponse(AsyncWebServerRequest* request, String* additionalString)
{
	String tempString_1;

	String response;
	response += F("Error");

	if (additionalString)
	{
		response += '\n';
		response += *additionalString;
	}

	tempString_1 += F("text/plain");
	request->send(200, tempString_1.c_str(), response);
}

String WebServer145617::GetContentType(const String& path)
{
	char buff[sizeof(mimeTable[0].mimeType)];
	// Check all entries but last one for match, return if found
	for (size_t i = 0; i < sizeof(mimeTable) / sizeof(mimeTable[0]) - 1; i++) {
		strcpy_P(buff, mimeTable[i].endsWith);
		if (path.endsWith(buff)) {
			strcpy_P(buff, mimeTable[i].mimeType);
			return String(buff);
		}
	}
	// Fall-through and just return default type
	strcpy_P(buff, mimeTable[sizeof(mimeTable) / sizeof(mimeTable[0]) - 1].mimeType);
	return String(buff);
}

// Jej zadaniem jest przygotowanie wszystkich
// folderow potrzebnych do dzialania tej klasy
void WebServer145617::PrepareMassMemory()
{
	String tempString_1;
	File file;

	tempString_1 += F("/Htdocs");
	if (!file.open(tempString_1.c_str(), O_READ))
		file.mkdir(File::cwd(), tempString_1.c_str());
	file.close();

	tempString_1 = String();
	tempString_1 += F("/Assets");
	if (!file.open(tempString_1.c_str(), O_READ))
		file.mkdir(File::cwd(), tempString_1.c_str());
	file.close();
}

void WebServer145617::CreateServer()
{
	String tempString_1;

	pServer = new AsyncWebServer(80);

	pServer->onNotFound(std::bind(&WebServer145617::CallbackNotFound, this, std::placeholders::_1));
	tempString_1 += F("/login");
	pServer->on(tempString_1.c_str(), HTTP_ANY, std::bind(&WebServer145617::CallbackLogin, this, std::placeholders::_1));
	tempString_1 = String();
	tempString_1 += F("/logout");
	pServer->on(tempString_1.c_str(), HTTP_ANY, std::bind(&WebServer145617::CallbackLogout, this, std::placeholders::_1));
	tempString_1 = String();
	tempString_1 += F("/format");
	pServer->on(tempString_1.c_str(), HTTP_ANY, std::bind(&WebServer145617::CallbackFormat, this, std::placeholders::_1));
	tempString_1 = String();
	tempString_1 += F("/shutdown");
	pServer->on(tempString_1.c_str(), HTTP_ANY, std::bind(&WebServer145617::CallbackShutdown, this, std::placeholders::_1));
	tempString_1 = String();
	tempString_1 += F("/restart");
	pServer->on(tempString_1.c_str(), HTTP_ANY, std::bind(&WebServer145617::CallbackRestart, this, std::placeholders::_1));
	tempString_1 = String();
	tempString_1 += F("/get");
	pServer->on(tempString_1.c_str(), HTTP_ANY, std::bind(&WebServer145617::CallbackGet, this, std::placeholders::_1));
	tempString_1 = String();
	tempString_1 += F("/set");
	pServer->on(tempString_1.c_str(), HTTP_ANY, std::bind(&WebServer145617::CallbackSet, this, std::placeholders::_1));

	pServer->begin();
	User::GenerateUUID();

	#if DEBUG_WEBSERVER145617					//
	String log;									//
	log += F(DEBUG_WEBSERVER145617_ID "501");	// Utworzono serwer HTTP
	DEBUG_WEBSERVER145617_LOG(log);				//
	#endif										//
}

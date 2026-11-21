/*
DataStore - klasa agregujaca w pamieci masowej wszystkie dane,
            ktore nie sa specyficzne dla jednej klasy, czyli te
			do ktorych musza miec dostep dwie, lub wiecej klas
*/
#ifndef DataStore_h
#define DataStore_h

#define DEBUG_DATASTORE 1
#if DEBUG_DATASTORE
#define DEBUG_DATASTORE_ID "3_"

//#include <SerialListener.h>
//#define DEBUG_DATASTORE_LOG(...) serialListener.Log(__VA_ARGS__)
#define DEBUG_DATASTORE_LOG(...) Serial1.println(__VA_ARGS__)
#endif

#include <stdint.h>
#include <SemaphoreManager.h>
#include <MessageReceiver.h>
#include <MessageUsers.h>
#include <DataBaseTable.h>
#include <LinkedList.h>


class DataStore : public MessageReceiver
{
public:
	struct Terminal : public DataBaseTable
	{
		enum Field
		{
			id,

			name,
			areaName,
			passageName,

			ssid,
			password,
			useStaticIp,
			staticIp,
			staticGateway,
			staticSubnet,
			staticDNS,
			connectRetries,
			connectRetriesCurrent,
			apChannel,
			username,
			userPassword,

			privateKey,
			publicKey,
			serverAddress,
			serverDomain,
			serverPublicKey,
			syncInterval,

			authenticateMethod,

			timeZoneOffset,
			timeZoneChange,
			timeZoneOffsetNext
		};

		struct Record
		{
			uint32_t id;

			String name;						// np. "Przy wejsciu glownym"
			String areaName;						// Nazwa strefy w ktorej znajduje sie terminal
			String passageName;							// Nazwa przejscia przy ktorym jest zainstalowany terminal

			String ssid;						// Nazwa AP do ktorego ma podlaczyc sie terminal
			String password;						// Haslo do AP
			uint8_t useStaticIp;						// 0 - uzywac DHCP, 1 - uzywac stalego IP
			String staticIp;								//
			String staticGateway;							// Potrzebne gdy 'useStaticIp'
			String staticSubnet;							// jest true
			String staticDNS;								//
			uint8_t connectRetries;								// Ile prob podlaczenia sie do AP ma podjac terminal, przed przejsciem w tryb offline
			uint8_t connectRetriesCurrent;							// Ilosc dotad nieudanych prob podlaczenia
			uint8_t apChannel;											// Na jakim kanale ma byc tworzony AP w trybie offline
			String username;					// Dane dostepowe do serwera lokalnego
			String userPassword;				// (serwer terminala)

			String privateKey;					// Klucz prywatny terminala (Do podpisu cyfrowego)
			String publicKey;						// Klucz publiczny terminala (Do podpisu cyfrowego)
			String serverAddress;						// Adres serwera glownego
			String serverDomain;						//
			String serverPublicKey;							// Klucz publiczny serwera glownego (Do podpisu cyfrowego)
			uint32_t syncInterval;								// Co ile terminal wysyla dane na serwer, a takze sprawdza czy nie zaszly jakies zmiany (np. nie zatrudniono nowego pracownika)

			uint8_t authenticateMethod;			// Sposob autentykacji pracownikow. 0 - RFID + numpad, 1 - RFID, 2 - numpad

			int32_t timeZoneOffset;				// Ofset (w sekundach) miedzy czasem Unixowym, a czasem systemowym
			uint32_t timeZoneChange;				// Punkt w przyszlosci (wg czasu systemowego) kiedy ma nastapic najblizsza zmiana czasu
			int32_t timeZoneOffsetNext;					// Nowy ofset ktory bedzie obowiazywal po zmianie czasu
		};



		Terminal()
		{
			String fileName;								// Okreslenie pliku w ktorym bedzie
			fileName += F("/DataStore/Terminal.bin");		// znajdowala sie tabela

			uint16_t* fieldLengths = new uint16_t[26];								//
			fieldLengths[0] = sizeof(Record::id);									//
			fieldLengths[1] = 32;													//
			fieldLengths[2] = 32;													//
			fieldLengths[3] = 32;													//
																					//
			fieldLengths[4] = 32;													//
			fieldLengths[5] = 32;													//
			fieldLengths[6] = sizeof(Record::useStaticIp);							//
			fieldLengths[7] = 5;													//
			fieldLengths[8] = 5;													// Okreslenie dlugosci poszczegolnych
			fieldLengths[9] = 5;													// pol rekordu
			fieldLengths[10] = 5;													//
			fieldLengths[11] = sizeof(Record::connectRetries);						//
			fieldLengths[12] = sizeof(Record::connectRetriesCurrent);				//
			fieldLengths[13] = sizeof(Record::apChannel);							//
			fieldLengths[14] = 32;													//
			fieldLengths[15] = 32;													//
																					//
			fieldLengths[16] = 33;													//
			fieldLengths[17] = 33;													//
			fieldLengths[18] = 5;													//
			fieldLengths[19] = 32;													//
			fieldLengths[20] = 33;													//
			fieldLengths[21] = sizeof(Record::syncInterval);						//
																					//
			fieldLengths[22] = sizeof(Record::authenticateMethod);					//
																					//
			fieldLengths[23] = sizeof(Record::timeZoneOffset);						//
			fieldLengths[24] = sizeof(Record::timeZoneChange);						//
			fieldLengths[25] = sizeof(Record::timeZoneOffsetNext);					//

			uint8_t* fieldTypes = new uint8_t[4];									//
			fieldTypes[0] = bit(1) | bit(2) | bit(3) | bit(4) | bit(5) | bit(7);	// Okreslenie typow poszczegolnych
			fieldTypes[1] = bit(0) | bit(1) | bit(2) | bit(6) | bit(7);				// pol rekordu
			fieldTypes[2] = bit(0) | bit(1) | bit(2) | bit(3) | bit(4);				//
			fieldTypes[3] = 0;														//

			begin(fileName, 26, fieldLengths, fieldTypes);

			Record record;															//
			if (GetFromTableMax<uint32_t>(Field::id, (uint8_t*)(&record)))			//
				nextFreePK = record.id + 1;											// Inicjujemy klucz glowny tabeli
			else																	//
				nextFreePK = 0;														//
		}

		uint16_t GetFromTableAll(Record* buffer, size_t bufferSize, uint32_t beginOffset = 0)
		{
			return DataBaseTable::GetFromTableAll((uint8_t*)buffer, bufferSize, beginOffset);
		}

		uint16_t GetFromTable(Ivanseidel::LinkedList<uint8_t>* fieldIndexes, Ivanseidel::LinkedList<uint8_t*>* fieldValues, Record* buffer, size_t bufferSize)
		{
			return DataBaseTable::GetFromTable(fieldIndexes, fieldValues, (uint8_t*)buffer, bufferSize);
		}

		void PutInTable(Ivanseidel::LinkedList<uint8_t>* fieldIndexes, Ivanseidel::LinkedList<uint8_t*>* fieldValues, Record* objectRecord)
		{
			DataBaseTable::PutInTable(fieldIndexes, fieldValues, (uint8_t*)objectRecord);
		}

		uint32_t GetNextFreePK()
		{
			uint32_t result = nextFreePK;
			++nextFreePK;
			return result;
		}

	protected:
		uint32_t nextFreePK;



		void HandleRead(uint8_t* objectRecord, uint8_t* fileRecord)
		{
			Record* objectRecord_2 = (Record*)objectRecord;
			uint16_t* offsets = new uint16_t[fieldCount];
			GetOffsets(NULL, offsets);

			memcpy(&(objectRecord_2->id),						fileRecord + offsets[0], fieldLengths[0]);

			objectRecord_2->name =								String((char*)fileRecord + offsets[1]);
			objectRecord_2->areaName =							String((char*)fileRecord + offsets[2]);
			objectRecord_2->passageName =						String((char*)fileRecord + offsets[3]);

			objectRecord_2->ssid =								String((char*)fileRecord + offsets[4]);
			objectRecord_2->password =							String((char*)fileRecord + offsets[5]);
			memcpy(&(objectRecord_2->useStaticIp),				fileRecord + offsets[6], fieldLengths[6]);
			objectRecord_2->staticIp =							String((char*)fileRecord + offsets[7]);
			objectRecord_2->staticGateway =						String((char*)fileRecord + offsets[8]);
			objectRecord_2->staticSubnet =						String((char*)fileRecord + offsets[9]);
			objectRecord_2->staticDNS =							String((char*)fileRecord + offsets[10]);
			memcpy(&(objectRecord_2->connectRetries),			fileRecord + offsets[11], fieldLengths[11]);
			memcpy(&(objectRecord_2->connectRetriesCurrent),	fileRecord + offsets[12], fieldLengths[12]);
			memcpy(&(objectRecord_2->apChannel),				fileRecord + offsets[13], fieldLengths[13]);
			objectRecord_2->username =							String((char*)fileRecord + offsets[14]);
			objectRecord_2->userPassword =						String((char*)fileRecord + offsets[15]);

			objectRecord_2->privateKey =						String((char*)fileRecord + offsets[16]);
			objectRecord_2->publicKey =							String((char*)fileRecord + offsets[17]);
			objectRecord_2->serverAddress =						String((char*)fileRecord + offsets[18]);
			objectRecord_2->serverDomain =						String((char*)fileRecord + offsets[19]);
			objectRecord_2->serverPublicKey =					String((char*)fileRecord + offsets[20]);
			memcpy(&(objectRecord_2->syncInterval),				fileRecord + offsets[21], fieldLengths[21]);

			memcpy(&(objectRecord_2->authenticateMethod),		fileRecord + offsets[22], fieldLengths[22]);

			memcpy(&(objectRecord_2->timeZoneOffset),			fileRecord + offsets[23], fieldLengths[23]);
			memcpy(&(objectRecord_2->timeZoneChange),			fileRecord + offsets[24], fieldLengths[24]);
			memcpy(&(objectRecord_2->timeZoneOffsetNext),		fileRecord + offsets[25], fieldLengths[25]);

			delete[] offsets;
		}

		void HandleWrite(uint8_t* objectRecord, uint8_t* fileRecord)
		{
			Record* objectRecord_2 = (Record*)objectRecord;
			uint16_t* offsets = new uint16_t[fieldCount];
			GetOffsets(NULL, offsets);

			memcpy(fileRecord + offsets[0],		&(objectRecord_2->id),								fieldLengths[0]);

			memcpy(fileRecord + offsets[1],		objectRecord_2->name.c_str(),						objectRecord_2->name.length() + 1);
			memcpy(fileRecord + offsets[2],		objectRecord_2->areaName.c_str(),					objectRecord_2->areaName.length() + 1);
			memcpy(fileRecord + offsets[3],		objectRecord_2->passageName.c_str(),				objectRecord_2->passageName.length() + 1);

			memcpy(fileRecord + offsets[4],		objectRecord_2->ssid.c_str(),						objectRecord_2->ssid.length() + 1);
			memcpy(fileRecord + offsets[5],		objectRecord_2->password.c_str(),					objectRecord_2->password.length() + 1);
			memcpy(fileRecord + offsets[6],		&(objectRecord_2->useStaticIp),						fieldLengths[6]);
			memcpy(fileRecord + offsets[7],		objectRecord_2->staticIp.c_str(),					objectRecord_2->staticIp.length() + 1);
			memcpy(fileRecord + offsets[8],		objectRecord_2->staticGateway.c_str(),				objectRecord_2->staticGateway.length() + 1);
			memcpy(fileRecord + offsets[9],		objectRecord_2->staticSubnet.c_str(),				objectRecord_2->staticSubnet.length() + 1);
			memcpy(fileRecord + offsets[10],	objectRecord_2->staticDNS.c_str(),					objectRecord_2->staticDNS.length() + 1);
			memcpy(fileRecord + offsets[11],	&(objectRecord_2->connectRetries),					fieldLengths[11]);
			memcpy(fileRecord + offsets[12],	&(objectRecord_2->connectRetriesCurrent),			fieldLengths[12]);
			memcpy(fileRecord + offsets[13],	&(objectRecord_2->apChannel),						fieldLengths[13]);
			memcpy(fileRecord + offsets[14],	objectRecord_2->username.c_str(),					objectRecord_2->username.length() + 1);
			memcpy(fileRecord + offsets[15],	objectRecord_2->userPassword.c_str(),				objectRecord_2->userPassword.length() + 1);

			memcpy(fileRecord + offsets[16],	objectRecord_2->privateKey.c_str(),					objectRecord_2->privateKey.length() + 1);
			memcpy(fileRecord + offsets[17],	objectRecord_2->publicKey.c_str(),					objectRecord_2->publicKey.length() + 1);
			memcpy(fileRecord + offsets[18],	objectRecord_2->serverAddress.c_str(),				objectRecord_2->serverAddress.length() + 1);
			memcpy(fileRecord + offsets[19],	objectRecord_2->serverDomain.c_str(),				objectRecord_2->serverDomain.length() + 1);
			memcpy(fileRecord + offsets[20],	objectRecord_2->serverPublicKey.c_str(),			objectRecord_2->serverPublicKey.length() + 1);
			memcpy(fileRecord + offsets[21],	&(objectRecord_2->syncInterval),					fieldLengths[21]);

			memcpy(fileRecord + offsets[22],	&(objectRecord_2->authenticateMethod),				fieldLengths[22]);

			memcpy(fileRecord + offsets[23],	&(objectRecord_2->timeZoneOffset),					fieldLengths[23]);
			memcpy(fileRecord + offsets[24],	&(objectRecord_2->timeZoneChange),					fieldLengths[24]);
			memcpy(fileRecord + offsets[25],	&(objectRecord_2->timeZoneOffsetNext),				fieldLengths[25]);

			delete[] offsets;
		}

		uint16_t HandleGetSize()
		{
			return sizeof(Record);
		}
	};

	struct Employee : public DataBaseTable
	{
		enum Field
		{
			id,

			name,
			surname,
			passcodeHash,
			tagCodeHash,
			accessDays,
			accessFromHour,
			accessToHour,
			rank
		};

		struct Record
		{
			uint32_t id;				// Id pracownika (w bazie danych glownego serwera)

			String name;					// Imie pracownika
			String surname;						// Nazwisko pracownika
			String passcodeHash;					// Kod dostepu pracownika (hash)
			String tagCodeHash;							// Identyfikator RFID pracownika (hash)
			uint8_t accessDays;								//
			uint8_t accessFromHour;							// Te pola okreslaja przedzial czasowy w jakim dany pracownik moze autoryzowac sie w tym terminalu
			uint8_t accessToHour;							//
			uint8_t rank;										// Ranga pracownika. 0 - administrator, 1 - zwykly pracownik (administrator ma dostep do konfiguracji terminala)
		};



		Employee()
		{
			String fileName;								// Okreslenie pliku w ktorym bedzie
			fileName += F("/DataStore/Employee.bin");		// znajdowala sie tabela

			uint16_t* fieldLengths = new uint16_t[9];				//
			fieldLengths[0] = sizeof(Record::id);					//
			fieldLengths[1] = 32;									//
			fieldLengths[2] = 32;									//
			fieldLengths[3] = 41;									// Okreslenie dlugosci poszczegolnych
			fieldLengths[4] = 41;									// pol rekordu
			fieldLengths[5] = sizeof(Record::accessDays);			//
			fieldLengths[6] = sizeof(Record::accessFromHour);		//
			fieldLengths[7] = sizeof(Record::accessToHour);			//
			fieldLengths[8] = sizeof(Record::rank);					//

			uint8_t* fieldTypes = new uint8_t[2];					//
			fieldTypes[0] = bit(1) | bit(2) | bit(3) | bit(4);		// Okreslenie typow poszczegolnych pol rekordu
			fieldTypes[1] = 0UL;									//

			begin(fileName, 9, fieldLengths, fieldTypes);
		}

		uint16_t GetFromTableAll(Record* buffer, size_t bufferSize, uint32_t beginOffset = 0)
		{
			return DataBaseTable::GetFromTableAll((uint8_t*)buffer, bufferSize, beginOffset);
		}

		uint16_t GetFromTable(Ivanseidel::LinkedList<uint8_t>* fieldIndexes, Ivanseidel::LinkedList<uint8_t*>* fieldValues, Record* buffer, size_t bufferSize)
		{
			return DataBaseTable::GetFromTable(fieldIndexes, fieldValues, (uint8_t*)buffer, bufferSize);
		}

		void PutInTable(Ivanseidel::LinkedList<uint8_t>* fieldIndexes, Ivanseidel::LinkedList<uint8_t*>* fieldValues, Record* objectRecord)
		{
			DataBaseTable::PutInTable(fieldIndexes, fieldValues, (uint8_t*)objectRecord);
		}

	protected:
		void HandleRead(uint8_t* objectRecord, uint8_t* fileRecord)
		{
			Record* objectRecord_2 = (Record*)objectRecord;
			uint16_t* offsets = new uint16_t[fieldCount];
			GetOffsets(NULL, offsets);

			memcpy(&(objectRecord_2->id),				fileRecord + offsets[0], fieldLengths[0]);

			objectRecord_2->name =						String((char*)fileRecord + offsets[1]);
			objectRecord_2->surname =					String((char*)fileRecord + offsets[2]);
			objectRecord_2->passcodeHash =				String((char*)fileRecord + offsets[3]);
			objectRecord_2->tagCodeHash =				String((char*)fileRecord + offsets[4]);
			memcpy(&(objectRecord_2->accessDays),		fileRecord + offsets[5], fieldLengths[5]);
			memcpy(&(objectRecord_2->accessFromHour),	fileRecord + offsets[6], fieldLengths[6]);
			memcpy(&(objectRecord_2->accessToHour),		fileRecord + offsets[7], fieldLengths[7]);
			memcpy(&(objectRecord_2->rank),				fileRecord + offsets[8], fieldLengths[8]);

			delete[] offsets;
		}

		void HandleWrite(uint8_t* objectRecord, uint8_t* fileRecord)
		{
			Record* objectRecord_2 = (Record*)objectRecord;
			uint16_t* offsets = new uint16_t[fieldCount];
			GetOffsets(NULL, offsets);

			memcpy(fileRecord + offsets[0],		&(objectRecord_2->id),					fieldLengths[0]);

			memcpy(fileRecord + offsets[1],		objectRecord_2->name.c_str(),			objectRecord_2->name.length() + 1);
			memcpy(fileRecord + offsets[2],		objectRecord_2->surname.c_str(),		objectRecord_2->surname.length() + 1);
			memcpy(fileRecord + offsets[3],		objectRecord_2->passcodeHash.c_str(),	objectRecord_2->passcodeHash.length() + 1);
			memcpy(fileRecord + offsets[4],		objectRecord_2->tagCodeHash.c_str(),	objectRecord_2->tagCodeHash.length() + 1);
			memcpy(fileRecord + offsets[5],		&(objectRecord_2->accessDays),			fieldLengths[5]);
			memcpy(fileRecord + offsets[6],		&(objectRecord_2->accessFromHour),		fieldLengths[6]);
			memcpy(fileRecord + offsets[7],		&(objectRecord_2->accessToHour),		fieldLengths[7]);
			memcpy(fileRecord + offsets[8],		&(objectRecord_2->rank),				fieldLengths[8]);

			delete[] offsets;
		}

		uint16_t HandleGetSize()
		{
			return sizeof(Record);
		}
	};

	struct EventType : public DataBaseTable
	{
		enum Field
		{
			id,

			name,
			comment
		};

		struct Record
		{
			uint32_t id;			// Id typu zdarzenia (w bazie danych glownego serwera)

			String name;				// np. "Rozpoczecie pracy"
			String comment;					// Dodatkowy opis slowny w przypadku gdy nazwa nie wystarcza
		};



		EventType()
		{
			String fileName;
			fileName += F("/DataStore/EventType.bin");

			uint16_t* fieldLengths = new uint16_t[3];
			fieldLengths[0] = sizeof(Record::id);
			fieldLengths[1] = 32;
			fieldLengths[2] = 128;

			uint8_t* fieldTypes = new uint8_t[1];
			fieldTypes[0] = bit(1) | bit(2);

			begin(fileName, 3, fieldLengths, fieldTypes);
		}

		uint16_t GetFromTableAll(Record* buffer, size_t bufferSize, uint32_t beginOffset = 0)
		{
			return DataBaseTable::GetFromTableAll((uint8_t*)buffer, bufferSize, beginOffset);
		}

		uint16_t GetFromTable(Ivanseidel::LinkedList<uint8_t>* fieldIndexes, Ivanseidel::LinkedList<uint8_t*>* fieldValues, Record* buffer, size_t bufferSize)
		{
			return DataBaseTable::GetFromTable(fieldIndexes, fieldValues, (uint8_t*)buffer, bufferSize);
		}

		void PutInTable(Ivanseidel::LinkedList<uint8_t>* fieldIndexes, Ivanseidel::LinkedList<uint8_t*>* fieldValues, Record* objectRecord)
		{
			DataBaseTable::PutInTable(fieldIndexes, fieldValues, (uint8_t*)objectRecord);
		}

	protected:
		void HandleRead(uint8_t* objectRecord, uint8_t* fileRecord)
		{
			Record* objectRecord_2 = (Record*)objectRecord;
			uint16_t* offsets = new uint16_t[fieldCount];
			GetOffsets(NULL, offsets);

			memcpy(&(objectRecord_2->id),		fileRecord + offsets[0],		fieldLengths[0]);

			objectRecord_2->name =				String((char*)fileRecord + offsets[1]);
			objectRecord_2->comment =			String((char*)fileRecord + offsets[2]);

			delete[] offsets;
		}

		void HandleWrite(uint8_t* objectRecord, uint8_t* fileRecord)
		{
			Record* objectRecord_2 = (Record*)objectRecord;
			uint16_t* offsets = new uint16_t[fieldCount];
			GetOffsets(NULL, offsets);

			memcpy(fileRecord + offsets[0],		&(objectRecord_2->id),				fieldLengths[0]);

			memcpy(fileRecord + offsets[1],		objectRecord_2->name.c_str(),		objectRecord_2->name.length() + 1);
			memcpy(fileRecord + offsets[2],		objectRecord_2->comment.c_str(),	objectRecord_2->comment.length() + 1);

			delete[] offsets;
		}

		uint16_t HandleGetSize()
		{
			return sizeof(Record);
		}
	};

	struct Event : public DataBaseTable
	{
		enum Field
		{
			id,

			time,
			employeeId,
			eventTypeId
		};

		struct Record
		{
			uint32_t id;

			time_t time;			// Czas wystapienia zdarzenia (Unix Timestamp)
			uint32_t employeeId;			// Pracownik ktorego dotyczy zdarzenie
			uint32_t eventTypeId;				// Rodzaj zdarzenia
		};



		Event()
		{
			String fileName;
			fileName += F("/DataStore/Event.bin");

			uint16_t* fieldLengths = new uint16_t[4];
			fieldLengths[0] = sizeof(Record::id);
			fieldLengths[1] = sizeof(Record::time);
			fieldLengths[2] = sizeof(Record::employeeId);
			fieldLengths[3] = sizeof(Record::eventTypeId);

			uint8_t* fieldTypes = new uint8_t[1];
			fieldTypes[0] = 0UL;

			begin(fileName, 4, fieldLengths, fieldTypes);

			Record record;															//
			if (GetFromTableMax<uint32_t>(Field::id, (uint8_t*)(&record)))			//
				nextFreePK = record.id + 1;											// Inicjujemy klucz glowny tabeli
			else																	//
				nextFreePK = 0;														//
		}

		uint16_t GetFromTableAll(Record* buffer, size_t bufferSize, uint32_t beginOffset = 0)
		{
			return DataBaseTable::GetFromTableAll((uint8_t*)buffer, bufferSize, beginOffset);
		}

		uint16_t GetFromTable(Ivanseidel::LinkedList<uint8_t>* fieldIndexes, Ivanseidel::LinkedList<uint8_t*>* fieldValues, Record* buffer, size_t bufferSize)
		{
			return DataBaseTable::GetFromTable(fieldIndexes, fieldValues, (uint8_t*)buffer, bufferSize);
		}

		void PutInTable(Ivanseidel::LinkedList<uint8_t>* fieldIndexes, Ivanseidel::LinkedList<uint8_t*>* fieldValues, Record* objectRecord)
		{
			DataBaseTable::PutInTable(fieldIndexes, fieldValues, (uint8_t*)objectRecord);
		}

		uint32_t GetNextFreePK()
		{
			uint32_t result = nextFreePK;
			++nextFreePK;
			return result;
		}

	protected:
		uint32_t nextFreePK;



		void HandleRead(uint8_t* objectRecord, uint8_t* fileRecord)
		{
			Record* objectRecord_2 = (Record*)objectRecord;
			uint16_t* offsets = new uint16_t[fieldCount];
			GetOffsets(NULL, offsets);

			memcpy(&(objectRecord_2->id),			fileRecord + offsets[0], fieldLengths[0]);

			memcpy(&(objectRecord_2->time),			fileRecord + offsets[1], fieldLengths[1]);
			memcpy(&(objectRecord_2->employeeId),	fileRecord + offsets[2], fieldLengths[2]);
			memcpy(&(objectRecord_2->eventTypeId),	fileRecord + offsets[3], fieldLengths[3]);

			delete[] offsets;
		}

		void HandleWrite(uint8_t* objectRecord, uint8_t* fileRecord)
		{
			Record* objectRecord_2 = (Record*)objectRecord;
			uint16_t* offsets = new uint16_t[fieldCount];
			GetOffsets(NULL, offsets);

			memcpy(fileRecord + offsets[0],		&(objectRecord_2->id),			fieldLengths[0]);

			memcpy(fileRecord + offsets[1],		&(objectRecord_2->time),		fieldLengths[1]);
			memcpy(fileRecord + offsets[2],		&(objectRecord_2->employeeId),	fieldLengths[2]);
			memcpy(fileRecord + offsets[3],		&(objectRecord_2->eventTypeId), fieldLengths[3]);

			delete[] offsets;
		}

		uint16_t HandleGetSize()
		{
			return sizeof(Record);
		}
	};



	Terminal* terminal;
	Employee* employee;
	EventType* eventType;
	Event* event;
	Terminal::Record terminalRecord;



	DataStore();
	void begin();
	bool TakeMessage(int, void*, void**);
	bool GetSynchronizing();

private:
	bool synchronizing;



	void PrepareMassMemory();
};

extern DataStore dataStore;

#endif
/*
WebServer145617 - serwer HTTP pozwalajacy na latwa konfiguracje terminala
*/
#ifndef WebServer145617_h
#define WebServer145617_h

#define DEBUG_WEBSERVER145617 1
#if DEBUG_WEBSERVER145617
#define DEBUG_WEBSERVER145617_ID "16_"

//#include <SerialListener.h>
//#define DEBUG_WEBSERVER145617_LOG(...) serialListener.Log(__VA_ARGS__)
#define DEBUG_WEBSERVER145617_LOG(...) Serial1.println(__VA_ARGS__)
#endif

#include <stdint.h>
#include <SemaphoreManager.h>
#include <UpdateManager.h>
#include <MessageReceiver.h>
#include <MessageUsers.h>
#include <TimeKeeper.h>
#include <DataStore.h>
#include <SerialListener.h>
#define FS_NO_GLOBALS
#include <ESPAsyncWebServer.h>
#include <ESP8266TrueRandom.h>
#include <LinkedList.h>
#include <mimetable.h>
using namespace mime;


class WebServer145617 : public UpdateManagerUser, public MessageReceiver
{
public:
	WebServer145617(MessageReceiver*, SdFat&);
	void begin();
	void Update();
	bool TakeMessage(int, void*, void**);
	void CallbackNotFound(AsyncWebServerRequest*);						//
	size_t CallbackNotFoundFiller(uint16_t, uint8_t*, size_t, size_t);	// 404
	void CallbackNotFoundDisconnect(uint16_t);							//
	void CallbackLogin(AsyncWebServerRequest*);								// login
	void CallbackLogout(AsyncWebServerRequest*);								// logout
	void CallbackFormat(AsyncWebServerRequest*);									// format
	void CallbackShutdown(AsyncWebServerRequest*);										// shutdown
	void CallbackRestart(AsyncWebServerRequest*);											// restart
	void CallbackGet(AsyncWebServerRequest*);													// Sluza do zarzadzania ustawieniami (wszystkimi na raz) terminala
	void CallbackSet(AsyncWebServerRequest*);													//

private:
	struct UpdateInfo
	{
		unsigned long time_1;
		uint8_t data_1;
	};

	struct DownloadRequest
	{
		static uint16_t nextFreeKey;		// Dzieki niej kazdy nowy obiekt otrzymuje niepowtarzalny identyfikator (przewiniecie nie jest problemem)
		uint16_t key;							// Klucz przypisany do jednego zadania downloadu. Umozliwia przeszukanie listy 'downloadRequests' wewnatrz metody CallbackNotFoundFiller()
		String fileName;



		DownloadRequest(const String& fileName)
		{
			key = nextFreeKey;
			++nextFreeKey;
			DownloadRequest::fileName = fileName;
		}
	};

	struct User
	{
		static String nextFreeUUID;
		String UUID;
		unsigned long userLifeLength;
		unsigned long kickTime;



		static void GenerateUUID()
		{
			byte arr[16];
			ESP8266TrueRandom.uuid(arr);
			nextFreeUUID = ESP8266TrueRandom.uuidToString(arr);
		}

		User()
		{
			UUID = nextFreeUUID;
			userLifeLength = 300000;				// 5 minut
			Renew();
		};

		void Renew()
		{
			kickTime = millis() + userLifeLength;
		}
	};



	MessageReceiver* mainController;
	SdFat& sdFat;
	AsyncWebServer* pServer;
	User* pUser;													// Aktualnie zalogowany uzytkownik
	Ivanseidel::LinkedList<DownloadRequest*> downloadRequests;			// Aktualnie majace miejsce akcje pobierania plikow przez klientow
	uint8_t* interruptData;													// Do przesylania danych z przerwan do petli glownej
	UpdateInfo updateInfo;



	bool AuthenticateUser(AsyncWebServerRequest*);
	bool EnterRestrictedResource(AsyncWebServerRequest*);
	static void SuccessResponse(AsyncWebServerRequest*, String* = NULL);
	static void ErrorResponse(AsyncWebServerRequest*, String* = NULL);
	static String GetContentType(const String&);
	void PrepareMassMemory();
	void CreateServer();
};

#endif
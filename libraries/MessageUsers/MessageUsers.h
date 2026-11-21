// Plik okreslajacy wiadomosci przesylane za pomoca interfejsu
// MessageReceiver w zakresie tego projektu

#ifndef MessageUsers_h
#define MessageUsers_h

#define MESSAGE_TASK_SYNC_BEGIN							1001			// Zadanie synchronizacji wlasnie sie rozpoczyna (TaskSync ---> DataStore)
#define MESSAGE_TASK_SYNC_END							1002				// Zadanie synchronizacji zostalo zakonczone (TaskSync ---> DataStore)
#define MESSAGE_TASK_CONNECT_END						1003					// Zadanie laczenia z siecia zostalo zakonczone (TaskConnect ---> MainController145617)
#define MESSAGE_TASK_NETWORK_SCAN_END					1004						// Zadanie skanowania sieci zostalo zakonczone (TaskNetworkScan ---> MainController145617)
#define MESSAGE_TASK_UPDATE_RTC_BEGIN					1005							// Rozpocznij aktualizacje RTC (TaskUpdateRTC ---> TimeKeeper ---> TaskUpdateRTC)
#define MESSAGE_TASK_UPDATE_RTC_TIME_ZONE_CHANGE		1006								// Wlasnie nastepuje zmiana strefy czasowej na czas letni, lub zimowy (TaskUpdateRTC ---> TimeKeeper ---> TaskUpdateRTC)
#define MESSAGE_TASK_UPDATE_RTC_END						1007									// Zakonczono aktualizacje RTC (TaskUpdateRTC ---> MainController145617)

#define MESSAGE_CHECK_WIFI_CONNECTION					1008			// Sprawdz czy dalej jestesmy podlaczeni do AP (MainController145617 ---> TimeKeeper ---> MainController145617)

#define MESSAGE_FORMAT									1009			// Zaplanuj pelny format pamieci (WebServer145617 ---> TimeKeeper ---> WebServer145617)
#define MESSAGE_RESTART									1010				// Zrestartuj urzadzenie (WebServer145617 ---> MainController145617)
#define MESSAGE_HALT									1011					// Zatrzymaj urzadzenie (WebServer145617 ---> MainController145617)
#define MESSAGE_CHANGE_SETTINGS							1012						// Aktualizuj ustawienia termianala (WebServer145617 ---> TimeKeeper ---> WebServer145617)
#define MESSAGE_GENERATE_USER_UUID						1013							// Uzytkownik wlasnie sie zalogowal tym samym wykorzystujac UUID, nalezy wygenerowac nowe (WebServer145617 ---> TimeKeeper ---> WebServer145617)

#endif
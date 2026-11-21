/*
SemaphoreManager - Ze wzgledu na uzycie klasy ESPAsyncWebServer
				   wymagana jest synchronizacja dostepu do SD.
				   Ta klasa ma za zadanie to zapewnic
*/
#ifndef SemaphoreManager_h
#define SemaphoreManager_h


#include <stdint.h>
#include <CriticalSection.h>


class SemaphoreManager
{
public:
	enum Operation
	{
		SD_unmount,
		SD_mainLoopRead,
		SD_mainLoopWrite,
		SD_mainLoopWritePresync,
		SD_interruptRead,
		SD_interruptWrite
	};



	SemaphoreManager();
	bool TryAcquire(Operation);
	void Release(Operation);

private:
	uint16_t semaphores;
	uint16_t types;
	uint16_t* avoidMasks;
	uint16_t* blockMasks;
	uint8_t* counters;
};

extern SemaphoreManager semaphoreManager;

#endif
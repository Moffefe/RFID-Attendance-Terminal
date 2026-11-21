#include "Arduino.h"
#include "SemaphoreManager.h"


SemaphoreManager semaphoreManager;			// Tutaj jest naprawde tworzony obiekt


SemaphoreManager::SemaphoreManager()
{
	semaphores = 0;							// Na poczatku zaden semafor nie jest zajety
	types = bit(1) | bit(2) | bit(3);			// bit ustawiony na 0 oznacza semafor binarny, na 1 oznacza semafor zliczajacy

	avoidMasks = new uint16_t[16];										//
	avoidMasks[0] = bit(1) | bit(2) | bit(3);							//
	avoidMasks[1] = bit(0);												//
	avoidMasks[2] = bit(0);												//
	avoidMasks[3] = bit(0);												//
	avoidMasks[4] = bit(0) | bit(2) | bit(5);							//
	avoidMasks[5] = bit(0) | bit(1) | bit(2) | bit(3) | bit(4);			// Przyklad: Operacja 0 (SD_unmount) nie moze uzyskac
	avoidMasks[6] = 0UL;												// semafora gdy aktualnie ma miejsce operacja 1, 2, lub 3
	avoidMasks[7] = 0UL;												//
	avoidMasks[8] = 0UL;												//
	avoidMasks[9] = 0UL;												//
	avoidMasks[10] = 0UL;												//
	avoidMasks[11] = 0UL;												//
	avoidMasks[12] = 0UL;												//
	avoidMasks[13] = 0UL;												//
	avoidMasks[14] = 0UL;												//
	avoidMasks[15] = 0UL;												//

	blockMasks = new uint16_t[16];										//
	blockMasks[0] = ~( bit(4) | bit(5) );								//
	blockMasks[1] = ~( bit(5) );										//
	blockMasks[2] = ~( bit(4) | bit(5) );								//
	blockMasks[3] = ~( bit(5) );										//
	blockMasks[4] = ~0UL;												//
	blockMasks[5] = ~0UL;												// Przyklad: Gdy operacja 0 (SD_unmount) uzyska semafor
	blockMasks[6] = ~0UL;												// to blokuje (kasuje semafor) dla operacji 4 i 5
	blockMasks[7] = ~0UL;												//
	blockMasks[8] = ~0UL;												//
	blockMasks[9] = ~0UL;												//
	blockMasks[10] = ~0UL;												//
	blockMasks[11] = ~0UL;												//
	blockMasks[12] = ~0UL;												//
	blockMasks[13] = ~0UL;												//
	blockMasks[14] = ~0UL;												//
	blockMasks[15] = ~0UL;												//

	counters = new uint8_t[16];				// Kazdy semafor ma swoj licznik (prawidlowo powinny go otrzymac tylko semafory zliczajace),
	for (uint8_t i = 0; i < 16; ++i)		// dzieki czemu TryAcquire() i Release() nie musza przeprowadzac zadnych
		counters[i] = UINT8_MAX;			// operacji "mapujacych" i moga dzialac szybko
}

bool SemaphoreManager::TryAcquire(Operation operation)
{
	bool result = false;
	CriticalSectionEnter();

	if (!(semaphores & avoidMasks[operation]))
	{
		if (bitRead(types, operation))
		{
			if (counters[operation] > 0)
			{
				--(counters[operation]);

				bitSet(semaphores, operation);
				semaphores &= blockMasks[operation];
				result = true;
			}
		}
		else
		{
			bitSet(semaphores, operation);
			semaphores &= blockMasks[operation];
			result = true;
		}
	}

	CriticalSectionExit();
	return result;
}

void SemaphoreManager::Release(Operation operation)
{
	CriticalSectionEnter();

	if (bitRead(types, operation))
	{
		if (counters[operation] == UINT8_MAX)
			ESP.restart();

		++(counters[operation]);

		if (counters[operation] == UINT8_MAX)
			bitClear(semaphores, operation);
	}
	else
	{
		if (!bitRead(semaphores, operation))
			ESP.restart();

		bitClear(semaphores, operation);
	}

	CriticalSectionExit();
}

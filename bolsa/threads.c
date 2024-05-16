#include "threads.h"


void WINAPI pause(LPVOID p) {
	SharedMemory* sharedMemory = (SharedMemory*)p;

	WaitForSingleObject(sharedMemory->hEventRunning, INFINITE);

	if (!sharedMemory->sharedData->pausedBolsa) {
		sharedMemory->sharedData->pausedBolsa = TRUE;
		int remainingSeconds = sharedMemory->sharedData->seconds;

		while (remainingSeconds > 0) {
			Sleep(1000);
			remainingSeconds--;
		}

		sharedMemory->sharedData->pausedBolsa = FALSE;
		ResetEvent(sharedMemory->hEventRunning);
		ExitThread(0);
	}
	else {
		_tprintf(TEXT("Operações de compra e venda suspensas!\nIntroduza um comando: "));
	}
}


#include "threads.h"

/*BOOL WINAPI updateInfo(LPVOID p) {

	SharedMemory* sharedMemory = (SharedMemory*)p;

	while (TRUE) {
		WaitForSingleObject(sharedMemory->hMutexUpdateBoard, INFINITE);

		Sleep(1000);

		ReleaseMutex(sharedMemory->hMutexUpdateBoard);

		SetEvent(sharedMemory->hEventRunning);
	}

	return TRUE;
}*/

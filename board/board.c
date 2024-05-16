#include "board.h"
#include "../utils/utils.h"
#include "threads.h"

//Verificar se já está uma bolsa em execução
BOOL isBolsaRunning() {
	HANDLE hMutex = CreateMutex(NULL, FALSE, MUTEX_NAME);

	if(hMutex == NULL){
		return FALSE;
	}
	else if (GetLastError() == ERROR_ALREADY_EXISTS)
		return TRUE;

	return FALSE;
}

BOOL initSharedMemory_Sync(SharedMemory* sharedMemory) {
	sharedMemory->hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(SharedData), TEXT("hMapsharedMemory"));
	if (sharedMemory->hMapFile == NULL) {
		_tprintf(TEXT("ERROR: CreateFileMapping [%d]!\n"), GetLastError());
		return FALSE;
	}

	sharedMemory->sharedData = MapViewOfFile(sharedMemory->hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedData));
	if (sharedMemory->sharedData == NULL) {
		_tprintf(TEXT("ERROR: MapViewOfFile [%d]!\n"), GetLastError());
		CloseHandle(sharedMemory->hMapFile);
		return FALSE;
	}

	sharedMemory->hMutexUpdateBoard = CreateMutex(NULL, FALSE, TEXT("hMutexUpdateBoard"));
	if (sharedMemory->hMutexUpdateBoard == NULL) {
		_tprintf(TEXT("ERROR: CreateMutex [%d]!\n"), GetLastError());
		UnmapViewOfFile(sharedMemory->sharedData);
		CloseHandle(sharedMemory->hMapFile);
		return FALSE;
	}

	sharedMemory->hEventUpdateBoard = CreateEvent(NULL, TRUE, FALSE, TEXT("hEventUpdateBoard"));
	if (sharedMemory->hEventUpdateBoard == NULL) {
		_tprintf(TEXT("ERROR: CreateEvent [%d]!\n"), GetLastError());
		UnmapViewOfFile(sharedMemory->sharedData);
		CloseHandle(sharedMemory->hMapFile);
		CloseHandle(sharedMemory->hMutexUpdateBoard);
		return FALSE;
	}

	sharedMemory->hEventRunning = CreateEvent(NULL, TRUE, FALSE, TEXT("hEventInPause"));
	if (sharedMemory->hEventRunning == NULL) {
		_tprintf(TEXT("ERROR: CreateEvent [%d]!\n"), GetLastError());
		UnmapViewOfFile(sharedMemory->sharedData);
		CloseHandle(sharedMemory->hMapFile);
		CloseHandle(sharedMemory->hMutexUpdateBoard);
		CloseHandle(sharedMemory->hEventUpdateBoard);
		return FALSE;
	}

	return TRUE;
}



int _tmain(int argc, TCHAR* argv[]) {
#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
	_setmode(_fileno(stderr), _O_WTEXT);
#endif

	if (argc != 2) {
		_tprintf(TEXT("Número de argumentos inválido! Espicifique o numero de empresas(MAX 10)!\n"));
		return;
	}
	else
		numMaxEmpresas = _ttoi(argv[1]);

	SharedMemory sharedMemory;

	if (isBolsaRunning()) {
		if (!initSharedMemory_Sync(&sharedMemory))
			exit(-1);
		_tprintf(TEXT("================================="));
		_tprintf(TEXT("\n|        BOARD       |"));
		_tprintf(TEXT("\n================================\n"));
		
	}
	else {
		_tprintf(TEXT("A bolsa ainda não está a correr!\n"));
		return 1;
	}

	ThreadsBoard threadsBoard;

	threadsBoard.hThreads[0] = CreateThread(NULL, 0, receiveInfoFromBolsa, &sharedMemory, 0, NULL);
	//threadsBolsa.hThreads[1] = CreateThread(NULL, 0, closeALlThreads, &sharedMemory, 0, NULL);

	if (threadsBoard.hThreads[0] == NULL) {
		_tprintf(TEXT("Erro ao criar a thread! [%d]\n"), GetLastError());
		return -1;
	}

	while (TRUE) {

	}
}
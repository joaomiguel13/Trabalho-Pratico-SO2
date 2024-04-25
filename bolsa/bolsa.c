#include "..\\library\utils.h"
#include "threads.h"

//Verificar se já está uma bolsa em execução
BOOL isBolsaRunning() {
	HANDLE hMutex = CreateMutex(NULL, FALSE, MUTEX_NAME);

	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		CloseHandle(hMutex);
		_tprintf(TEXT("Já existe uma bolsa a correr!\n"));
		return TRUE;
	}
	else {
		_tprintf(TEXT("================================="));
		_tprintf(TEXT("\n|        BOLSA DE VALORES       |"));
		_tprintf(TEXT("\n================================\n"));
		return FALSE;
	}
}


//Apenas podem estar ligados ao bolsa, em simultâneo, NCLIENTES
//programas cliente.Caso se pretendam executar mais do que NCLIENTES clientes, os restantes devem manterse em execução e aguardar por uma vaga.O valor NCLIENTES encontra - se especificado no Registry.Na
//primeira execução do bolsa, caso este valor não esteja definido, deve ser utilizado o valor 5.

DWORD readRegistry(const TCHAR* keyName) {
	HKEY hKey;
	DWORD value;
	DWORD size = sizeof(DWORD);

	if (RegOpenKeyEx(HKEY_CURRENT_USER, REGISTRY_PATH, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
		if (RegQueryValueEx(hKey, keyName, NULL, NULL, (LPBYTE)&value, &size) == ERROR_SUCCESS) {
			RegCloseKey(hKey);
			return value;
		}
		else {
			RegCloseKey(hKey);
			_tprintf("\nErro ao ler o valor da chave do Registry! [%d]", GetLastError());

			if (keyName == "NCLIENTES")
				return DEFAULT_VALUE_NCLIENTES;
			else
				_tprintf("\nChave inválida!");
		}
	}
	else {
		_tprintf("\nErro ao abrir a chave do Registry! [%d]", GetLastError());

		if (keyName == "NCLIENTES")
			return DEFAULT_VALUE_NCLIENTES;
		else
			_tprintf("\nChave VAZIA!");
	}
}

void writeRegistry(const TCHAR* keyName, DWORD value) {
	HKEY hKey;

	if (RegCreateKeyEx(HKEY_CURRENT_USER, REGISTRY_PATH, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
		if (RegSetValueEx(hKey, keyName, 0, REG_DWORD, (LPBYTE)&value, sizeof(DWORD)) == ERROR_SUCCESS) {
			//_tprintf(TEXT("CHAVE ESCRITA COM SUCESSO!\n"));
		}
		else
			_tprintf(TEXT("ERRO AO ESCREVER CHAVE!\n"));
		RegCloseKey(hKey);
	}
	else {
		_tprintf("\nErro ao criar/abrir a chave do Registry! [%d]", GetLastError());
	}
}

void initSharedMemory_Sync(SharedMemory *sharedMemory) {
	sharedMemory->sharedData->numEmpresas = 0;
	sharedMemory->sharedData->lastTransacao.numAcoes = 0;
	sharedMemory->sharedData->lastTransacao.precoAcoes = 0;
	sharedMemory->sharedData->lastTransacao.tipo = ' ';
	sharedMemory->sharedData->lastTransacao.user = NULL;


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
	
	sharedMemory->hEventInPause = CreateEvent(NULL, TRUE, FALSE, TEXT("hEventInPause"));
	if (sharedMemory->hEventInPause == NULL) {
		_tprintf(TEXT("ERROR: CreateEvent [%d]!\n"), GetLastError());
		UnmapViewOfFile(sharedMemory->sharedData);
		CloseHandle(sharedMemory->hMapFile);
		CloseHandle(sharedMemory->hMutexUpdateBoard);
		CloseHandle(sharedMemory->hEventUpdateBoard);
		return FALSE;
	}
}


int _tmain(int argc, TCHAR* argv[]) {
}
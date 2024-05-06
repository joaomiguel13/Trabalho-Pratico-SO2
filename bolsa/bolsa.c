#include "bolsa.h"
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

BOOL updateInfo(SharedMemory* sharedMemory) {
	/*WaitForSingleObject(sharedMemory->hMutexUpdateBoard, INFINITE);

	ReleaseMutex(sharedMemory->hMutexUpdateBoard); */


	SetEvent(sharedMemory->hEventUpdateBoard);

	return TRUE;
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

void initBolsa(SharedMemory* sharedMemory) {
	sharedMemory->sharedData->lastTransacao.empresa = NULL;
	sharedMemory->sharedData->lastTransacao.numAcoes = 0;
	sharedMemory->sharedData->lastTransacao.precoAcoes = 0;
	sharedMemory->sharedData->lastTransacao.tipo = ' ';
	sharedMemory->sharedData->lastTransacao.user = NULL;
	sharedMemory->sharedData->numEmpresas = 0;
}


void add_empresa(SharedMemory* sharedMemory, TCHAR* nomeEmpresa, float numAcoes, float precoAcao) {
	if (sharedMemory->sharedData->numEmpresas < MAX_EMPRESAS) {
		for(int i = 0; i < sharedMemory->sharedData->numEmpresas; i++) {
			if (_tcscmp(sharedMemory->sharedData->empresas[i].nome, nomeEmpresa) == 0) {
				_tprintf(TEXT("Empresa já existente!\n"));
				return;
			}
		}
		memcpy(sharedMemory->sharedData->empresas[sharedMemory->sharedData->numEmpresas].nome, nomeEmpresa, 99);
		sharedMemory->sharedData->empresas[sharedMemory->sharedData->numEmpresas].acoesDisponiveis = numAcoes;
		sharedMemory->sharedData->empresas[sharedMemory->sharedData->numEmpresas].precoAcao = precoAcao;
		sharedMemory->sharedData->numEmpresas++;
		_tprintf(TEXT("Empresa %s adicionada com sucesso!\n"), nomeEmpresa);
	}
	else
		_tprintf(TEXT("Número máximo de empresas atingido!\n"));
}

void list_empresas(SharedMemory* sharedMemory) {
	_tprintf(TEXT("================================="));
	_tprintf(TEXT("\n|        EMPRESAS EXISTENTES      |"));
	_tprintf(TEXT("\n================================\n"));

	for (int i = 0; i < sharedMemory->sharedData->numEmpresas; i++) {
		_tprintf(TEXT("Nome: %s\n"), sharedMemory->sharedData->empresas[i].nome);
		_tprintf(TEXT("Preço da ação: %.2f\n"), sharedMemory->sharedData->empresas[i].precoAcao);
		_tprintf(TEXT("Ações disponíveis: %d\n"), sharedMemory->sharedData->empresas[i].acoesDisponiveis);
		_tprintf(TEXT("=================================\n"));
	}
}

void stock(SharedMemory* sharedMemory, TCHAR* nomeEmpresa, float precoAcao) {
	for (int i = 0; i < sharedMemory->sharedData->numEmpresas; i++) {
		if (_tcscmp(sharedMemory->sharedData->empresas[i].nome, nomeEmpresa) == 0) {
			sharedMemory->sharedData->empresas[i].precoAcao = precoAcao;
			_tprintf(TEXT("Preço da ação da empresa %s alterado para %.2f\n"), nomeEmpresa, precoAcao);
			return;
		}
	}
	_tprintf(TEXT("Empresa não encontrada!\n"));
}


/*void list_users(SharedMemory* sharedMemory) {
	_tprintf(TEXT("================================="));
	_tprintf(TEXT("\n|        UTILIZADORES REGISTADOS      |"));
	_tprintf(TEXT("\n================================\n"));

	for (int i = 0; i < sharedMemory->sharedData->numUsers; i++) {
		_tprintf(TEXT("Username: %s\n"), sharedMemory->sharedData->users[i].username);
		_tprintf(TEXT("Saldo: %.2f\n"), sharedMemory->sharedData->users[i].saldo);
		_tprintf(TEXT("Estado: %s\n"), sharedMemory->sharedData->users[i].isOnline ? TEXT("Online") : TEXT("Offline"));
		_tprintf(TEXT("=================================\n"));
	}
}*/

void pause(SharedMemory* sharedMemory, DWORD seconds) {
	_tprintf(TEXT("Operações de compra e venda suspensas durante %d segundos!\n"), seconds);
	Sleep(seconds * 1000);
	_tprintf(TEXT("Operações de compra e venda retomadas!\n"));
}

//void close(SharedMemory* sharedMemory) {
	//_tprintf(TEXT("A bolsa de valores foi terminada!\n"));
	//CloseHandle(sharedMemory->hMapFile);
	//CloseHandle(sharedMemory->hMutexUpdateBoard);
	//CloseHandle(sharedMemory->hEventUpdateBoard);
	//CloseHandle(sharedMemory->hEventInPause);
//}

/*void help() {
	_tprintf(TEXT("================================="));
	_tprintf(TEXT("\n|        COMANDOS DISPONÍVEIS      |"));
	_tprintf(TEXT("\n================================\n"));
	_tprintf(TEXT("add_empresa <nome> <numAcoes> <precoAcao> - Adiciona uma nova empresa\n"));
	_tprintf(TEXT("list_empresas - Lista todas as empresas existentes\n"));
	_tprintf(TEXT("stock <nome> <precoAcao> - Altera o preço da ação de uma empresa\n"));
	_tprintf(TEXT("pause <segundos> - Pausa as operações de compra e venda durante o tempo especificado\n"));
	_tprintf(TEXT("help - Mostra os comandos disponíveis\n"));
	_tprintf(TEXT("close - Termina a bolsa de valores\n"));
}*/

void readFileEmpresas(SharedMemory* sharedMemory) {
	HANDLE hFile = CreateFile(EMPRESAS_FILE, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		_tprintf(_T("Error opening file: %s\n"), EMPRESAS_FILE);
		return;
	}

	char buffer[1024]; // A small buffer for reading chunks of data
	DWORD bytesRead;

	while (ReadFile(hFile, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead > 0) {
		buffer[bytesRead] = '\0'; // Null-terminate the string
		_tprintf(TEXT("Buffer: %hs\n"), buffer);

		//Dividir o buffer em linhas
		TCHAR* next_line = NULL;
		TCHAR* line = _tcstok_s(buffer, TEXT("\n"), &next_line);
		while (line != NULL) {
			TCHAR* nomeEmpresa = NULL;
			TCHAR* numAcoes = NULL;
			TCHAR* precoAcao = NULL;
			TCHAR* next_param = NULL;

			nomeEmpresa = _tcstok_s(line, TEXT(" "), &next_param);
			numAcoes = _tcstok_s(NULL, TEXT(" "), &next_param);
			precoAcao = _tcstok_s(NULL, TEXT(" "), &next_param);

			_tprintf(TEXT("Nome: %hs\n"), nomeEmpresa);
			_tprintf(TEXT("Ações: %s\n"), numAcoes);
			_tprintf(TEXT("Preço: %s\n"), precoAcao);

			if (nomeEmpresa != NULL && numAcoes != NULL && precoAcao != NULL) {
				add_empresa(&sharedMemory, nomeEmpresa, _tstof(numAcoes), _tstof(precoAcao));
			}

			line = _tcstok_s(NULL, TEXT("\n"), &next_line);
		}
	}

CloseHandle(hFile);
	
}

int _tmain(int argc, TCHAR* argv[]) {
#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
	_setmode(_fileno(stderr), _O_WTEXT);
#endif 

	SharedMemory sharedMemory;
	TCHAR opcao[2];

	if (!isBolsaRunning()) {
		if (!initSharedMemory_Sync(&sharedMemory)) {
			exit(-1);
		}
	}


	initBolsa(&sharedMemory);
	_tprintf(TEXT("Deseja carregar as empresas do ficheiro empresas.txt? (S/N): "));
	_getts_s(&opcao, 2);
	if (_tcscmp(opcao, TEXT("S")) == 0 || _tcscmp(opcao,TEXT("s")) == 0) {
		readFileEmpresas(&sharedMemory);
	}
	else if(_tcscmp(opcao, TEXT("N")) == 0 || _tcscmp(opcao, TEXT("n")) == 0) {
		_tprintf(TEXT("Empresas não carregadas!\n"));
	}
	else {
		_tprintf(TEXT("Opção inválida!\n"));
	}
	updateInfo(&sharedMemory);

	//ThreadsBolsa threadsBolsa;
	//threadsBolsa.hEventCloseAllThreads = SharedData

	//threadsBolsa.hThreads[0] = CreateThread(NULL, 0, updateInfo, &sharedMemory, 0, NULL);
	//threadsBolsa.hThreads[1] = CreateThread(NULL, 0, closeALlThreads, &sharedMemory, 0, NULL);

	/*if (threadsBolsa.hThreads[0] == NULL) {
		_tprintf(TEXT("Erro ao criar a thread! [%d]\n"), GetLastError());
		return -1;
	}*/
	while (TRUE) {
		TCHAR next_param = NULL;
		TCHAR command[50];
		_tprintf(TEXT("Introduza um comando: "));
		_getts_s(&command, 50);

		TCHAR* cmd = _tcstok_s(command, TEXT(" "), &next_param);
		if (_tcscmp(cmd, TEXT("addc")) == 0) {
			TCHAR* firstParam = _tcstok_s(NULL, TEXT(" "), &next_param);
			TCHAR* secondParam = _tcstok_s(NULL, TEXT(" "), &next_param);
			TCHAR* thirdParam = _tcstok_s(NULL, TEXT(" "), &next_param);

			if (firstParam == NULL || secondParam == NULL || thirdParam == NULL) {
				_tprintf(TEXT("\nComando não reconhecido!\n"));
				return;
			}

			float second = _tstof(secondParam);
			float third = _tstof(thirdParam);

			add_empresa(&sharedMemory, firstParam, second, third);
			updateInfo(&sharedMemory);

		}
		if(_tcscmp(cmd, TEXT("listc")) == 0) {
			list_empresas(&sharedMemory);
		}
		if(_tcscmp(cmd, TEXT("stock")) == 0) {
			TCHAR* firstParam = _tcstok_s(NULL, TEXT(" "), &next_param);
			TCHAR* secondParam = _tcstok_s(NULL, TEXT(" "), &next_param);

			if (firstParam == NULL || secondParam == NULL) {
				_tprintf(TEXT("\nComando não reconhecido!\n"));
				return;
			}

			float second = _tstof(secondParam);

			stock(&sharedMemory, firstParam, second);
			updateInfo(&sharedMemory);
		}

	}

}
#include "bolsa.h"
#include "threads.h"
#include "../utils/utils.h"

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

void add_empresa(SharedMemory* sharedMemory, TCHAR* nomeEmpresa, DWORD numAcoes, float precoAcao) {
	if (sharedMemory->sharedData->numEmpresas < MAX_EMPRESAS) {
		for(int i = 0; i < sharedMemory->sharedData->numEmpresas +1; i++) {
			if (_tcscmp(sharedMemory->sharedData->empresas[i].nome, nomeEmpresa) == 0) {
				_tprintf(TEXT("Empresa já existente!\n"));
				return;
			}
		}
		memcpy(sharedMemory->sharedData->empresas[sharedMemory->sharedData->numEmpresas].nome, nomeEmpresa, 99);
		sharedMemory->sharedData->empresas[sharedMemory->sharedData->numEmpresas].acoesDisponiveis = numAcoes;
		sharedMemory->sharedData->empresas[sharedMemory->sharedData->numEmpresas].precoAcao = precoAcao;
		sharedMemory->sharedData->numEmpresas++;
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
	FILE* file;
	TCHAR fileName[] = _T("empresas.txt");
	errno_t err = _wfopen_s(&file, fileName, _T("r"));
	if (err != 0 || file == NULL) {
		_tprintf(_T("Erro ao abrir o arquivo.\n"));
		return 1;
	}
	_tprintf(_T("Arquivo aberto com sucesso.\n"));
	int i = 0;
	
	while (i < MAX_EMPRESAS && fwscanf_s(file, _T("%s %d %f"), sharedMemory->sharedData->empresas[i].nome, _countof(sharedMemory->sharedData->empresas[i].nome), &sharedMemory->sharedData->empresas[i].acoesDisponiveis, &sharedMemory->sharedData->empresas[i].precoAcao) == 3) {
		// Exibir os dados lidos
		sharedMemory->sharedData->numEmpresas++;
		i++;
	}
	updateInfo(&sharedMemory);
	
	fclose(file);

	/*HANDLE hFile = CreateFile(EMPRESAS_FILE, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
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
			TCHAR* next_param = NULL;
			TCHAR* nomeEmpresa = _tcstok_s(line, TEXT(" "), &next_param);
			TCHAR* numAcoes = _tcstok_s(NULL, TEXT(" "), &next_param);
			TCHAR* precoAcao = _tcstok_s(NULL, TEXT(" "), &next_param);


			_tprintf(TEXT("Nome: %hs\n"), nomeEmpresa);
			_tprintf(TEXT("Ações: %s\n"), numAcoes);
			_tprintf(TEXT("Preço: %s\n"), precoAcao);

			if (nomeEmpresa != NULL && numAcoes != NULL && precoAcao != NULL) {
				add_empresa(&sharedMemory, nomeEmpresa, _tstof(numAcoes), _tstof(precoAcao));
			}

			line = _tcstok_s(NULL, TEXT("\n"), &next_line);
		}
	}
	CloseHandle(hFile);*/
}


DWORD WINAPI InstanciaThread(LPVOID lpvParam) {
	DWORD cbBytesRead = 0, cbWritten = 0, cbWritXXXXten = 0;
	int numResp = 0;
	BOOL fSuccess = FALSE;
	HANDLE hPipe = (HANDLE)lpvParam;
	HANDLE ReadReady;
	OVERLAPPED ov;

	if (hPipe == INVALID_HANDLE_VALUE) {
		_tprintf(TEXT("[ERRO] Falha ao criar o Named Pipe! (CreateNamedPipe)\n"));
		return -1;
	}

	_tprintf(_T("\n[SERVIDOR] Thread de cliente criada!\n"));
	ReadReady = CreateEvent(NULL, TRUE, FALSE, NULL);
	while (1){
		ZeroMemory(&ov, sizeof(ov));
		ResetEvent(ReadReady);
		ov.hEvent = ReadReady;

		fSuccess = ReadFile(hPipe, &utilizador, Msg_Sz, &cbBytesRead, &ov);
		if (!fSuccess && GetLastError() != ERROR_IO_PENDING) {
			_tprintf(TEXT("[ERRO] Falha na leitura do pipe! (ReadFile)\n"));
			return -1;
		}

		// Verifica se a operação está pendente (não há dados imediatamente disponíveis)
		if (!fSuccess) {
			// A operação está pendente, aguardar até que ela seja concluída
			WaitForSingleObject(ReadReady, INFINITE);
			// Verifica se a operação foi concluída com sucesso
			fSuccess = GetOverlappedResult(hPipe, &ov, &cbBytesRead, FALSE);
			if (!fSuccess) {
				_tprintf(TEXT("[ERRO] Falha na leitura do pipe! (GetOverlappedResult)\n"));
				return -1;
			}
		}
		if (utilizador.login == FALSE) {
			int i = 0;
			while (i < MAX_USERS)
			{
				if (_tcscmp(utilizador.username, users[i].username) == 0 && _tcscmp(utilizador.password, users[i].password) == 0) {
					utilizador.login = TRUE;
					utilizador.saldo = users[i].saldo;
					_tprintf(_T("Username: %s\n"), utilizador.username);
					_tprintf(_T("Password: %s\n"), utilizador.password);
					WriteClienteASINC(hPipe);
					break;
				}
				i++;
			}
			if (!utilizador.login) {
				_tprintf(_T("Invalid username or password\n"));
			}
		}
		else if(utilizador.tipo == 1){
			//MANDAR A LISTA DE EMPRESAS

			/*for (int i = 0; i < sharedMemory->sharedData->numEmpresas; i++) {
				_tprintf(TEXT("Nome: %s\n"), sharedMemory->sharedData->empresas[i].nome);
				_tprintf(TEXT("Preço da ação: %.2f\n"), sharedMemory->sharedData->empresas[i].precoAcao);
				_tprintf(TEXT("Ações disponíveis: %d\n"), sharedMemory->sharedData->empresas[i].acoesDisponiveis);
				_tprintf(TEXT("=================================\n"));
			}*/

			WriteClienteASINC(hPipe);
			
		}else if(utilizador.tipo == 2) {
			//fazer a compra de acoes
			
			WriteClienteASINC(hPipe);
			
		}else if(utilizador.tipo == 3) {
			//fazer a venda de acoes
			WriteClienteASINC(hPipe);

		}else if(utilizador.tipo == 4) {
			//ver o saldo
			int i=0;
			while (i < MAX_USERS)
			{
				if (_tcscmp(utilizador.username, users[i].username) == 0 && _tcscmp(utilizador.password, users[i].password) == 0) {
					utilizador.saldo = users[i].saldo;
					break;
				}
				i++;
			}
			WriteClienteASINC(hPipe);
		}

		// Processamento do pedido

		//numResp = broadcastCliented(Resposta);
	}
	FlushFileBuffers(hPipe);
	DisconnectNamedPipe(hPipe);
	CloseHandle(hPipe);
	_tprintf(TEXT("[SERVIDOR] Pipe fechado!\n"));
	return 1;
}

int WriteClienteASINC(HANDLE hPipe) {
	DWORD cbWritten = 0;
	BOOL fSuccess = FALSE;
	OVERLAPPED OverlWr = { 0 };
	HANDLE WriteReady;

	WriteReady = CreateEvent(NULL, TRUE, FALSE, NULL);
	ZeroMemory(&OverlWr, sizeof(OverlWr));
	ResetEvent(WriteReady);
	OverlWr.hEvent = WriteReady;
	fSuccess = WriteFile(hPipe, &utilizador, Msg_Sz, &cbWritten, &OverlWr);
	WaitForSingleObject(WriteReady, INFINITE);
	GetOverlappedResult(hPipe, &OverlWr, &cbWritten, FALSE);
	if (cbWritten == Msg_Sz) {
		_tprintf(TEXT("\nWrite para 1 cliente concluido\nIntroduza um comando:"));
	}
	else {
		_tprintf(TEXT("\nErro no Write para 1 cliente"));
		return 0;
	}
	return 1;
}


BOOL WINAPI ConectarClientes() {
	HANDLE hPipe;
	BOOL fConnected = FALSE;
	HANDLE hThread;
	DWORD dwThreadID;

	_tprintf(TEXT("[SERVIDOR] À espera de clientes...\n"));

	while (1) {
		hPipe = CreateNamedPipe(PIPE_NAME_CLIENTS, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, MAX_USERS, MAX_TAM, MAX_TAM, 5000, NULL);
		if (hPipe == INVALID_HANDLE_VALUE) {
			_tprintf(TEXT("[ERRO] Falha ao criar o Named Pipe! (CreateNamedPipe)\n"));
			exit(-1);
		}
		_tprintf(TEXT("[SERVIDOR] Aguardando conexão... (ConnectNamedPipe)\n"));


		fConnected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

		if (fConnected) {
			_tprintf(TEXT("[SERVIDOR] Cliente conectado.\n"));
			hThread = CreateThread(NULL, 0, InstanciaThread, (LPVOID)hPipe, 0, &dwThreadID);
			if (hThread == NULL) {
				_tprintf(TEXT("[ERRO] Falha ao criar a thread de cliente! (CreateThread)\n"));
				CloseHandle(hPipe);
				exit(-1);
			}
			else {
				CloseHandle(hThread);
			}
		}
		else {
			_tprintf(TEXT("[ERRO] Falha ao conectar ao pipe! (ConnectNamedPipe)\n"));
			CloseHandle(hPipe);
		}
	}
	return TRUE;
}


int _tmain(int argc, TCHAR* argv[]) {
	HANDLE hThread;
	DWORD DwThreadID;
	BOOL fConnect = FALSE;
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

	//ler o ficheiro e guardar os utilizadores
	FILE* file;
	TCHAR fileName[] = _T("utilizadores.txt");
	errno_t err = _wfopen_s(&file, fileName, _T("r"));
	if (err != 0 || file == NULL) {
		_tprintf(_T("Erro ao abrir o arquivo.\n"));
		return 1;
	}
	_tprintf(_T("Arquivo aberto com sucesso.\n"));
	int i = 0;
	while (i < MAX_USERS && fwscanf_s(file, _T("%s %s %f"), users[i].username, _countof(users[i].username), users[i].password, _countof(users[i].password), &users[i].saldo) == 3) {
		// Exibir os dados lidos
		_tprintf(_T("Nome: %s\n"), users[i].username);
		_tprintf(_T("Senha: %s\n"), users[i].password);
		_tprintf(_T("Saldo: %.2f\n\n"), users[i].saldo);
		i++;
	}
	fclose(file);
	//---------------------------------------

	//initBolsa(&sharedMemory);
	_tprintf(TEXT("Bolsa de valores iniciada!\n"));
	_tprintf(TEXT("Deseja carregar as empresas do ficheiro empresas.txt? (S/N): "));
	_getts_s(&opcao, 2);
	if (_tcscmp(opcao, TEXT("S")) == 0 || _tcscmp(opcao,TEXT("s")) == 0) {
		readFileEmpresas(&sharedMemory);
		updateInfo(&sharedMemory);
	}
	else if(_tcscmp(opcao, TEXT("N")) == 0 || _tcscmp(opcao, TEXT("n")) == 0) {
		_tprintf(TEXT("Empresas não carregadas!\n"));
	}
	else {
		_tprintf(TEXT("Opção inválida!\n"));
	}
	updateInfo(&sharedMemory);

	ThreadsBolsa threadsBolsa;
	//threadsBolsa.hEventCloseAllThreads = SharedData

	//threadsBolsa.hThreads[0] = CreateThread(NULL, 0, updateInfo, &sharedMemory, 0, NULL);
	//threadsBolsa.hThreads[1] = CreateThread(NULL, 0, closeALlThreads, &sharedMemory, 0, NULL);

	/*if (threadsBolsa.hThreads[0] == NULL) {
		_tprintf(TEXT("Erro ao criar a thread! [%d]\n"), GetLastError());
		return -1;
	}*/
	threadsBolsa.hThreads[0] = CreateThread(NULL, 0, ConectarClientes, NULL, 0, NULL);



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

			DWORD second = _wtoi(secondParam);
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
	return 0;

}

#include "bolsa.h"
#include "threads.h"

//Verificar se já está uma bolsa em execução
BOOL isBolsaRunning() {
	HANDLE hMutex = CreateMutex(NULL, FALSE, MUTEX_NAME);

	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		CloseHandle(hMutex);
		_tprintf(TEXT("Ja existe uma bolsa a correr!\n"));
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
				_tprintf("\nChave invalida!");
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
	sharedMemory->sharedData->lastTransacao.numAcoes = 0;
	sharedMemory->sharedData->lastTransacao.precoAcoes = 0;
	sharedMemory->sharedData->lastTransacao.tipo = ' ';
	sharedMemory->sharedData->lastTransacao.user = NULL;
	sharedMemory->sharedData->numEmpresas = 0;
	sharedMemory->sharedData->pausedBolsa = FALSE;
	sharedMemory->sharedData->seconds = 0;
}

void add_empresa(SharedMemory* sharedMemory, TCHAR* nomeEmpresa, DWORD numAcoes, float precoAcao) {
	if (sharedMemory->sharedData->numEmpresas < MAX_EMPRESAS) {
		for (int i = 0; i < sharedMemory->sharedData->numEmpresas + 1; i++) {
			if (_tcscmp(sharedMemory->sharedData->empresas[i].nome, nomeEmpresa) == 0) {
				_tprintf(TEXT("Empresa ja existente!\nIntroduza um comando:"));
				return;
			}
		}
		memcpy(sharedMemory->sharedData->empresas[sharedMemory->sharedData->numEmpresas].nome, nomeEmpresa, 99);
		sharedMemory->sharedData->empresas[sharedMemory->sharedData->numEmpresas].acoesDisponiveis = numAcoes;
		sharedMemory->sharedData->empresas[sharedMemory->sharedData->numEmpresas].precoAcao = precoAcao;
		sharedMemory->sharedData->numEmpresas++;
	}
	else
		_tprintf(TEXT("Numero maximo de empresas atingido!\nIntroduza um comando:"));
}

void list_empresas(SharedMemory* sharedMemory) {
	_tprintf(TEXT("================================="));
	_tprintf(TEXT("\n|        EMPRESAS EXISTENTES      |"));
	_tprintf(TEXT("\n================================\n"));

	for (int i = 0; i < sharedMemory->sharedData->numEmpresas; i++) {
		_tprintf(TEXT("Nome: %s\n"), sharedMemory->sharedData->empresas[i].nome);
		_tprintf(TEXT("Preço da ação: %.2f\n"), sharedMemory->sharedData->empresas[i].precoAcao);
		_tprintf(TEXT("Ações disponíveis: %d\n"), sharedMemory->sharedData->empresas[i].acoesDisponiveis);
		_tprintf(TEXT("=================================\nIntroduza Um comando:"));
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

void list_users(SharedMemory* sharedMemory) {
	_tprintf(TEXT("================================="));
	_tprintf(TEXT("\n|        UTILIZADORES REGISTADOS      |"));
	_tprintf(TEXT("\n================================\n"));

	int i = 0;
	while (i < MAX_USERS)
	{
		if (_tcslen(sharedMemory->sharedData->users[i].username) > 0) {
			_tprintf(TEXT("Username: %s\n"), sharedMemory->sharedData->users[i].username);
			_tprintf(TEXT("Saldo: %.2f\n"), sharedMemory->sharedData->users[i].saldo);
			_tprintf(TEXT("Estado: %s\n"), sharedMemory->sharedData->users[i].login ? TEXT("Online") : TEXT("Offline"));
			_tprintf(TEXT("=================================\n"));
		}
		i++;
	}
}

void WINAPI pause(LPVOID p) {
	SharedMemory* sharedMemory = (SharedMemory*)p;

	WaitForSingleObject(sharedMemory->hEventRunning, INFINITE);

	if (!sharedMemory->sharedData->pausedBolsa) {
		sharedMemory->sharedData->pausedBolsa = TRUE;
		_tprintf(TEXT("Operações de compra e venda suspensas durante %d segundos!\nIntroduza um comando: "), sharedMemory->sharedData->seconds);
		Sleep(sharedMemory->sharedData->seconds * 1000);
		sharedMemory->sharedData->pausedBolsa = FALSE;
		ResetEvent(sharedMemory->hEventRunning);
	}
}

//void close(SharedMemory* sharedMemory) {
	//_tprintf(TEXT("A bolsa de valores foi terminada!\n"));
	//CloseHandle(sharedMemory->hMapFile);
	//CloseHandle(sharedMemory->hMutexUpdateBoard);
	//CloseHandle(sharedMemory->hEventUpdateBoard);
	//CloseHandle(sharedMemory->hEventInPause);
//}

void readFileEmpresas(SharedMemory* sharedMemory) {
	FILE* file;
	TCHAR fileName[] = _T("empresas.txt");
	errno_t err = _wfopen_s(&file, fileName, _T("r"));
	if (err != 0 || file == NULL) {
		_tprintf(_T("Erro ao abrir o ficheiro.\n"));
		return 1;
	}
	_tprintf(_T("Ficheiro aberto com sucesso.\n"));
	int i = 0;

	while (i < MAX_EMPRESAS && fwscanf_s(file, _T("%s %d %f"), sharedMemory->sharedData->empresas[i].nome, _countof(sharedMemory->sharedData->empresas[i].nome), &sharedMemory->sharedData->empresas[i].acoesDisponiveis, &sharedMemory->sharedData->empresas[i].precoAcao) == 3) {
		// Exibir os dados lidos
		sharedMemory->sharedData->numEmpresas++;
		i++;
	}
	updateInfo(&sharedMemory);
	fclose(file);
}


DWORD WINAPI InstanciaThread(LPVOID lpParam) {
	DWORD cbBytesRead = 0, cbWritten = 0, cbWritXXXXten = 0;
	int numResp = 0;
	BOOL fSuccess = FALSE;
	SharedMemory* sharedMemory = (SharedMemory*)lpParam;
	HANDLE hPipe = sharedMemory->sharedData->hPipe;
	HANDLE ReadReady;
	OVERLAPPED ov;

	if (hPipe == INVALID_HANDLE_VALUE) {
		_tprintf(TEXT("[ERRO] Falha ao criar o Named Pipe!\n"));
		return -1;
	}

	//_tprintf(_T("\n[BOLSA] Thread de cliente criada!\n"));
	ReadReady = CreateEvent(NULL, TRUE, FALSE, NULL);
	while (1) {
		ZeroMemory(&ov, sizeof(ov));
		ResetEvent(ReadReady);
		ov.hEvent = ReadReady;

		fSuccess = ReadFile(hPipe, &utilizador, Msg_Sz, &cbBytesRead, &ov);
		if (!fSuccess && GetLastError() != ERROR_IO_PENDING) {
			_tprintf(TEXT("[ERRO] Falha na leitura do pipe!\n"));
			return -1;
		}

		// Verifica se a operação está pendente (não há dados imediatamente disponíveis)
		if (!fSuccess) {
			// A operação está pendente, aguardar até que ela seja concluída
			WaitForSingleObject(ReadReady, INFINITE);
			// Verifica se a operação foi concluída com sucesso
			fSuccess = GetOverlappedResult(hPipe, &ov, &cbBytesRead, FALSE);
			if (!fSuccess) {
				_tprintf(TEXT("[ERRO] Falha na leitura do pipe!\n"));
				int j = 0;
				while (1) {
					if (_tcscmp(utilizador.username, sharedMemory->sharedData->users[j].username) == 0) {
						sharedMemory->sharedData->users[j].login = FALSE;
						sharedMemory->sharedData->users[j].hPipe = NULL;
						j++;
						break;
					}
				}
				_tprintf(_T("O utilizador %s saiu\nIntroduza um comando:"), utilizador.username);

				return -1;
			}
		}
		if (utilizador.login == FALSE) {
			int i = 0;
			while (i < MAX_USERS)
			{
				if (_tcscmp(utilizador.username, sharedMemory->sharedData->users[i].username) == 0 && _tcscmp(utilizador.password, sharedMemory->sharedData->users[i].password) == 0) {
					utilizador.login = TRUE;
					utilizador.saldo = sharedMemory->sharedData->users[i].saldo;
					sharedMemory->sharedData->users[i].login = TRUE;
					sharedMemory->sharedData->users[i].hPipe = hPipe;
					_tprintf(_T("\nUsername: %s\n"), utilizador.username);
					_tprintf(_T("\nPassword: %s\n"), utilizador.password);
					WriteClienteASINC(hPipe);
					break;
				}
				i++;
			}
			if (!utilizador.login) {
				_tprintf(_T("Username ou password incorretos\n"));
			}
		}
		else if (utilizador.tipo == 1) {
			//MANDAR A LISTA DE EMPRESAS
			utilizador.numEmpresas = sharedMemory->sharedData->numEmpresas;
			for (int i = 0; i < sharedMemory->sharedData->numEmpresas; i++) {
				wcscpy_s(utilizador.empresas[i].nome, _countof(utilizador.empresas[i].nome), sharedMemory->sharedData->empresas[i].nome);
				utilizador.empresas[i].acoesDisponiveis = sharedMemory->sharedData->empresas[i].acoesDisponiveis;
				utilizador.empresas[i].precoAcao = sharedMemory->sharedData->empresas[i].precoAcao;
			}

			WriteClienteASINC(hPipe);

		}
		else if (utilizador.tipo == 2) {
			//fazer a compra de acoes
			if (sharedMemory->sharedData->pausedBolsa == FALSE) {
				for (int i = 0; i < sharedMemory->sharedData->numEmpresas; i++) {
					if (_tcscmp(utilizador.NomeEmpresa, sharedMemory->sharedData->empresas[i].nome) == 0) {
						if (sharedMemory->sharedData->empresas[i].acoesDisponiveis == 0) {
							utilizador.Sucesso = FALSE;
							utilizador.tipoResposta = 2;
							break;
						}
						else if (utilizador.qtAcoes > sharedMemory->sharedData->empresas[i].acoesDisponiveis) {
							utilizador.Sucesso = FALSE;;
							utilizador.tipoResposta = 2;
							break;
						}
						else {
							if (utilizador.qtAcoes * sharedMemory->sharedData->empresas[i].precoAcao > utilizador.saldo) {
								utilizador.Sucesso = FALSE;
								utilizador.tipoResposta = 1;
								break;
							}
							sharedMemory->sharedData->empresas[i].acoesDisponiveis -= utilizador.qtAcoes;
							int j = 0;
							while (j < MAX_USERS)
							{
								if (_tcscmp(utilizador.username, sharedMemory->sharedData->users[j].username) == 0) {
									sharedMemory->sharedData->users[j].saldo -= utilizador.qtAcoes * sharedMemory->sharedData->empresas[i].precoAcao;
									utilizador.saldo = sharedMemory->sharedData->users[j].saldo;
									utilizador.Sucesso = TRUE;
									updateInfo(&sharedMemory);

									wcscpy_s(sharedMemory->sharedData->lastTransacao.empresa.nome, _countof(sharedMemory->sharedData->lastTransacao.empresa.nome), utilizador.NomeEmpresa);
									sharedMemory->sharedData->lastTransacao.numAcoes = sharedMemory->sharedData->empresas[i].acoesDisponiveis;
									sharedMemory->sharedData->lastTransacao.precoAcoes = sharedMemory->sharedData->empresas[i].precoAcao;
									break;
								}
								j++;
							}
							break;
						}
					}
					else
					{
						utilizador.Sucesso = FALSE;
						utilizador.tipoResposta = 3;
					}
				}
			}
			else {
				utilizador.Sucesso = FALSE;
				utilizador.tipoResposta = 4;
				_tprintf(TEXT("Operações de compra e venda suspensas!\n"));
			}

			WriteClienteASINC(hPipe);

		}
		else if (utilizador.tipo == 3) {
			//fazer a venda de acoes
			if (sharedMemory->sharedData->pausedBolsa == FALSE) {
				for (int i = 0; i < sharedMemory->sharedData->numEmpresas; i++) {
					if (_tcscmp(utilizador.NomeEmpresa, sharedMemory->sharedData->empresas[i].nome) == 0) {
						sharedMemory->sharedData->empresas[i].acoesDisponiveis += utilizador.qtAcoes;
						int j = 0;
						while (j < MAX_USERS)
						{
							if (_tcscmp(utilizador.username, sharedMemory->sharedData->users[j].username) == 0) {
								sharedMemory->sharedData->users[j].saldo += (utilizador.qtAcoes * sharedMemory->sharedData->empresas[i].precoAcao);
								utilizador.saldo = sharedMemory->sharedData->users[j].saldo;
								utilizador.Sucesso = TRUE;
								wcscpy_s(sharedMemory->sharedData->lastTransacao.empresa.nome, _countof(sharedMemory->sharedData->lastTransacao.empresa.nome), utilizador.NomeEmpresa);
								sharedMemory->sharedData->lastTransacao.numAcoes = sharedMemory->sharedData->empresas[i].acoesDisponiveis;
								sharedMemory->sharedData->lastTransacao.precoAcoes = sharedMemory->sharedData->empresas[i].precoAcao;
								updateInfo(&sharedMemory);
								break;
							}
							j++;
						}
						break;
					}
					else {
						utilizador.tipoResposta = 3;
						utilizador.Sucesso = FALSE;
					}
				}
			}
			else {
				utilizador.Sucesso = FALSE;
				utilizador.tipoResposta = 4;
				_tprintf(TEXT("Operações de compra e venda suspensas!\n"));
			}
				
			
			WriteClienteASINC(hPipe);

		}
		else if (utilizador.tipo == 4) {
			//ver o saldo
			int i = 0;
			while (i < MAX_USERS)
			{
				if (_tcscmp(utilizador.username, sharedMemory->sharedData->users[i].username) == 0 && _tcscmp(utilizador.password, sharedMemory->sharedData->users[i].password) == 0) {
					utilizador.saldo = sharedMemory->sharedData->users[i].saldo;
					break;
				}
				i++;
			}
			WriteClienteASINC(hPipe);
		}
		updateInfo(&sharedMemory);
		// Processamento do pedido

		//numResp = broadcastCliented(Resposta);
	}
	FlushFileBuffers(hPipe);
	DisconnectNamedPipe(hPipe);
	CloseHandle(hPipe);
	_tprintf(TEXT("[BOLSA] Pipe fechado!\n"));
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


BOOL WINAPI ConectarClientes(LPVOID lpParam) {
	SharedMemory* sharedMemory = (SharedMemory*)lpParam;
	HANDLE hPipe = sharedMemory->sharedData->hPipe;

	BOOL fConnected = FALSE;
	HANDLE hThread;
	DWORD dwThreadID;

	_tprintf(TEXT("\n[BOLSA] À espera de clientes...\nIntroduza um comando:\n"));

	while (1) {
		hPipe = CreateNamedPipe(PIPE_NAME_CLIENTS, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, MAX_USERS, MAX_TAM, MAX_TAM, 5000, NULL);
		if (hPipe == INVALID_HANDLE_VALUE) {
			_tprintf(TEXT("[ERRO] Falha ao criar o Named Pipe!\n"));
			exit(-1);
		}
		_tprintf(TEXT("\n[BOLSA] À espera de conexão...\nIntroduza um comando:"));
		sharedMemory->sharedData->hPipe = hPipe;

		fConnected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

		if (fConnected) {
			_tprintf(TEXT("\n[BOLSA] Cliente conectado.\n"));
			hThread = CreateThread(NULL, 0, InstanciaThread, lpParam, 0, &dwThreadID);
			if (hThread == NULL) {
				_tprintf(TEXT("[ERRO] Falha ao criar a thread de cliente!\n"));
				CloseHandle(hPipe);
				exit(-1);
			}
			else {
				CloseHandle(hThread);
			}
		}
		else {
			_tprintf(TEXT("[ERRO] Falha ao conectar ao pipe!\n"));
			CloseHandle(hPipe);
		}
	}
	return TRUE;
}


int _tmain(int argc, TCHAR* argv[]) {

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
	_setmode(_fileno(stderr), _O_WTEXT);
#endif 

	HANDLE hThread;
	DWORD DwThreadID;
	BOOL fConnect = FALSE;

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
		_tprintf(_T("Erro ao abrir o ficheiro.\n"));
		return 1;
	}
	_tprintf(_T("Ficheiro aberto com sucesso.\n"));
	int i = 0;
	while (i < MAX_USERS && fwscanf_s(file, _T("%s %s %f"), sharedMemory.sharedData->users[i].username, _countof(sharedMemory.sharedData->users[i].username), sharedMemory.sharedData->users[i].password, _countof(sharedMemory.sharedData->users[i].password), &sharedMemory.sharedData->users[i].saldo) == 3) {
		// Exibir os dados lidos
		_tprintf(_T("Nome: %s\n"), sharedMemory.sharedData->users[i].username);
		_tprintf(_T("Senha: %s\n"), sharedMemory.sharedData->users[i].password);
		_tprintf(_T("Saldo: %.2f\n\n"), sharedMemory.sharedData->users[i].saldo);
		i++;
	}
	fclose(file);
	//---------------------------------------

	initBolsa(&sharedMemory);
	_tprintf(TEXT("Bolsa de valores iniciada!\n"));
	_tprintf(TEXT("Deseja carregar as empresas do ficheiro empresas.txt? (S/N): "));
	_getts_s(&opcao, 2);
	if (_tcscmp(opcao, TEXT("S")) == 0 || _tcscmp(opcao, TEXT("s")) == 0) {
		readFileEmpresas(&sharedMemory);
		updateInfo(&sharedMemory);
	}
	else if (_tcscmp(opcao, TEXT("N")) == 0 || _tcscmp(opcao, TEXT("n")) == 0) {
		_tprintf(TEXT("Empresas não carregadas!\n"));
	}
	else {
		_tprintf(TEXT("Opção inválida!\n"));
	}
	updateInfo(&sharedMemory);

	ThreadsBolsa threadsBolsa;
	//threadsBolsa.hEventCloseAllThreads = SharedData

	//threadsBolsa.hThreads[1] = CreateThread(NULL, 0, closeALlThreads, &sharedMemory, 0, NULL);

	/*if (threadsBolsa.hThreads[0] == NULL) {
		_tprintf(TEXT("Erro ao criar a thread! [%d]\n"), GetLastError());
		return -1;
	}*/
	threadsBolsa.hThreads[0] = CreateThread(NULL, 0, ConectarClientes, (LPVOID)&sharedMemory, 0, NULL);



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
		else if (_tcscmp(cmd, TEXT("listc")) == 0) {
			list_empresas(&sharedMemory);
		}
		else if (_tcscmp(cmd, TEXT("stock")) == 0) {
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
		else if (_tcscmp(cmd, TEXT("users")) == 0) {
			list_users(&sharedMemory);
		}
		else if (_tcscmp(cmd, TEXT("pause")) == 0) {
			TCHAR* firstParam = _tcstok_s(NULL, TEXT(" "), &next_param);

			if (firstParam == NULL) {
				_tprintf(TEXT("\nComando não reconhecido!\n"));
				return;
			}

			DWORD second = _wtoi(firstParam);

			//pause(&sharedMemory, second);
			SetEvent(sharedMemory.hEventRunning);
			sharedMemory.sharedData->seconds = second;
			threadsBolsa.hThreads[1] = CreateThread(NULL, 0, pause, &sharedMemory, 0, NULL);
			//TerminateThread(threadsBolsa.hThreads[1], 0);

		}
		else {
			_tprintf(TEXT("\nComando não reconhecido!\n"));
		}
	}
	return 0;

}

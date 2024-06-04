#include "bolsa.h"

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


	SetEvent(sharedMemory->hEventUpdateBoard);

	return TRUE;
}

BOOL eventoEnvia() {
	SetEvent(eventoLer.hEventoLer);
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
	eventoLer.hEventoLer = CreateEvent(NULL, TRUE, FALSE, TEXT("hEventoLer"));



	return TRUE;
}

void writeRegistry(const TCHAR* keyName, DWORD value) {
	HKEY hKey;

	if (RegCreateKeyEx(HKEY_CURRENT_USER, REGISTRY_PATH, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
		if (RegSetValueEx(hKey, keyName, 0, REG_DWORD, (LPBYTE)&value, sizeof(DWORD)) == ERROR_SUCCESS) {
			_tprintf(TEXT("CHAVE ESCRITA COM SUCESSO!\n"));
		}
		else
			_tprintf(TEXT("ERRO AO ESCREVER CHAVE!\n"));
		RegCloseKey(hKey);
	}
	else {
		_tprintf(_T("\nErro ao criar/abrir a chave do Registry! [%d]"), GetLastError());
	}
}

DWORD readRegistry(const TCHAR* keyName) {
	HKEY hKey;
	DWORD value;
	DWORD size = sizeof(DWORD);

	if (RegOpenKeyEx(HKEY_CURRENT_USER, REGISTRY_PATH, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
		if (RegQueryValueEx(hKey, keyName, NULL, NULL, (LPBYTE)&value, &size) == ERROR_SUCCESS) {
			RegCloseKey(hKey);
			_tprintf(_T("\nValor da chave do Registry lido com sucesso: %d!\n"), value);
			return value;
		}
		else {
			RegCloseKey(hKey);	
			_tprintf(_T("\nErro ao ler o valor da chave do Registry! [%d]"), GetLastError());

			if (_tcscmp(keyName,_T("NCLIENTES"))==0){
				writeRegistry(keyName, DEFAULT_VALUE_NCLIENTES);
				return DEFAULT_VALUE_NCLIENTES;
				
			}
			else
				_tprintf(_T("\nChave invalida!"));
		}
	}
	else {
		_tprintf(_T("\nErro ao abrir a chave do Registry! [%d]"), GetLastError());

		if (_tcscmp(keyName, _T("NCLIENTES")) == 0) {
			writeRegistry(keyName, DEFAULT_VALUE_NCLIENTES);
			return DEFAULT_VALUE_NCLIENTES;
		}
		else
			_tprintf(_T("\nChave VAZIA!"));
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

void add_empresa(SharedMemory* sharedMemory, TCHAR *nomeEmpresa, DWORD numAcoes, float precoAcao) {
	
	if (sharedMemory->sharedData->numEmpresas < MAX_EMPRESAS) {
		for (int i = 0; i < sharedMemory->sharedData->numEmpresas; i++) {
			if (_tcscmp(sharedMemory->sharedData->empresas[i].nome, nomeEmpresa) == 0) {
				_tprintf(TEXT("Empresa ja existente!\nIntroduza um comando:"));
				return;
			}
		}
		memcpy(sharedMemory->sharedData->empresas[sharedMemory->sharedData->numEmpresas].nome, nomeEmpresa, 99);
		sharedMemory->sharedData->empresas[sharedMemory->sharedData->numEmpresas].acoesDisponiveis = numAcoes;
		sharedMemory->sharedData->empresas[sharedMemory->sharedData->numEmpresas].precoAcao = precoAcao;
		sharedMemory->sharedData->numEmpresas++;
		_tprintf(TEXT("Empresa %s adicionada com sucesso!\n"),nomeEmpresa);
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
		_tprintf(TEXT("Preco da acao: %.2f\n"), sharedMemory->sharedData->empresas[i].precoAcao);
		_tprintf(TEXT("Acoes disponiveis: %d\n"), sharedMemory->sharedData->empresas[i].acoesDisponiveis);
		_tprintf(TEXT("=================================\n"));
	}
}

void stock(SharedMemory* sharedMemory, TCHAR* nomeEmpresa, float precoAcao) {
	for (int i = 0; i < sharedMemory->sharedData->numEmpresas; i++) {
		if (_tcscmp(sharedMemory->sharedData->empresas[i].nome, nomeEmpresa) == 0) {
			sharedMemory->sharedData->empresas[i].precoAcao = precoAcao;
			_tprintf(TEXT("Preco da acao da empresa %s alterado para %.2f\n"), nomeEmpresa, precoAcao);
			return;
		}
	}
	_tprintf(TEXT("Empresa nao encontrada!\n"));
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

void closee(SharedMemory* sharedMemory) {
	CloseHandle(sharedMemory->hMapFile);
	CloseHandle(sharedMemory->hMutexUpdateBoard);
	CloseHandle(sharedMemory->hEventUpdateBoard);
	//CloseHandle(sharedMemory->hEventInPause);
}

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
	HANDLE ReadReady,hMutex;
	OVERLAPPED ov;
	BOOL x = FALSE;
	hMutex = CreateMutex(NULL, FALSE, TEXT("MutexOperacoes"));
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
				WaitForSingleObject(hMutex, INFINITE);
				while (1) {
					if (_tcscmp(utilizador.username, sharedMemory->sharedData->users[j].username) == 0) {
						sharedMemory->sharedData->users[j].login = FALSE;
						sharedMemory->sharedData->users[j].hPipe = NULL;
						j++;
						break;
					}
				}
				_tprintf(_T("O utilizador %s saiu\nIntroduza um comando:"), utilizador.username);
				ReleaseMutex(hMutex);
				return -1;
			}
		}
		WaitForSingleObject(hMutex, INFINITE);
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
					break;
				}
				i++;
			}
			if (!utilizador.login) {
				utilizador.login = FALSE;
				_tprintf(_T("Username ou password incorretos\n"));
			}
		}
		else if (utilizador.tipo == 1) {
			//MANDAR A LISTA DE EMPRESAS
			//WaitForSingleObject(hMutex, INFINITE);
			utilizador.numEmpresas = sharedMemory->sharedData->numEmpresas;
			for (int i = 0; i < sharedMemory->sharedData->numEmpresas; i++) {
				wcscpy_s(utilizador.empresas[i].nome, _countof(utilizador.empresas[i].nome), sharedMemory->sharedData->empresas[i].nome);
				utilizador.empresas[i].acoesDisponiveis = sharedMemory->sharedData->empresas[i].acoesDisponiveis;
				utilizador.empresas[i].precoAcao = sharedMemory->sharedData->empresas[i].precoAcao;
			}
		}
		else if (utilizador.tipo == 2) {
			//fazer a compra de acoes
			x = FALSE;
			BOOL EMPRESA = FALSE,ACOES=FALSE,SALDO =FALSE;
			int n;
			int aux = 0;
			utilizador.Sucesso= FALSE;
			if (sharedMemory->sharedData->pausedBolsa == FALSE) { // Verificar se a bolsa está em pausa
				for (int i = 0; i < MAX_USERS && !x; i++) {
					if(ACOES == TRUE) {
						break;
					}
					if(EMPRESA == TRUE) {
						break;
					}
					if (_tcscmp(sharedMemory->sharedData->users[i].username, utilizador.username) == 0) { // vai buscar o utilizador que está a fazer a compra
						for (int j = 0; j < sharedMemory->sharedData->numEmpresas; j++){ // iterar sobre as empresas
							if (_tccmp(sharedMemory->sharedData->empresas[j].nome, utilizador.NomeEmpresa) == 0) { // verificar se a empresa existe
								EMPRESA = TRUE;
								n = j;
								if (sharedMemory->sharedData->empresas[j].acoesDisponiveis < utilizador.qtAcoes) {
									utilizador.tipoResposta = 2;
									ACOES = TRUE;
									break;
								}
								else if (sharedMemory->sharedData->empresas[j].precoAcao * utilizador.qtAcoes > sharedMemory->sharedData->users[i].saldo) {
									utilizador.tipoResposta = 1;
									SALDO = TRUE;
									break;
								}
							}
						} 
						if(SALDO == TRUE) {
							break;
						}
						else if (ACOES == TRUE) {
							break;
						}
						if(EMPRESA == FALSE) {  // se a empresa não existir sai do ciclo
							utilizador.tipoResposta = 3;
							break;
						}if(sharedMemory->sharedData->users[i].nAcoes == 5) {
							utilizador.tipoResposta = 5;
							break;
						}else{
							for (int j = 0; j < 5; j++) {
								if (_tcscmp(sharedMemory->sharedData->users[i].carteira[j].empresa, utilizador.NomeEmpresa) == 0) {
									sharedMemory->sharedData->users[i].carteira[j].qtAcoes += utilizador.qtAcoes;
									x = TRUE;
									break;
								}
							}
							if (!x) {
								for (int a = 0; a < 5; a++) {
									if (_tcscmp(sharedMemory->sharedData->users[i].carteira[a].empresa, TEXT("")) == 0) {
										wcscpy_s(sharedMemory->sharedData->users[i].carteira[a].empresa, _countof(sharedMemory->sharedData->users[i].carteira[a].empresa), utilizador.NomeEmpresa);
										sharedMemory->sharedData->users[i].carteira[a].qtAcoes = utilizador.qtAcoes;
										sharedMemory->sharedData->users[i].nAcoes++;
										x = TRUE;
										break;
									}
								}
							}
							sharedMemory->sharedData->empresas[n].acoesDisponiveis -= utilizador.qtAcoes;
							sharedMemory->sharedData->users[i].saldo -= utilizador.qtAcoes * sharedMemory->sharedData->empresas[n].precoAcao;
							sharedMemory->sharedData->empresas[n].precoAcao += ((utilizador.qtAcoes *sharedMemory->sharedData->empresas[n].acoesDisponiveis) * 0.1);
							utilizador.saldo = sharedMemory->sharedData->users[i].saldo;
							utilizador.Sucesso = TRUE;
							// Guardar a última transação
							wcscpy_s(sharedMemory->sharedData->lastTransacao.empresa.nome, _countof(sharedMemory->sharedData->lastTransacao.empresa.nome), utilizador.NomeEmpresa);
							sharedMemory->sharedData->lastTransacao.numAcoes = utilizador.qtAcoes;
							sharedMemory->sharedData->lastTransacao.precoAcoes = sharedMemory->sharedData->empresas[n].precoAcao - ((utilizador.qtAcoes * sharedMemory->sharedData->empresas[n].acoesDisponiveis) * 0.1);

							//----------------
							break;
						}
					}
				}
			}
			else {
				utilizador.tipoResposta = 4;
				_tprintf(TEXT("Operacoes de compra e venda suspensas!\n"));
			}
		}
		else if (utilizador.tipo == 3) {
			//fazer a venda de acoes
			x = FALSE;
			BOOL EMPRESA = FALSE, ACOES = FALSE; //empresa == true se a empresa existir 
			int n;
			int aux = 0;
			utilizador.Sucesso = FALSE;
			if (sharedMemory->sharedData->pausedBolsa == FALSE) { // Verificar se a bolsa está em pausa
				for (int i = 0; i < MAX_USERS && !x; i++) {
					if (ACOES) {
						break;
					}
					else if (EMPRESA) {
						break;
					}
					if (_tcscmp(sharedMemory->sharedData->users[i].username, utilizador.username) == 0) {	// vai buscar o utilizador que está a fazer a compra
						for (int j = 0; j < sharedMemory->sharedData->numEmpresas; j++) {
							if (_tcscmp(sharedMemory->sharedData->empresas[j].nome, utilizador.NomeEmpresa) == 0) { // entra caso a empresa exista
								EMPRESA = TRUE;
								n = j;
								break;
							}
						}
						if (EMPRESA == FALSE) {  // se a empresa não existir sai do ciclo
							utilizador.tipoResposta = 3;
							break;
						}
						else {
							for (int j = 0; j < 5; j++) {
								if (_tcscmp(sharedMemory->sharedData->users[i].carteira[j].empresa, utilizador.NomeEmpresa) == 0) {
									if (sharedMemory->sharedData->users[i].carteira[j].qtAcoes < utilizador.qtAcoes) { // se o utilizador não tiver a quantidade de ações que quer vender
										utilizador.tipoResposta = 2;
										ACOES = TRUE;
										break;
									}
									else {
										sharedMemory->sharedData->users[i].carteira[j].qtAcoes -= utilizador.qtAcoes; // se tiver a quantidade de ações que quer vender
										x = TRUE;
										break;
									}
								}
							}if (ACOES) {
								break;
							}
							if (!x) {
								utilizador.tipoResposta = 1;
								break;
							}
							sharedMemory->sharedData->empresas[n].acoesDisponiveis += utilizador.qtAcoes;
							sharedMemory->sharedData->users[i].saldo += utilizador.qtAcoes * sharedMemory->sharedData->empresas[n].precoAcao;
							sharedMemory->sharedData->empresas[n].precoAcao -= ((sharedMemory->sharedData->empresas[n].acoesDisponiveis * utilizador.qtAcoes) * 0.1);
							utilizador.saldo = sharedMemory->sharedData->users[i].saldo;
							utilizador.Sucesso = TRUE;
							// Guardar a última transação
							wcscpy_s(sharedMemory->sharedData->lastTransacao.empresa.nome, _countof(sharedMemory->sharedData->lastTransacao.empresa.nome), utilizador.NomeEmpresa);
							sharedMemory->sharedData->lastTransacao.numAcoes = utilizador.qtAcoes;
							sharedMemory->sharedData->lastTransacao.precoAcoes = sharedMemory->sharedData->empresas[n].precoAcao;
							//----------------
							break;
						}
					}
				}
			}
			else {
				utilizador.tipoResposta = 4;
				_tprintf(TEXT("Operacoes de compra e venda suspensas!\n"));
			}
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
		}
		
		updateInfo(sharedMemory);
		WriteClienteASINC(hPipe);
		ReleaseMutex(hMutex);
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
	eventoEnvia();
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

	_tprintf(TEXT("\n[BOLSA] A espera de clientes...\nIntroduza um comando:\n"));

	while (1) {
		hPipe = CreateNamedPipe(PIPE_NAME_CLIENTS, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, MAX_USERS, MAX_TAM, MAX_TAM, 5000, NULL);
		if (hPipe == INVALID_HANDLE_VALUE) {
			_tprintf(TEXT("[ERRO] Falha ao criar o Named Pipe!\n"));
			exit(-1);
		}
		_tprintf(TEXT("\n[BOLSA] A espera de conexao...\nIntroduza um comando:"));
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

	MAX_USERS = readRegistry(TEXT("NCLIENTES"));
	_tprintf(TEXT("Numero maximo de clientes: %d\n"), MAX_USERS);

	//ler o ficheiro e guardar os utilizadores
	if (argv[1] != NULL) {
		FILE* file;;
		errno_t err = _wfopen_s(&file, argv[1], _T("r"));
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
	}
	
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
		_tprintf(TEXT("Empresas nao carregadas!\n"));
	}
	else {
		_tprintf(TEXT("Opcao invalida!\n"));
	}
	updateInfo(&sharedMemory);
	ThreadsBolsa threadsBolsa;
	threadsBolsa.hThreads[0] = CreateThread(NULL, 0, ConectarClientes, (LPVOID)&sharedMemory, 0, NULL);

	TCHAR context[MAX_TAM];
	TCHAR comando[MAX_TAM];
	TCHAR* argumentos[MAX_TAM];
	TCHAR firstParam[MAX_TAM];
	int nArgs;
	do{
		nArgs = 0;
		_tprintf(TEXT("Introduza um comando: "));
		_fgetts(comando, MAX_TAM, stdin);
		comando[_tcslen(comando) - 1] = '\0'; // Remover o caractere de nova linha
		argumentos[0] = _tcstok_s(comando, TEXT(" "), &context);
		while (argumentos[nArgs] != NULL) {
			argumentos[++nArgs] = _tcstok_s(NULL, TEXT(" "), &context);
		}
		nArgs -= 1;
		if (_tcscmp(argumentos[0], TEXT("addc")) == 0 && nArgs==3) {
			wcscpy_s(firstParam, _countof(firstParam), argumentos[1]);
			DWORD second = _wtoi(argumentos[2]);
			float third = _tstof(argumentos[3]);
			add_empresa(&sharedMemory, firstParam, second, third);
			updateInfo(&sharedMemory);
		}
		else if (_tcscmp(argumentos[0], TEXT("listc")) == 0 && nArgs == 0) {
			updateInfo(&sharedMemory);
			list_empresas(&sharedMemory);
		}
		else if (_tcscmp(argumentos[0], TEXT("stock")) == 0 && nArgs == 2) {
			wcscpy_s(firstParam, _countof(firstParam), argumentos[1]);
			DWORD second = _wtoi(argumentos[2]);
			stock(&sharedMemory, firstParam, second);
			updateInfo(&sharedMemory);
		}
		else if (_tcscmp(argumentos[0], TEXT("users")) == 0 && nArgs==0) {
			updateInfo(&sharedMemory);
			list_users(&sharedMemory);
		}
		else if (_tcscmp(argumentos[0], TEXT("pause")) == 0 && nArgs == 1) {
			DWORD second = _wtoi(argumentos[1]);
			//pause(&sharedMemory, second);
			SetEvent(sharedMemory.hEventRunning);
			sharedMemory.sharedData->seconds = second;
			threadsBolsa.hThreads[1] = CreateThread(NULL, 0, pause, &sharedMemory, 0, NULL);
		}
		else if (_tcsicmp(argumentos[0], TEXT("close")) != 0) {
			//TEMOS DE FAZER PARA MANDAR A MENSAGEM PARA TODOS OS CLIENTES QUE A BOLSA VAI FECHAR
			_tprintf(TEXT("Comando invalido!\n"));
		}
	} while (_tcsicmp(argumentos[0], TEXT("close")) != 0);
	utilizador.BOLSA = TRUE;
	for (int i = 0; i < MAX_USERS; i++) {
		if (sharedMemory.sharedData->users[i].login) {
			WriteClienteASINC(sharedMemory.sharedData->users[i].hPipe);
		}
	}
	_tprintf(TEXT("A bolsa de valores foi terminada!\n"));
	closee(&sharedMemory);
	return 0;
}

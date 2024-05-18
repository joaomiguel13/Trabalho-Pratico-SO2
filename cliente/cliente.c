#include "cliente.h"

BOOL isBolsaRunning() {
    HANDLE hMutex = CreateMutex(NULL, FALSE, MUTEX_NAME);

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        _tprintf(TEXT("================================="));
        _tprintf(TEXT("\n|        [CLIENTE]      |"));
        _tprintf(TEXT("\n================================\n"));
        return FALSE;
    }
    else {
        CloseHandle(hMutex);
        _tprintf(TEXT("Não exite nenhuma bolsa a correr!\n"));
        return TRUE;
    }
}

void listaEmpresas() {
    _tprintf(TEXT("================================="));
    _tprintf(TEXT("\n|        EMPRESAS EXISTENTES      |"));
    _tprintf(TEXT("\n================================\n"));
    int i = 0;
    while (i < utilizador.numEmpresas) {
        _tprintf(TEXT("Empresa: %s\n"), utilizador.empresas[i].nome);
        _tprintf(TEXT("Valor: %.2f\n"), utilizador.empresas[i].precoAcao);
        _tprintf(TEXT("Quantidade de acoes: %d\n"), utilizador.empresas[i].acoesDisponiveis);
        _tprintf(TEXT("=================================\n"));
        i++;
    }
}

int Envia(HANDLE hPipe) {
    DWORD cbBytesWrite = 0;
    BOOL fSuccess = FALSE;
    OVERLAPPED OvW = { 0 };
    HANDLE WriteReady;
    WriteReady = CreateEvent(NULL, TRUE, FALSE, NULL);
    ZeroMemory(&OvW, sizeof(OvW));
    ResetEvent(WriteReady);
    OvW.hEvent = WriteReady;
    ZeroMemory(&OvW, sizeof(OvW));
    ResetEvent(WriteReady);
    OvW.hEvent = WriteReady;
    fSuccess = WriteFile(hPipe, &utilizador, Msg_Sz, &cbBytesWrite, &OvW);
    if (!fSuccess) {
        _tprintf(TEXT("[ERRO] Falha ao enviar a mensagem! (WriteFile)\n"));
        return -1;
    }
    WaitForSingleObject(WriteReady, INFINITE);
    return 0;
}

BOOL WINAPI Recebe(LPVOID lpParam) {
    HANDLE hPipe = (HANDLE)lpParam;
    DWORD cbBytesRead = 0;
    BOOL fSuccess = FALSE;
    OVERLAPPED OvR = { 0 };
    HANDLE ReadReady;
    ReadReady = CreateEvent(NULL, TRUE, FALSE, NULL);
    BOOL loginnn = FALSE;
    BOOL sair = FALSE;
    do{
        if(SAIR) {
			break;
		}
        WaitForSingleObject(eventoLer.hEventoLer, INFINITE);
        ZeroMemory(&OvR, sizeof(OvR));
        ResetEvent(ReadReady);
        OvR.hEvent = ReadReady;

        fSuccess = ReadFile(hPipe, &utilizador, Msg_Sz, &cbBytesRead, &OvR);
        if (!fSuccess && GetLastError() != ERROR_IO_PENDING) {
            _tprintf(TEXT("[ERRO] Falha ao receber a mensagem!\n"));
            return -1;
        }
        // Verifica se a opera  o est  pendente (n o h  dados imediatamente dispon veis)
        if (!fSuccess) {
            // A opera  o est  pendente, aguardar at  que ela seja conclu da
            WaitForSingleObject(ReadReady, INFINITE);
            // Verifica se a opera  o foi conclu da com sucesso
            fSuccess = GetOverlappedResult(hPipe, &OvR, &cbBytesRead, FALSE);
                 if (!fSuccess) {
                _tprintf(TEXT("[ERRO] Falha na leitura do pipe!\n"));
                return -1;
            }
        }
        if(utilizador.BOLSA == TRUE) {
			_tprintf(TEXT("Bolsa fechada!\n"));
            sair = TRUE;
        }
        else {
            if (utilizador.login == FALSE) {
                _tprintf(TEXT("Invalid username or password\n"));
            }
            else {
                if (loginnn == FALSE) {
                    _tprintf(TEXT("\nLogin efetuado com sucesso!\nBem vindo: %s\t Saldo:%.2f\n"), utilizador.username, utilizador.saldo);
                    loginnn = TRUE;
                }
                else {
                    if (utilizador.tipo == 1) { //listc
                        listaEmpresas();
                    }
                    else if (utilizador.tipo == 2) { //buy
                        if (utilizador.Sucesso == TRUE) {
                            _tprintf(TEXT("Comprou %d acoes a empresa %s.\nSaldo: %.2f\n"), utilizador.qtAcoes, utilizador.NomeEmpresa, utilizador.saldo);
                        }
                        else {
                            if (utilizador.tipoResposta == 1)
                                _tprintf(TEXT("Saldo Insuficiente!\n"));
                            else if (utilizador.tipoResposta == 2) {
                                _tprintf(TEXT("Nao ha acoes suficientes\n"));

                            }
                            else if (utilizador.tipoResposta == 3) {
                                _tprintf(TEXT("Empresa nao encontrada\n"));

                            }
                            else if (utilizador.tipoResposta == 4) {
                                _tprintf(TEXT("Operacoes de compra e venda foram suspensas\n"));
                            }
                            else if (utilizador.tipoResposta == 5) {
                                _tprintf(TEXT("Ja tem acoes em 5 empresas, para comprar outra tem de vender\n"));
                            }
                        }
                    }
                    else if (utilizador.tipo == 3) { //sell
                        if (utilizador.Sucesso == TRUE)
                            _tprintf(TEXT("Vendeu %d acoes a empresa %s.\n Saldo: %.2f\n"), utilizador.qtAcoes, utilizador.NomeEmpresa, utilizador.saldo);
                        else {
                            if (utilizador.tipoResposta == 1) {
                                _tprintf(TEXT("Nao tem nehuma acoes nessa Empresa\n"));
                            }
                            if (utilizador.tipoResposta == 2) {
                                _tprintf(TEXT("Nao tem essa quantidade de acoes para vender\n"));
                            }
                            else if (utilizador.tipoResposta == 3) {
                                _tprintf(TEXT("Empresa nao encontrada\n"));
                            }
                            else if (utilizador.tipoResposta == 4) {
                                _tprintf(TEXT("Operacoes de compra e venda foram suspensas\n"));
                            }
                        }
                    }
                    else if (utilizador.tipo == 4) {
                        _tprintf_s(TEXT("saldo: %.2f\n"), utilizador.saldo);

                    }
                    else if (utilizador.tipo == 5) {
                        _tprintf_s(TEXT("Operações de compra e venda foram suspensas\n"));
                    }
                    else if (utilizador.tipo == 6) {
                        _tprintf_s(TEXT("Board fechada!\n"));
                    }
                }
            }
        }
        ResetEvent(eventoLer.hEventoLer);
	} while (!sair);
    SAIR = TRUE;
    return TRUE;
}

BOOL WINAPI Comandos(LPVOID lpParam) {
    HANDLE hPipe = (HANDLE)lpParam;
    TCHAR context[MAX_TAM];
    TCHAR comando[MAX_TAM];
    TCHAR* argumentos[MAX_TAM];
    int nArgs;
    do {
        if (SAIR) {
            break;
        }
        nArgs = 0;
        Sleep(50);
        if (utilizador.login == FALSE) {
            _tprintf(TEXT("\nlogin <username> <password> para entrar\n"));
            _fgetts(comando, MAX_TAM, stdin);
            comando[_tcslen(comando) - 1] = '\0'; // Remover o caractere de nova linha
            argumentos[0] = _tcstok_s(comando, TEXT(" "), &context);
            while (argumentos[nArgs] != NULL) {
                argumentos[++nArgs] = _tcstok_s(NULL, TEXT(" "), &context);
            }
            nArgs -= 1;
            if (_tcscmp(argumentos[0], _T("login")) == 0 && nArgs == 2) {
                wcscpy_s(utilizador.username, _countof(utilizador.username), argumentos[1]);
                wcscpy_s(utilizador.password, _countof(utilizador.password), argumentos[2]);
                Envia(hPipe);
            }
            else if (_tcsicmp(argumentos[0], TEXT("exit")) != 0) {
                _tprintf(TEXT("Comando invalido!\n"));
            }
        }
        else {
            _tprintf(TEXT("\nComando: "));
            _fgetts(comando, MAX_TAM, stdin);
            comando[_tcslen(comando) - 1] = '\0'; // Remover o caractere de nova linha
            argumentos[0] = _tcstok_s(comando, TEXT(" "), &context);
            while (argumentos[nArgs] != NULL) {
                argumentos[++nArgs] = _tcstok_s(NULL, TEXT(" "), &context);
            }
            nArgs -= 1;
            if (_tcsicmp(argumentos[0], TEXT("listc")) == 0 && nArgs == 0) {
                utilizador.tipo = 1;
                Envia(hPipe);
            }
            else if (_tcsicmp(argumentos[0], TEXT("buy")) == 0 && nArgs == 2) {
                utilizador.tipo = 2;
                wcscpy_s(utilizador.NomeEmpresa, _countof(utilizador.NomeEmpresa), argumentos[1]);
                utilizador.qtAcoes = _wtoi(argumentos[2]);
                Envia(hPipe);
            }
            else if (_tcsicmp(argumentos[0], TEXT("sell")) == 0 && nArgs == 2) {
                utilizador.tipo = 3; 
                utilizador.Sucesso = FALSE;
                wcscpy_s(utilizador.NomeEmpresa, _countof(utilizador.NomeEmpresa), argumentos[1]);
                utilizador.qtAcoes = _wtoi(argumentos[2]);
                Envia(hPipe);
            }
            else if (_tcsicmp(argumentos[0], TEXT("balance")) == 0 && nArgs == 0) {
                utilizador.tipo = 4;
                Envia(hPipe);
            }
            else if (_tcscmp(argumentos[0], TEXT("closeboard")) == 0 && nArgs == 0) {
                utilizador.tipo = 6;
                Envia(hPipe);
            }
            else if (_tcsicmp(argumentos[0], TEXT("exit")) != 0) {
                _tprintf(TEXT("Comando invalido!\n"));
            }
        }
    } while (_tcsicmp(argumentos[0], TEXT("exit")) != 0);
    SAIR = TRUE;
    return TRUE;
}

BOOL WINAPI CloseThreads(LPVOID lpParam) {
    ThreadsCliente* threadsCliente = (ThreadsCliente*)lpParam;
    while (1) {
        Sleep(1000);
        if (SAIR) {
            TerminateThread(threadsCliente->hThreads[0], 0);
            TerminateThread(threadsCliente->hThreads[1], 0);
        }
    }
	return TRUE;
}

int _tmain() {
    HANDLE hPipe;
    DWORD cbBytesRead = 0, cbBytesWrite = 0, resposta = 0;
    BOOL fSuccess = FALSE;
    OVERLAPPED OvR = { 0 };
    OVERLAPPED OvW = { 0 };
    TCHAR context[MAX_TAM];

#ifdef UNICODE
    _setmode(_fileno(stdin), _O_WTEXT);
    _setmode(_fileno(stdout), _O_WTEXT);
    _setmode(_fileno(stderr), _O_WTEXT);
#endif

    //HANDLE hSemaphore;
    /*HANDLE hSemaphore = CreateSemaphore(NULL, 0, MAX_USERS, TEXT("MyNamedSemaphore"));
    _tprintf(_T("MAX_USERS:%d"), MAX_USERS);
    if (hSemaphore == NULL) {
        _tprintf(TEXT("[ERRO] Falha ao criar o semaforo!\n"));
        return -1;
    }
    if (WaitForSingleObject(hSemaphore, 0) == WAIT_TIMEOUT) {
        _tprintf(TEXT("Numero maximo de clientes atingiado! A espera por uma vaga...\n"));
        WaitForSingleObject(hSemaphore, INFINITE);
    }*/

    utilizador.login = FALSE;
    utilizador.BOLSA = FALSE;
    isBolsaRunning();
    ThreadsCliente threadsCliente;
    hPipe = CreateFile(PIPE_NAME_CLIENTS, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
    if (hPipe == INVALID_HANDLE_VALUE) {
        _tprintf(TEXT("[ERRO] Falha ao conectar-se a bolsa!\n"));
        return -1;
    }
    if (!WaitNamedPipe(PIPE_NAME_CLIENTS, 30000)) {
        _tprintf(TEXT("[ERRO] Falha ao conectar-se a bolsa!\n"));
        return -1;
    }
    _tprintf(TEXT("[CLIENTE] Conectado a bolsa!\n Bem-vindo(a)... \n"));

    eventoLer.hEventoLer = CreateEvent(NULL, TRUE, FALSE, TEXT("hEventoLer"));

    threadsCliente.hThreads[0]= CreateThread(NULL, 0, Comandos, (LPVOID)hPipe, 0, NULL);
    threadsCliente.hThreads[1] = CreateThread(NULL, 0, Recebe, (LPVOID)hPipe, 0, NULL);
    HANDLE hThread = CreateThread(NULL, 0, CloseThreads, (LPVOID)&threadsCliente, 0, NULL);

    WaitForSingleObject(threadsCliente.hThreads[0], INFINITE);
    WaitForSingleObject(threadsCliente.hThreads[1], INFINITE);

    CloseHandle(threadsCliente.hThreads[0]);
    CloseHandle(threadsCliente.hThreads[1]);
    _tprintf(TEXT("A sair..."));
    CloseHandle(hPipe);
    //ReleaseSemaphore(hSemaphore, 1, NULL);
    return 0;
}
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
    _tprintf(TEXT("[CLIENTE] Mensagem enviada à bolsa!\n"));
    return 0;
}

int Recebe(HANDLE hPipe) {
    DWORD cbBytesRead = 0;
    BOOL fSuccess = FALSE;
    OVERLAPPED OvR = { 0 };
    HANDLE ReadReady;
    ReadReady = CreateEvent(NULL, TRUE, FALSE, NULL);
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
    _tprintf(TEXT("[CLIENTE] Mensagem recebida da bolsa!\n"));
    return 0;
}


int _tmain() {
    HANDLE hPipe;
    DWORD cbBytesRead = 0, cbBytesWrite = 0;
    BOOL fSuccess = FALSE, resposta = FALSE;
    OVERLAPPED OvR = { 0 };
    OVERLAPPED OvW = { 0 };
    TCHAR context[MAX_TAM];

#ifdef UNICODE
    _setmode(_fileno(stdin), _O_WTEXT);
    _setmode(_fileno(stdout), _O_WTEXT);
    _setmode(_fileno(stderr), _O_WTEXT);
#endif
    utilizador.login = FALSE;

    isBolsaRunning();

    hPipe = CreateFile(PIPE_NAME_CLIENTS, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
    if (hPipe == INVALID_HANDLE_VALUE) {
        _tprintf(TEXT("[ERRO] Falha ao conectar-se à bolsa!\n"));
        return -1;
    }
    if (!WaitNamedPipe(PIPE_NAME_CLIENTS, 30000)) {
        _tprintf(TEXT("[ERRO] Falha ao conectar-se à bolsa!\n"));
        return -1;
    }
    _tprintf(TEXT("[CLIENTE] Conectado à bolsa!\n Bem-vindo(a)... \n"));


    TCHAR comando[MAX_TAM];
    TCHAR* argumentos[MAX_TAM];
    int nArgs;
    do {
        nArgs = 0;
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
                Sleep(1000);
                Recebe(hPipe);

                if (utilizador.login == TRUE) {
                    _tprintf(TEXT("Login efetuado com sucesso!\nBem vindo: %s\t Saldo:%.2f"), utilizador.username, utilizador.saldo);
                }
                else {
                    _tprintf(TEXT("Invalid username or password\n"));
                }
            }
            else if (_tcsicmp(argumentos[0], TEXT("exit")) != 0) {
                _tprintf(TEXT("Comando invalido!!!!!!!!!!!!!\n"));
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
                Recebe(hPipe);
                listaEmpresas();
            }
            else if (_tcsicmp(argumentos[0], TEXT("buy")) == 0 && nArgs == 2) {
                utilizador.tipo = 2;
                wcscpy_s(utilizador.NomeEmpresa, _countof(utilizador.NomeEmpresa), argumentos[1]);
                utilizador.qtAcoes = _wtoi(argumentos[2]);
                Envia(hPipe);
                Recebe(hPipe);

                if (utilizador.Sucesso == TRUE) {
                    _tprintf(TEXT("Comprou %d ações a empresa %s.\nSaldo: %.2f\n"), utilizador.qtAcoes, utilizador.NomeEmpresa, utilizador.saldo);
                }
                else {
                    if (utilizador.tipoResposta == 1)
                        _tprintf(TEXT("Saldo Insuficiente!\n"));
                    else if (utilizador.tipoResposta == 2) {
                        _tprintf(TEXT("Não há ações suficientes\n"));
                    }
                    else if (utilizador.tipoResposta == 3) {
                        _tprintf(TEXT("Empresa não encontrada\n"));
                    }
                    else if(utilizador.tipoResposta == 4) {
						_tprintf(TEXT("Operações de compra e venda foram suspensas\n"));
					}
                }
            }
            else if (_tcsicmp(argumentos[0], TEXT("sell")) == 0 && nArgs == 2) {
                utilizador.tipo = 3;
                utilizador.Sucesso = FALSE;
                wcscpy_s(utilizador.NomeEmpresa, _countof(utilizador.NomeEmpresa), argumentos[1]);
                utilizador.qtAcoes = _wtoi(argumentos[2]);
                Envia(hPipe);
                Recebe(hPipe);

                if (utilizador.Sucesso == TRUE)
                    _tprintf(TEXT("Vendeu %d ações a empresa %s.\n Saldo: %.2f\n"), utilizador.qtAcoes, utilizador.NomeEmpresa, utilizador.saldo);
                else {
                    if (utilizador.tipoResposta == 3) {
                        _tprintf(TEXT("Empresa não encontrada\n"));
                    }
                    else if (utilizador.tipoResposta == 4) {
                        _tprintf(TEXT("Operações de compra e venda foram suspensas\n"));
                    }
                }
            }
            else if (_tcsicmp(argumentos[0], TEXT("balance")) == 0 && nArgs == 0) {
                utilizador.tipo = 4;
                Envia(hPipe);
                Recebe(hPipe);
                _tprintf_s(TEXT("saldo: %.2f\n"), utilizador.saldo);
            }
            else if (_tcsicmp(argumentos[0], TEXT("exit")) != 0) {
                _tprintf(TEXT("Comando inválido\n"));
            }
        }
    } while (_tcsicmp(argumentos[0], TEXT("exit")) != 0);
    _tprintf(TEXT("A sair..."));
    CloseHandle(hPipe);
    return 0;
}
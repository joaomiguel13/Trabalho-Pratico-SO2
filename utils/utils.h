#pragma once
#include <windows.h>
#include <tchar.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

#ifndef UTILS_SO2_TP_H
#define UTILS_SO2_TP_H
#endif

#define PIPE_NAME_CLIENTS TEXT("\\\\.\\pipe\\pipeClientes")
#define REGISTRY_PATH TEXT("SO2\\TrabalhoPratico\\")
#define SHARED_MEMORY_NAME TEXT("sharedMemoryBolsa") 

#define EMPRESAS_FILE TEXT("empresas.txt")

#define MUTEX_NAME TEXT("mutexBolsa")

#define MAX_TAM_BUFFER 1024
#define DEFAULT_VALUE_NCLIENTES 5
#define MAX_TAM 100
#define MAX_USERS 20
#define MAX_EMPRESAS 30
#define Msg_Sz sizeof(utilizador)

typedef struct {
	TCHAR username[MAX_TAM];
	TCHAR password[20];
	float saldo;
	//Acoes carteira[40]; //........ associar as acoes de compra
} User;

User users[MAX_USERS];

typedef struct {
	User users[MAX_USERS];
	DWORD numUsers;
} Bolsa;

typedef struct {
	TCHAR nome[100];
	float precoAcao;
	DWORD acoesDisponiveis;
}Empresa;

typedef struct {
	TCHAR tipo;
	Empresa *empresa;
	User* user;
	DWORD numAcoes;
	float precoAcoes;
} Transacoes;

typedef struct {
	TCHAR empresa[20];
	DWORD qtAcoes;
	float qtAcoesEmpresa;
}Acoes;

typedef struct {
	Empresa empresas[MAX_EMPRESAS];
	int numEmpresas; //contador de empresas
	Transacoes lastTransacao;
}SharedData;

typedef struct {
	HANDLE hMapFile;
	HANDLE hMutexUpdateBoard;
	HANDLE hEventUpdateBoard;
	HANDLE hEventRunning;

	SharedData* sharedData;
}SharedMemory;

#define N_THREADS_BOLSA 2
typedef struct {
	HANDLE hThreads[N_THREADS_BOLSA];
	HANDLE hEventCloseAllThreads;
}ThreadsBolsa;

#define N_THREADS_BOARD 2
typedef struct {
	HANDLE hThreads[N_THREADS_BOARD];
	HANDLE hEventCloseAllThreads;
}ThreadsBoard;

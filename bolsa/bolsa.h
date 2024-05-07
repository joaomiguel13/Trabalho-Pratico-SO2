#pragma once
#include "../utils/utils.h"



typedef struct {
	int tipo;
	TCHAR username[MAX_TAM];
	TCHAR password[20];
	double saldo;
	BOOL login;
	TCHAR empresa[20];
	DWORD qtAcoes;
	float qtAcoesEmpresa;
}NovosUsers;
NovosUsers utilizador;




void add_empresa(SharedMemory* sharedMemory, TCHAR* nomeEmpresa, DWORD numAcoes, DWORD precoAcao);
/*void list_empresas(SharedMemory* sharedMemory);
void stock(SharedMemory* sharedMemory, TCHAR* nomeEmpresa, float precoAcao);
void list_users(SharedMemory* sharedMemory);
void pause(SharedMemory* sharedMemory, DWORD seconds);
//void close(SharedMemory* sharedMemory);*/

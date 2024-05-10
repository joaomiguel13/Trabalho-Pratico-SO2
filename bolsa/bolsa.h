#pragma once
#include "../utils/utils.h"

typedef struct {
	TCHAR nome[100];
	float precoAcao;
	DWORD acoesDisponiveis;
}Empresas;

typedef struct {
	int tipo;
	//---------------
	BOOL login;
	TCHAR username[MAX_TAM];
	TCHAR password[20];
	double saldo;
	//----------------
	BOOL Sucesso;
	TCHAR NomeEmpresa[20];
	DWORD qtAcoes;
	int numEmpresas;
	//---------------
	Empresas empresas[MAX_EMPRESAS];
}NovosUsers;
NovosUsers utilizador;








void add_empresa(SharedMemory* sharedMemory, TCHAR* nomeEmpresa, DWORD numAcoes, DWORD precoAcao);
/*void list_empresas(SharedMemory* sharedMemory);
void stock(SharedMemory* sharedMemory, TCHAR* nomeEmpresa, float precoAcao);
void list_users(SharedMemory* sharedMemory);
void pause(SharedMemory* sharedMemory, DWORD seconds);
//void close(SharedMemory* sharedMemory);*/

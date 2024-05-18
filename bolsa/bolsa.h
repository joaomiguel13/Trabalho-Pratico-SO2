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
	int tipoResposta; // 1 saldo insuficiente, 2 Não há ações suficientes,3 empresa nao existe
	//---------------
	BOOL Sucesso;
	TCHAR NomeEmpresa[20];
	DWORD qtAcoes;
	//---------------
	int numEmpresas;
	//---------------
	Empresas empresas[MAX_EMPRESAS];
	//---------------
	BOOL BOLSA;
}NovosUsers;
NovosUsers utilizador;

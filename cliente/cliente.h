#pragma once

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
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
	int tipoResposta; // 1 saldo insuficiente, 2 N�o h� a��es suficientes,3 empresa nao existe
	//---------------
	BOOL Sucesso;
	TCHAR NomeEmpresa[20];
	DWORD qtAcoes;
	//---------------
	int numEmpresas;
	//---------------
	Empresas empresas[MAX_EMPRESAS];
}NovosUsers;
NovosUsers utilizador;





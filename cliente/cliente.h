#pragma once

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include "../utils/utils.h"

#define MUTEX_NAME TEXT("mutexBolsa")
#define MAX_MSG 60
#define Msg_Sz sizeof(utilizador)

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


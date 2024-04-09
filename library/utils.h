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
#define MAX_TAM 50

#define MAX_USERS 20
#define REGISTRY_PATH TEXT("SO2\\TrabalhoPratico\\")
#define DEFAULT_VALUE_NCLIENTES 5
#define SHARED_MEMORY_NAME TEXT("sharedMemoryBolsa")


typedef struct {
	char username[20];
	char password[20];
	float saldo;
} User;

typedef struct {
	User users[MAX_USERS];
	int numUsers;
} Bolsa;


#define MAX_TAM_BUFFER 10
typedef struct {
	TCHAR message[MAX_TAM];
}BufferCell;

typedef struct {
	DWORD wP;
	DWORD rP;

	BufferCell buffer[MAX_TAM_BUFFER];
}BufferCircular;



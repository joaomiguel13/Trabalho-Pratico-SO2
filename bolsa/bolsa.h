#pragma once
#include "../utils/utils.h"


void add_empresa(SharedMemory* sharedMemory, TCHAR* nomeEmpresa, DWORD numAcoes, DWORD precoAcao);
/*void list_empresas(SharedMemory* sharedMemory);
void stock(SharedMemory* sharedMemory, TCHAR* nomeEmpresa, float precoAcao);
void list_users(SharedMemory* sharedMemory);
void pause(SharedMemory* sharedMemory, DWORD seconds);
//void close(SharedMemory* sharedMemory);*/

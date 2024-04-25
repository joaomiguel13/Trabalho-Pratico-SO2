#include "board.h"
#include "../library/utils.h"

//Verificar se já está uma bolsa em execução
BOOL isBolsaRunning() {
	HANDLE hMutex = CreateMutex(NULL, FALSE, MUTEX_NAME);

	if(hMutex == NULL){
		return FALSE;
	}
	else if (GetLastError() == ERROR_ALREADY_EXISTS)
		return TRUE;

	return FALSE;
}

BOOL initSharedMemory_Sync(SharedMemory *sharedMemory){


}
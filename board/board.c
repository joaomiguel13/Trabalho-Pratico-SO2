#include "board.h"
#include "../library/utils.h"

//Verificar se j� est� uma bolsa em execu��o
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
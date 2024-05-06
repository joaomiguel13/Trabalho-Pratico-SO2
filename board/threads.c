#include "threads.h"

BOOL WINAPI receiveInfoFromBolsa(LPVOID p) {
	SharedMemory* sharedMemory = (SharedMemory*)p;

	while (TRUE) {
		WaitForSingleObject(sharedMemory->hEventUpdateBoard, INFINITE);
	

		_tprintf(_T("=============================\n"));
		for(int i = 0; i < numMaxEmpresas; i++) {
			_tprintf(_T("Empresa %d\n"), i);
			_tprintf(_T("Nome: %s\n"), sharedMemory->sharedData->empresas[i].nome);
			_tprintf(_T("Ações disponíveis: %d\n"), sharedMemory->sharedData->empresas[i].acoesDisponiveis);
			_tprintf(_T("Preço da ação: %.2f\n"), sharedMemory->sharedData->empresas[i].precoAcao);
			_tprintf(_T("----------------------------\n"));
		}
		_tprintf(_T("Ultima transação\n"));
		_tprintf(_T("Empresa: %s\n"), sharedMemory->sharedData->lastTransacao.empresa->nome);
		_tprintf(_T("Numero de ações: %d\n"), sharedMemory->sharedData->lastTransacao.numAcoes);
		_tprintf(_T("Preço das ações: %.2f\n"), sharedMemory->sharedData->lastTransacao.precoAcoes);
		_tprintf(_T("=============================\n"));


		ResetEvent(sharedMemory->hEventUpdateBoard);

	}

	return TRUE;
}
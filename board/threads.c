#include "threads.h"

int comparaEmpresas(Empresa empresas[], int numEmpresas) {
	for (int i = 1; i < numEmpresas; i++){
		Empresa temp = empresas[i];
		int j = i - 1;
		while(j >= 0 && empresas[j].precoAcao < temp.precoAcao) {
			empresas[j + 1] = empresas[j];
			j--;
		}
		empresas[j + 1] = temp;
	}
}

BOOL WINAPI receiveInfoFromBolsa(LPVOID p) {
	SharedMemory* sharedMemory = (SharedMemory*)p;

	while (TRUE) {
		WaitForSingleObject(sharedMemory->hEventUpdateBoard, INFINITE);
	
		system("cls");
		_tprintf(_T("=============================\n"));

		for(int i = 0; i < numMaxEmpresas; i++) {
			comparaEmpresas(sharedMemory->sharedData->empresas, sharedMemory->sharedData->numEmpresas);
			if (i + 1 <= sharedMemory->sharedData->numEmpresas) {
				_tprintf(_T("Empresa %d\n"), i);
				_tprintf(_T("Nome: %s\n"), sharedMemory->sharedData->empresas[i].nome);

				_tprintf(_T("A��es dispon�veis: %d\n"), sharedMemory->sharedData->empresas[i].acoesDisponiveis);

				_tprintf(_T("Pre�o da a��o: %.2f\n"), sharedMemory->sharedData->empresas[i].precoAcao);
				
				
				_tprintf(_T("----------------------------\n"));
			}
		}
		_tprintf(_T("Ultima transa��o\n"));
		_tprintf(_T("Empresa: %s\n"), sharedMemory->sharedData->lastTransacao.empresa.nome);
		_tprintf(_T("N�mero de a��es: %d\n"), sharedMemory->sharedData->lastTransacao.numAcoes);
		_tprintf(_T("Pre�o das a��es: %.2f\n"), sharedMemory->sharedData->lastTransacao.precoAcoes);
		_tprintf(_T("=============================\n"));
		ResetEvent(sharedMemory->hEventUpdateBoard);
	}

	return TRUE;
}
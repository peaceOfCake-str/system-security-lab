// Assignment 8—Anti-debugging framework 구현
// IT공학전공 김정윤(2115227)


// Launcher.exe
// 1. 디버깅 여부 점검
// 2. Client 프로그램 실행 - CreateProcess
// 3. Client 프로그램 디버깅 - Debugger

#include <windows.h>
#include <stdio.h>

#pragma comment(linker, "/INCLUDE:__tls_used")			// 프로젝트(링커) 설정. TLS 사용함
#pragma comment(linker, "/INCLUDE:_pCallBacks")		// pCallBacks 변수를 링커에 전달


// 1. 디버깅 여부 점검
// watchdog 함수를 재활용하여 디버깅을 탐지하였다.

int is_running = 1;			// Thread Flag
HANDLE hThread = NULL;

__inline BOOL IsDebuggerPresentAsm() {
	__asm {
		mov eax, dword ptr fs : [0x30]				// Thread Environment Block -> Process Environment Block
		movzx eax, byte ptr ds : [eax + 0x02]		// Process Environment Block -> BeingDebugged
	}
}

DWORD WINAPI debugger_watchdog(void* arg) {
	printf("[LAUNCHER] 디버깅 탐지 시작합니다..\n");
	while (is_running) {
		if (IsDebuggerPresentAsm()) {
			printf("[LAUNCHER] 디버깅 당하는 중입니다. 프로그램을 종료합니다.\n");
			ExitProcess(1);		// 프로세스 종료
		}
		else Sleep(1000);
	}
	printf("[LAUNCHER] 스레드를 종료합니다.\n");
	return 0;
}

// tls_callback도 재활용하여 프로세스를 실행하자마자 디버깅을 탐지하도록 하였다.
void NTAPI TLS_CALLBACK1(PVOID DllHandle, DWORD Reason, PVOID Reserved) {

	printf("[LAUNCHER] No. 1 with Reason=%u\n", Reason);

	if (IsDebuggerPresentAsm()) {
		printf("[LAUNCHER] 디버깅 당하는 중입니다. 프로그램을 종료합니다.\n");
		ExitProcess(1);		// 프로세스 종료
	}

	if (Reason == 1)
		hThread = CreateThread(0, 0, debugger_watchdog, 0, 0, 0);		// debugger_watchdog을 실행할 새로운 쓰레드 생성
	else
		Sleep(100);
}

#pragma data_seg(".CRT$XLX")
extern "C" PIMAGE_TLS_CALLBACK pCallBacks[] = { TLS_CALLBACK1, 0 };
#pragma data_seg()


int main() {

	printf("[LAUNCHER] 프로그램을 시작했습니다.\n");

	// 2. Client.exe 실행
	// CreateProcessA로 Client.exe를 실행한다. 
	STARTUPINFOA si = { 0 };
	PROCESS_INFORMATION pi = { 0 };
	si.cb = sizeof(si);
	char cmdLine[] = "Client.exe";

	if (!CreateProcessA(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
		printf("[LAUNCHER] Client.exe 실행을 실패했습니다.\n");
		return 0;
	}

	printf("[LAUNCHER] Client.exe를 실행하였습니다. (PID: %d)\n", pi.dwProcessId);

	// 3. Client.exe 디버깅
	// DebugActiveProcess로 Launcher를 Client에 부착하여 디버깅한다.
	if (!DebugActiveProcess(pi.dwProcessId)) {
		printf("[LAUNCHER] Client.exe 디버거 부착에 실패하였습니다.\n");
		return 0;
	}

	printf("[LAUNCHER] Client.exe 디버거 부착에 성공하였습니다.\n");

	// 디버깅 과정에서 발생하는 각종 DEBUG_EVENT들에 대한 처리를 다루는 무한 루프이다.
	DEBUG_EVENT de;
	BOOL isDebugging = true;
	DWORD dwContinueStatus = DBG_CONTINUE;

	// Debugger's Main Loop
	while (isDebugging) {
		if (!WaitForDebugEvent(&de, INFINITE)) break;
		
		dwContinueStatus = DBG_CONTINUE;
		switch (de.dwDebugEventCode) {
			case EXCEPTION_DEBUG_EVENT:	
				break;
			case CREATE_THREAD_DEBUG_EVENT:
				break;
			case CREATE_PROCESS_DEBUG_EVENT:
				break;
			case EXIT_THREAD_DEBUG_EVENT:
				break;

			// 부착한 프로세스가 종료했을 때((Client.exe에 exit이 입력됐을 경우)는 Launcher.exe도 함께 정상 종료시키도록 하였다.
			case EXIT_PROCESS_DEBUG_EVENT:
				printf("[LAUNCHER] Client.exe가 종료되었습니다.\n");
				isDebugging = false;
				break;
			case LOAD_DLL_DEBUG_EVENT:
				break;
			case UNLOAD_DLL_DEBUG_EVENT:
				break;
			case OUTPUT_DEBUG_STRING_EVENT:
				break;
			case RIP_EVENT:
				break;
			default:
				break;
		}
		
		if (isDebugging) ContinueDebugEvent(de.dwProcessId, de.dwThreadId, dwContinueStatus);
	}

	is_running = 0;

	if(hThread) WaitForSingleObject(hThread, 1000);
	CloseHandle(hThread);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	printf("[LAUNCHER] 정상 종료되었습니다.\n");
	return 0;
}
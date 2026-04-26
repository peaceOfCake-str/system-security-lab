// Assignment 8—Anti-debugging framework 구현
// IT공학전공 김정윤(2115227)


// Client.exe
// 1. 중요한 기능
// 2. 부모 프로세스(Launcher.exe) 체크 기능
// 3. 디버깅 여부 점검 - Launcher로 디버깅 중이어야 함(선점)

#include <Windows.h>
#include <stdio.h>
#include <TlHelp32.h>
#include <string.h>

bool checkParentProcess() {
	// Launcher에 의해 호출됨
	// 프로그램 실행 즉시 부모 프로그램을 검사하여 부모 프로세스 이름이 Launcher.exe일 때만 프로그램을 정상 실행
	// 윈도우즈 탐색기에서 더블 클릭했을 때 실행되면 안됨

	DWORD clientPID = GetCurrentProcessId();				// client의 pid
	DWORD parentPID = 0;										// launcher의 pid

	printf("[CLIENT] Client.exe의 PID는 %lu입니다.\n", clientPID);


	HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (h == INVALID_HANDLE_VALUE) return false;

	PROCESSENTRY32W pe = { 0 };
	pe.dwSize = sizeof(PROCESSENTRY32W);

	// 프로세스 목록을 처음부터 순회 -> clientPID를 통해
	// 부모 프로세스(Launcher.exe)의 PID를 찾아냄
	if (Process32FirstW(h, &pe)) {
		do {
			// 순회 중 clientPID를 찾았다면 변수에 부모 PID 저장
			if (pe.th32ProcessID == clientPID) {
				parentPID = pe.th32ParentProcessID;
				printf("[CLIENT] Client.exe의 부모 프로세스의 PID는 %lu입니다.\n", parentPID);
				break;
			}
		} while (Process32NextW(h, &pe));		// 다음 프로세스로 이동
	}

	if (parentPID == 0) {
		printf("[CLIENT] 부모 프로세스를 찾을 수 없습니다.\n");
		CloseHandle(h);
		return false;
	}

	// 프로세스 목록을 처음부터 순회 -> parentPID를 통해
	// 부모 프로세스의 이름을 확인
	bool isLauncher = false;

	if (Process32FirstW(h, &pe)) {
		do {
			if (pe.th32ProcessID == parentPID) {
				printf("[CLIENT] 부모 프로세스의 이름은 %ws입니다.\n", pe.szExeFile);
				if (_wcsicmp(pe.szExeFile, L"Launcher.exe") == 0) {
					isLauncher = true;
				}
				else {
					printf("[CLIENT] 부모 프로세스의 이름은 %ws입니다.\n", pe.szExeFile);
				}
				break;
			}
		} while (Process32NextW(h, &pe));
	}
	CloseHandle(h);
	return isLauncher;
}

int is_running = 1;			// Thread Flag
HANDLE hThread = NULL;

__inline BOOL checkBeingDebugged() {
	__asm {
		mov eax, dword ptr fs : [0x30]				// Thread Environment Block -> Process Environment Block
		movzx eax, byte ptr ds : [eax + 0x02]		// Process Environment Block -> BeingDebugged
	}
}

DWORD WINAPI debugger_watchdog(void* arg) {
	printf("[CLIENT] 디버깅 탐지 시작합니다..\n");
	while (is_running) {
		if (!checkBeingDebugged()) {
			printf("[CLIENT] 디버깅 당하고 있지 않습니다. 프로그램을 종료합니다.\n");
			ExitProcess(1);		// 프로세스 종료
		}
		else Sleep(1000);
	}
	printf("[CLIENT] 스레드를 종료합니다.\n");
	return 0;
}

int main() {

	Sleep(1000);

	if (!checkParentProcess()) {
		printf("[CLIENT] Launcher.exe에 의해 실행되고 있지 않습니다. 프로그램을 종료합니다.\n");
		ExitProcess(1);
	}

	if (!checkBeingDebugged()) {
		printf("[CLIENT] Launcher.exe에 의해 디버깅 당하고 있지 않습니다. 프로그램을 종료합니다.\n");
		ExitProcess(1);
	}

	hThread = CreateThread(0, 0, debugger_watchdog, 0, 0, 0);

	printf("[CLIENT] Launcher.exe.에 의해 정상 실행되었습니다.\n");
	// 1. 중요한 기능
	char strbuffer[256];
	while (1) {
		printf("[CLIENT] 문자열을 입력하시오\n");
		scanf_s("%s", strbuffer);
		if (strcmp(strbuffer, "exit") == 0) {
			printf("[CLIENT] 프로그램을 종료합니다.\n");
			break;
		}
		printf("[CLIENT] %s\n", strbuffer);
	}

	is_running = 0;
	WaitForSingleObject(hThread, 1000);
	CloseHandle(hThread);

	return 0;
}
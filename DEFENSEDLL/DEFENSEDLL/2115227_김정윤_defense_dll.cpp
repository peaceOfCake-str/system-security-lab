// IT공학전공 김정윤(2115227)
// Final Term Project - 2115227_김정윤_defense_dll.cpp
// 2025.12.21
// Subclassing으로 R1, R2를 탐지함

#include "pch.h"
#include <Windows.h>
#include <stdio.h>
#include <time.h>

// ======= 전역 변수 =======
HWND hMainWnd = NULL;
HWND hRegWnd = NULL;
HWND hNameEdit = NULL;
HWND hSerialEdit = NULL;
HWND hOKButton = NULL;

WNDPROC OldMainProc = NULL;
WNDPROC OldRegProc = NULL;
WNDPROC OldNameProc = NULL;
WNDPROC OldSerialProc = NULL;

DWORD dwLastNameChangeTime = 0;
DWORD dwLastNameReadTime = 0;

int nEditFound = 0;
// =======================

BOOL CALLBACK EnumChildProc(HWND hWnd, LPARAM lParam) {
	wchar_t className[256];
	wchar_t buttonText[256];

	GetClassNameW(hWnd, className, 256);

	if (_wcsicmp(className, L"Edit") == 0) {
		nEditFound++;

		if (nEditFound == 1) hNameEdit = hWnd;
		else if (nEditFound == 2) hSerialEdit = hWnd;
	}
	else if (_wcsicmp(className, L"Button") == 0) {
		GetWindowTextW(hWnd, buttonText, 256);
		if (_wcsicmp(buttonText, L"OK") == 0) hOKButton = hWnd;
	}
	return TRUE;
}

void printTimestamp(const wchar_t* msg) {
	time_t timestamp = time(NULL);
	struct tm t;
	localtime_s(&t, &timestamp);

	wprintf(L"[%04d - %02d - %02d %02d:%02d:%02d] %s\n", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, msg);
}

LRESULT CALLBACK MyMainProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	return CallWindowProcW(OldMainProc, hWnd, uMsg, wParam, lParam);
}

// Register - OK 버튼
LRESULT CALLBACK MyRegProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if(uMsg == WM_COMMAND){
		// 메시지를 보낸 컨트롤의 핸들
		HWND hSender = (HWND)lParam;
		int code = HIWORD(wParam);

		if (hSender == hOKButton && code == BN_CLICKED) {
			DWORD currentTime = GetTickCount();
			DWORD dwTimeDiff = currentTime - dwLastNameReadTime;
			
			if (dwTimeDiff < 500) {
				printTimestamp(L"[R2 DETECT]");
				//return 0;
			}
		}
	}
	return CallWindowProcW(OldRegProc, hWnd, uMsg, wParam, lParam);
}

// Register - Name 칸
LRESULT CALLBACK MyNameProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (uMsg == WM_CHAR || uMsg == WM_SETTEXT || uMsg == EM_REPLACESEL) {
		dwLastNameChangeTime = GetTickCount();
	}
	if (uMsg == WM_GETTEXT || uMsg == WM_GETTEXTLENGTH) {
		dwLastNameReadTime = GetTickCount();
	}
	return CallWindowProcW(OldNameProc, hWnd, uMsg, wParam, lParam);
}

// Register - Serial 칸	
LRESULT CALLBACK MySerialProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (uMsg == WM_SETTEXT || uMsg == EM_REPLACESEL) {
		LPCWSTR pStr = (LPCWSTR)lParam;
		BOOL hasContent = (pStr && wcslen(pStr) > 0);

		if (hasContent) {
			HWND hFocus = GetFocus();
			DWORD dwTimeDiff = GetTickCount() - dwLastNameChangeTime;

			wchar_t Buf[512];

			if (hFocus != hWnd && dwTimeDiff < 500) {
				printTimestamp(L"[R1_DETECT] Serial Auto Input, BLOCKED");
				return 1;
			}
			else if (hFocus != hWnd) {

			}
		}
	}
	return CallWindowProcW(OldSerialProc, hWnd, uMsg, wParam, lParam);
}


DWORD WINAPI InitDefense(LPVOID lpParam) {
	printTimestamp(L"[InitDefense] Looking for Main Window(CrackMe v1.0)");
	while (hMainWnd == NULL) {
		hMainWnd = FindWindowW(NULL, L"CrackMe v1.0");
		Sleep(100);
	}

	OldMainProc = (WNDPROC)SetWindowLongPtrW(hMainWnd, GWLP_WNDPROC, (LONG_PTR)MyMainProc);

	printTimestamp(L"[InitDefense] Looking for Register Window");
	while (true) {
		if (hRegWnd == NULL || !IsWindow(hRegWnd)) {
			hRegWnd = NULL;
			hNameEdit = NULL;
			hSerialEdit = NULL;
			nEditFound = 0;

			HWND hFound = FindWindowW(NULL, L"Register");

			if (hFound) {
				hRegWnd = hFound;
				
				EnumChildWindows(hRegWnd, EnumChildProc, 0);

				if (hNameEdit && hSerialEdit && hOKButton) {
					OldRegProc = (WNDPROC)SetWindowLongPtrW(hRegWnd, GWLP_WNDPROC, (LONG_PTR)MyRegProc);
					OldNameProc = (WNDPROC)SetWindowLongPtrW(hNameEdit, GWLP_WNDPROC, (LONG_PTR)MyNameProc);
					OldSerialProc = (WNDPROC)SetWindowLongPtrW(hSerialEdit, GWLP_WNDPROC, (LONG_PTR)MySerialProc);
					printTimestamp(L"[InitDefense] Subclassing Started.");
				}
				else {
					hRegWnd = NULL;
				}
			}
		}
		else {
			if (!IsWindow(hRegWnd)) {
				printTimestamp(L"[InitDefense] Register Closed");
				hRegWnd = NULL;
			}
		}
		Sleep(200);
	}
	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hModule);
		AllocConsole();
		FILE* fp;
		freopen_s(&fp, "CONOUT$", "w", stdout);
		CreateThread(NULL, 0, InitDefense, NULL, 0, NULL);
		break;
	case DLL_PROCESS_DETACH:
		if (OldMainProc && IsWindow(hMainWnd)) SetWindowLongPtrW(hMainWnd, GWLP_WNDPROC, (LONG_PTR)OldMainProc);
		if (OldRegProc && IsWindow(hRegWnd)) SetWindowLongPtrW(hRegWnd, GWLP_WNDPROC, (LONG_PTR)OldRegProc);
		if (OldNameProc && IsWindow(hNameEdit)) SetWindowLongPtrW(hNameEdit, GWLP_WNDPROC, (LONG_PTR)OldNameProc);
		if (OldSerialProc && IsWindow(hSerialEdit)) SetWindowLongPtrW(hSerialEdit, GWLP_WNDPROC, (LONG_PTR)OldSerialProc);
		break;
	}
	return TRUE;
}
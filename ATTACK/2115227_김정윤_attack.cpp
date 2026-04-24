// IT공학전공 김정윤(2115227)
// Final Term Project - 2115227_김정윤_attack.cpp
// 2025.12.10

#include <windows.h>
#include <stdio.h>
#include <TlHelp32.h>
#include <string.h>

// DLL 이름 하드코딩, 같은 폴더 내에 있어야 함
const wchar_t* MY_DLL = L"ATTACKDLL.dll";

DWORD FindProcessID(const wchar_t* processName) {
    DWORD pid = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hSnapshot == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32);

    if (Process32FirstW(hSnapshot, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, processName) == 0) {
                pid = pe.th32ProcessID;
                break;
            }
        } while (Process32NextW(hSnapshot, &pe));
    }
    CloseHandle(hSnapshot);
    return pid;
}

BOOL InjectDLL(DWORD pid, LPCWSTR dllPath) {

    HANDLE hProcess = NULL;
    hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (hProcess == NULL) {
        wprintf(L"[InjectDLL] Process NOT Found.\n");
        return FALSE;
    }

    size_t pathLen = wcslen(dllPath) + 1;
    size_t pathSize = pathLen * sizeof(wchar_t);

    LPVOID lpAddr = VirtualAllocEx(hProcess, NULL, pathSize, MEM_COMMIT, PAGE_READWRITE);
    if (lpAddr == NULL) {
        wprintf(L"[InjectDLL] VirtualAllocEx Failed.\n");
        CloseHandle(hProcess);
        return FALSE;
    }
    
    if (lpAddr) {
        WriteProcessMemory(hProcess, lpAddr, dllPath, pathSize, NULL);
    }
    else {
        wprintf(L"[InjectDLL] WriteProcessMemory Failed.\n");
        VirtualFreeEx(hProcess, lpAddr, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return FALSE;
    }

    LPTHREAD_START_ROUTINE pfnLoadLibraryW = (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryW");

    if (pfnLoadLibraryW) {
        HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, pfnLoadLibraryW, lpAddr, 0, NULL);
        DWORD dwExitCode = NULL;

        if (hThread) {
            wprintf(L"[InjectDLL] Injection SUCCESSFUL!\n");
            WaitForSingleObject(hThread, INFINITE);			// 스레드 종료를 무한히 기다림 -> 타겟 프로세스에서 LoadLibraryA 종료할 때까지 Blocking
            if (GetExitCodeThread(hThread, &dwExitCode))	wprintf(L"Injected DLL ImageBase: %#x\n", dwExitCode);
            CloseHandle(hThread);
        }
        else {
            wprintf(L"[InjectDLL] Injection FAILURE.\n");
            VirtualFreeEx(hProcess, lpAddr, 0, MEM_RELEASE);
            CloseHandle(hProcess);
            return FALSE;
        }
    }

    VirtualFreeEx(hProcess, lpAddr, 0, MEM_RELEASE);
    return TRUE;
}



int main()
{
    const wchar_t* targetName = L"CRACKME.exe";
    wchar_t dllPath[MAX_PATH] = { 0 };

    
    DWORD pid = FindProcessID(targetName);
    if (pid == 0) {
        wprintf(L"[main] CRACKME.exe 프로세스 찾기 실패\n");
        return 1;
    }

    wprintf(L"[main] CRACKME.exe 프로세스를 찾았습니다. pid: %d\n", pid);

    GetFullPathNameW(MY_DLL, MAX_PATH, dllPath, NULL);
    wprintf(L"[main] DLL Path: %s\n", dllPath);

    if (GetFileAttributesW(dllPath) == 0xffffffff) {
        printf("[main] DLL NOT FOUND.\n");
        return 1;
    }

    wprintf(L"[main] DLL 인젝션 path: %s\n", dllPath);

    if (InjectDLL(pid, dllPath)) {
        printf("[main] DLL 인젝션 성공\n");
    }
    else {
        printf("[main] DLL 인젝션 실패\n");
    }

    return 0;
}

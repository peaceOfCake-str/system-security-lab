// IT공학전공 김정윤(2115227)
// Final Term Project - 2115227_김정윤_defense.cpp
// 2025.12.21
// CRACKME.exe에 DEFENSEDLL.dll을 주입하고 R3를 탐지 및 방지함

#include <windows.h>
#include <stdio.h>
#include <TlHelp32.h>
#include <string.h>
#include <time.h>
#include <locale.h>

// DLL 이름 하드코딩, 같은 폴더 내에 있어야 함
const wchar_t* MY_DLL = L"DEFENSEDLL.dll";
const wchar_t* targetName = L"CRACKME.exe";

LPVOID BaseAddress = NULL;
BYTE* OriginalCode = NULL;
SIZE_T CodeSize = 0;
LPVOID TextBaseAddr = NULL;

// 타임스탬프 
void printTimestamp(const wchar_t* msg) {
    time_t timestamp = time(NULL);
    struct tm t;
    localtime_s(&t, &timestamp);

    wprintf(L"[%04d - %02d - %02d %02d:%02d:%02d] %s\n", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, msg);
}

// CRACKME PID 찾기
DWORD FindProcessID(const wchar_t* processName) {
    DWORD pid = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hSnapshot == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);

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

// CRACKME 모듈 베이스 주소 찾기
LPVOID GetModuleBaseAddress(DWORD pid, const wchar_t* modName) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return NULL;
    }

    MODULEENTRY32W me;
    me.dwSize = sizeof(MODULEENTRY32W);

    if (!Module32FirstW(hSnapshot, &me)) {
        CloseHandle(hSnapshot);
        return NULL;
    }
    
        do {
            if (_wcsicmp(me.szModule, modName) == 0) {
                CloseHandle(hSnapshot);
                return me.modBaseAddr;
            } 
        } while (Module32NextW(hSnapshot, &me));
    
    CloseHandle(hSnapshot);
    return NULL;
}

// PE 파일 구조로 Executable Section 찾기
BOOL GetExecutableSection(HANDLE hProcess, BYTE* imageBase, LPVOID* outTextBase, SIZE_T* outTextSize) {
    *outTextBase = NULL;
    *outTextSize = 0;

    IMAGE_DOS_HEADER dos = { 0 };
    SIZE_T br = 0;

    if (!ReadProcessMemory(hProcess, imageBase, &dos, sizeof(dos), &br) || br != sizeof(dos)) {
        return FALSE;
    }
    if (dos.e_magic != IMAGE_DOS_SIGNATURE) {
        return FALSE;
    }

    BYTE* ntBase = imageBase + dos.e_lfanew;

    DWORD sig = 0;
    br = 0;
    if (!ReadProcessMemory(hProcess, ntBase, &sig, sizeof(sig), &br) || br != sizeof(sig)) {
        return FALSE;
    }
    if (sig != IMAGE_NT_SIGNATURE) {
        return FALSE;
    }

    IMAGE_FILE_HEADER fh = { 0 };
    br = 0;
    if (!ReadProcessMemory(hProcess, ntBase + sizeof(DWORD), &fh, sizeof(fh), &br) || br != sizeof(fh)) {
        return FALSE;
    }

    WORD optMagic = 0;
    br = 0;
    BYTE* optBase = ntBase + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER);
    if (!ReadProcessMemory(hProcess, optBase, &optMagic, sizeof(optMagic), &br) || br != sizeof(optMagic)) {
        return FALSE;
    }


    // 섹션 헤더 시작 주소
    BYTE* secBase = ntBase + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) + fh.SizeOfOptionalHeader;

    for (WORD i = 0; i < fh.NumberOfSections; i++) {
        IMAGE_SECTION_HEADER sh = { 0 };
        br = 0;

        if (!ReadProcessMemory(hProcess, secBase + i * sizeof(sh), &sh, sizeof(sh), &br) || br != sizeof(sh)) {
            return FALSE;
        }

        char name[9] = { 0 };
        memcpy(name, sh.Name, 8);


        if (sh.Characteristics & IMAGE_SCN_MEM_EXECUTE) {
            *outTextBase = imageBase + sh.VirtualAddress;
            *outTextSize = (SIZE_T)sh.Misc.VirtualSize;

            return TRUE;
        }
    }
    return FALSE;
}

// DEFENSEDLL.dll inject
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
            if (GetExitCodeThread(hThread, &dwExitCode))	wprintf(L"[InjectDLL] Injected DLL ImageBase: %#x\n", dwExitCode);
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

// R3 탐지 + 방어(Code Integrity)
void detectR3(HANDLE hProcess) {
    static BOOL isDetected = FALSE;
    LPVOID targetAddr = TextBaseAddr;

    if (OriginalCode == NULL) {
        OriginalCode = (BYTE*)malloc(CodeSize);
        if (!OriginalCode) return;

        SIZE_T bytesRead = 0;
        if (ReadProcessMemory(hProcess, targetAddr, OriginalCode, CodeSize, &bytesRead) && bytesRead == CodeSize) {
            wprintf(L"[detectR3 | DEBUG] 원본 코드 스냅샷 확보 완료. R3 감시 시작\n");
        }
        else {
            wprintf(L"[detectR3 | DEBUG] 원본 코드 스냅샷 확보 실패. %lu\n", GetLastError());
            free(OriginalCode);
            OriginalCode = NULL;
        }
        return;
    }

    BYTE* currentCode = (BYTE*)malloc(CodeSize);
    if (!currentCode) return;

    SIZE_T bytesRead = 0;
    if (!ReadProcessMemory(hProcess, targetAddr, currentCode, CodeSize, &bytesRead) || bytesRead != CodeSize) {
        return;
    }

    if (memcmp(OriginalCode, currentCode, CodeSize) != 0) {
        if (!isDetected) {
            isDetected = TRUE;
            printTimestamp(L"[detectR3 | R3 DETECT] 프로세스 패치 감지\n");
            wprintf(L"[detectR3 | R3 DEFNESE] 원본 코드로 자동 복구합니다.\n");
        }

        DWORD oldProtection = 0;

        if (VirtualProtectEx(hProcess, targetAddr, CodeSize, PAGE_EXECUTE_READWRITE, &oldProtection)) {
            SIZE_T bytesWritten = 0;
            WriteProcessMemory(hProcess, targetAddr, OriginalCode, CodeSize, &bytesWritten);

            DWORD oldProtection2 = 0;
            VirtualProtectEx(hProcess, targetAddr, CodeSize, oldProtection, &oldProtection2);
            printTimestamp(L"[detectR3 | R3 DEFENSE] 복구 완료\n");

        }
        else {
            wprintf(L"[detectR3 | R3 DEFENSE] 복구 실패 %lu", GetLastError());
        }
        
    } 
    else {
        if (isDetected) {
            isDetected = FALSE;
        } 
    }
    free(currentCode);
}


static BOOL IsExecuteProtect(DWORD p) {
    p &= 0xFF; 
    return (p == PAGE_EXECUTE ||
        p == PAGE_EXECUTE_READ ||
        p == PAGE_EXECUTE_READWRITE ||
        p == PAGE_EXECUTE_WRITECOPY);
}

// Memory Protection, VirtualQueryEx를 활용한 Code Injection 탐지
void DetectCodeInjection32(HANDLE hProcess) {
    MEMORY_BASIC_INFORMATION mbi;
    BYTE* address = 0;

    static BOOL detectInjection = FALSE;

    while (VirtualQueryEx(hProcess, address, &mbi, sizeof(mbi)) == sizeof(mbi)) {

        BYTE* next = (BYTE*)mbi.BaseAddress + (DWORD)mbi.RegionSize;
        if (next <= address) break;
        address = next;

        if (mbi.State != MEM_COMMIT) continue;
        if (mbi.Protect == PAGE_NOACCESS) continue;
        if (mbi.Protect & PAGE_GUARD) continue;

        // MEM_PRIVATE + EXECUTE
        if (mbi.Type == MEM_PRIVATE && IsExecuteProtect(mbi.Protect)) {

            wchar_t buf[256];
            if (detectInjection == FALSE) {
                swprintf_s(buf,
                    L"[DETECT] 코드 인젝션 의심: MEM_PRIVATE+EXEC Base=%p Size=%lu Protect=%#lx",
                    mbi.BaseAddress, (DWORD)mbi.RegionSize, (DWORD)mbi.Protect);
                printTimestamp(buf);
                detectInjection = TRUE;
            }
        }
    }
}


int main()
{
    setlocale(LC_ALL, "Korean");
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
        wprintf(L"[main] DLL 인젝션 성공\n");
    }
    else {
        wprintf(L"[main] DLL 인젝션 실패\n");
    }

    Sleep(500);

    BaseAddress = GetModuleBaseAddress(pid, targetName);
    if (!BaseAddress) {
        wprintf(L"[main] BaseAddress 획득 실패\n");
        getchar();
        return 1;
    }

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess) return 1;

    if (!GetExecutableSection(hProcess, (BYTE*)BaseAddress, &TextBaseAddr, &CodeSize)) {
        getchar();
        return 1;
    }


    while (pid) {
        detectR3(hProcess);
        DetectCodeInjection32(hProcess);
        Sleep(100);
    }

    if (OriginalCode) free(OriginalCode);
    CloseHandle(hProcess);
    getchar();

    return 0;
}


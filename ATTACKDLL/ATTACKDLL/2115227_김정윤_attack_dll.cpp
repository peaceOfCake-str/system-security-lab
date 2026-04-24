// IT공학전공 김정윤(2115227)
// Final Term Project - 2115227_김정윤_attack_dll.cpp
// 2025.12.10

// dllmain.cpp : DLL 애플리케이션의 진입점을 정의합니다.
#include "pch.h" 


#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <shlobj.h>
#include <locale.h>
#pragma comment(lib, "Shell32.lib")


// ============= 전역 변수 =============

typedef UINT(WINAPI* PGETDLGITEMTEXTA)(HWND, int, LPSTR, int);
PGETDLGITEMTEXTA pOriginalGetDlgItemTextA = NULL;

HINSTANCE g_hInstance = NULL;
HHOOK g_hHook = NULL;


BOOL r2enable = TRUE;

int NameID = 0;
int SerialID = 0;

// ==================================

// PEB Unlinking
typedef struct PEB_LDR_DATA {
    ULONG Length;
    BOOLEAN Initialized;
    PVOID SsHandle;
    LIST_ENTRY InLoadOrderModuleList;
    LIST_ENTRY InMemoryOrderModuleList;
    LIST_ENTRY InInitializationOrderModuleList;
} PEB_LDR_DATA, *PPEB_LDR_DATA;
typedef struct PEB {
    BOOLEAN InheritedAddressSpace;
    BOOLEAN ReadImageFileExecOptions;
    BOOLEAN BeingDebugged;
    BOOLEAN SpareBool;
    HANDLE Mutant;
    PVOID ImageBaseAddress;
    PPEB_LDR_DATA Ldr;
} PEB, *PPEB;
typedef struct LDR_DATA_TABLE_ENTRY{
    LIST_ENTRY InLoadOrderLinks;
    LIST_ENTRY InMemoryOrderLinks;
    LIST_ENTRY InInitializationOrderLinks;
    PVOID DllBase;
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

void hideDLL(HINSTANCE hModule) {
#ifdef _WIN64
    PPEB pPEB = (PPEB)__readgsqword(0x60);
#else
    PPEB pPEB = (PPEB)__readfsdword(0x30);
#endif
    PPEB_LDR_DATA pLdr = pPEB->Ldr;
    PLIST_ENTRY pListHead = &pLdr->InLoadOrderModuleList;
    PLIST_ENTRY pListEntry = pListHead->Flink;

    while (pListEntry != pListHead) {
        PLDR_DATA_TABLE_ENTRY pEntry = (PLDR_DATA_TABLE_ENTRY)pListEntry;

        if (pEntry->DllBase == hModule) {
            //printf("[Stealth] FOUND my DLL at %p! Unlinking now...\n", hModule);
            pEntry->InLoadOrderLinks.Blink->Flink = pEntry->InLoadOrderLinks.Flink;
            pEntry->InLoadOrderLinks.Flink->Blink = pEntry->InLoadOrderLinks.Blink;
            pEntry->InMemoryOrderLinks.Blink->Flink = pEntry->InMemoryOrderLinks.Flink;
            pEntry->InMemoryOrderLinks.Flink->Blink = pEntry->InMemoryOrderLinks.Blink;
            pEntry->InInitializationOrderLinks.Blink->Flink = pEntry->InInitializationOrderLinks.Flink;
            pEntry->InInitializationOrderLinks.Flink->Blink = pEntry->InInitializationOrderLinks.Blink;
            //printf("[Stealth] SUCCESS! DLL Removed from PEB Lists.\n");
            break;
        }
        pListEntry = pListEntry->Flink;
    }
}


void openConsole() {
    AllocConsole();
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    setlocale(LC_ALL, "Korean");
    printf("ATTACK.exe working | Press N, if you want to turn spy mode off.\n");
    printf("ATTACK.exe working | Press F12, if you want to exit process.\n");
}


void SaveText(const char* text) {
    char path[MAX_PATH];
    HRESULT hr = SHGetFolderPathA(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, path);

    if (SUCCEEDED(hr)) {
        strcat_s(path, MAX_PATH, "\\names.txt");

        HANDLE hFile = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile) {
            SetFilePointer(hFile, 0, NULL, FILE_END);

            char buffer[512];
            sprintf_s(buffer, "%s\n", text);

            DWORD dwBytesWritten;
            WriteFile(hFile, buffer, (DWORD)strlen(buffer), &dwBytesWritten, NULL);

            CloseHandle(hFile);
        }
    }
    //else OutputDebugStringA("[SaveText] Failed to save Text\n");
}


UINT WINAPI FakeGetDlgItemTextA(HWND hDlg, int nIDDlgItem, LPSTR lpString, int cchMax) {
    UINT result = pOriginalGetDlgItemTextA(hDlg, nIDDlgItem, lpString, cchMax);

    if (r2enable && result > 0 && nIDDlgItem == NameID) {
        //char debugMsg[256];
        //sprintf_s(debugMsg, sizeof(debugMsg), "ID: %d, Hooked Name: %s\n", nIDDlgItem, lpString);
        //OutputDebugStringA(debugMsg);
        
        SaveText(lpString);
    }
    else GetLastError();

    return result;
}

void PatchIAT(LPDWORD lpAddress, DWORD func) {
    DWORD flOldProtect, flOldProtect2;

    // VirtualProtect(): 파일 Protection(Permission) 변경하는 WinAPI
    VirtualProtect((LPVOID)lpAddress, sizeof(DWORD), PAGE_EXECUTE_READWRITE, &flOldProtect);
    if (pOriginalGetDlgItemTextA == NULL) {
        pOriginalGetDlgItemTextA = (PGETDLGITEMTEXTA)(*lpAddress);
    }
    *lpAddress = func;  // FakeGetDlgItemTextA의 시작 지점을 IAT의 (실제)GetDlgItemTextA에 overwrite
    VirtualProtect((LPVOID)lpAddress, sizeof(DWORD), flOldProtect, &flOldProtect2);
}

char buf[4096];
LPVOID FindTargetVA(LPCSTR lpTargetDllName, LPCSTR lpTargetFuncName) {
    HMODULE hModule = GetModuleHandleA(NULL);
    LPBYTE lpFileBase = (LPBYTE)hModule;

    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)lpFileBase;
    PIMAGE_NT_HEADERS pNtHeader = (PIMAGE_NT_HEADERS)(lpFileBase + pDosHeader->e_lfanew);
    DWORD offset = pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;    // IAT RVA
    PIMAGE_IMPORT_DESCRIPTOR pImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)(lpFileBase + offset);  // VA = ImageBse + RVA


    // user32.dll(target Dll) 찾기
    DWORD i = 0, dllIdx = 0xFFFFFFFF, funcIdx = 0xFFFFFFFF;
    while (pImportDescriptor[i].Name != 0) {
        LPCSTR lpDllName = (LPCSTR)(lpFileBase + pImportDescriptor[i].Name);
        //sprintf_s(buf, "ImportDescriptor[%d].Name = %s\n", i, lpDllName);
        //OutputDebugStringA(buf);

        // 내가 찾고자 하는 DLL을 발견하면 루프 종료
        if (_stricmp(lpDllName, lpTargetDllName) == 0) {
            dllIdx = i;
            break;
        }
        i++;
    }

    // writefile(target function) 찾기
    LPDWORD lpRvaFuncName = (LPDWORD)(lpFileBase + pImportDescriptor[dllIdx].Characteristics);      // Characteristics: 문자열 위치가 담긴 RVA
    i = 0;

    while (lpRvaFuncName[i] != NULL) {
        LPCSTR lpFuncName = (LPCSTR)lpFileBase + lpRvaFuncName[i] + 2;  // va
        if (strcmp(lpFuncName, lpTargetFuncName) == 0) {
            funcIdx = i;
            break;
        }
        i++;
    }

    LPVOID lpFuncPtr = (LPVOID)(((LPDWORD)(lpFileBase + pImportDescriptor[dllIdx].FirstThunk)) + funcIdx);      // FirstThunk: 함수 포인터가 담긴 RVA
    sprintf_s(buf, "Successfully identified %s! %s() at %#x\n", lpTargetDllName, lpTargetFuncName, (DWORD)lpFuncPtr);
    OutputDebugStringA(buf);

    return lpFuncPtr;
}

// IAT 주소에 overwrite
BOOL apiHook() {
    LPVOID lpTarget = FindTargetVA("user32.dll", "GetDlgItemTextA");
    PatchIAT((LPDWORD)lpTarget, (DWORD)FakeGetDlgItemTextA);      // 함수 이름이 포인터 -> casting하면 시작 주소 얻을 수 있음
    return TRUE;
}

// 핵심 로직: 시리얼 생성
void GenerateSerial(HWND hDlg){
    char name[100] = { 0 };
    char serial[100] = { 0 };
    char dbg[256];

    //sprintf_s(dbg, "[R1] GenerateSerial: hDlg=%p\n", hDlg);
    //OutputDebugStringA(dbg);

    typedef UINT(WINAPI* PGETDLGITEMTEXTA)(HWND, int, LPSTR, int);
    static PGETDLGITEMTEXTA pmygetText = NULL;
    if (!pmygetText) {
        HMODULE hUser32 = GetModuleHandleA("user32.dll");
        pmygetText = (PGETDLGITEMTEXTA)GetProcAddress(hUser32, "GetDlgItemTextA");
    }

    if (!pmygetText) {
        //OutputDebugStringA("[R1] GenerateSerial: pmygetText is NULL\n");
        return;
    }

    UINT len = pmygetText(hDlg, NameID, name, 100);
    //sprintf_s(dbg, "[R1] GetDlgItemTextA returned len=%u, name=\"%s\"\n", len, name);
    //OutputDebugStringA(dbg);

    if(len == 0) {
        //OutputDebugStringA("[R1] Name is empty. Skip.\n");
        return;
    }

    // 이름 없으면 시리얼 생성 x
    //sprintf_s(dbg, "[R1] Generating Serial for Name: %s\n", name);
    //OutputDebugStringA(dbg);

    unsigned int sum = 0;
    for (int i = 0; name[i] != '\0'; i++) {
        if (i > 9) break;
        unsigned char c = (unsigned char) name[i];

        // 소문자라면 대문자로 변환 (ASCII - 32)
        // upper()
        if (c >= 'a' && c <= 'z') c -= 32;
        sum += c;
    }

    unsigned int serialResult = sum ^ 0x444C;

    sprintf_s(serial, "%u", serialResult);
    //sprintf_s(dbg, "[R1] Calculated Serial: %s\n", serial);
    //OutputDebugStringA(dbg);
    
    // 숫자를 문자열로 변환
    sprintf_s(serial, "%d", serialResult);

    // Serial 업데이트
    char currentSerial[100] = { 0 };
    if(pOriginalGetDlgItemTextA) pOriginalGetDlgItemTextA(hDlg, SerialID, currentSerial, 100);

    //sprintf_s(dbg, "[R1] Current Serial Edit: \"%s\"\n", currentSerial);
    //OutputDebugStringA(dbg);

    if (strcmp(serial, currentSerial) != 0) SetDlgItemTextA(hDlg, 1001, serial);
}


BOOL CALLBACK FindSerialID(HWND hwnd, LPARAM lParam) {
    char className[64] = { 0 };
    GetClassNameA(hwnd, className, sizeof(className));

    if (_stricmp(className, "Edit") == 0) {
        int id = GetDlgCtrlID(hwnd);

        if (id != 0 && id != NameID) {
            SerialID = id;
            return FALSE;
        }
    }
    return TRUE;
}

// 훅 프로시저
// 윈도우 메시지 WM_COMMAND 감시
LRESULT CALLBACK MsgProc(int nCode, WPARAM wParam, LPARAM lParam) {
    SHORT spyOn = GetAsyncKeyState('Y');
    SHORT spyOff = GetAsyncKeyState('N');
    BOOL shiftDown = GetAsyncKeyState(VK_SHIFT);
    BOOL f12Down = GetAsyncKeyState(VK_F12);

    if (nCode >= 0) {
        // CWPSTRUCT 구조체: lParam에 메시지 정보가 들어있음
        if (f12Down) {
            printf("F12 is Pressed: Exit Process\n");
            Sleep(1000);
            ExitProcess(0);
        }
        else if ((spyOn & 0x0001) && shiftDown) {
            r2enable = TRUE;
            OutputDebugStringA("Spy Mode 켜짐\n");
            printf("Y is Pressed: Spy Mode On | Press N, if you want to turn spy mode off.\n");
        }
        else if ((spyOff & 0x0001) && shiftDown) {
            r2enable = FALSE;
            OutputDebugStringA("Spy Mode 꺼짐\n");
            printf("N is Pressed: Spy Mode Off | Press Y, if you want to turn spy mode on.\n");
        }

        CWPSTRUCT* pMsg = (CWPSTRUCT*)lParam;

        // 감시할 메시지: WM_COMMAND
        if (pMsg->message == WM_COMMAND) {
            // 누가 보냈는지 (control ID)
            int ctrlID = LOWORD(pMsg->wParam);

            // 무슨 일이 생겼는지 (notification code)
            int notifyCode = HIWORD(pMsg->wParam);
            HWND hDlg = pMsg->hwnd;
            if ((notifyCode == EN_CHANGE || notifyCode == EN_UPDATE) && NameID == 0 && ctrlID != 0) {
                NameID = ctrlID;
                char dbg[128];
                //sprintf_s(dbg, "NameID = %d\n", NameID);
                //OutputDebugStringA(dbg);

                EnumChildWindows(hDlg, FindSerialID, 0);
            }
            // name 입력창의 내용이 변경(EN_Change)되었는가?
            if (NameID != 0 && SerialID != 0 && ctrlID == NameID && notifyCode == EN_CHANGE || notifyCode == EN_UPDATE) GenerateSerial(pMsg->hwnd);
        }
    }
    return CallNextHookEx(g_hHook, nCode, wParam, lParam);
}

void HookStart(DWORD tid) {
    if (g_hHook) return;
  
    g_hHook = SetWindowsHookExA(WH_CALLWNDPROC, MsgProc, g_hInstance, tid);

    if (!g_hHook) {
        sprintf_s(buf, "[R1] Hook Failed: %d\n", GetLastError());
        OutputDebugStringA(buf);
    }
    else {
        //OutputDebugStringA("[R1] Hook Installed Successfully!\n");
    }
}

void HookStop(){
    if (g_hHook) {
        UnhookWindowsHookEx(g_hHook);
        g_hHook = NULL;
        //OutputDebugStringA("[R1] HookStop: Hook Removed.\n");
    }
}

DWORD WINAPI HookThread(LPVOID) {
    HWND hMain = NULL;
    while ((hMain = FindWindowA(NULL, "CrackMe v1.0")) == NULL) {
        Sleep(200);
    }

  
    DWORD tid = GetWindowThreadProcessId(hMain, NULL);
  

    apiHook();
    HookStart(tid);

    while (true) Sleep(1000);

    return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        g_hInstance = hModule;

        // DLLMain이 여러 번 호출되는 것을 막음
        DisableThreadLibraryCalls(hModule);
        openConsole();
        
        
        CreateThread(NULL, 0, HookThread, NULL, 0, NULL);

        hideDLL(hModule);

        //apihook();
        //HookStart();
        break;
    //case DLL_THREAD_ATTACH:
    //case DLL_THREAD_DETACH:
        //break;
    case DLL_PROCESS_DETACH:
        HookStop();
        break;
    }
    return TRUE;
}


/// 2025.10.06
/// System Security Assignment 4 - PE Parser
/// IT공학전공 김정윤(2115227)


#include <Windows.h>
#include <stdio.h>


// RVA를 RAW로 변환하는 함수
DWORD RVAtoRAW(DWORD fRVA, DWORD fVA, DWORD fPointerToRawData) {
	// RAW = RVA - VA + PointerToRawData
	DWORD RAW = fRVA - fVA + fPointerToRawData;
	//printf("%#x = %#x - %#x + %#x", RAW, fRVA, fVA, fPointerToRawData);
	return RAW;
}


int main(void) {
	char path_pefile[] = "C:\\Users\\김정윤\\Documents\\abex_crackme1.exe";
	//char path_pefile[] = "C:\\Users\\김정윤\\source\\repos\\DLLStudy_SystemSecrity\\Release\\EXE.exe";


	HANDLE hFile = NULL, hFileMap = NULL;
	LPBYTE lpFileBase = NULL;
	DWORD dwSize = 0;

	PIMAGE_DOS_HEADER pDosHeader = NULL;
	PIMAGE_NT_HEADERS pNtHeader = NULL;
	PIMAGE_FILE_HEADER pFileHeader = NULL;
	PIMAGE_IMPORT_DESCRIPTOR pImportDescriptor = NULL;
	PIMAGE_THUNK_DATA32 pThunkData = NULL;
	PIMAGE_IMPORT_BY_NAME pImportByName = NULL;

	hFile = CreateFileA(path_pefile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE) {
		printf("CreateFileA() failed. Error Code = %lu\n", GetLastError());
		return GetLastError();
	}


	/* 파일 사이즈 */
	dwSize = GetFileSize(hFile, 0);
	printf("File Size = %lu bytes\n\n", dwSize);


	hFileMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	lpFileBase = (LPBYTE)MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, dwSize);

	printf("File Signature = %c%c\n", lpFileBase[0], lpFileBase[1]);

	pDosHeader = (PIMAGE_DOS_HEADER)lpFileBase;
	printf("Offset to the NT header = %#x\n\n\n", pDosHeader->e_lfanew);


	/* NT HEADER */
	pNtHeader = (PIMAGE_NT_HEADERS)(lpFileBase + pDosHeader->e_lfanew);
	printf("-----------------------OPTIONAL HEADER-----------------------\n");
	printf("OptionalHeader.BaseOfCode = %#x\n", pNtHeader->OptionalHeader.BaseOfCode);
	printf("OptionalHeader.SizeOfCode = %#x\n", pNtHeader->OptionalHeader.SizeOfCode);
	printf("OptionalHeader.AddressOfEntryPoint = %#x\n", pNtHeader->OptionalHeader.AddressOfEntryPoint);
	printf("OptionalHeader.BaseOfData = %#x\n", pNtHeader->OptionalHeader.BaseOfData);
	printf("OptionalHeader.ImageBase = %#x\n\n\n", pNtHeader->OptionalHeader.ImageBase);



	/* SECTION HEADER*/
	printf("-----------------------SECTION INFORMATION-----------------------\n\n");
	PIMAGE_SECTION_HEADER pSectionHeader = NULL;			// Section Header 베이스 가리키는 포인터

	LPBYTE lpOptionalHeaderBase = (LPBYTE)&pNtHeader->OptionalHeader;		// Optional Header 베이스 가리키는 포인터
	pSectionHeader = (PIMAGE_SECTION_HEADER)(lpOptionalHeaderBase + pNtHeader->FileHeader.SizeOfOptionalHeader);	// Optional Header 바로 다음으로 Section Header가 오기 때문에 Optional Base에 File Header에 있는 Optional Header 크기 정보 가져와서 Section Header의 시작 지점을 구함

	for (int i = 1; i <= pNtHeader->FileHeader.NumberOfSections; i++) {
		printf("--------%d번째 section: %s--------\n", i, pSectionHeader->Name);
		printf("PointerToRawData: %#x\n", pSectionHeader->PointerToRawData);
		printf("SizeOfRawData: %#x\n", pSectionHeader->SizeOfRawData);
		printf("VirtualAddress: %#x\n", pSectionHeader->VirtualAddress);
		printf("VirtualSize: %#x\n\n", pSectionHeader->Misc.VirtualSize);

		pSectionHeader++;
	}

	printf("\n\n");
	

	/* IMPORT ADDRESS TABLE */
	printf("-----------------------IAT-----------------------\n");
	DWORD dwIATVA = pNtHeader->OptionalHeader.DataDirectory[1].VirtualAddress;				// IAT VA 
	DWORD dwIATRAW = NULL;

	DWORD VA = NULL;
	DWORD PointerToRawData = NULL;
	BYTE SectionName = NULL;

	pSectionHeader = (PIMAGE_SECTION_HEADER)(lpOptionalHeaderBase + pNtHeader->FileHeader.SizeOfOptionalHeader);	// Section Header 초기화


	for (int i = 0; i < pNtHeader->FileHeader.NumberOfSections; i++) {
		if (dwIATVA >= pSectionHeader[i].VirtualAddress && dwIATVA < pSectionHeader[i + 1].VirtualAddress) {
			PointerToRawData = pSectionHeader[i].PointerToRawData;
			VA = pSectionHeader[i].VirtualAddress;
			//printf("ptr %#x\n", PointerToRawData);		//1400
			//printf("va %#x\n", VA);								//2000

			dwIATRAW = RVAtoRAW(dwIATVA, pSectionHeader[i].VirtualAddress, pSectionHeader[i].PointerToRawData);
			//printf("raw %#x\n", dwIATRAW);
			SectionName = pSectionHeader[i].Name;
			pImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)(lpFileBase + dwIATRAW);

			printf("IAT가 저장된 섹션: %s\n", pSectionHeader[i].Name);
			printf("RVA to RAW: %#x -> %#x\n\n\n", VA, PointerToRawData);

		}
	}



	/* DLL NAME */
	DWORD dllNameRVA = NULL;
	DWORD dllNameRAW = NULL;
	char* dllName = NULL;

	/* Function NAME*/
	DWORD OFTRVA = NULL;			/// Original First Thunk
	DWORD OFTRAW = NULL;
	DWORD FnNameRVA = NULL;
	DWORD FnNameRAW = NULL;

	int i = 0;
	while (pImportDescriptor->Name != 0) {
		dllNameRVA = pImportDescriptor->Name;
		dllNameRAW = RVAtoRAW(dllNameRVA, VA, PointerToRawData);
		dllName = (char*)(lpFileBase + dllNameRAW);

		printf("ImportDescriptor[%d].Name = %s\n", i, dllName);

		OFTRVA = pImportDescriptor->OriginalFirstThunk;		// 0x303c
		OFTRAW = RVAtoRAW(OFTRVA, VA, PointerToRawData);				// 0xa3c

		pThunkData = (PIMAGE_THUNK_DATA32)(lpFileBase + OFTRAW);

		while (pThunkData->u1.AddressOfData != 0) {

			FnNameRVA = pThunkData->u1.AddressOfData;
			FnNameRAW = RVAtoRAW(FnNameRVA, VA, PointerToRawData);

			pImportByName = (PIMAGE_IMPORT_BY_NAME)(lpFileBase + FnNameRAW);
			printf("- function name = %s, (RVA = %#x) \n", pImportByName->Name, FnNameRVA);
			pThunkData++;
		}
		printf("\n\n");
		i++;
		pImportDescriptor++;
	}
	

	/*Windows로부터 할당받은 리소스를 역순으로 반환*/
	UnmapViewOfFile(lpFileBase);
	CloseHandle(hFileMap);
	CloseHandle(hFile);

	/*main() 함수가 끝까지 실행되었음을 알리기 위해 0을 반환*/
	return 0;
}


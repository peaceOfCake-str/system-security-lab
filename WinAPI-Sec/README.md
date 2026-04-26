# System-Security-Lab 🛡️

2025-2학기 시스템 보안 텀 프로젝트 저장소입니다. 
Windows 운영체제 환경에서 `Cruehead CrackMe`를 타겟으로 공격(Red Team) 및 방어(Blue Team) 메커니즘을 구현하고 분석하였습니다.

## 프로젝트 개요
* **기간**: 2025.11.29 ~ 2025.12.21
* **타겟**: Cruehead CrackMe
* **환경**: Windows 11(VMware), Visual Studio 2022, x64dbg

## Attack: Red Team Analysis
공격 프로그램은 대상 프로세스의 취약점을 활용하여 데이터 탈취 및 자동화를 수행합니다.

### 주요 기술
* **DLL Injection**: `CreateToolhelp32Snapshot`을 통한 타겟 PID 획득 및 인젝션.
* **IAT Hooking**: `GetDlgItemTextA`를 후킹하여 사용자 입력값(Name) 실시간 탈취 및 저장.
* **Stealth**: `PEB Unlinking`을 통해 로드된 DLL 리스트에서 자신을 은닉.
* **Message Hooking**: `WH_CALLWNDPROC`를 이용한 UI 이벤트 감시 및 Serial 자동 생성/입력.

### Hooking Flow
`CrackMe.exe` → `[IAT: GetDlgItemTextA]` → `ATTACKDLL.dll!FakeGetDlgItemTextA`
1. 원본 `User32!GetDlgItemTextA` 호출 및 데이터 획득.
2. `SaveText()`를 통해 `%USERPROFILE%\Documents\name.txt`에 기록.
3. 데이터 반환.

## Defense: Blue Team Strategy
단순 API 후킹 탐지의 한계를 극복하기 위해 `Window Subclassing` 기반의 심층 방어를 구현하였습니다.

### 주요 기술
* **Window Subclassing**: `SetWindowLongPtrW`로 메시지 루프를 제어하여 비정상적인 입력 차단.
* **Code Injection Detection**: 메모리 맵 스캔을 통해 `MEM_PRIVATE` 및 `PAGE_EXECUTE_READWRITE` 속성 영역의 비인가 코드 탐지.
* **Self-Healing**: 코드 섹션 원본 스냅샷을 보관하여 `R3(코드 패치)` 공격 발생 시 자동 복구.
* **상관관계 분석(Heuristic)**: 
    * `GetTickCount()`를 이용해 500ms 이내의 비정상적인 고속 입력 탐지.
    * 포커스 없는 상태에서의 데이터 입력 감지.

## 공격 유형 분석 (Defense Logic)
공격 프로그램의 다양한 변주(Attack01~25)를 분석하여 정형화된 탐지 로직을 수립하였습니다.

| 공격 유형 | 특징 | 대응 기술 |
| :--- | :--- | :--- |
| **R1 (자동화)** | 비정상적 입력 속도 | Window Subclassing, TickCount 분석 |
| **R2 (탈취)** | WM_GETTEXT 발생 | OK 버튼 클릭 시점 상관관계 분석 |
| **R3 (패치)** | 프로세스 변조 | Memory Scanning 및 Self-Healing |

## 기술 스택
| 구분 | 기술 | 설명 |
| :--- | :--- | :--- |
| **분석** | x64dbg | 어셈블리 분석 및 Serial 생성 알고리즘 파악 |
| **공격** | C++, WinAPI, Hooking | 프로세스 제어 및 자동화 매커니즘 구현 |
| **방어** | C++, Subclassing | 행위 기반 분석 및 탐지 로직 구현 |

---
*본 프로젝트는 시스템 보안 학습 목적으로 제작되었습니다.*

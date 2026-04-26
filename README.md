# System-Security-Lab

시스템 보안 실습 및 Windows API 기반 공격/방어 기술 연구 저장소입니다.

---

## 프로젝트 목록

### 1. [WinAPI-Sec]
Cruehead CrackMe를 대상으로 Windows API 기반 공격 및 방어 프로그램을 제작한 프로젝트입니다.

* **구조:**
    * `ATTACK` / `ATTACKDLL` : Windows API를 활용한 공격 기법 구현
    * `DEFENSE` / `DEFENSEDLL` : 공격 탐지 및 방어 로직 구현

---

### 2. [PE_Parser]
Windows API를 사용하여 PE(Portable Executable) 파일을 Parsing한 프로그램입니다.

#### 프로젝트 개요
Windows PE File Parser를 구현합니다.

#### 핵심 목표
Windows 실행 파일의 구조를 이해하기 위해 PE 포맷을 분석하고, 파일 내 섹션 정보 및 IAT(Import Address Table)를 추출하는 프로그램을 구현합니다.

---

### 3. [AntiDebugging_Framework]
Windows API를 사용하여 Anti-Debugging Framework를 제작한 프로그램입니다.

#### 프로젝트 개요
Anti-Debugging Framework 및 프로세스 보호 메커니즘을 구현합니다.

#### 핵심 목표
디버거로부터 타겟 프로세스(`Client.exe`)를 보호하기 위해, 부모 프로세스인 `Launcher.exe`가 디버거 역할을 선점하여 디버깅을 방지하고 상호 인증을 수행하는 프레임워크를 구현합니다.

| 분류 | 상세 설명 |
| :--- | :--- |
| **Anti-Debugging** | `Launcher`가 선제적으로 `Client`에 디버거를 부착하여 외부 디버거 접근 차단 |
| **무결성 보호** | 부모 프로세스 검증 및 자체 디버깅 탐지 기술을 통한 상호 보호 체계 수립 |
| **이벤트 처리** | 디버깅 루프를 통해 `Client` 상태 변화 감시 및 안정적 종료 처리 |

---

> 본 저장소는 시스템 보안 학습 및 연구 목적으로 작성되었습니다.

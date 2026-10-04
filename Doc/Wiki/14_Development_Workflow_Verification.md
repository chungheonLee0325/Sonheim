# 14. Development Workflow & Verification

> **핵심 구현 범위**
>
> “기능 구현이 끝났다는 것을 무엇으로 확인했고, Unreal Editor에서 반복되는 작성/검증 작업을 어떻게 줄였는가?”

이 문서는 Runtime architecture가 아니라 **개발 과정의 검증 계층과 Editor automation**을 설명합니다.

---

## 1. 검증을 세 층으로 나눈다

```text
Authoring Validation
      ↓
Scenario Verification
      ↓
Manual Visual / Gameplay Check
```

각 층은 잡아낼 수 있는 문제가 다릅니다.

### 1.1 Authoring Validation

실행 전에 구조적으로 판단 가능한 오류를 검사합니다.

예:

- Dungeon StartStage 누락
- 잘못된 Transition target
- Action 필수 필드 누락
- GameplayTag namespace 오류
- Boss Pattern timing 오류

이 부분은 [[13. Content Authoring & Validation|13_Content_Authoring_Validation]]에서 자세히 설명합니다.

### 1.2 Scenario Verification

실제로 Runtime을 실행해야 확인할 수 있는 흐름을 반복 검증합니다.

대표 시나리오:

- Shortcut path
- ExtraWave path
- Success
- Timeout
- Owner death
- Owner leave
- Boss wake / phase / down / capture
- HUD / Result
- Minimap / Marker

### 1.3 Visual / Gameplay Check

자동 검증만으로 판단하기 어려운:

- Animation 연결
- Telegraph 가독성
- HUD 배치
- Minimap marker overlap
- 실제 play feel

은 Editor viewport / PIE에서 확인합니다.

---

## 2. Agent MCP는 Sonheim의 개발 도구다

Sonheim repository에는 Codex / Claude Code가 Unreal Editor의 Agent MCP에 연결되는 project configuration을 둡니다.

AgentMcp 자체는 별도 프로젝트이며, Sonheim에서는 **Editor 작업을 수행하는 workflow dependency**로 사용합니다.

활용 예:

- Animation Blueprint 편집
- Montage / Section 구성
- BlendSpace
- Inertialization
- Blueprint Class Default 적용/검증
- Viewport render capture
- Dungeon asset authoring
- Editor 상태 확인

즉 Runtime gameplay code와 Editor automation code를 같은 기능으로 설명하지 않습니다.

---

## 3. 코드 변경과 Editor 결과를 한 흐름에서 확인한다

예를 들어 Boss animation 작업이라면:

```text
C++ Runtime 변경
   ↓
Build
   ↓
Agent MCP로 Montage / ABP 구성
   ↓
PIE
   ↓
Runtime State 확인
   ↓
Viewport Capture
   ↓
Scenario Verification
```

Editor asset 변경이 수동 작업으로 완전히 분리되지 않게 하는 것이 목적입니다.

---

## 4. Regression 사례

### Guardian Capture Client Crash

Boss capture를 실제 multiplayer path로 실행했을 때 Client의 `OnRep_PartnerOwner`에서 Widget reference가 없는 상황이 드러났습니다.

Server path에 있던 null-state 방어를 Client replication path에도 추가했습니다.

핵심은 Capture UI 하나를 고친 것이 아니라 **실제 Client replication lifecycle에서 발생하는 상태를 재현했다는 점**입니다.

### Boss Wake Multicast 유실

Player teleport 직후 Boss wake multicast가 Client에 보이지 않는 문제가 있었습니다.

원인은 RPC 자체가 아니라 **Boss가 아직 Client에게 net-relevant하지 않은 시점**에 multicast가 발생한 것이었습니다.

현재 상태가 반드시 남아야 하는 정보와 순간 RPC를 구분해야 한다는 [[2. Gameplay Architecture|02_Gameplay_Architecture]]의 원칙으로 이어집니다.

### Barrier Rule

Stage별 Barrier 상태를 World Actor 내부 규칙으로 두지 않고 Definition → Snapshot으로 이동했습니다.

Regression을 개별 Actor patch로 끝내지 않고 state ownership을 수정한 사례입니다.

---

## 5. Verification 숫자를 어떻게 취급하는가

개발 과정에서는 여러 반복 검증 script와 check count를 사용했습니다.

일부 verification script는 공개 저장소에 포함되어 있지 않습니다.

따라서 Wiki에서는:

1. 현재 repository에서 직접 확인 가능한 Validation / Source
2. 개발 checkout에서 반복 수행한 Scenario Verification

을 구분합니다.

숫자 자체보다 **무엇을 어떤 조건에서 검증했는가**를 우선합니다.

---

## 6. Source-only Mirror

전체 Unreal Project는 asset 때문에 코드 리뷰에 불필요한 binary가 많습니다.

GitHub Actions가 별도 source-only repository에 다음을 동기화합니다.

- Source
- Config
- Plugin Source
- UProject
- Doc
- README

[Sonheim.Source](https://github.com/chungheonLee0325/Sonheim.Source)

전체 프로젝트 실행과 코드 검토 목적을 분리하기 위한 repository view입니다.

---

## 7. AgentMcp 자체의 개선은 별도 프로젝트에서 다룬다

Sonheim에서 사용한 Editor automation과 AgentMcp 자체 기능 개발을 구분합니다.

AgentMcp 쪽 주요 개선은 별도 repository에서:

- project-file centered agent config
- Blueprint class default apply / verify
- viewport render capture
- JSON-safe truncation
- AGENTS.md rules
- UI texture mipmap
- BlendSpace / Montage / Animation Blueprint support

등으로 관리합니다.

Sonheim Wiki에서는 이 기능들이 **게임 Runtime 기능인 것처럼 섞이지 않도록** workflow 관점에서만 설명합니다.

---

## 연관 문서

- Editor validation 자체 → [[13. Content Authoring & Validation|13_Content_Authoring_Validation]]
- 실제 Dungeon runtime → [[9. Branching Dungeon Runtime|09_Branching_Dungeon_Runtime]]
- 프로젝트 변화 과정 → [[15. Development History & Retrospective|15_Development_History_Retrospective]]

---

## 관련 저장소

- [Sonheim](https://github.com/chungheonLee0325/Sonheim)
- [Sonheim.Source](https://github.com/chungheonLee0325/Sonheim.Source)
- [AgentMcp](https://github.com/chungheonLee0325/AgentMcp)

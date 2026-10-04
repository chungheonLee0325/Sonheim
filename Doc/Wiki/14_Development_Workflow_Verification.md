# 14. Development Workflow & Verification

Sonheim 개발에서는 C++ 변경 이후 **Blueprint/DataAsset/Animation/UMG와 PIE 결과까지 같은 변경 단위에서 확인**합니다.

검증 흐름은 **Authoring Validation → Scenario Verification → Visual Review**로 나누고, Editor 작업은 **Inspect → Edit → Compile → PIE → Log/Capture → Review** 순서로 반복합니다.

---

## 개발 루프

~~~mermaid
flowchart LR
    INSPECT["<b>Inspect</b><br/>Code · Asset · Editor State"]
    EDIT["<b>Edit</b><br/>C++ · Blueprint · Data"]
    BUILD["<b>Compile / Save</b>"]
    PIE["<b>Play In Editor</b>"]
    VERIFY["<b>Verify</b><br/>State · Log · Scenario"]
    CAPTURE["<b>Viewport Capture</b><br/>Visual Review"]
    FIX["<b>Iterate</b>"]

    INSPECT --> EDIT
    EDIT --> BUILD
    BUILD --> PIE
    PIE --> VERIFY
    VERIFY --> CAPTURE
    CAPTURE --> FIX
    FIX --> INSPECT
~~~

코드·Asset 변경과 실제 Editor/PIE 결과를 같은 작업 흐름에서 확인합니다.

---

## 1. 검증을 세 종류로 나눈다

오류 유형에 따라 검증 단계를 나눕니다.

### Authoring Validation

실행하지 않아도 판단 가능한 구조 오류:

- 잘못된 Dungeon Transition
- 없는 Stage / Group
- GameplayTag namespace 오류
- Boss Strike timing 오류
- Montage Section contract 오류

는 Editor/Data Validation 단계에서 차단합니다.

→ [[13. Content Authoring & Validation|13_Content_Authoring_Validation]]

### Scenario Verification

실제 Runtime을 실행해야 확인 가능한 흐름:

- Shortcut / ExtraWave branch
- Success / Timeout
- Owner death / leave
- Boss wake / phase / down / exhaust / capture
- Reward / Result
- HUD / Minimap / Marker

은 PIE scenario로 반복 확인합니다.

### Visual / Gameplay Review

자동 state check만으로 판단하기 어려운:

- Telegraph가 실제로 읽히는가
- Montage와 Hit timing이 맞는가
- HUD가 gameplay를 가리지 않는가
- Minimap marker가 panel 밖으로 나가지 않는가
- Result 정보가 한눈에 읽히는가

는 viewport와 실제 play 화면을 확인합니다.

~~~text
정적 오류       → Authoring Validation
Runtime 흐름    → Scenario Verification
화면 / 감각     → Visual Review
~~~

---

## 2. AgentMcp — UE 5.5에서 Agent가 Editor 결과까지 다루도록 연결

[AgentMcp](https://github.com/chungheonLee0325/AgentMcp)는 **Unreal Engine 5.8의 실험적 MCP/toolset과 Agent Skill 개념을 참고해 UE 5.5용으로 재구현한 Editor MCP plugin**입니다.

AgentMcp는 coding agent가 Unreal Editor에서 **inspect → edit → run → verify**까지 이어서 수행할 수 있도록 연결합니다.

~~~text
Coding Agent
   ↓ MCP
Unreal Editor
   ├─ Asset / Blueprint Inspect
   ├─ Data / Class Default Edit
   ├─ UMG Widget Blueprint Authoring
   ├─ Animation / Montage / BlendSpace Authoring
   ├─ Compile / Save
   ├─ PIE
   ├─ Log
   └─ Viewport Capture
~~~

### Dynamic Agent Skills

Plugin / project의 <code>SKILL.md</code>를 Editor가 연결된 agent에 제공합니다.

- skill file을 매 호출 시 읽어 수정 내용을 Editor 재시작 없이 반영
- project skill이 plugin 기본 skill을 override
- Claude Code / Codex가 같은 project-specific authoring rule을 사용

### UMG에 특화한 도구

UMG 작업에는 다음 전용 도구를 사용합니다.

- Widget Tree / Named Slot inspect
- C++ <code>BindWidget / BindWidgetOptional</code> contract 검사
- Widget Blueprint 생성
- subtree 단위 Widget 추가
- widget / slot property 수정
- destructive edit 전 영향 범위 확인
- compile → PIE → viewport capture

까지 한 흐름으로 연결합니다.

AgentMcp는 Sonheim Runtime과 분리된 Editor authoring / verification plugin입니다.

---

## 3. 코드 밖에 있는 작업도 같은 변경 단위에서 처리

Boss 기능 변경은 Runtime C++과 Pattern/Animation/Blueprint asset을 함께 수정합니다.

~~~text
Boss Runtime C++ 변경
      ↓
Pattern DataAsset 수정
      ↓
Montage Section / Animation 구성
      ↓
Blueprint Default 적용
      ↓
Compile
      ↓
PIE
      ↓
Boss State / Log 확인
      ↓
Viewport Capture
~~~

AgentMcp를 통해 이 작업을 같은 agent session에서 이어갈 수 있습니다.

Sonheim에서 사용한 대표 작업:

- Animation Blueprint 구성/검사
- Montage / Section 구성
- BlendSpace / Inertialization
- Blueprint Class Default 수정 후 read-back
- DataAsset property 편집
- StringTable 편집
- UI texture import / mipmap 설정
- PIE 실행
- Editor log 확인
- viewport capture

---

## 4. Editor 변경 후 Read-back / PIE 검증

AgentMcp 작업은 Editor write 이후 **read-back → compile → PIE → capture**를 연속해서 수행합니다.

예:

~~~text
Blueprint Default 변경
 → object_set_properties
 → read-back
 → blueprint_compile
 → PIE
 → viewport_capture
~~~

Animation:

~~~text
Montage / BlendSpace 구성
 → compile
 → PIE
 → 실제 전환 확인
 → capture
~~~

UI:

~~~text
Widget 수정
 → BindWidget / tree 검사
 → compile
 → PIE
 → viewport capture
 → 다시 조정
~~~

Editor 수정 후 compile·PIE·capture까지 같은 session에서 이어서 확인합니다.

---

## 5. Dungeon Scenario Matrix

Dungeon scenario는 Route, Terminal, Objective, Boss, Client, Presentation 축으로 나눠 확인합니다.

| 축 | 대표 Scenario |
|---|---|
| Route | Shortcut / ExtraWave |
| Terminal | Success / Timeout / OwnerDown / OwnerLeft |
| Objective | Defeat / Capture / Optional |
| Boss | Wake / Pattern / Phase2 / Down / Exhaust / Capture |
| Client | Server / Remote Client |
| Presentation | HUD / Minimap / Marker / Result |

이 조합을 통해 한 기능 수정이 다른 branch나 Client 화면을 깨뜨리지 않는지 확인합니다.

---

## 6. Regression 사례와 구조 수정

### Boss Wake가 Remote Client에서 보이지 않음

Dungeon 이동 직후 Boss Wake Multicast가 Client에 보이지 않는 문제가 있었습니다.

원인은 Montage 자체가 아니라 **Server에서 해당 Client에 Boss가 아직 net-relevant하지 않은 시점에 순간 RPC가 발생한 것**이었습니다.

이 문제를 통해:

~~~text
반드시 복구되어야 하는 현재 상태
→ Replicated Status

그 순간의 연출
→ Multicast / Presentation
~~~

을 더 명확히 분리했습니다.

Boss의 현재 Action / Phase / Break는 persistent status로 유지하고, 순간 animation/effect와 구분합니다.

---

### Guardian Capture 후 Client Crash

실제 multiplayer capture scenario에서 Remote Client의 <code>OnRep_PartnerOwner</code>가 실행될 때 UI Widget이 아직 존재하지 않는 상태가 드러났습니다.

Server path에서는 이미 처리하던 null-state를 Client replication lifecycle에서도 안전하게 다루도록 수정했습니다.

수정 기준은 **복제 callback과 Local UI lifecycle의 순서를 동일하다고 가정하지 않는 것**이었습니다.

---

### Barrier Rule의 소유 위치 변경

초기에는 World Barrier 쪽에서 Stage별 동작을 알고 있었습니다.

Dungeon branch가 늘어나면서:

~~~text
Barrier가 Stage 규칙을 앎
        ↓
Dungeon Definition이 SealedBarriers를 소유
        ↓
Runtime State가 현재 결과를 발행
        ↓
Barrier는 자신의 ID 포함 여부만 반영
~~~

으로 변경했습니다.

버그를 Actor 조건문 하나로 막기보다 **규칙의 ownership 자체를 옮긴 사례**입니다.

---

## 7. 공개 Source와 개발 Checkout의 검증 근거

개발 과정에서는 반복 scenario script와 check count를 사용했습니다.

하지만 일부 verification script는 공개 저장소에 포함되어 있지 않습니다.

Wiki에는 **공개 Source에서 직접 확인 가능한 구조/Validation**과 **개발 checkout에서 반복 수행한 scenario verification**을 구분해 기록합니다.

검사 횟수 자체보다 **어떤 실패 경로까지 확인했는지와 코드에서 재현 가능한 근거**를 우선합니다.

---

## 8. Source-only Mirror로 코드 리뷰 경로 분리

전체 Unreal Project에는 Content asset과 binary가 많아 Source 리뷰가 불편합니다.

main branch가 갱신되면 GitHub Actions가 별도 [Sonheim.Source](https://github.com/chungheonLee0325/Sonheim.Source) repository에 다음을 동기화합니다.

~~~text
Source/
Config/
Plugins/*/Source/
Sonheim.uproject
Doc/
README.md
~~~

제외:

~~~text
Content/
Binaries/
Intermediate/
~~~

전체 프로젝트를 실행하려는 경로와 **코드만 빠르게 검토하려는 경로**를 분리합니다.

---

## 9. AgentMcp Verification

AgentMcp repository의 testbed / smoke test는 다음 Editor 기능을 검증합니다.

- MCP transport
- Reflection 기반 tool schema
- Undo / rollback
- PIE
- viewport capture
- Live Coding
- Blueprint / UMG
- DataAsset / DataTable
- Animation authoring
- Agent Skill

을 검증하며, 현재 AgentMcp README 기준 testbed smoke test는 **216 checks**를 수행합니다.

Sonheim에서는 이 Editor tool을 실제 authoring / PIE 검증 workflow에 사용합니다.

---

## 설계 선택과 비용

| 선택 | 얻은 것 | 비용 / 제약 |
|---|---|---|
| **검증 층 분리** | 정적 오류·Runtime 오류·Visual 문제를 각자 적합한 단계에서 발견 | 여러 종류의 검사 workflow 유지 필요 |
| **AgentMcp 기반 Editor loop** | C++과 Editor asset 작업을 한 session에서 수정/검증 | UE Editor가 실행 중이어야 하고 Editor API 범위에 영향받음 |
| **Scenario 중심 회귀 검사** | Branch / Client / Failure path를 반복 확인 | 전체 조합을 완전 탐색하는 자동 테스트는 아님 |
| **Source-only Mirror** | 채용/리뷰 시 코드 접근성 향상 | mirror sync workflow 유지 필요 |

---

## 연관 문서

- [[13. Content Authoring & Validation|13_Content_Authoring_Validation]] — 실행 이전의 정적 검증
- [[09. Branching Dungeon Runtime|09_Branching_Dungeon_Runtime]] — scenario 검증의 중심 콘텐츠
- [[10. Boss Encounter Runtime|10_Boss_Encounter_Runtime]] — Animation / Pattern / Capture 검증 사례
- [[11. UI Architecture & Client Presentation|11_Client_State_Presentation_Pipeline]] — Client UI lifecycle 검증 대상
- [[15. Development History & Retrospective|15_Development_History_Retrospective]] — 구조가 변경된 과정

---

## 관련 저장소

- [Sonheim](https://github.com/chungheonLee0325/Sonheim)
- [Sonheim.Source](https://github.com/chungheonLee0325/Sonheim.Source)
- [AgentMcp](https://github.com/chungheonLee0325/AgentMcp)

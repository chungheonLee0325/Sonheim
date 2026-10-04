# 15. Development History & Retrospective

Sonheim은 **Core Gameplay → Shared World Systems → Dungeon Vertical Slice** 순으로 확장되면서 상태 ownership과 시스템 경계를 여러 차례 수정했습니다.

주요 구조 변화와 결과는 다음과 같습니다.

---

## 변화 요약

| 초기 구조 / 문제 | 변경 | 결과 |
|---|---|---|
| Player class에 Pal 관련 책임 증가 | Capture / Inventory / Partner를 Component로 분리 | ownership lifecycle별 책임 분리 |
| 보유 Skill Logic을 일찍 생성 | Skill Spec과 Logic 분리, 필요 시 Instance 생성 | Runtime object 수명 분리 |
| 장비가 Skill을 단순 Add/Remove | GrantId + RefCount | 여러 Source가 같은 Skill을 안전하게 공유 |
| Item 상태 변화와 획득 의미가 섞임 | Inventory Changed / Item Acquired Event 분리 | 장비 해제·slot 이동에서 잘못된 획득 UI 방지 |
| 개인/공유/협력 Item state의 ownership 경계가 불명확 | Inventory / Container / Crafting ownership 분리 | 각 상태에 맞는 lifecycle 적용 |
| Dungeon Stage 규칙을 World Actor가 일부 앎 | Barrier / Transition rule을 Definition으로 이동 | World Actor와 콘텐츠 진행 규칙 분리 |
| 문자열 중심 Dungeon ID | GameplayTag namespace | Stage/Group/Branch/Barrier 관계와 validation 강화 |
| Dungeon UI가 여러 gameplay source를 직접 해석 | Snapshot → Presenter → ViewData | Runtime / UMG lifecycle 분리 |
| Dungeon 전용 Toast | 공용 NoticeSubsystem | Level-up / Capture / Crafting / Region에서도 재사용 |
| 사람이 Stage flow를 별도 문서로 관리 | Definition → Mermaid Stage Graph 생성 | 문서와 실제 데이터의 drift 감소 |
| 플레이 후 오류 발견 | Editor / Graph Validation 강화 | Cycle, unreachable, producer 순서, Boss timing을 실행 전 검사 |
| C++ 변경과 Editor 작업이 분리 | AgentMcp 기반 Inspect→Edit→PIE→Review | Asset 작업과 runtime 검증을 같은 loop에서 수행 |

---

## 1. Pal 기능의 Component 분리

Pal 기능이 늘면서 Player class가 다음 책임을 함께 다루기 시작했습니다.

~~~text
Capture
Pal List
Selection
Summon
Partner Skill
~~~

각 기능의 state와 lifecycle을 분리하기 위해:

~~~text
UPalCaptureComponent
→ 포획 과정

UPalInventoryComponent
→ 소유 / 선택

UPalPartnerSkillComponent
→ 소환 / Partner Action
~~~

으로 책임을 나눴습니다.

이후 Dungeon에서도 **state owner와 lifetime**을 기준으로 Runtime/World/UI 책임을 배치했습니다.

---

## 2. 새 콘텐츠에서 기존 경계 재사용

Dungeon 구현에서는 다음 기존 시스템 경계를 그대로 재사용했습니다.

- Interaction → Entrance / Lever / Reward Chest
- Inventory → Dungeon Reward
- Capture → Guardian Capture
- Monster Death / Capture → Objective Tracker
- \`FAttackData\` → Boss Strike
- Health Component → Party HUD
- Notice → Dungeon / Level-up / Capture / Crafting

새 콘텐츠가 기존 시스템 내부 상태를 직접 알아야 했던 부분은 ownership 위치를 다시 조정했습니다.

---

## 3. Barrier 규칙 — World Actor에서 Content Definition으로 이동

초기에는 Barrier가 현재 Stage를 기준으로 자신의 동작을 판단하는 방향이 자연스러웠습니다.

하지만 분기가 늘어나면 Barrier가 Dungeon 진행 규칙을 알아야 합니다.

~~~text
Before

Barrier Actor
 └─ "Combat Stage면 닫힘"
~~~

이를 다음처럼 변경했습니다.

~~~text
After

Dungeon Definition
 └─ Stage.SealedBarriers
       ↓
Dungeon Runtime State
       ↓
Barrier Actor
 └─ "내 ID가 현재 sealed 목록에 있는가?"
~~~

**어떤 문을 닫을지는 콘텐츠 규칙**, **실제로 문을 닫아 표현하는 것은 World Actor**로 나눴습니다.

---

## 4. DataTable 중심 구조에서 콘텐츠별 데이터 모델로

초기에는 DataTable이 주요 gameplay data를 담당했습니다.

Item / Skill처럼 같은 schema의 row를 관리하는 데는 여전히 적합합니다.

Dungeon을 만들면서 추가 요구가 생겼습니다.

- 자체 콘텐츠 identity
- nested Stage graph
- asset dependency
- soft loading
- GameplayTag namespace
- validation
- Presentation 분리

DataTable은 반복 row data에 유지하고, Dungeon/Boss 같은 독립 콘텐츠는 DataAsset으로 분리했습니다.

~~~text
Repeated Row Data
→ DataTable

Independent Content Definition
→ PrimaryDataAsset / DataAsset

Runtime Identity
→ GameplayTag

Player-facing Text
→ StringTable
~~~

“Data-driven = DataTable 사용”에서 **데이터 성격에 맞는 authoring/runtime 모델을 선택한다**는 방향으로 확장됐습니다.

---

## 5. Dungeon UI — Event 연결에서 현재 상태 모델로 확장

Health나 Inventory처럼 독립 값은 Delegate 기반 UI로 충분했습니다.

Dungeon은:

- Stage
- Objective
- Branch
- Timer
- Party
- Boss
- Reward
- Result

가 동시에 하나의 화면을 구성합니다.

개별 callback을 Widget에 계속 추가하면 UMG가 gameplay 구조를 너무 많이 알아야 합니다.

그래서:

~~~text
Server Runtime
    ↓
Replicated Snapshot
    ↓
Presenter
    ↓
ViewData
    ↓
UMG
~~~

로 변경했습니다.

Snapshot/Presenter/ViewData 구조로 HUD 재생성 시 현재 Run state를 복원하고, gameplay schema와 Widget layout의 변경 경계를 분리했습니다.

---

## 6. Dungeon Toast → NoticeSubsystem

Dungeon 개발 중 처음 필요했던 Toast를 Dungeon 전용 helper로 계속 유지할 수 있었습니다.

하지만 같은 요구가:

- Level-up
- Capture
- Crafting
- Region / Dungeon Title

에서도 나타났습니다.

그래서 \`UNoticeSubsystem\`으로 이동해:

- Slot
- Queue / Replace policy
- Producer Channel
- Style / Widget config

를 공통화했습니다.

**처음부터 범용 시스템을 예측해 만드는 것보다, 두 번째 실제 사용처가 생겼을 때 공통 경계를 추출**하는 쪽을 선택했습니다.

---

## 7. Validation은 콘텐츠 복잡도와 함께 강화

초기 시스템은 개별 row나 reference가 올바른지 확인하는 정도로도 관리할 수 있었습니다.

Dungeon graph와 Boss Pattern은 field 하나가 유효해도 전체 조합이 잘못될 수 있습니다.

예:

~~~text
존재하는 Stage 두 개
+ 각각 유효한 Transition
→ 서로 Cycle이면 콘텐츠는 잘못됨

존재하는 GroupId
+ 유효한 WaveCompleted Rule
→ 앞선 경로에서 Group을 Spawn하지 않으면 완료될 수 없음
~~~

그래서 Validation 범위를:

~~~text
Field
 → Relation
 → Graph
 → Timing / Animation Contract
~~~

까지 확대했습니다.

Stage Graph도 Definition에서 직접 생성하도록 바꿔 문서와 콘텐츠 source를 분리하지 않았습니다.

---

## 8. Server Authority + Limited Prediction

공유 gameplay 결과를 Server가 결정하는 원칙은 유지했습니다.

하지만 Inventory UI처럼 round-trip latency가 직접 느껴지는 영역에서는 Authority만 강조하면 조작감이 떨어집니다.

그래서:

~~~text
Final Result
→ Server Authority

Immediate Feedback
→ Limited Client Prediction

Correction
→ Reconciliation
~~~

으로 나눴습니다.

반대로 Crafting resource 소비나 Capture 성공처럼 잘못 예측했을 때 결과가 큰 상태는 Client가 먼저 확정하지 않습니다.

기술 하나를 전체 프로젝트에 일괄 적용하기보다 **결과의 중요도와 복구 비용에 따라 범위를 정했습니다.**

---

## 9. AgentMcp로 Editor 검증 Loop 연결

Agent를 사용해 C++만 작성하면 Unreal 프로젝트의 실제 변경은 절반만 끝난 경우가 많았습니다.

Blueprint / DataAsset / Animation / UMG를 수정한 뒤 사람이 다시 Editor를 열어 확인해야 했기 때문입니다.

AgentMcp를 통해:

~~~text
Inspect
 → Edit
 → Compile
 → PIE
 → Log / Viewport
 → Review
~~~

를 같은 작업 흐름에서 연결했습니다.

목표는 작성 속도 자체보다 **변경과 검증 사이의 수동 전환을 줄이고, agent가 자신이 만든 결과를 다시 확인하게 하는 것**이었습니다.

---

## 짧은 개발 Timeline

### 2025 — Core Gameplay

Player / AreaObject, Combat / Skill, Interaction / Item, Pal Capture / Partner, Inventory / Container / Crafting을 구축했습니다.

이 시기에 만들어진 Component / Interface / Damage / Item 경계가 이후 Dungeon의 기반이 됐습니다.

### 2026 — Dungeon Vertical Slice

기존 시스템을 하나의 콘텐츠에서 결합하는 Forgotten Ruins를 추가했습니다.

~~~text
Entrance
 → Branch
 → Objective
 → Boss
 → Success / Failure
 → Reward
 → Record / Grade
~~~

이 과정에서 DataAsset / GameplayTag / Snapshot / Presenter / Validation / AgentMcp workflow가 크게 확장됐습니다.

---

## 현재 남아 있는 제약

현재 구현에서 확인된 제약은 다음과 같습니다.

- Skill Cost rollback은 Item 부분 실패를 복구하지만 Stamina까지 포함한 전체 transaction은 아님
- Condition timer는 같은 condition을 여러 Source가 독립 duration으로 중첩하는 요구에 제한이 있음
- Animation-driven melee correctness를 위해 Server animation/bone update 비용을 더 사용
- Container subscriber 방식은 connection별 세밀한 replication filtering까지는 수행하지 않음
- 일부 scenario verification script는 공개 저장소에 포함되어 있지 않음
- Sonheim networking은 UE Listen Server 기반이며 외부 dedicated-server protocol stack 구현과는 범위가 다름

각 제약은 [[05. Combat, Skill & Animation|05_Combat_Skill_Animation]], [[12. Multiplayer Synchronization|12_Multiplayer_Synchronization]], [[14. Development Workflow & Verification|14_Development_Workflow_Verification]] 등 실제 시스템 문서에 더 구체적으로 남깁니다.

---

## 연관 문서

- [[02. Gameplay Architecture|02_Gameplay_Architecture]] — 현재 책임 경계
- [[03. Data & Content Architecture|03_Data_Content_Architecture]] — 변화된 데이터 모델
- [[09. Branching Dungeon Runtime|09_Branching_Dungeon_Runtime]] — 기존 시스템을 결합한 Vertical Slice
- [[13. Content Authoring & Validation|13_Content_Authoring_Validation]] — 콘텐츠 규모 증가에 따른 검증
- [[14. Development Workflow & Verification|14_Development_Workflow_Verification]] — Editor automation / regression workflow

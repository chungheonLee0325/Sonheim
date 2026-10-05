# Sonheim Engineering Wiki

> **Unreal Engine 5.5 / C++ 기반 멀티플레이 액션 어드벤처**
>
> 전투·상호작용·인벤토리/제작·Pal 포획/파트너·UI·데이터 기반 콘텐츠를 구현하고, 각 시스템을 서버 권위 gameplay와 client presentation 구조로 연결했습니다.

[[01. Project Overview|01_Project_Overview]]에서는 실제 gameplay loop와 구현 범위를 먼저 볼 수 있습니다.

---

## System Architecture

**현재 코드의 주요 시스템과 책임 구조**입니다.

- **굵은 첫 줄** — 역할
- **둘째 줄 이하** — 실제 UE / Sonheim class·data
- **점선** — 동기화·표현 연결

~~~mermaid
flowchart TB
    subgraph DATA["Content / Authoring"]
        ROW["<b>반복 Gameplay Data</b><br/>DataTable · StringTable"]
        DEF["<b>콘텐츠 정의</b><br/>UDungeonDefinitionDataAsset<br/>UBossPatternDataAsset · GameplayTag"]
    end

    subgraph GAME["Gameplay Runtime"]
        BASE["<b>공통 Character 기반</b><br/>AAreaObject : ACharacter<br/>Health · Condition · Skill Components"]
        BODY["<b>World Body</b><br/>ASonheimPlayer / ABaseMonster"]
        PSTATE["<b>Player 지속 상태</b><br/>ASonheimPlayerState<br/>Inventory · StatBonus · PalInventory"]
        WORLD["<b>World 상호작용</b><br/>UInteractionComponent<br/>IInteractableInterface"]
        CONTENT["<b>콘텐츠 실행</b><br/>UDungeonStageRuntimeSubsystem<br/>UBossFSM : UBaseAiFSM"]
    end

    subgraph STATE["Authoritative / Replicated State"]
        ACTORSTATE["<b>Actor 단위 상태</b><br/>Health · SkillSpec · Inventory · BossStatus"]
        GAMESTATE["<b>공유 콘텐츠 상태</b><br/>ASonheimGameState<br/>FDungeonStageRuntimeState"]
    end

    subgraph UI["Client Presentation"]
        HUD["<b>기본 HUD / Screen</b><br/>PlayerController · UMG Widgets"]
        NOTICE["<b>일시적 알림</b><br/>UNoticeSubsystem : ULocalPlayerSubsystem"]
        VIEW["<b>복합 콘텐츠 UI</b><br/>Presenter → ViewData<br/>LocalPlayer UI Router"]
    end

    ROW --> BASE
    ROW --> PSTATE
    DEF --> CONTENT

    BASE --> BODY
    PSTATE --> BODY
    WORLD --> BODY
    BODY --> CONTENT

    BODY --> ACTORSTATE
    CONTENT --> GAMESTATE

    ACTORSTATE -. "Delegate / Replication" .-> HUD
    ACTORSTATE -. "Replicated State" .-> VIEW
    GAMESTATE -. "Snapshot" .-> VIEW
    CONTENT -. "Transient Event" .-> NOTICE
~~~

### 구현 경계

**C++ Runtime**
- Server authority와 state ownership
- Character/Component/Interface 공통 로직
- Combat·Inventory·Crafting·Capture·Dungeon/Boss 실행
- RPC / Replication / FastArray와 Presenter/ViewData 변환

**Data / Content**
- Item·Skill·AreaObject·Level·Resource처럼 반복되는 값은 DataTable
- Dungeon Stage/Branch/Objective, Boss Pattern/Strike처럼 중첩된 콘텐츠는 DataAsset
- Stage·Group·Barrier·Pattern처럼 계층 관계가 필요한 runtime ID는 GameplayTag
- 화면 문구·아이콘·Map 정보는 gameplay rule과 분리된 presentation data / StringTable

**Blueprint / UMG / Animation**
- C++ parent와 <code>BindWidget</code> contract 위에서 Widget Blueprint 구성
- Animation Blueprint·Montage·Notify로 locomotion과 action timing 조정
- DataAsset/Blueprint default로 콘텐츠 asset 연결과 presentation tuning
- UMG는 gameplay rule을 직접 소유하지 않고 delegate 또는 ViewData를 소비

즉 **C++이 실행 규칙과 상태를 소유하고, Data가 콘텐츠 조합과 tuning을 제공하며, Blueprint/UMG/Animation이 asset과 표현을 구성**하는 형태입니다.

---

## Core Architecture

- [[02. Gameplay Architecture|02_Gameplay_Architecture]] — PlayerState/Pawn/Subsystem의 **상태 소유 위치**, ActorComponent·Interface 구성, Data/Runtime/Presentation 경계
- [[03. Data & Content Architecture|03_Data_Content_Architecture]] — DataTable·DataAsset·GameplayTag·Soft Reference·StringTable의 적용 기준과 실제 사용처
- [[12. Multiplayer Synchronization|12_Multiplayer_Synchronization]] — Request / State / Scope / Prediction / transient event를 UE Listen Server에서 어떻게 동기화했는지 정리합니다.

---

## Gameplay Systems

- [[04. Player & Character Systems|04_Player_Character_Systems]] — Pawn/PlayerState 수명 분리, Attribute/Condition/Stat과 장비 변경의 실제 상태 흐름
- [[05. Combat, Skill & Animation|05_Combat_Skill_Animation]] — Skill Data/State/Logic, AnimNotify 기반 action timing, melee interpolation, Damage context
- [[06. World Interaction Systems|06_World_Interaction_Systems]] — Item·Container·Crafting·Lever·Portal을 하나의 <code>IInteractableInterface</code> 입력/Prompt/Hold 흐름으로 연결
- [[07. Inventory & Crafting|07_Inventory_Crafting]] — Item/Equipment/Container/Crafting의 데이터와 UI, 협력 제작 lifecycle
- [[08. Pal Capture & Partner Lifecycle|08_Pal_Capture_Partner_Lifecycle]] — Wild Monster → Capture → Ownership → Storage → Summon → Partner AI

---

## UI Architecture

- [[11. UI Architecture & Client Presentation|11_Client_State_Presentation_Pipeline]] — Delegate HUD, Inventory/Crafting screen, Confirm popup, Notice/Toast queue, Dungeon Presenter/ViewData와 LocalPlayer UI routing

- **Player HUD** — Health·Stamina·Level 등 gameplay state의 Delegate를 구독해 갱신
- **Inventory / Container / Crafting** — PlayerController와 각 Widget이 화면 lifecycle과 데이터 바인딩을 관리
- **Notice / Toast** — <code>UNoticeSubsystem</code>이 Banner·Title의 Queue와 Channel을 관리
- **Dungeon HUD / Result** — Snapshot → Presenter → ViewData → UI Router로 복합 상태를 화면에 전달

---

## Integrated Content Case Study — Forgotten Ruins

현재 가장 큰 통합 적용 사례는 **분기형 Dungeon Vertical Slice**입니다.

Dungeon Definition과 Runtime은 Combat·Interaction·Inventory·Capture의 결과를 objective / branch / reward / boss encounter로 연결합니다.

- [[09. Branching Dungeon Runtime|09_Branching_Dungeon_Runtime]] — Branch, Objective, Barrier, Timer, Reward, Record를 Event/Condition/Action/Transition 데이터로 진행
- [[10. Boss Encounter Runtime|10_Boss_Encounter_Runtime]] — 기존 Monster/Combat 기반 위에 Boss FSM, Pattern/Strike, Telegraph, Phase, Down/Exhaust를 구성
- [[11. UI Architecture & Client Presentation|11_Client_State_Presentation_Pipeline]] — HUD, 목표 추적, Party 상태, Minimap/Marker, Boss HUD, Result를 current snapshot에서 구성

Forgotten Ruins는 **Combat·Interaction·Inventory·Capture·UI가 하나의 콘텐츠 흐름에서 연결되는 통합 구현 사례**입니다.

---

## Content Authoring & Engineering

- [[13. Content Authoring & Validation|13_Content_Authoring_Validation]] — Stage graph, dependency, Boss timing처럼 데이터 조합에서 생기는 오류를 Editor / graph validation으로 검사
- [[14. Unreal Editor Automation & Verification|14_Unreal_Editor_Automation_Verification]] — AgentMcp를 이용한 UE Editor authoring과 Compile → PIE → Log/Viewport 검증
- [[15. Development History & Retrospective|15_Development_History_Retrospective]] — 기능 추가보다 ownership과 시스템 경계가 실제 확장에서 어떻게 바뀌었는지 정리

### AgentMcp

[AgentMcp](https://github.com/chungheonLee0325/AgentMcp)는 **UE 5.8의 실험적 MCP/toolset과 Agent Skill 개념을 참고해 UE 5.5용으로 재구현한 Editor MCP plugin**입니다.

Sonheim에서는 **UMG/BindWidget authoring, Blueprint/Data/Animation 편집, Compile → PIE → Log/Viewport 검증**에 사용했습니다. 자세한 구현과 실제 검증 흐름은 [[14. Unreal Editor Automation & Verification|14_Unreal_Editor_Automation_Verification]]에서 확인할 수 있습니다.

---

## Source

- [Sonheim](https://github.com/chungheonLee0325/Sonheim) — 전체 Unreal Engine 프로젝트
- [Sonheim.Source](https://github.com/chungheonLee0325/Sonheim.Source) — Source / Config / Docs 중심 코드 검토용
- [AgentMcp](https://github.com/chungheonLee0325/AgentMcp) — UE 5.5 Editor MCP / agent workflow plugin

---

## 코드 표기

Wiki의 코드 블록은 구현 구조를 설명하는 데 필요한 선언과 함수만 발췌합니다. 생략된 <code>UPROPERTY</code> metadata나 보조 필드는 각 문서 하단의 **관련 코드** 링크에서 확인할 수 있습니다.

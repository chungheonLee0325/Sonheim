# 14. Unreal Editor Automation & Verification

**[AgentMcp](https://github.com/chungheonLee0325/AgentMcp)를 이용해 C++ 코드 수정만으로 끝나지 않는 Unreal Editor 작업을 조사·편집·실행 검증까지 연결**했습니다. Sonheim에서는 Widget Blueprint의 실제 구조와 C++ `BindWidget` 계약을 확인하고, 에디터에서 수정한 에셋을 다시 읽은 뒤 Compile·PIE·Viewport Capture로 결과를 검토했습니다.

AgentMcp는 Unreal Engine 5.8의 실험적 MCP/Agent Skill 개념을 참고해 **UE 5.5.4용으로 별도 구현한 Editor MCP 플러그인**입니다.

## 에디터 작업 시연

https://github.com/user-attachments/assets/e61d53e3-c94b-4062-99dc-be11dd261ea5

에이전트가 에디터 상태와 Blueprint 값을 조회하고, 변경한 값을 다시 확인한 뒤 **Compile·Save → PIE → 실행 중 값/로그·화면 확인**까지 이어가는 과정입니다.

![AgentMcp의 에디터 작업과 PIE 실행 결과](../Media/Wiki/14_Unreal_Editor_Automation_Verification/14_agentmcp_editor.png)

## 핵심 구현

| 경계 | 사용한 기능 | 확인 대상 |
|---|---|---|
| **코드 ↔ Widget Blueprint** | `umg_inspect` · `blueprint_compile` | Widget Tree와 `BindWidget / BindWidgetOptional`의 이름·타입 일치 |
| **에셋 조회·수정** | `object_get_properties / object_set_properties` · UMG/DataTable Tool | 변경 전후 Class Default·Widget/Slot·Data 값 |
| **변경 확정** | 별도 Read-back · `asset_save` | 실제 에셋에 반영된 값과 저장 상태 |
| **실행 검증** | `pie_start` · `log_get_recent` · `viewport_capture` | Runtime 인스턴스·오류 로그·실제 화면 |
| **작업 규칙** | `skills_list / skills_get` | 프로젝트별 제작·검증 절차 제공 |

## 목차

- [Sonheim 적용 — UMG v2 개편](#sonheim-적용--umg-v2-개편)
- [Inspect → Edit → Verify 작업 흐름](#inspect--edit--verify-작업-흐름)
- [프로젝트 데이터와 애니메이션 작업](#프로젝트-데이터와-애니메이션-작업)
- [AgentMcp 자체 검증 사례](#agentmcp-자체-검증-사례)
- [Tool과 Skill의 역할 분리](#tool과-skill의-역할-분리)
- [에디터 변경의 안전장치](#에디터-변경의-안전장치)
- [검증 범위와 구현 선택](#검증-범위와-구현-선택)
- [관련 문서 및 저장소](#관련-문서-및-저장소)

---

## Sonheim 적용 — UMG v2 개편

Sonheim의 UMG v2 작업에서는 **기존 C++ Widget의 데이터·이벤트 연결을 유지하면서 Widget Blueprint 구조와 표현을 수정**했습니다.

특히 도감·퀘스트·인벤토리·던전 HUD처럼 여러 UI를 개편할 때, 화면을 수정하기 전에 **실제 Widget Tree와 C++ 바인딩을 대조**하는 작업이 필요했습니다.

### Widget 구조와 C++ 계약 확인

~~~text
C++ UUserWidget
  └─ UPROPERTY(meta = (BindWidget / BindWidgetOptional))
         ↓ 이름·타입 계약
Widget Blueprint
  └─ Widget Tree · Named Slot · Slot Layout
         ↓
umg_inspect → 실제 구조·바인딩 확인
~~~

- **조회:** `umg_inspect`로 위젯 트리·Named Slot·부모 클래스의 바인딩 정보 확인
- **수정:** `umg_add_widgets`·`umg_set_widget_properties` 등으로 위젯과 슬롯 속성 변경
- **검사:** 다시 `umg_inspect`를 호출해 C++ 선언과 실제 위젯 이름·타입 확인
- **확인:** `blueprint_compile` 후 PIE에서 실제 Widget 생성·입력·표시 동작 검토

예를 들어 `WBP_MonsterDex`와 `WBP_MonsterDexTier`에서는 목록·상세·보상 위젯 구조를 에디터 캡처와 함께 확인했습니다. **실제 개편된 Widget Tree와 화면 전후 비교**는 [[18. UMG v2 Migration & Editor Verification|18_UMG_v2_Migration]]에 정리했습니다.

이 문서에서는 **에디터에 접근한 방법과 검증 단계**를 설명하고, UI Stack·Notice·Feed·Headline 등의 런타임 설계는 18번에서 다룹니다.

---

## Inspect → Edit → Verify 작업 흐름

에이전트의 수정 지시만으로 완료 처리하지 않고 **에디터의 실제 상태를 독립적으로 다시 읽는 단계**를 둡니다.

~~~mermaid
flowchart LR
    TARGET["Inspect<br/>대상·속성·참조 확인"] --> WRITE["Edit<br/>에디터 Tool로 변경"]
    WRITE --> READ["Read-back<br/>값·트리 재조회"]
    READ --> BUILD["Compile / Save<br/>오류·저장 확인"]
    BUILD --> PIE["PIE<br/>Runtime 실행"]
    PIE --> EVIDENCE["Log + Viewport<br/>결과 검토"]
    EVIDENCE -. "수정 필요" .-> TARGET
~~~

### 에디터 Class Default 변경 사례

앞의 시연 영상은 **Blueprint Class Default 수정과 Runtime 확인을 연결하는 작업**을 보여 줍니다. 구체적인 프로퍼티 값보다 **에디터의 저장 값과 실행 중 인스턴스 값이 일치하는지**가 검증 대상입니다.

| 순서 | 사용하는 도구 | 확인 내용 |
|---|---|---|
| 1. 대상 확인 | `editor_get_state` · `blueprint_inspect` | 열려 있는 프로젝트·부모 클래스·컴포넌트 |
| 2. 변경 전 조회 | `object_get_properties` | 기존 Class Default 값 |
| 3. 편집 | `object_set_properties` | 지정한 프로퍼티 변경 |
| 4. 독립 조회 | `object_get_properties` | 변경 후 값 재확인 |
| 5. 에셋 검사 | `blueprint_compile` · `asset_save` | 컴파일 오류와 저장 여부 |
| 6. 실행 | `pie_start` | 변경한 Blueprint의 Runtime 상태 |
| 7. 결과 확인 | `object_get_properties` · `log_get_recent` · `viewport_capture` | 런타임 값·로그·화면 |
| 8. 종료 | `pie_stop` | 검증 세션 종료 |

`object_set_properties`의 반환값만 확인하는 것보다, **다른 조회 호출과 PIE 결과를 비교**해야 에디터 설정이 의도대로 실행됐는지 판단할 수 있습니다.

### 결과를 남기는 방식

**Compile 성공**, **Runtime 상태 일치**, **Viewport 화면이 의도대로 보이는지**는 서로 다른 검증 조건입니다.

~~~text
Blueprint Compile  →  에셋의 문법·바인딩 오류
Read-back          →  변경한 설정 값
PIE Runtime        →  실제 실행 인스턴스·이벤트
Log                →  실패·경고·상태 출력
Viewport Capture   →  UI 배치·표시·연출
~~~

에셋 수정은 PIE 시작 전에 완료하고, 실행 중에는 상태를 조회해 결과를 확인합니다.

---

## 프로젝트 데이터와 애니메이션 작업

AgentMcp는 Widget Blueprint 외에도 **에디터에서만 접근할 수 있는 프로젝트 에셋**을 같은 방식으로 조회·편집할 수 있게 합니다.

| 에셋 | 주요 Tool | 확인할 내용 |
|---|---|---|
| **DataAsset / Blueprint Default** | `object_get_properties` · `object_set_properties` | 설정값 변경 전후 |
| **DataTable** | `datatable_get_schema` · `datatable_get_rows / set_rows` | Row 구조·실제 필드 값 |
| **StringTable** | `stringtable_get_entries / set_entries` | UI 문자열 키와 원문 |
| **Animation** | `anim_build_blend_space` · `anim_build_montage` · `anim_build_anim_blueprint` | Animation 에셋과 Section·Blend 설정 |
| **UMG** | `umg_inspect` · `umg_add_widgets` · `umg_set_widget_properties` | Widget Tree·Slot·바인딩 |

Sonheim의 Dungeon HUD·Result와 기타 UI 에셋 작업에서는 에디터 조회·수정과 실제 PIE 결과를 연결했습니다. **Dungeon Stage 규칙이나 Boss Strike의 유효성 자체를 AgentMcp가 대신 판단하는 것은 아닙니다.** 콘텐츠 데이터의 규칙 검증은 [[13. Content Authoring & Validation|13_Content_Authoring_Validation]]의 `ValidateDefinition`·`UBossPatternDataAsset::Validate`에서 담당합니다.

---

## AgentMcp 자체 검증 사례

AgentMcp 저장소에는 **Sonheim과 별개인 UE 5.5 테스트베드 프로젝트**가 있습니다. 여기서는 에이전트가 UMG Designer를 열지 않고 Dungeon HUD와 Result Widget을 만든 과정을 실제 사례로 검증했습니다.

| 검증 과정 | 확인한 내용 |
|---|---|
| 첫 UI 생성 | 한 화면의 Widget Tree를 직접 생성·컴파일·PIE 실행 |
| 구조 리뷰 | 복사된 보상 슬롯·목표 행·통계 타일의 재사용 문제 확인 |
| 공통 컴포넌트화 | Widget Blueprint 부품과 C++ `BindWidget` 계약으로 교체 |
| 데이터 분리 | DataTable·Theme DataAsset·`DynamicEntryBox` 적용 |
| 재실행·캡처 | 화면을 다시 캡처하고 구조·스타일 변경 결과 확인 |

이 과정에서 **첫 UMG Tool 호출의 과도한 Read-back 결과**도 수정했습니다. 전체 Widget 속성을 불필요하게 반환하는 대신 **요청한 필드 위주로 응답**하도록 개선한 사례입니다.

자세한 실험 기록과 전후 결과는 [AgentMcp — Dungeon UI 워크플로 검증 사례](https://github.com/chungheonLee0325/AgentMcp/blob/main/Docs/Samples/DungeonUi.ko.md)에 있습니다.

이 테스트베드의 Tool 검증과 **Sonheim 게임에서 검증한 기능은 서로 다른 범위**입니다.

---

## Tool과 Skill의 역할 분리

**Tool은 에디터가 할 수 있는 작업을 제공하고, Skill은 그 작업을 어떤 순서·기준으로 수행할지 정의**합니다.

| 구분 | 예 |
|---|---|
| **Tool** | `umg_inspect`, `umg_add_widgets`, `blueprint_compile`, `viewport_capture` |
| **Skill** | 반복 UI를 컴포넌트로 분리, `BindWidget` 확인, 수정 후 PIE 캡처 검토 |
| **프로젝트별 규칙** | Plugin의 기본 `SKILL.md`보다 프로젝트 `AgentMcp/Skills`의 같은 이름 Skill 우선 적용 |

AgentMcp는 `skills_list`와 `skills_get`으로 연결된 에이전트에 Skill을 제공합니다. Skill 파일은 조회 시 다시 읽기 때문에 지침을 고쳐도 **다음 조회에서 새 내용을 반영**할 수 있습니다.

이렇게 Tool 구현에 특정 프로젝트의 UI 제작 규칙을 고정하지 않고, 프로젝트가 필요한 검증 절차를 별도 지침으로 관리합니다.

---

## 에디터 변경의 안전장치

에디터 자동화는 조회와 실제 쓰기를 구분해야 합니다. AgentMcp의 Tool은 `Read / Write / Destructive / Control`로 분류됩니다.

| 처리 | 구현된 동작 |
|---|---|
| **Write** | 입력값을 먼저 검사하고 에디터 Transaction 안에서 변경·실패 시 Rollback |
| **Destructive** | 영향 범위를 사전 확인하고 `bConfirm=true`가 있어야 실행 |
| **PIE 중 편집** | 기본 설정에서는 Editor Write 요청을 차단 |
| **Save** | 일반 에셋은 `asset_save`, 레벨은 `level_save`로 명시적으로 저장 |
| **Tool 접근** | `AllowedTools / BlockedTools / BlockedProperties`로 허용 범위 제한 |

**에디터에 값을 썼다는 사실과 에셋이 저장·컴파일·실행 검증됐다는 사실을 구분**하기 위한 정책입니다. Tool의 동작과 설정은 [AgentMcp README](https://github.com/chungheonLee0325/AgentMcp/blob/main/README.ko.md)에서 확인할 수 있습니다.

---

## 검증 범위와 구현 선택

| 검증 | 확인하는 것 | 근거 |
|---|---|---|
| **Editor 구조** | Widget Tree, `BindWidget`, DataAsset 필드 | Tool의 Read-back |
| **Editor 컴파일** | Blueprint 컴파일 오류 | `blueprint_compile` 결과 |
| **PIE 실행** | 실제 에셋 적용·Runtime 프로퍼티·로그 | PIE + Runtime 조회 |
| **시각 검토** | HUD 배치, 표시 상태, 애니메이션 결과 | Viewport Capture·게임 영상 |
| **게임플레이 시나리오** | 분기·보스·실패·멀티플레이 결과 | 각 Runtime 구현과 별도의 시나리오 검증 |

이 작업의 의미는 **에이전트가 수정한 에셋을 에디터의 실제 상태와 플레이 화면에서 다시 확인할 수 있도록 연결했다는 것**입니다. 데이터 유효성 검사, PIE 실행, 게임플레이 시나리오 확인은 서로 다른 증거를 제공하며 하나의 도구 호출로 대체하지 않습니다.

## 관련 문서 및 저장소

- [AgentMcp — UE 5.5 Editor MCP Plugin](https://github.com/chungheonLee0325/AgentMcp) — 도구 구현·Skill·테스트베드
- [AgentMcp — Dungeon UI 사례](https://github.com/chungheonLee0325/AgentMcp/blob/main/Docs/Samples/DungeonUi.ko.md) — UMG 작성·재구성·PIE 검증의 실제 기록
- [[11. UI Architecture & Client Presentation|11_Client_State_Presentation_Pipeline]] — Sonheim UMG의 데이터·이벤트 연결
- [[13. Content Authoring & Validation|13_Content_Authoring_Validation]] — Dungeon·Boss 작성 단계의 규칙 검증
- [[18. UMG v2 Migration & Editor Verification|18_UMG_v2_Migration]] — Sonheim UI 개편 전후 화면·Widget Tree

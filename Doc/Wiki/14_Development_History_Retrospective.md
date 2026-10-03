# 14. Development History & Retrospective

Sonheim은 처음부터 현재 규모의 시스템으로 설계된 프로젝트가 아닙니다.  
2025년에는 핵심 gameplay loop와 multiplayer 기반을 만들었고, 2026년에는 그 기반 위에 **콘텐츠 하나를 처음부터 결과 정산까지 완성하는 Dungeon Vertical Slice**를 추가하면서 구조를 다시 검증했습니다.

---

## 2025.03–04 — Core Foundation

2인 팀으로 시작해 Player와 World gameplay의 기본 구조를 구축했습니다.

주요 작업:

- Player movement / action
- `AAreaObject` 공통 framework
- Attribute / Skill / Combat
- DataTable 기반 gameplay data
- Interaction
- Resource
- Multiplayer authority 기본 원칙

이 시기에 만든 핵심 경계 중 일부는 이후 시스템에서도 유지됩니다.

예:

- ActorComponent 단위 기능 분리
- `TakeDamage` 기반 response
- Interface 기반 interaction
- Server authority / replication

---

## 2025.06 — Pal Lifecycle와 구조 분리

Capture / Inventory / Partner 기능을 Player에 직접 계속 추가하기보다 component로 분리했습니다.

- `UPalCaptureComponent`
- `UPalInventoryComponent`
- `UPalPartnerSkillComponent`

동시에:

- Capture reveal
- Pal Activate / Deactivate
- Partner AI
- IFF
- UI event 연결

을 정리했습니다.

이 시기의 가장 큰 변화는 “기능 구현”보다 **소유권과 lifecycle 기준으로 책임을 다시 나눈 것**입니다.

---

## 2025.07 — Combat / Item 확장

- Shotgun
- 다양한 Attack 형태
- Item rarity
- Combat feedback
- Damage / weak point / element
- Item drop / pickup flow

공격 종류가 늘어나면서 `FAttackData`, `UMeleeAttack`, Animation Notify 기반 timing 같은 공통 구조의 필요성이 커졌습니다.

---

## 2025.08 — Shared World Systems

- Container
- Crafting
- 협력 작업
- Crafting UI
- Queue / Collect
- Inventory / Container interaction

개인 소유 state와 shared world state가 처음 본격적으로 충돌한 시기입니다.

그 결과:

- Inventory owner-only replication
- Container subscriber-based replication
- Crafting `UIOwner` 동시성 제어

처럼 system별 network policy가 분화됐습니다.

---

## 2025.09–10 — Networking / UI 안정화

- Inventory FastArray
- Client prediction
- Skill FastArray spec
- Skill Grant / Revoke
- Crafting interaction 개선
- Event-driven UI
- UI pooling
- Wiki / source documentation

특히 UI에서는 “Widget에서 gameplay state를 직접 polling하지 않는다”는 방향을 정리했습니다.

---

# 2026 — Forgotten Ruins Vertical Slice

2026년에는 새로운 개별 feature를 여러 개 추가하는 대신, 기존 시스템이 실제 콘텐츠 하나에서 함께 동작하도록 Dungeon을 만들었습니다.

---

## 2026.09.15 — Dungeon Runtime

첫 단계에서 구현한 핵심 flow:

```text
Catalog
 → PrimaryAsset
 → Definition DataAsset
 → Server Runtime
 → Objective / Condition / Transition
 → Replicated Snapshot
 → Client UI
```

동시에 Shortcut / ExtraWave 두 경로와 성공/실패 path를 테스트 공간에 연결했습니다.

---

## 2026.09.16–17 — Content Flow 완성

추가된 항목:

- Dungeon Entrance / Switch interaction
- Physical branch layout
- Branch-specific reward
- Result settlement
- Required Level
- Stage timeout
- SaveGame progress
- Definition validation / graph

이 시점부터 Dungeon은 단순 Stage FSM이 아니라 **입장 → 진행 → 분기 → 실패/성공 → 정산 → 기록**을 가진 콘텐츠 단위가 됐습니다.

---

## 2026.09.26–28 — World / HUD / Barrier

- Modular dungeon space
- Island ↔ Dungeon Portal
- HUD 구조 개편
- Objective hierarchy
- Optional objective
- Stage barrier
- Barrier rule의 Definition 이동
- Dungeon identifier의 GameplayTag 전환

Barrier를 Actor 내부에 stage별로 하드코딩했다가 Definition의 `SealedBarriers`로 옮긴 것은 중요한 리팩토링입니다.

**“현재 Stage에서 어떤 문이 닫혀야 하는가”는 World Actor보다 Dungeon Definition이 소유해야 한다**고 판단했습니다.

---

## 2026.09.28–30 — Guardian Boss

일반 Monster 스킬 조합에서 별도 Boss Runtime으로 확장했습니다.

- `UBossFSM`
- `UBossPatternDataAsset`
- Telegraph
- Phase 2
- Re-aim / movement
- Leap / Charge
- Down / Exhaust
- Capture Window
- HUD Boss state

Boss도 Dungeon Runtime에 별도 특수 UI callback을 직접 넣지 않고 Snapshot의 Boss state를 통해 presentation에 연결했습니다.

---

## 2026.09.30–10.01 — Navigation / Result

- Dungeon UI icon
- Objective world marker
- Minimap
- Player / remote marker
- elapsed time
- best record
- grade
- result comparison

Minimap은 수동 이미지 위에 임의 좌표를 찍는 대신 Dungeon을 생성하는 동일 cell grid의 데이터를 기반으로 map data를 만들었습니다.

---

## 2026.10.02–03 — Workflow / Common Systems

- Agent MCP project config
- Boss presentation polish
- Dungeon Toast → `UNoticeSubsystem`
- Notice의 Level-up / Capture / Crafting / Region 확장
- StringTable localization

Dungeon을 위해 만든 기능이 범용성이 생긴 경우 Dungeon namespace에 남겨두지 않고 common system으로 이동했습니다.

`UNoticeSubsystem`이 대표적입니다.

---

# Retrospective

## 1. “처음부터 완벽한 구조”보다 ownership을 계속 수정했다

Sonheim의 구조는 한 번 설계하고 유지된 것이 아닙니다.

예:

- Barrier state → Actor에서 Definition으로 이동
- Dungeon Toast → NoticeSubsystem으로 이동
- String literal → StringTable
- 문자열 ID → GameplayTag tree
- Skill instance → 필요 시 생성
- UI direct state access → Presenter/ViewData가 필요한 영역 분리

기능이 늘어날수록 **현재 책임이 어느 layer에 있어야 하는가**를 다시 판단했습니다.

---

## 2. Data-driven은 DataTable 하나를 의미하지 않는다

초기에는 DataTable이 대부분의 gameplay data를 담당했습니다.

Dungeon을 만들면서 다음 요구가 생겼습니다.

- 콘텐츠 identity
- asset dependency
- nested stage graph
- soft loading
- validation
- presentation 분리

그래서 DataTable을 버린 것이 아니라:

- Row data → DataTable
- Content definition → PrimaryDataAsset
- Runtime identifier → GameplayTag
- Player text → StringTable

로 역할을 나눴습니다.

---

## 3. Server Authority만으로 UX가 좋아지지는 않는다

모든 결과를 Server가 결정해도 Client가 매번 round-trip을 기다리면 Inventory 같은 UI는 답답해집니다.

그래서:

- 결과 권위 → Server
- 즉각적인 조작 피드백 → Client Prediction
- 최종 일치 → Replication

으로 역할을 분리했습니다.

---

## 4. 재사용 여부가 추상화의 실제 검증이었다

2025년에 만든 시스템 중 2026 Dungeon에서 다시 사용된 것:

- Interaction → Portal / Lever / Chest
- Inventory → Reward
- Capture → Boss Capture
- Monster lifecycle event → Objective
- Damage / Skill → Boss Combat
- Notice → 여러 gameplay producer

새 콘텐츠에서 재사용되지 못한 추상화는 다시 경계를 조정했습니다.

---

## 5. 자동 검증은 기능 규모가 커질수록 중요해졌다

Dungeon은 한 path만 확인해서는 충분하지 않습니다.

- Shortcut / ExtraWave
- Success / Timeout / Death / Leave
- Server / Client
- HUD / Result
- Definition validity
- Boss state

조합이 늘어나면서 반복 가능한 verification과 editor-side validation의 가치가 커졌습니다.

자세한 내용은 [[Development Workflow & Verification|13_Development_Workflow_Verification]]에서 다룹니다.

---

## 현재 남아 있는 개선 지점

문서에서는 구현된 내용을 과장하지 않고 현재 trade-off도 함께 남깁니다.

예:

- Skill Cost는 Item 부분 rollback은 지원하지만 Stamina까지 포함한 전체 transaction rollback은 아님
- Condition timer는 동일 condition의 여러 source 중첩에 한계가 있음
- Animation-driven gameplay는 off-screen server animation tick 비용을 요구함
- 일부 자동 verification script는 public main repository에 포함돼 있지 않음

이런 항목은 “향후 계획” 목록보다 각 시스템의 실제 설계 제약으로 관리합니다.

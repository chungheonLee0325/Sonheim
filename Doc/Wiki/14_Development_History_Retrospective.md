# 14. Development History & Retrospective

## 2025 — Core Gameplay Systems

초기 팀 개발과 이후 개인 확장을 통해 다음 기반을 구축했습니다.

- Server-authoritative multiplayer
- Player / Combat / Skill
- Inventory / Equipment
- Pal Capture / Partner
- Interaction / Item / Resource
- Container / Crafting
- Steam Session
- UMG

이 시기의 핵심은 **각 gameplay system이 multiplayer에서 실제로 동작하도록 기본 구조를 완성하는 것**이었습니다.

---

## 2026 — Content Runtime Vertical Slice

기존 시스템 위에서 Forgotten Ruins Dungeon을 end-to-end 콘텐츠로 구축했습니다.

### Runtime

- Catalog → PrimaryAsset → Definition
- Event / Condition / Action / Transition
- Shortcut / ExtraWave branch
- Time limit / failure
- Reward / Result
- Persistent record / Grade

### World

- Portal
- Hidden dungeon space
- Shortcut / Lever
- Stage barrier
- Objective marker
- Minimap

### Boss

- Grizzbolt dedicated Boss FSM
- Data-driven Pattern
- Telegraph
- Phase / Rage
- Down / Exhaust
- Capture integration

### UI

- Replicated Snapshot
- Presenter / ViewData
- Participant-aware HUD
- Objective hierarchy
- Party / Boss panel
- Result / Record
- Common Notice
- StringTable localization

### Authoring

- GameplayTag hierarchy
- Data Validation
- CallInEditor validation
- Mermaid Stage Graph

---

## 구조 변화에서 얻은 점

초기 Sonheim Wiki는 많은 기능을 각각 독립 시스템으로 설명했습니다.

프로젝트가 커진 뒤에는 **기능 수보다 시스템 경계와 재사용 결과가 더 중요한 정보**가 됐습니다.

예를 들어 Interaction의 가치는 Item prompt를 띄운다는 사실보다, 1년 뒤 Dungeon Portal / Switch에도 동일 contract를 적용할 수 있었다는 데 있습니다.

DataTable 역시 “모든 것을 DataTable로 만든다”가 목표가 아니라, 데이터 성격에 따라 DataTable / PrimaryDataAsset / GameplayTag / Soft Reference를 선택하는 쪽으로 발전했습니다.

---

## 문서 구조도 같은 기준으로 재편

기존 Wiki의 세부 페이지를 그대로 유지하면 구현량은 잘 보이지만 현재 중요한 설계가 묻힙니다.

따라서 새 Wiki는:

- Featured Engineering
- Architecture
- Gameplay Systems
- Engineering Workflow

중심으로 통합하고, 2025 세부 문서는 repository에 Legacy reference로 보존합니다.

---

## Future Work를 별도 목록으로 두지 않는 이유

과거 Wiki의 Future Work는 GAS 도입 등 당시의 가정을 별도 문서로 정리했습니다.

현재는 “언젠가 바꾸고 싶은 것”을 큰 페이지로 두기보다, 각 시스템 문서에서 실제 limitation / trade-off를 현재 코드와 함께 설명하는 편이 더 정확하다고 판단했습니다.

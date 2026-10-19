# Sonheim — UE5 Multiplayer Action Adventure

![Sonheim](Sonheim.png)

**Unreal Engine 5.5 · C++ · Listen Server 기반 멀티플레이 액션 어드벤처**

전투·장비·제작·몬스터 포획을 기반으로 도감, 퀘스트, 분기형 던전과 보스 전투를 구현했습니다. 초기 2인 팀 프로젝트에서 시작해 콘텐츠를 확장하며 **게임플레이 상태의 소유권, 서버 실행, 클라이언트 UI 갱신 경로**를 정리했습니다.

[**전체 플레이 영상**](https://www.youtube.com/watch?v=TDRRWp6M_9E) · [**기술 문서 01–21**](https://github.com/chungheonLee0325/Sonheim/wiki) · [**소스 코드 중심 보기**](https://github.com/chungheonLee0325/Sonheim.Source)

[![Sonheim 전체 게임플레이](Doc/Gifs/Project_Overview.gif)](https://www.youtube.com/watch?v=TDRRWp6M_9E)

## 주요 구현

| 시스템 | 구현한 내용 | 상세 문서 |
|---|---|---|
| **캐릭터·전투** | PlayerState의 보유 스탯과 Pawn의 행동 분리, 장비별 Skill Grant, Montage Notify 기반 서버 타격 판정 및 Socket Sweep 보간 | [04. Player](https://github.com/chungheonLee0325/Sonheim/wiki/04_Player_Character_Systems) · [05. Combat](https://github.com/chungheonLee0325/Sonheim/wiki/05_Combat_Skill_Animation) |
| **인벤토리·협동 제작** | OwnerOnly FastArray 슬롯 복제, 월드 보관함, 공동 작업량 누적, 부분 완료·수령·미완료분 환불 | [07. Inventory & Crafting](https://github.com/chungheonLee0325/Sonheim/wiki/07_Inventory_Crafting) |
| **포획·Partner** | 서버 포획 판정과 Reveal 동기화, Monster Actor 활성/비활성 재사용, 소유자에 따른 AI·피아식별 전환 | [08. Pal Capture](https://github.com/chungheonLee0325/Sonheim/wiki/08_Pal_Capture_Partner_Lifecycle) |
| **분기 던전·보스** | DataAsset의 Event/Action/Transition, 서버 Event Queue·RunId, PatternClock 기반 보스 경고·타격·Down/Resting | [09. Dungeon](https://github.com/chungheonLee0325/Sonheim/wiki/09_Branching_Dungeon_Runtime) · [10. Boss](https://github.com/chungheonLee0325/Sonheim/wiki/10_Boss_Encounter_Runtime) |
| **도감·퀘스트** | 처치·포획·제작·지역 도착 이벤트를 플레이어별 기록·단계 목표에 연결, 데이터 정의와 ViewData 분리 | [16. MonsterDex](https://github.com/chungheonLee0325/Sonheim/wiki/16_MonsterDex_System) · [17. Quest](https://github.com/chungheonLee0325/Sonheim/wiki/17_Quest_System) |
| **지도·빠른 이동** | 플레이어별 탐험 안개를 미니맵·월드맵에 공유, 퀘스트·도감 서식지 마커와 표석 활성화 기록 연동 | [19. World Map](https://github.com/chungheonLee0325/Sonheim/wiki/19_World_Map_System) |

## UI 구조와 성능

게임플레이 컴포넌트와 서버 Runtime이 원본 상태를 관리하고, 클라이언트는 Delegate 또는 복제된 상태를 받아 화면을 갱신합니다. 던전 HUD처럼 여러 출처의 값이 필요한 화면은 **GameState Snapshot → Presenter → ViewData → Widget** 경로로 구성합니다.

- **[GameUIFramework](https://github.com/chungheonLee0325/Sonheim/wiki/21_UI_Framework):** 화면을 GameplayTag로 열고 HUD·Menu·Popup·Toast 레이어, 입력 모드·커서, 뒤로 가기, 비동기 화면 로딩과 재사용을 관리하는 UE 플러그인
- **공통 알림:** `UNoticeSubsystem`의 게임 알림 데이터를 `UGameUIToastSubsystem`에서 Feed·Headline·Title 슬롯 정책에 맞춰 표시. 키보드·게임패드 입력 안내는 `UInputHintSubsystem`에서 관리
- **화면 구현:** [Client State Presentation](https://github.com/chungheonLee0325/Sonheim/wiki/11_Client_State_Presentation_Pipeline) · [UMG v2 변경 전후](https://github.com/chungheonLee0325/Sonheim/wiki/18_UMG_v2_Migration)

### UI 조작 순간의 지연 개선

RTX 2070 SUPER, 1920×1080 독립 실행 환경에서 조작 이벤트가 발생한 프레임 시간의 중앙값을 비교했습니다. 별도로 측정한 Game Thread 시간과 수집 조건은 Wiki 20에 정리했습니다.

| 조작 | 변경 전 | 변경 후 |
|---|---:|---:|
| 아이템 교환 | 32.6ms | **17.1ms** |
| 인벤토리 열기 | 57.5ms | **22.1ms** |

CSV·Unreal Insights로 원인을 조사하고 슬롯 갱신·화면 재생성 경로를 수정했습니다. 측정 조건, 원인 분석과 비교 결과는 [20. UI Performance](https://github.com/chungheonLee0325/Sonheim/wiki/20_UI_Performance_Profiling)에 정리했습니다.

## 화면과 플레이 시연

| **MonsterDex — 발견·기록·보상** | **Quest — 단계 목표·월드 변화** |
|---|---|
| ![도감 목록·3D 몬스터·처치/포획 기록·보상](Doc/Media/Wiki/16_MonsterDex_System/16_dex_screen_areas.svg) | ![퀘스트 수락·완료 보고 화면](Doc/Media/Wiki/16_UMG_Migration_with_Claude/16_dialog_after.jpg) |
| 처치·포획 기록에 따라 도감 페이지와 단계별 보상을 갱신. [도감 등록 시연](https://github.com/user-attachments/assets/d1d96b5f-1076-4e29-872d-3399a540acac) | NPC 수락부터 제작·전투 목표, 자재 전달과 완료 보고까지. [퀘스트 시연](https://github.com/user-attachments/assets/356d1438-7c3c-417d-8bd4-b8dcb7a3efb6) |

| **World Map — 탐험·빠른 이동** | **Forgotten Ruins — 분기 던전** |
|---|---|
| ![탐험 범위와 이동 지점이 표시된 월드맵](Doc/Media/Wiki/19_World_Map_System/19_world_map.jpg) | ![스테이지·목표·미니맵 HUD](Doc/Media/Wiki/16_UMG_Migration_with_Claude/16_dungeon_hud_after.jpg) |
| 서버에서 기록한 탐험 안개·표석 활성화와 마커를 지도에 표시. [지도 구현](https://github.com/chungheonLee0325/Sonheim/wiki/19_World_Map_System) | 레버 사용 여부를 Wave 종료 시 평가하여 지름길·추가 웨이브 분기. [지름길](https://github.com/user-attachments/assets/29858da6-7d13-4297-9d71-41f53272a952) · [추가 웨이브](https://github.com/user-attachments/assets/af8228c3-f403-41e1-947b-ba3f3d3f5da6) |

![도감 등록 시 공통 알림 Feed·Headline·Title](Doc/Media/Wiki/16_UMG_Migration_with_Claude/16_toast_after_dex_title.jpg)

**기타 시연:** [협동 제작](https://github.com/user-attachments/assets/df9a30d1-8d52-4b86-9f18-790474fdbda2) · [몬스터 포획](https://github.com/user-attachments/assets/57246d79-bd3b-473f-85fc-762670023729) · [보스 텔레그래프·타격](https://github.com/user-attachments/assets/cd81b345-d192-4374-9573-904b561f40bd)

## 기술 문서 및 개발 환경

구현별 상세 설계와 실제 코드 링크는 [**Wiki 전체 목차(01–21)**](https://github.com/chungheonLee0325/Sonheim/wiki)에서 확인할 수 있습니다.

- **아키텍처·데이터:** [02. Gameplay Architecture](https://github.com/chungheonLee0325/Sonheim/wiki/02_Gameplay_Architecture) · [03. Data & Content Architecture](https://github.com/chungheonLee0325/Sonheim/wiki/03_Data_Content_Architecture)
- **멀티플레이:** [12. Multiplayer Synchronization](https://github.com/chungheonLee0325/Sonheim/wiki/12_Multiplayer_Synchronization) — Server RPC, FastArray, UObject 복제, OwnerOnly, GameState Snapshot
- **콘텐츠 검증·개발 기록:** [13. Content Validation](https://github.com/chungheonLee0325/Sonheim/wiki/13_Content_Authoring_Validation) · [15. Development History](https://github.com/chungheonLee0325/Sonheim/wiki/15_Development_History_Retrospective)
- **에디터 도구:** [AgentMcp](https://github.com/chungheonLee0325/AgentMcp) — UE Editor MCP를 통한 에셋·Widget·Blueprint 조사·편집 및 성능 분석

**개발 환경:** Unreal Engine 5.5 · C++ · Blueprint/UMG · Visual Studio 2022 / Rider · Steam/Null OSS

**프로젝트:** `Sonheim/Sonheim.uproject` — 프로젝트 파일로 IDE 솔루션을 생성하고 `Development Editor` 구성에서 빌드합니다. 멀티플레이는 Listen Server와 클라이언트 PIE 환경에서 사용했습니다.

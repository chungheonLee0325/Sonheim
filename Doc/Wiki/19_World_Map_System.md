# 19. World Map & Fast Travel

섬의 지도는 **한 장의 지도 정의(`DA_WorldMap`)** 를 중심으로 만듭니다. 월드 좌표와 지도 좌표의 변환, 마커 모양, 안개 격자 크기를 이 데이터 애셋 하나가 알고 있고, HUD의 원형 미니맵과 메뉴의 지도 탭은 같은 머티리얼(`M_UI_Map`)과 같은 마커 목록을 읽어 그립니다. 플레이어가 지나간 자리의 안개는 서버가 기록·저장하고, 빠른 이동은 플레이어가 직접 활성화한 표석으로만 허용합니다.

## 실제 지도 화면

| 지도 탭 | 필드 HUD의 미니맵 |
|---|---|
| ![전체 화면 지도: 탐험한 자리만 걷힌 안개, 지역 이름, 마커, 오른쪽 패널과 아래 키 안내](../Media/Wiki/19_World_Map_System/19_world_map.jpg) | ![오른쪽 위 원형 미니맵과 그 아래 퀘스트 추적 카드](../Media/Wiki/19_World_Map_System/19_hud_minimap.jpg) |
| 플레이어가 지나간 자리만 안개가 걷히고, 오른쪽 패널에 커서 아래 지역·활성화한 표석·범례가 뜹니다. | 북쪽 고정 원형 미니맵입니다. 화살표는 시점 방향으로 돌고, 추적 퀘스트의 목적지는 핀으로 표시합니다. |

| 빠른 이동 표석 — 활성화 전과 후 | 도감에서 서식지 보기 |
|---|---|
| ![빨간 빛기둥의 비활성 표석과 파란 빛기둥의 활성 표석](../Media/Wiki/19_World_Map_System/19_stone_states.jpg) | ![새싹콩 서식지 배너와 서식지 표시가 뜬 지도](../Media/Wiki/19_World_Map_System/19_habitat_map.jpg) |
| 표석 앞에서 F로 활성화하면 그 플레이어에게만 빛기둥이 빨강에서 파랑으로 바뀝니다. | 도감의 `지도에서 보기`가 지도 탭을 열고 서식지 지역에 표시를 띄웁니다. |

## 목차

- [지도 그림과 좌표 — 한 곳만 아는 변환](#지도-그림과-좌표--한-곳만-아는-변환)
- [지역과 탐험 — 두 가지 "발견"](#지역과-탐험--두-가지-발견)
- [안개 렌더링 — 격자 텍스처와 머티리얼](#안개-렌더링--격자-텍스처와-머티리얼)
- [마커 — 액터가 스스로 등록하는 목록](#마커--액터가-스스로-등록하는-목록)
- [미니맵과 지도 탭](#미니맵과-지도-탭)
- [빠른 이동 표석](#빠른-이동-표석)
- [도감 서식지 연결](#도감-서식지-연결)
- [실행 검증](#실행-검증)
- [기술적 선택 요약](#기술적-선택-요약)
- [관련 코드](#관련-코드)

---

## 지도 그림과 좌표 — 한 곳만 아는 변환

### 탑뷰 캡처를 바탕으로 그린 지도

지도 그림은 손으로 처음부터 그리지 않고 **실제 레벨을 위에서 찍은 사진**에서 시작했습니다. 그래서 해안선과 바위 배치가 게임 월드와 일치합니다.

1. `capture_topdown.py` — 레벨 위 60,000 cm에 직교(Orthographic) `SceneCapture2D`를 놓고 안개·대기·구름 표시를 끈 채 2048 px로 찍습니다. 찍은 범위(월드 X −41,800 ~ −15,800, Y −2,000 ~ 24,000)를 그대로 지도 범위로 씁니다.
2. 이미지 생성 도구로 **배치는 그대로 두고** 소품·잡음을 지운 손그림 지도로 다듬었습니다.
3. `bake_map_art.py` — 2048로 맞추고, 원본 캡처를 겹친 확인용 이미지(지역 상자 포함)를 만들어 어긋남을 눈으로 확인합니다.

![원본 탑뷰 캡처, 다듬은 지도, 둘을 겹치고 지역 상자를 그린 확인 이미지](../Media/Wiki/19_World_Map_System/19_art_pipeline.jpg)

*왼쪽부터 원본 직교 캡처 · 이미지 생성으로 다듬은 지도 · 두 이미지를 겹치고 지역 상자를 그린 확인 이미지입니다.*

### 좌표 변환은 `UWorldMapDefinition` 한 곳에

캡처 카메라의 위쪽이 월드 +X, 오른쪽이 +Y입니다. 이 약속을 미니맵·지도 탭·마커·안개가 각자 계산하면 하나만 틀려도 마커가 엉뚱한 곳에 찍히므로, **변환은 지도 정의에만 둡니다.**

~~~cpp
// WorldMapDefinition.h — UPROPERTY·UFUNCTION 매크로 생략
FVector2D ToUV(const FVector& World) const
{
    return FVector2D((World.Y - WorldMin.Y) / (WorldMax.Y - WorldMin.Y),
                     (WorldMax.X - World.X) / (WorldMax.X - WorldMin.X));
}
bool Shows(const FVector& World) const;   // 지도 밖(던전)인지
~~~

`Shows`는 "이 지점이 섬 지도에 그려지는가"를 답합니다. 던전은 같은 월드 안이지만 지도 범위 밖에 있으므로, 미니맵 숨김과 빠른 이동 거부가 모두 이 함수 하나로 결정됩니다.

| 선택 | 대안 | 이유 |
|---|---|---|
| 실제 레벨 탑뷰 캡처 + 이미지 생성으로 다듬기 | 지도를 처음부터 그리기 | 해안선·바위가 월드와 어긋나지 않고, 레벨이 바뀌면 다시 찍으면 됨 |
| 좌표 변환을 지도 정의 한 곳에 | 위젯마다 변환 계산 | 축 방향 약속(+X 위, +Y 오른쪽)이 한 곳에만 존재 |

## 지역과 탐험 — 두 가지 "발견"

지도에는 성격이 다른 두 종류의 "발견"이 있습니다.

| | 지역 (`ARegionZone`) | 탐험 칸 (안개) |
|---|---|---|
| 단위 | 이름 붙은 상자 (오두막 야영지, 해변 들판, 바위 동굴 …) | 지도를 64×64로 나눈 칸 |
| 기록 시점 | 지역에 들어갈 때 (`EPlayerDeed::Reach`) | 서버가 0.25초마다 Pawn 주변 40 m |
| 쓰임 | 지역 이름·`???`, 퀘스트 `Reach` 목표, 도감 서식지 | 지도·미니맵의 안개 |

### 처음에는 지역 단위 안개였다

첫 구현은 지역 상자마다 안개를 걷었습니다. 지역 번호를 픽셀에 담은 마스크 텍스처를 만들고, 머티리얼이 발견한 지역의 픽셀만 밝히는 방식이었습니다. 실제로 걸어 보니 두 가지 문제가 있었습니다.

- 지역 경계 바로 앞에 서면 **미니맵 안에 안개가 보여** 어색했습니다.
- 넓은 해변이 한 지역이라 시작 오두막과 몬스터 들판이 함께 걷혔습니다. 지역을 세분하려니 머티리얼 파라미터 4개(지역 4개)가 상한이었습니다.

그래서 지역은 이름·퀘스트·서식지에만 쓰고, 안개는 **탐험 반경** 방식으로 바꿨습니다. 해변은 `오두막 야영지`(시작 지점·제작대·섬지기)와 `해변 들판`으로 나눴습니다.

### 서버가 기록하는 탐험 칸

`UWorldMapComponent`(PlayerState)는 서버에서 0.25초마다 Pawn 위치 주변의 칸을 비트로 기록합니다. 64×64 칸은 **512바이트**이고, 소유 클라이언트에만 복제됩니다.

~~~cpp
// WorldMapComponent.cpp — Explore 핵심
Explored.SetNumZeroed(FMath::DivideAndRoundUp(Definition->FogCells * Definition->FogCells, 8));
for (const int32 Cell : Definition->CellsNear(Pawn->GetActorLocation()))
{
    if (IsExplored(Cell)) continue;
    Explored[Cell / 8] |= 1 << (Cell % 8);
    bChanged = true;
}
if (!bChanged) return;
OnMapChanged.Broadcast();
bUnstored = true;
if (GetWorld()->GetTimeSeconds() - LastStored > 5.0) Store();   // 걷는 내내 저장하지 않도록 5초 간격 + EndPlay
~~~

- **탐험 반경 40 m는 미니맵 반지름(35 m)보다 넓습니다.** 그래서 미니맵 안에서는 플레이어 주변 안개가 보이지 않습니다.
- 위치는 서버가 직접 읽습니다. 클라이언트가 "여기 탐험했다"고 보고하지 않으므로 조작할 수 없습니다.
- 저장은 호스트의 `WorldMap` 슬롯에 지역·표석 목록과 함께 들어갑니다. 고유 ID가 정해지기 전에 기록된 칸(스폰 지점)은 저장 기록을 읽을 때 OR로 합칩니다.

~~~mermaid
flowchart LR
    A["서버 0.25초 타이머<br/>Pawn 위치"] --> B["CellsNear<br/>반경 40 m 칸"]
    B --> C["Explored 비트 512 B"]
    C -->|"COND_OwnerOnly"| D["소유 클라이언트 OnRep"]
    C -->|"5초 간격·EndPlay"| E["WorldMap SaveGame"]
    D --> F["UMapFog 텍스처"]
    F --> G["미니맵·지도 탭 M_UI_Map"]
~~~

### 스폰한 지역도 발견

처음 검증에서 **시작 오두막에서 스폰한 플레이어가 그 지역을 발견하지 못하는** 문제가 나왔습니다. 지역 발견은 "들어가는 순간(Begin Overlap)"에 일어나는데, 지역 안에서 생성된 Pawn은 들어간 적이 없기 때문입니다. 막 생성된 Pawn은 겹침 목록도 아직 비어 있어서, Pawn이 정해질 때(`OnPawnSet`) 지역 상자에 위치가 들어 있는지를 직접 묻도록 고쳤습니다.

~~~cpp
// WorldMapComponent.cpp
void UWorldMapComponent::HandlePawnSet(APlayerState*, APawn* NewPawn, APawn*)
{
    // 막 생성된 Pawn은 겹침 정보가 없으므로 지역 상자에 직접 묻는다.
    if (!NewPawn) return;
    for (TActorIterator<ARegionZone> It(GetWorld()); It; ++It)
        if (It->Contains(NewPawn->GetActorLocation())) Discover(It->RegionId);
}
~~~

## 안개 렌더링 — 격자 텍스처와 머티리얼

### `UMapFog` — 칸마다 따로 밝아지는 64×64 텍스처

클라이언트는 받은 비트를 한 칸이 한 텍셀인 `PF_G8` 텍스처로 만듭니다. "스르륵" 걷히는 연출은 **칸마다 현재 밝기를 목표 밝기로 조금씩 옮기는 방식**입니다.

~~~cpp
// MapFog.cpp — Apply 핵심
const int32 Step = FMath::Max(1, FMath::RoundToInt(255.f * DeltaSeconds / FadeSeconds));
for (int32 Cell = 0; Cell < Shown.Num(); ++Cell)
{
    if (Shown[Cell] == Target[Cell]) continue;
    Shown[Cell] = FMath::Min(Shown[Cell] + Step, (int32)Target[Cell]);
    bFading |= Shown[Cell] != Target[Cell];
}
Upload();   // UpdateTextureRegions — 4 KB
~~~

처음 생각한 방식은 "이전 텍스처와 새 텍스처를 1초 동안 섞기"였습니다. 하지만 걷는 동안 0.25초마다 새 칸이 들어와 섞기가 계속 처음부터 다시 시작되고, 이미 반쯤 걷힌 칸이 한 번에 튀어 보입니다. 칸마다 따로 진행하면 늦게 들어온 칸도 제 속도로 걷힙니다. 지도를 처음 열 때는 이미 탐험한 칸을 페이드 없이 바로 보여줍니다.

### `M_UI_Map` — 미니맵과 지도 탭이 함께 쓰는 머티리얼

머티리얼은 Custom HLSL 노드 하나로 작성했습니다. 미니맵(`Round = 1`, 원형 테두리)과 지도 탭(`Round = 0`, 화면 비율 `Aspect`)이 같은 머티리얼을 파라미터만 바꿔 씁니다.

~~~hlsl
// M_UI_Map 안개 부분 — 요약 (전체 HLSL은 author_minimap.py)
float lit = 5탭 평균(Fog, saturate(p));              // 쌍선형 + 살짝 흐림
lit = smoothstep(0.3, 0.7, lit + (n - 0.5) * 0.35);   // n: 값 노이즈 → 구름 같은 가장자리
float2 m = 1 - abs(1 - abs(p));                        // 그림 밖은 거울 반사로 이어서
float g = 밝기(Texture2DSample(Art, ArtSampler, m));
c = lerp(안개색(g) * (0.34 + 0.16 * cloud), c, lit);   // cloud: 굵은 노이즈로 얼룩
~~~

- **64×64 격자의 계단 모양**은 쌍선형 보간·5탭 흐림·노이즈로 숨깁니다. 결과는 스크린샷처럼 구름 가장자리처럼 보입니다.
- 안개 색은 아래 그림의 명암을 따라가므로 바위 윤곽이 희미하게 비칩니다. 지도 탭에서 화면이 그림보다 넓을 때는 그림을 거울 반사해 명암을 이어서, 그림 경계에 이음매가 생기지 않습니다.

### 작성 과정에서 부딪힌 엔진 제약

머티리얼은 에디터 Python으로 노드를 만들어 작성했고, 다음 두 가지에 걸렸습니다.

| 문제 | 원인 | 해결 |
|---|---|---|
| 벡터 파라미터의 알파(`.w`)를 HLSL에서 쓰면 컴파일 실패 | Vector Parameter 출력 핀 이름이 모두 비어 있어 Python에서 RGBA 핀을 지정할 수 없음 | 스칼라 파라미터로 분리 |
| 머티리얼 재작성 중 에디터가 `!IsRooted()` 어서트로 종료 | HUD 위젯 CDO가 머티리얼을 잡고 있어 노드 삭제 불가 | 기존 머티리얼은 Custom 노드 코드만 갱신, 입력이 바뀔 때만 에디터를 닫고 애셋을 다시 만듦 |

## 마커 — 액터가 스스로 등록하는 목록

### `UMapMarkerComponent`

플레이어·섬지기·던전 포탈·빠른 이동 표석은 `UMapMarkerComponent`를 붙이고, `BeginPlay`에서 `UWorldMapSubsystem`의 목록에 스스로 등록합니다. 지도 위젯은 목록만 읽으므로 **어떤 액터 클래스가 있는지 알지 못합니다.** 새로운 종류의 표시 대상이 생겨도 컴포넌트만 붙이면 됩니다.

- `Kind` — 아이콘·색·크기는 `DA_WorldMap.MarkerStyles`에서 종류별로 정하므로, 미니맵과 지도 탭이 같은 모양을 씁니다.
- `DiscoveryId` — 값이 있으면 그 플레이어가 발견·활성화한 뒤에만 보입니다. 표석이 이 방식을 씁니다.
- 추적 퀘스트 목적지(`QuestTarget`)는 컴포넌트가 아니라 퀘스트 기록에서 계산합니다. `Reach`는 지역 중심, `Talk/Deliver`는 대상 NPC, `ClearDungeon`은 던전 포탈, 완료 보고 대기는 보고 대상 위치입니다.

### 겹침 정리 — 우선순위, 핀 끝, 마우스 오버

같은 자리에 아이콘이 몰리면 읽기 어렵습니다(유적의 문: 던전 포탈 + 퀘스트 핀 + 지역 이름). 다음 규칙으로 정리했습니다.

| 규칙 | 내용 |
|---|---|
| 종류별 그리기 순서 | `LayerOf`: 추적 퀘스트 핀 > 플레이어 > 표석 > 의뢰인 > 던전, 지역 이름은 항상 아래 |
| 핀 끝 기준 배치 | 퀘스트 핀은 아래쪽 끝(`PinTip`)이 목적지를 가리켜 같은 자리의 아이콘을 덮지 않음 |
| 마우스 오버 | 커서 아래 아이콘이 1.35배·맨 위로 올라오고 이름(플레이어 이름, 표석·NPC 이름)을 띄움 |
| 지역 이름 | 어두운 반투명 판 위에 표시, 미탐험 `???`는 흐리게, 작은 지역은 확대했을 때만, 아이콘과 겹치면 아래로 비킴 |

아이콘은 `FMapIconLayer`가 이미지 위젯을 프레임마다 다시 쓰는 풀로 그립니다. 마커 수만큼만 `UImage`가 생기고 이후 프레임에서는 위치·크기·Z 순서만 갱신합니다.

## 미니맵과 지도 탭

### 미니맵 (`UMinimapWidget`)

- **북쪽 고정**입니다. 지도는 돌리지 않고 플레이어 화살표만 컨트롤러 Yaw로 돌립니다(Yaw 0 = +X = 지도 위쪽).
- 반경 밖의 추적 퀘스트 목적지는 **테두리에 붙여** 방향을 알려 줍니다.
- 지도 밖(던전)에서는 `Shows`가 거짓이 되어 미니맵을 숨깁니다.
- 퀘스트 추적 카드는 미니맵 아래로 옮겼습니다.

### 지도 탭 (`UWorldMapWidget`)

메뉴 탭(인벤토리·도감·퀘스트·**지도**)의 하나로 `UUIStackSubsystem`에 등록했고, M 키로 바로 열 수도 있습니다. K·J·M 탭 키는 `ToggleTab` 하나로 정리했습니다.

- **전체 화면 지도와 떠 있는 패널.** 지도는 탭 바 아래를 모두 채우고, 지역·표석·범례 패널은 오른쪽에 반투명으로 띄웁니다. 키 안내는 지도 아래 한 줄 막대입니다.
- **화면 비율.** 위젯이 정사각형이 아니어도 그림이 늘어나지 않도록 시야를 `Span`(세로) × `Span × Aspect`(가로)로 계산합니다. 화면 좌표 ↔ 지도 UV 변환(`ViewSize`·`ImageToUV`)은 이 한 쌍의 함수만 씁니다.
- **입력.** 마우스는 드래그로 이동하고 휠은 커서 지점을 고정한 채 확대합니다. 패드는 `IMC_Menu`의 `IA_UI_MapPan`(왼쪽 스틱)·`IA_UI_MapZoom`(트리거) 값을 Tick에서 읽는데, 현재 입력 장치가 패드일 때만 읽습니다.
- **키 표시.** 메뉴가 열려 있으면 `IMC_Menu`가 F를 확인 키로 가져가 게임 상호작용 동작에 키가 없어집니다. `UInputHintSubsystem`은 이때 게임 입력 컨텍스트(`IMC_Default`)에서 키를 찾아, "표석에서 [F] 눌러 활성화"처럼 게임 키를 안내할 수 있습니다.

## 빠른 이동 표석

### 상호작용으로 활성화

`AFastTravelPoint`는 처음에 "다가가면 자동 발견" 방식이었습니다. 플레이해 보니 언제 켜졌는지 알기 어려워서, 포탈과 같은 `IInteractableInterface`로 바꿨습니다.

~~~mermaid
flowchart TD
    A["표석 앞에서 F"] --> B["서버: Interact<br/>거리·생존 확인"]
    B --> C{"이미 활성화했는가?"}
    C -->|아니요| D["Discover(PointId)<br/>알림 · 저장 · OwnerOnly 복제"]
    C -->|예| E["Client_OpenTab(WorldMap)<br/>그 자리에서 지도 열기"]
    D --> F["그 클라이언트에서만 빛기둥 빨강 → 파랑 + 1.2초 섬광"]
    E --> G["지도의 표석 목록 클릭 / 확인 키"]
    G --> H["ServerFastTravel(PointId)"]
~~~

`ServerFastTravel`은 클라이언트 요청을 그대로 믿지 않고 다음을 확인합니다.

| 검사 | 막는 경우 |
|---|---|
| `IsDiscovered(PointId)` | 활성화하지 않은 표석으로 이동 |
| Pawn 유효·생존 | 쓰러진 상태에서 이동 |
| `Shows(Pawn 위치)` | 던전 안에서 지도로 탈출 — 던전은 포탈로 나가야 런이 정상 종료 |

도착 처리는 던전 포탈과 같습니다. 화살표 위치로 `TeleportTo`, 컨트롤러 회전 맞추기, 화면 페이드입니다.

### 플레이어별로 다른 빛기둥

활성화 여부는 플레이어마다 다르므로 빛기둥 색은 서버가 복제하는 값이 아닙니다. **각 클라이언트가 자기 플레이어의 기록을 보고** 칠합니다. 같은 표석이 한 사람에게는 빨강, 다른 사람에게는 파랑일 수 있습니다.

연출은 Niagara 없이 머티리얼로 작게 시도했습니다. 프로젝트에 있던 스파크 이펙트는 표석에 비해 너무 크고 색 파라미터를 조절할 수 없었기 때문입니다.

- `M_TravelBeam` — 가산 무광 원기둥. 아래쪽과 중심이 밝고, 위로 흐르는 빛줄기가 있습니다.
- `M_TravelRing` — 바닥 5 m 판에 바깥으로 퍼지는 링
- 활성화 순간 `Flash` 파라미터와 점광원이 1.2초 동안 밝아졌다 잦아듭니다.

![활성화 순간: 빨강에서 파랑으로 바뀌며 기둥과 링이 밝아졌다 잦아드는 연속 캡처](../Media/Wiki/19_World_Map_System/19_stone_flash.jpg)

*활성화 직후를 0.15초 간격으로 찍은 연속 캡처입니다. 빛기둥 색이 바뀌고 바닥 링이 퍼집니다.*

## 도감 서식지 연결

도감 페이지(`FMonsterDexData`)에 `Habitats`(지역 ID 목록, 던전은 `Dungeon`)를 추가했습니다. 값은 플레이 중 실제 몬스터 위치를 지역 상자로 판정해 정했습니다(예: 새싹콩 = 해변 들판 · 이끼 골짜기).

| 도감 상세의 서식지 줄 | 지도에서 보기 |
|---|---|
| ![도감 상세: 서식지와 지도에서 보기 버튼](../Media/Wiki/19_World_Map_System/19_dex_habitat.jpg) | ![지도: 서식지 배너와 지역 표시](../Media/Wiki/19_World_Map_System/19_habitat_map.jpg) |

- 찾은 지역은 이름으로, 못 찾은 지역은 `???`로 표시합니다. 만나지 않은 팰은 서식지를 보여주지 않습니다.
- `지도에서 보기`는 도감 대신 지도 탭을 열고 `ShowHabitat`으로 서식지에 표시를 띄운 뒤, 화면 비율을 고려해 모든 서식지가 보이게 맞춥니다. 버튼을 처리하는 동안 도감 화면이 닫히므로, 필요한 값을 먼저 복사한 뒤 탭을 바꿉니다.

## 실행 검증

`verify_map.py`는 Listen Server + Client 플레이 세션에서 다섯 가지를 확인하며 **5/5 통과**입니다.

| # | 확인 내용 | 경로 |
|---|---|---|
| 1 | 오두막에서 스폰한 두 플레이어가 야영지를 발견하고 주변 안개가 걷힘(클라이언트 복제본 포함). 골짜기 칸은 그곳에 선 뒤에만 걷힘 | 서버 기록 · OwnerOnly 복제 |
| 2 | 활성화 전 표석으로는 이동 거부. 옆에 서 있기만 해서는 활성화되지 않고 F로 활성화. 두 표석 활성화 후 이동. 지도 밖에서는 다시 거부 | **실제 F 입력** · 서버 검증 |
| 3 | M으로 지도 탭이 열리고, 확인 키로 선택한 표석에 이동한 뒤 지도가 닫힘 | **실제 입력 · 클라이언트 RPC** |
| 4 | 도감의 `지도에서 보기`가 도감 자리에 지도 탭을 엶 | 화면 전환 |
| 5 | 새 플레이 세션에서 호스트의 지역·표석·탐험 칸이 그대로 돌아옴 | SaveGame |

검증을 만들며 알게 된 점도 기록해 둡니다.

- **에디터 Python에서 클라이언트 쪽 Server RPC를 부르면 네트워크를 타지 않고 그 자리에서 실행됩니다.** 그래서 클라이언트 → 서버 경로는 실제 입력 주입(M, F, 확인 키)으로만 검증했습니다(3번).
- F 상호작용은 카메라 앞쪽 500 단위 트레이스입니다. 어깨 카메라가 Pawn보다 50 단위 오른쪽에 있어서, 검증에서는 Pawn을 표석 왼쪽으로 50 단위 비켜 세웁니다.
- 활성화한 표석에서 F를 한 번 더 누르면 지도가 열립니다. 그래서 검증은 서버 기록을 확인한 뒤에만 다음 키를 누릅니다.

## 기술적 선택 요약

| 주제 | 선택 | 고려한 대안 | 이유 |
|---|---|---|---|
| 지도 그림 | 실제 레벨 직교 캡처 → 이미지 생성으로 다듬기 | 수작업 일러스트 | 월드와 형태가 일치하고 레벨 수정 시 다시 찍으면 됨 |
| 좌표 | 변환은 `UWorldMapDefinition` 한 곳 | 위젯마다 계산 | 축 약속이 한 곳에만 있음 |
| 안개 단위 | 탐험 반경 64×64 칸 | 지역 단위 안개(첫 구현) | 미니맵 안의 안개가 어색, 지역 수 상한, 지역 세분과 무관 |
| 안개 기록 | 서버가 Pawn 위치로 기록, 512 B OwnerOnly 복제 | 클라이언트 보고 | 조작 불가, 데이터가 작음 |
| 안개 연출 | 칸마다 밝기를 옮기는 CPU 텍스처 + 머티리얼 노이즈 | 두 텍스처 교차 페이드 | 걷는 동안 잦은 갱신에도 칸별로 끊김 없이 걷힘 |
| 머티리얼 | Custom HLSL 노드 하나, 미니맵·지도 공용 | 노드 그래프 | 스크립트로 작성·재현 가능, 파라미터만 다르게 재사용 |
| 마커 | 액터가 컴포넌트로 스스로 등록 | 지도가 액터 클래스를 검색 | 새 표시 대상은 컴포넌트만 붙이면 됨 |
| 빠른 이동 | F로 활성화, 활성 표석만, 지도 밖 거부 | 근접 자동 발견 | 활성화 시점이 분명, 던전 런 종료 경로 보존 |
| 표석 연출 | 플레이어별 클라이언트 색 + 머티리얼 VFX | 서버 복제 색 · Niagara | 활성화가 플레이어마다 다름, 작은 시도는 머티리얼로 충분 |

### 남은 과제

- 마우스 오버 강조는 코드와 정적 화면으로만 확인했습니다. 에디터 자동화에서 마우스 이동을 주입하기 어려워 아직 화면으로 보지 못했습니다.
- 소환한 팰은 빠른 이동을 따라오지 않습니다.
- 서식지는 플레이 한 번에서 얻은 고정 데이터입니다. 몬스터 배치가 바뀌면 `author_dex_data.py`의 표를 다시 맞춰야 합니다.
- 패드로 표석 목록을 고르는 포커스 이동은 아직 없습니다. 패드에서는 첫 표석이 선택됩니다.

## 관련 코드

- [WorldMapDefinition.h](https://github.com/chungheonLee0325/Sonheim/blob/feat-world-map/Sonheim/Source/Sonheim/Map/WorldMapDefinition.h) — 지도 범위·좌표 변환·안개 칸·마커 모양
- [WorldMapSubsystem.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-world-map/Sonheim/Source/Sonheim/Map/WorldMapSubsystem.cpp) — 지도 정의 로드·마커 목록·SaveGame
- [MapMarkerComponent.h](https://github.com/chungheonLee0325/Sonheim/blob/feat-world-map/Sonheim/Source/Sonheim/Map/MapMarkerComponent.h) — 마커 종류·발견 조건
- [MapView.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-world-map/Sonheim/Source/Sonheim/Map/MapView.cpp) — 지역 수집·추적 퀘스트 목적지·아이콘 레이어
- [MapFog.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-world-map/Sonheim/Source/Sonheim/Map/MapFog.cpp) — 탐험 칸 텍스처와 칸별 페이드
- [WorldMapComponent.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-world-map/Sonheim/Source/Sonheim/AreaObject/Player/Utility/WorldMapComponent.cpp) — 지역·표석·탐험 기록, 빠른 이동 서버 검증
- [FastTravelPoint.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-world-map/Sonheim/Source/Sonheim/GameObject/Map/FastTravelPoint.cpp) — 상호작용 활성화·빛기둥·도착 처리
- [MinimapWidget.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-world-map/Sonheim/Source/Sonheim/UI/Map/MinimapWidget.cpp) — 원형 미니맵
- [WorldMapWidget.cpp](https://github.com/chungheonLee0325/Sonheim/blob/feat-world-map/Sonheim/Source/Sonheim/UI/Map/WorldMapWidget.cpp) — 지도 탭·이동·확대·표석 목록·범례·서식지
- [[16. MonsterDex System|16_MonsterDex_System]] — 도감 페이지와 서식지
- [[17. Quest System|17_Quest_System]] — `Reach` 목표와 추적 퀘스트
- [[18. UMG v2 Migration|18_UMG_v2_Migration]] — 메뉴 탭·키 안내·알림 구조

#include "PerfScenarioSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerController.h"
#include "Sonheim/UI/Map/WorldMapWidget.h"
#include "Sonheim/UI/MonsterDex/MonsterDexWidget.h"
#include "Sonheim/UI/Notice/NoticeSubsystem.h"
#include "Sonheim/UI/System/UIStackSubsystem.h"

namespace
{
	// Every scene starts with the player here, on the beach's field by the camp, looking toward the hut, so the world behind the
	// UI draws the same each time.
	const FVector PerfSpot(-33200.f, 12800.f, 3900.f);
	const FRotator PerfView(-10.f, 200.f, 0.f);
}

void UPerfScenarioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
#if !UE_BUILD_SHIPPING && CSV_PROFILER
	FString List;
	if (!FParse::Value(FCommandLine::Get(), TEXT("PerfScenes="), List, false)) return;
	List.ParseIntoArray(Scenes, TEXT(","));
	FParse::Value(FCommandLine::Get(), TEXT("PerfLabel="), Label);
	FParse::Value(FCommandLine::Get(), TEXT("PerfRun="), Run);
	FParse::Value(FCommandLine::Get(), TEXT("PerfWarmup="), WarmupSeconds);
	FParse::Value(FCommandLine::Get(), TEXT("PerfSeconds="), RecordSeconds);
	if (Label.IsEmpty()) Label = TEXT("run");
	TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UPerfScenarioSubsystem::Tick));
	UE_LOG(LogTemp, Display, TEXT("[Perf] %d scenes, label %s, run %d, %.0fs warmup, %.0fs each"), Scenes.Num(), *Label, Run, WarmupSeconds, RecordSeconds);
#endif
}

void UPerfScenarioSubsystem::Deinitialize()
{
	FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
	Super::Deinitialize();
}

ASonheimPlayerController* UPerfScenarioSubsystem::Controller() const
{
	const UWorld* World = GetGameInstance()->GetWorld();
	return World ? Cast<ASonheimPlayerController>(World->GetFirstPlayerController()) : nullptr;
}

bool UPerfScenarioSubsystem::Tick(const float DeltaSeconds)
{
#if !UE_BUILD_SHIPPING && CSV_PROFILER
	PhaseTime += DeltaSeconds;
	const FString Scene = Scenes.IsValidIndex(SceneIndex) ? Scenes[SceneIndex].TrimStartAndEnd() : FString();
	switch (Phase)
	{
	case EPhase::WaitingForPlayer:
		// The player's character comes a moment after the map, and the first seconds load and compile; the scenes wait them out.
		if (PhaseTime < 5.f || !Controller() || !Controller()->GetPawn() || !Controller()->GetPawn()->IsA<ACharacter>()) return true;
		SetUp(Scene);
		Phase = EPhase::Settling;
		PhaseTime = 0.f;
		return true;
	case EPhase::Settling:
		Drive(Scene, PhaseTime);
		if (PhaseTime < WarmupSeconds) return true;
		FCsvProfiler::SetMetadata(TEXT("PerfScene"), *Scene);
		FCsvProfiler::SetMetadata(TEXT("PerfLabel"), *Label);
		FCsvProfiler::Get()->BeginCapture(-1, FPaths::ProjectSavedDir() / TEXT("Perf") / Label / TEXT("csv"),
		                                  FString::Printf(TEXT("%s_r%d.csv"), *Scene, Run));
		Phase = EPhase::Recording;
		PhaseTime = 0.f;
		return true;
	case EPhase::Recording:
		Drive(Scene, WarmupSeconds + PhaseTime);
		if (PhaseTime < RecordSeconds) return true;
		// The profiler ends the capture at the next frame's start and writes the file on its own thread: waiting for it here
		// would hold the very frame it needs, so the next frames look for it instead.
		Written = FCsvProfiler::Get()->EndCapture();
		TearDown();
		Phase = EPhase::Writing;
		return true;
	case EPhase::Writing:
		if (!Written.IsReady()) return true;
		UE_LOG(LogTemp, Display, TEXT("[Perf] wrote %s"), *Written.Get());
		if (++SceneIndex < Scenes.Num())
		{
			SetUp(Scenes[SceneIndex].TrimStartAndEnd());
			Phase = EPhase::Settling;
			PhaseTime = 0.f;
			return true;
		}
		Phase = EPhase::Done;
		UE_LOG(LogTemp, Display, TEXT("[Perf] done"));
		FPlatformMisc::RequestExit(false);
		return false;
	default:
		return false;
	}
#else
	return false;
#endif
}

void UPerfScenarioSubsystem::SetUp(const FString& Scene)
{
	ASonheimPlayerController* PC = Controller();
	if (!PC) return;
	TearDown();
	if (APawn* Pawn = PC->GetPawn()) Pawn->SetActorLocation(PerfSpot, false, nullptr, ETeleportType::TeleportPhysics);
	PC->SetControlRotation(PerfView);
	NextNotice = 0.f;
	if (Scene == TEXT("Inventory")) PC->OpenTab(UIScreen::Menu);
	else if (Scene == TEXT("Dex")) PC->OpenTab(UIScreen::MonsterDex);
	else if (Scene == TEXT("Map")) PC->OpenTab(UIScreen::WorldMap);
	UE_LOG(LogTemp, Display, TEXT("[Perf] scene %s"), *Scene);
}

void UPerfScenarioSubsystem::Drive(const FString& Scene, const float Time)
{
	ASonheimPlayerController* PC = Controller();
	const ULocalPlayer* Local = PC ? PC->GetLocalPlayer() : nullptr;
	if (!Local) return;
	if (Scene == TEXT("Notices") && Time >= NextNotice)
	{
		// A feed line every 0.4 s and a headline every 2 s, as a busy fight with a level up and a new species would bring.
		FNoticeData Data;
		Data.Category = INVTEXT("PERF");
		Data.Title = FText::AsNumber(FMath::FloorToInt(Time * 10.f));
		Data.Detail = INVTEXT("Perf scene notice");
		if (UNoticeSubsystem* Notices = Local->GetSubsystem<UNoticeSubsystem>())
		{
			Notices->Push(ENoticeSlot::Feed, TEXT("Perf"), Data);
			if (FMath::Fmod(Time, 2.f) < 0.4f) Notices->Push(ENoticeSlot::Headline, TEXT("PerfHeadline"), Data);
		}
		NextNotice = Time + 0.4f;
	}
	else if (Scene == TEXT("Map"))
	{
		// The view circles the island and breathes in and out, as a player dragging and zooming would move it.
		const UUIStackSubsystem* UIStack = Local->GetSubsystem<UUIStackSubsystem>();
		if (auto* Map = UIStack ? Cast<UWorldMapWidget>(UIStack->GetWidget(UIScreen::WorldMap)) : nullptr)
		{
			const float Angle = Time * UE_TWO_PI / 6.f;
			Map->SetView(FVector2D(0.5f + 0.15f * FMath::Cos(Angle), 0.5f + 0.15f * FMath::Sin(Angle)), 0.55f + 0.2f * FMath::Sin(Time * 0.9f));
		}
	}
	else if (Scene == TEXT("Dex") && Time < 0.1f)
	{
		const UUIStackSubsystem* UIStack = Local->GetSubsystem<UUIStackSubsystem>();
		if (auto* Book = UIStack ? Cast<UMonsterDexWidget>(UIStack->GetWidget(UIScreen::MonsterDex)) : nullptr) Book->Pick(100);
	}
}

void UPerfScenarioSubsystem::TearDown()
{
	const ASonheimPlayerController* PC = Controller();
	const ULocalPlayer* Local = PC ? PC->GetLocalPlayer() : nullptr;
	if (!Local) return;
	if (UUIStackSubsystem* UIStack = Local->GetSubsystem<UUIStackSubsystem>())
		while (UIStack->CloseTop()) {}
	if (UNoticeSubsystem* Notices = Local->GetSubsystem<UNoticeSubsystem>())
	{
		Notices->Clear(TEXT("Perf"));
		Notices->Clear(TEXT("PerfHeadline"));
	}
}

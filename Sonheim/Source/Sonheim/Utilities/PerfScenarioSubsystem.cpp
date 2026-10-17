#include "PerfScenarioSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerController.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerState.h"
#include "Sonheim/AreaObject/Player/Utility/InventoryComponent.h"
#include "Sonheim/AreaObject/Player/Utility/QuestComponent.h"
#include "Sonheim/GameObject/InteractableInterface.h"
#include "Sonheim/GameObject/Buildings/Crafting/CraftingStation.h"
#include "Sonheim/GameObject/Buildings/Storage/BaseContainer.h"
#include "Sonheim/GameObject/Buildings/Utility/ContainerComponent.h"
#include "Sonheim/Quest/QuestSubsystem.h"
#include "Sonheim/UI/FloatingDamagePool.h"
#include "Sonheim/UI/Quest/QuestWidget.h"
#include "EngineUtils.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "Sonheim/UI/Map/WorldMapWidget.h"
#include "Sonheim/UI/MonsterDex/MonsterDexWidget.h"
#include "Sonheim/UI/Notice/NoticeSubsystem.h"
#include "GameUIManagerSubsystem.h"
#include "Sonheim/UI/System/UITags.h"
#include "Sonheim/UI/Widget/Player/Inventory/InventoryScreenWidget.h"
#include "Sonheim/UI/Widget/Player/Inventory/InventoryWidget.h"
#include "Sonheim/UI/Widget/Player/Inventory/SlotWidget.h"

#if CSV_PROFILER
CSV_DEFINE_CATEGORY(PerfUI, true);
#endif

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
	FParse::Value(FCommandLine::Get(), TEXT("PerfProfileGPU="), ProfileGpuScene);
	FParse::Value(FCommandLine::Get(), TEXT("PerfEndCmds="), EndCommands, false);
	bShots = FParse::Param(FCommandLine::Get(), TEXT("PerfShots"));
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
		if (Scene == ProfileGpuScene) GEngine->Exec(GetGameInstance()->GetWorld(), TEXT("ProfileGPU"));
		if (bShots)
			FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Perf") / Label / TEXT("shots") / (Scene + TEXT(".png")), true, false);
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
		{
			TArray<FString> Commands;
			EndCommands.TrimQuotes().ParseIntoArray(Commands, TEXT(";"));
			for (const FString& Command : Commands) GEngine->Exec(GetGameInstance()->GetWorld(), *Command.TrimStartAndEnd());
		}
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
	Steps = 0;
	if (Scene == TEXT("Inventory")) PC->OpenTab(UITags::Inventory);
	else if (Scene == TEXT("InvSwap") || Scene == TEXT("DragSwap"))
	{
		// Every slot holds an item, each with its tooltip, as a late game bag would.
		if (UInventoryComponent* Inventory = Cast<ASonheimPlayer>(PC->GetPawn()) ? Cast<ASonheimPlayer>(PC->GetPawn())->GetInventoryComponent() : nullptr)
		{
			FillBag(PC);
			PC->OpenTab(UITags::Inventory);
			// The menu's widgets are kept and opened again, so this shows whether a reopening bound them twice.
			UE_LOG(LogTemp, Display, TEXT("[Perf] OnInventoryChanged bound objects: %d"), Inventory->OnInventoryChanged.GetAllObjects().Num());
		}
		bPointerDown = false;
		DragIndex = -1;
	}
	else if (Scene == TEXT("OpenClose")) PC->ConsoleCommand(TEXT("DexMeet 100"));
	else if (Scene == TEXT("Dex"))
	{
		// A met species, so the picture is the lit model and not a silhouette.
		PC->ConsoleCommand(TEXT("DexMeet 100"));
		PC->OpenTab(UITags::MonsterDex);
	}
	else if (Scene == TEXT("Map")) PC->OpenTab(UITags::WorldMap);
	else if (Scene == TEXT("Loot"))
	{
		// Stacks, so the bursts add to the same slots and the bag never fills.
		Ids.Reset();
		for (const TPair<int32, FItemData>& Item : USonheimGameInstance::Get(GetGameInstance()->GetWorld())->dt_Item)
			if (Item.Value.bStackable && Ids.Num() < 5) Ids.Add(Item.Key);
	}
	else if (Scene == TEXT("Kills") || Scene == TEXT("Journal"))
	{
		// Every quest of the game taken (in the journal every other one finished), as a late game journal lists them.
		const UQuestSubsystem* Book = GetGameInstance()->GetSubsystem<UQuestSubsystem>();
		const ASonheimPlayerState* State = PC->GetPlayerState<ASonheimPlayerState>();
		UQuestComponent* Quests = State ? State->m_QuestComponent : nullptr;
		Ids.Reset();
		if (Book && Quests)
		{
			int32 Index = 0;
			for (const FQuestData& Quest : Book->GetQuests())
			{
				const bool bDone = Scene == TEXT("Journal") && Index++ % 2;
				Quests->DebugSet(Quest.QuestID, bDone ? EQuestState::Completed : EQuestState::Active);
				if (Scene == TEXT("Journal")) Ids.Add(Quest.QuestID);
				else
					for (const FQuestStep& Step : Quest.Steps)
						for (const FQuestObjective& Objective : Step.Objectives)
							if (Objective.Goal == EQuestGoal::Kill && Objective.Id) Ids.AddUnique(Objective.Id);
			}
		}
		if (Scene == TEXT("Journal"))
		{
			PC->OpenTab(UITags::QuestJournal);
			const UGameUIManagerSubsystem* UI = PC->GetLocalPlayer()->GetSubsystem<UGameUIManagerSubsystem>();
			if (auto* Journal = UI ? Cast<UQuestJournalWidget>(UI->Find(UITags::QuestJournal)) : nullptr) Journal->SetFilter(2);
		}
		UE_LOG(LogTemp, Display, TEXT("[Perf] %d quests, %d ids"), Book ? Book->GetQuests().Num() : 0, Ids.Num());
	}
	else if (Scene == TEXT("Craft"))
	{
		// The station with the longest recipe list, used with a full bag.
		FillBag(PC);
		int32 Most = -1;
		for (TActorIterator<ACraftingStation> It(PC->GetWorld()); It; ++It)
		{
			const int32 Count = It->GetRecipeTable() ? It->GetRecipeTable()->GetRowMap().Num() : 0;
			if (Count > Most) { Most = Count; Prop = *It; }
		}
		UE_LOG(LogTemp, Display, TEXT("[Perf] station %s, %d recipes"), *GetNameSafe(Prop.Get()), Most);
	}
	else if (Scene == TEXT("Chest") && !PC->HasAuthority())
	{
		// A client of a listen server uses the chest its host serves (ServeChest), through the server as a player's F does.
		FillBag(PC);
		for (TActorIterator<ABaseContainer> It(PC->GetWorld()); It; ++It)
			if (!Prop.IsValid() || FVector::Dist(It->GetActorLocation(), PerfSpot) < FVector::Dist(Prop->GetActorLocation(), PerfSpot)) Prop = *It;
		UE_LOG(LogTemp, Display, TEXT("[Perf] served chest %s"), *GetNameSafe(Prop.Get()));
	}
	else if (Scene == TEXT("Chest") || Scene == TEXT("ServeChest"))
	{
		// A medium chest beside the player, every slot holding an item, and a full bag. ServeChest only puts the chest there,
		// for a client of this listen server to use.
		if (Scene == TEXT("Chest")) FillBag(PC);
		if (!Prop.IsValid() || !Prop->IsA<ABaseContainer>())
		{
			UClass* Class = LoadClass<ABaseContainer>(nullptr, TEXT("/Game/_BluePrint/GameObject/Building/Chest/BP_Contaniner_Medium.BP_Contaniner_Medium_C"));
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Prop = Class ? PC->GetWorld()->SpawnActor<ABaseContainer>(Class, PerfSpot + FVector(-300.f, 0.f, -80.f), FRotator::ZeroRotator, Params) : nullptr;
		}
		if (UContainerComponent* Box = Prop.IsValid() ? Cast<ABaseContainer>(Prop.Get())->GetContainerComponent() : nullptr)
		{
			for (const TPair<int32, FItemData>& Item : USonheimGameInstance::Get(GetGameInstance()->GetWorld())->dt_Item)
				if (!Box->AddItem(Item.Key, 1)) break;
			UE_LOG(LogTemp, Display, TEXT("[Perf] chest %s, %d slots"), *GetNameSafe(Prop.Get()), Box->GetMaxSlots());
		}
	}
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
		const UGameUIManagerSubsystem* UI = Local->GetSubsystem<UGameUIManagerSubsystem>();
		if (auto* Map = UI ? Cast<UWorldMapWidget>(UI->Find(UITags::WorldMap)) : nullptr)
		{
			const float Angle = Time * UE_TWO_PI / 6.f;
			Map->SetView(FVector2D(0.5f + 0.15f * FMath::Cos(Angle), 0.5f + 0.15f * FMath::Sin(Angle)), 0.55f + 0.2f * FMath::Sin(Time * 0.9f));
		}
	}
	else if (Scene == TEXT("OpenClose") && Time >= Steps * 0.6f)
	{
		// The inventory, the book and the map in turn: open for 0.6 s, closed for 0.6 s.
		// The events keep the screens' names from before the tags (Menu, MonsterDex, WorldMap), so runs compare with older ones.
		static const TPair<FGameplayTag, const TCHAR*> Screens[] = {
			{UITags::Inventory, TEXT("Menu")}, {UITags::MonsterDex, TEXT("MonsterDex")}, {UITags::WorldMap, TEXT("WorldMap")}};
		const FGameplayTag Screen = Screens[(Steps / 2) % UE_ARRAY_COUNT(Screens)].Key;
		// Each step is also a named scope in an Insights trace (-trace=cpu), to find its frames there.
		const FString Event = FString::Printf(TEXT("%s %s"), Steps % 2 == 0 ? TEXT("Open") : TEXT("Close"), Screens[(Steps / 2) % UE_ARRAY_COUNT(Screens)].Value);
		CSV_EVENT(PerfUI, TEXT("%s"), *Event);
		TRACE_CPUPROFILER_EVENT_SCOPE_TEXT(*(TEXT("PerfUI ") + Event));
		if (Steps % 2 == 0) PC->OpenTab(Screen);
		else TearDown();
		++Steps;
	}
	else if (Scene == TEXT("InvSwap") && Time >= Steps * 0.25f)
	{
		// The first and last slots trade places, as a drag and drop between them does.
		if (UInventoryComponent* Inventory = Cast<ASonheimPlayer>(PC->GetPawn()) ? Cast<ASonheimPlayer>(PC->GetPawn())->GetInventoryComponent() : nullptr)
		{
			CSV_EVENT(PerfUI, TEXT("Swap"));
			TRACE_CPUPROFILER_EVENT_SCOPE_STR("PerfUI Swap");
			Inventory->SwapItems(0, Inventory->GetInventory().Num() - 1);
		}
		++Steps;
	}
	else if (Scene == TEXT("DragSwap"))
	{
		DriveDrag(Local, Time);
	}
	else if (Scene == TEXT("ServeChest"))
	{
		// The server checks a player's distance to the chest, so a client's player is put on the spot here, where it counts.
		for (FConstPlayerControllerIterator It = PC->GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			APawn* Pawn = It->IsValid() && !(*It)->IsLocalController() ? (*It)->GetPawn() : nullptr;
			if (Pawn && FVector::Dist(Pawn->GetActorLocation(), PerfSpot) > 300.f) Pawn->SetActorLocation(PerfSpot, false, nullptr, ETeleportType::TeleportPhysics);
		}
	}
	else if (Scene == TEXT("Loot") && Time >= Steps * 1.5f)
	{
		// Five stacks at once, as a defeated monster's drops are picked up together.
		if (UInventoryComponent* Inventory = Cast<ASonheimPlayer>(PC->GetPawn()) ? Cast<ASonheimPlayer>(PC->GetPawn())->GetInventoryComponent() : nullptr)
		{
			CSV_EVENT(PerfUI, TEXT("Loot"));
			TRACE_CPUPROFILER_EVENT_SCOPE_STR("PerfUI Loot");
			for (const int32 Id : Ids) Inventory->AddItem(Id, 1);
		}
		++Steps;
	}
	else if (Scene == TEXT("Kills") && Ids.Num() && Time >= Steps * 0.5f)
	{
		// A quest's target defeated every half second; every 8th time the quests are taken again, so their counts keep moving.
		if (ASonheimPlayerState* State = PC->GetPlayerState<ASonheimPlayerState>())
		{
			CSV_EVENT(PerfUI, TEXT("Kill"));
			TRACE_CPUPROFILER_EVENT_SCOPE_STR("PerfUI Kill");
			State->ReportDeed(EPlayerDeed::Kill, Ids[Steps % Ids.Num()]);
		}
		if (++Steps % 8 == 0)
		{
			const int32 Done = Steps;
			SetUp(Scene);
			Steps = Done;
		}
	}
	else if (Scene == TEXT("Journal") && Ids.Num() && Time >= Steps * 0.3f)
	{
		// The next quest picked every 0.3 s, as a player reads down the list.
		const UGameUIManagerSubsystem* UI = Local->GetSubsystem<UGameUIManagerSubsystem>();
		if (auto* Journal = UI ? Cast<UQuestJournalWidget>(UI->Find(UITags::QuestJournal)) : nullptr)
		{
			CSV_EVENT(PerfUI, TEXT("Pick Quest"));
			TRACE_CPUPROFILER_EVENT_SCOPE_STR("PerfUI Pick Quest");
			Journal->Pick(Ids[Steps % Ids.Num()]);
		}
		++Steps;
	}
	else if (Scene == TEXT("Hits") && Time >= Steps * 0.5f)
	{
		// Twenty numbers at once in front of the player, as a wide skill hitting a pack would show.
		if (AFloatingDamagePool* Pool = AFloatingDamagePool::GetInstance(PC->GetWorld()))
		{
			CSV_EVENT(PerfUI, TEXT("Hits"));
			TRACE_CPUPROFILER_EVENT_SCOPE_STR("PerfUI Hits");
			const FVector Ahead = PC->GetPawn()->GetActorLocation() + PerfView.Vector() * 600.f;
			for (int32 i = 0; i < 20; ++i)
			{
				FDamageNumberRequest Request;
				Request.Damage = 100 + i * 37;
				Request.WorldLocation = Ahead + FVector(0.f, (i % 5 - 2) * 80.f, (i / 5) * 60.f);
				Request.WeakPointType = i % 4 ? EFloatingOutLineDamageType::Normal : EFloatingOutLineDamageType::CriticalDamaged;
				Pool->RequestDamageNumber(Request);
			}
		}
		++Steps;
	}
	else if ((Scene == TEXT("Craft") || Scene == TEXT("Chest")) && Time >= Steps * 0.6f)
	{
		// Open, close, then two items of the bag trade places with the screen shut, as a player sorts after putting things away.
		if (Steps % 3 != 2) UseOrClose(PC, Scene == TEXT("Craft") ? TEXT("Crafting") : TEXT("Chest"));
		else if (UInventoryComponent* Inventory = Cast<ASonheimPlayer>(PC->GetPawn()) ? Cast<ASonheimPlayer>(PC->GetPawn())->GetInventoryComponent() : nullptr)
		{
			CSV_EVENT(PerfUI, TEXT("Bag Swap"));
			TRACE_CPUPROFILER_EVENT_SCOPE_STR("PerfUI Bag Swap");
			Inventory->SwapItems(0, Inventory->GetInventory().Num() - 1);
			UE_LOG(LogTemp, Display, TEXT("[Perf] OnInventoryChanged bound objects: %d"), Inventory->OnInventoryChanged.GetAllObjects().Num());
			++Steps;
		}
	}
	else if (Scene == TEXT("Dex") && Time < 0.1f)
	{
		const UGameUIManagerSubsystem* UI = Local->GetSubsystem<UGameUIManagerSubsystem>();
		if (auto* Book = UI ? Cast<UMonsterDexWidget>(UI->Find(UITags::MonsterDex)) : nullptr) Book->Pick(100);
	}
}

void UPerfScenarioSubsystem::FillBag(const ASonheimPlayerController* PC) const
{
	if (UInventoryComponent* Inventory = Cast<ASonheimPlayer>(PC->GetPawn()) ? Cast<ASonheimPlayer>(PC->GetPawn())->GetInventoryComponent() : nullptr)
		for (const TPair<int32, FItemData>& Item : USonheimGameInstance::Get(GetGameInstance()->GetWorld())->dt_Item)
			Inventory->AddItem(Item.Key, 1);
}

void UPerfScenarioSubsystem::UseOrClose(ASonheimPlayerController* PC, const TCHAR* Name)
{
	// The player's own way in and out: the object's interaction (F) opens its screen, the menu key (Tab) closes the top screen.
	ASonheimPlayer* Player = Cast<ASonheimPlayer>(PC->GetPawn());
	if (!Prop.IsValid() || !Player) return;
	const bool bOpen = Steps % 3 == 0;
	const FString Event = FString::Printf(TEXT("%s %s"), bOpen ? TEXT("Open") : TEXT("Close"), Name);
	CSV_EVENT(PerfUI, TEXT("%s"), *Event);
	TRACE_CPUPROFILER_EVENT_SCOPE_TEXT(*(TEXT("PerfUI ") + Event));
	if (bOpen && PC->HasAuthority()) IInteractableInterface::Execute_Interact(Prop.Get(), Player);
	else if (bOpen) Player->GetInteractionComponent()->Server_TryInteract(Prop.Get());
	else if (UGameUIManagerSubsystem* UI = PC->GetLocalPlayer()->GetSubsystem<UGameUIManagerSubsystem>()) UI->CloseTop();
	++Steps;
}

void UPerfScenarioSubsystem::DriveDrag(const ULocalPlayer* Local, const float Time)
{
	// Every 0.8 s the item of the first slot and of the sixth trade places by the mouse, back and forth: the pointer rests on one
	// slot, presses, moves to the other over 0.4 s (Slate detects the drag on the way) and lets go there. The events go through
	// FSlateApplication as the platform's would, so the drag visual, the hover highlights and the drop all run.
	const UGameUIManagerSubsystem* UI = Local->GetSubsystem<UGameUIManagerSubsystem>();
	const UInventoryScreenWidget* Screen = UI ? UI->Find<UInventoryScreenWidget>(UITags::Inventory) : nullptr;
	const UInventoryWidget* Bag = Screen ? Screen->GetInventory() : nullptr;
	const USlotWidget* First = Bag ? Bag->GetSlot(0) : nullptr;
	const USlotWidget* Sixth = Bag ? Bag->GetSlot(5) : nullptr;
	const TSharedPtr<SWindow> Window = GEngine->GameViewport ? GEngine->GameViewport->GetWindow() : nullptr;
	if (!First || !Sixth || !Window.IsValid() || !FSlateApplication::IsInitialized()) return;
	FSlateApplication& Slate = FSlateApplication::Get();
	const int32 Index = FMath::FloorToInt(Time / 0.8f);
	const float Into = Time - Index * 0.8f;
	const FVector2D From = (Index % 2 ? Sixth : First)->GetCachedGeometry().GetAbsolutePositionAtCoordinates(FVector2D(0.5f, 0.5f));
	const FVector2D To = (Index % 2 ? First : Sixth)->GetCachedGeometry().GetAbsolutePositionAtCoordinates(FVector2D(0.5f, 0.5f));
	auto Event = [&](const FVector2D& At, const bool bDown)
	{
		TSet<FKey> Pressed;
		if (bDown) Pressed.Add(EKeys::LeftMouseButton);
		const FPointerEvent Pointer(FSlateApplication::CursorUserIndex, FSlateApplication::CursorPointerIndex, At, PointerAt, Pressed,
		                            EKeys::LeftMouseButton, 0.f, Slate.GetModifierKeys());
		PointerAt = At;
		return Pointer;
	};
	if (Index != DragIndex && !bPointerDown)
	{
		if (Into < 0.1f) { Slate.ProcessMouseMoveEvent(Event(From, false)); return; }
		CSV_EVENT(PerfUI, TEXT("DragPress"));
		TRACE_CPUPROFILER_EVENT_SCOPE_STR("PerfUI DragPress");
		Slate.ProcessMouseButtonDownEvent(Window->GetNativeWindow(), Event(From, true));
		bPointerDown = true;
		DragIndex = Index;
		return;
	}
	if (!bPointerDown || Index != DragIndex) return;
	if (Into < 0.5f)
	{
		const bool bFirstMove = PointerAt.Equals(From, 0.5);
		const FVector2D At = FMath::Lerp(From, To, FMath::Clamp((Into - 0.1f) / 0.4f, 0.f, 1.f));
		if (bFirstMove) CSV_EVENT(PerfUI, TEXT("DragMove"));
		TRACE_CPUPROFILER_EVENT_SCOPE_STR("PerfUI DragMove");
		Slate.ProcessMouseMoveEvent(Event(At, true));
		return;
	}
	CSV_EVENT(PerfUI, TEXT("Drop"));
	TRACE_CPUPROFILER_EVENT_SCOPE_STR("PerfUI Drop");
	Slate.ProcessMouseMoveEvent(Event(To, true));
	Slate.ProcessMouseButtonUpEvent(Event(To, false));
	bPointerDown = false;
}

void UPerfScenarioSubsystem::TearDown()
{
	const ASonheimPlayerController* PC = Controller();
	const ULocalPlayer* Local = PC ? PC->GetLocalPlayer() : nullptr;
	if (!Local) return;
	if (UGameUIManagerSubsystem* UI = Local->GetSubsystem<UGameUIManagerSubsystem>())
		while (UI->CloseTop()) {}
	if (UNoticeSubsystem* Notices = Local->GetSubsystem<UNoticeSubsystem>())
	{
		Notices->Clear(TEXT("Perf"));
		Notices->Clear(TEXT("PerfHeadline"));
	}
}

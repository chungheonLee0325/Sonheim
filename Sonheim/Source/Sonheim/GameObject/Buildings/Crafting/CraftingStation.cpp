#include "Sonheim/GameObject/Buildings/Crafting/CraftingStation.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Sonheim/UI/Widget/DetectWidget.h"
#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerController.h"
#include "Sonheim/AreaObject/Player/SonheimPlayerState.h"
#include "Sonheim/AreaObject/Player/Utility/InventoryComponent.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "Sonheim/UI/Widget/GameObject/Crafting/CraftingQueueWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Sonheim/Utilities/InventoryResourceProvider.h"

ACraftingStation::ACraftingStation()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	StationMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StationMesh"));
	RootComponent = StationMesh;

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(RootComponent);
	InteractionSphere->InitSphereRadius(220.f);

	DetectWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("DetectWidget"));
	DetectWidget->SetupAttachment(RootComponent);
	DetectWidget->SetRelativeLocation(FVector(0, 0.f, 130.f));
	DetectWidget->SetWidgetSpace(EWidgetSpace::Screen);
	DetectWidget->SetDrawAtDesiredSize(true);
	DetectWidget->SetDrawSize(FVector2D(200.f, 50.f));

	static ConstructorHelpers::FClassFinder<UUserWidget> WBP_DetectClass(
		TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/_BluePrint/Widget/WBP_Detect.WBP_Detect_C'"));
	if (WBP_DetectClass.Succeeded())
	{
		DetectWidgetClass = WBP_DetectClass.Class;
	}

	QueueWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("QueueWidgetComp"));
	QueueWidgetComp->SetupAttachment(RootComponent);
	QueueWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	QueueWidgetComp->SetDrawAtDesiredSize(true);
	QueueWidgetComp->SetDrawSize(FVector2D(200.f, 50.f));
	QueueWidgetComp->SetRelativeLocation(FVector(0, 0.f, 260.f)); // Detect보다 살짝 위
	QueueWidgetComp->SetInitialLayerZOrder(10); // Detect보다 높게

	static ConstructorHelpers::FClassFinder<UUserWidget> WBP_QueueWidgetClass(
		TEXT(
			"/Script/UMGEditor.WidgetBlueprint'/Game/_BluePrint/Widget/GameObject/WB_CraftingQueue.WB_CraftingQueue_C'"));
	if (WBP_QueueWidgetClass.Succeeded())
	{
		QueueWidgetClass = WBP_QueueWidgetClass.Class;
	}
}

void ACraftingStation::BeginPlay()
{
	Super::BeginPlay();

	if (DetectWidget && DetectWidgetClass)
	{
		DetectWidget->SetWidgetClass(DetectWidgetClass);
		if (UDetectWidget* DW = Cast<UDetectWidget>(DetectWidget->GetUserWidgetObject()))
		{
			DW->SetInteractionInfo(GetInteractionName_Implementation(), TEXT(""));
			DW->SetVisibility(ESlateVisibility::Hidden);
		}
	}
	if (QueueWidgetClass)
		QueueWidgetComp->SetWidgetClass(QueueWidgetClass);

	if (UUserWidget* W = QueueWidgetComp->GetUserWidgetObject())
	{
		if (UCraftingQueueWidget* QW = Cast<UCraftingQueueWidget>(W))
		{
			// 각 클라에서 자기 위젯에 Station 주입
			QW->Initialise(this);
			QW->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void ACraftingStation::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACraftingStation, SelectedRecipe);
	DOREPLIFETIME(ACraftingStation, UIOwner);
	DOREPLIFETIME(ACraftingStation, ActiveWork);
	DOREPLIFETIME(ACraftingStation, bHasActiveWork);
	DOREPLIFETIME(ACraftingStation, CompletedToCollect);
}

bool ACraftingStation::CanInteract_Implementation() const
{
	// 레시피 선택 UI 소유주(레시피 설정하고 있는 사람) 이 없을때만 상호 작용 가능
	// UI로 체크하는 이유는 작업 돕기 기능 때문에 레시피 선택 작업이아닌 제작 작업할때는 같이 상호작용 할 수 있도록 레시피 UI로 체크
	return UIOwner == nullptr;
}


void ACraftingStation::OnDetected_Implementation(bool bDetected)
{
	bIsDetected = bDetected;
	if (DetectWidget)
	{
		if (UDetectWidget* DW = Cast<UDetectWidget>(DetectWidget->GetUserWidgetObject()))
		{
			if (bDetected)
			{
				DW->UpdateHoldProgressByPurpose(0.f, EHoldPurpose::Interact);
				UpdateDetectWidgetText();
				DW->PlayShowAnimation();
				DW->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
			else
			{
				DW->UpdateInteractProgress(0.f);
				DW->UpdateCancelHoldProgress(0.f);
				DW->SetCancelVisible(false);
				DW->PlayHideAnimation();
				DW->SetVisibility(ESlateVisibility::Hidden);
			}
		}
		if (QueueWidgetComp)
		{
			if (bDetected)
			{
				QueueWidgetComp->GetUserWidgetObject()->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
			else
			{
				QueueWidgetComp->GetUserWidgetObject()->SetVisibility(ESlateVisibility::Hidden);
			}
		}
	}
}

FString ACraftingStation::GetInteractionName_Implementation() const
{
	if (CompletedToCollect > 0) return TEXT("취득");
	if (bHasActiveWork) return TEXT("제작");
	return TEXT("레시피 선택");
}

void ACraftingStation::UpdateHoldProgressUI_Implementation(float Progress, EHoldPurpose Purpose)
{
	if (UDetectWidget* DW = GetDetectWidget())
	{
		DW->UpdateHoldProgressByPurpose(Progress, Purpose);
	}
}

void ACraftingStation::ExecuteCancel_Implementation(ASonheimPlayer* Player)
{
	if (!Player) return;

	if (HasAuthority())
	{
		ServerCancelUnfinished(Player);
		return;
	}

	if (ASonheimPlayerController* PC = Cast<ASonheimPlayerController>(Player->GetController()))
	{
		PC->Server_Crafting_CancelUnfinished(this);
	}
}

void ACraftingStation::Interact_Implementation(ASonheimPlayer* Player)
{
	// 우선순위 : 아이템 수령 -> 제작 작업 -> UI 열기
	// 제작 작업중에 자동 수령 X. 작업을 멈추고 다시 상호 작용할시 아이템 수령
	if (!Player) return;
	// 자동수령 방지: 최근 작업 직후에는 수령 금지, 홀드 중단 후 재상호작용으로 수령 허용
	if (bHasActiveWork)
	{
		const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
		 const bool bAllowManualCollect = (CompletedToCollect > 0) && (Now - LastWorkAddServerTime >=
		 	ManualCollectDelay);
		 if (bAllowManualCollect)
		{
			ServerCollectAll(Player);
		}
		else
		{
			ServerAddWork(ResolvePlayerWorkSpeed_Internal(Player), Player);
		}
		return;
	}
	if (CompletedToCollect > 0)
	{
		ServerCollectAll(Player);
		return;
	}
	// 수령 직후 즉시 UI 오픈 방지(같은 틱 연쇄 방지)
	{
		const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
		if (Now - LastCollectServerTime < ManualOpenUIDelay)
		{
			return;
		}
	}
	ServerRequestOpenUI(Player);
}

float ACraftingStation::GetCurrentProgress() const
{
	return (bHasActiveWork && ActiveWork.WorkPerUnit > 0)
		       ? FMath::Clamp(ActiveWork.WorkAccumulated / (float)ActiveWork.WorkPerUnit, 0.f, 1.f)
		       : 0.f;
}

int32 ACraftingStation::GetInteractionModeCode_Implementation() const
{
	// 작업 중
	if (bHasActiveWork) return 1;
	// 수령 가능
	if (CompletedToCollect > 0) return 2;
	// 레시피 선택/대기
	return 3;
}

float ACraftingStation::ResolvePlayerWorkSpeed_Internal(ASonheimPlayer* Player) const
{
	if (!Player) return DefaultWorkSpeed;
	if (auto* PS = Player->GetPlayerState<ASonheimPlayerState>())
	{
		return PS->GetStatValue(EAreaObjectStatType::WorkSpeed);
	}
	return DefaultWorkSpeed;
}

void ACraftingStation::OnRep_ActiveWork()
{
	OnWorkChanged.Broadcast();
	if (bIsDetected)
	{
		UpdateDetectWidgetText();
	}
}

void ACraftingStation::OnRep_CompletedToCollect()
{
	OnCompletedChanged.Broadcast();
	OnWorkChanged.Broadcast();
	if (bIsDetected)
	{
		UpdateDetectWidgetText();
	}
}

void ACraftingStation::ServerStartWork_Implementation(ASonheimPlayer* Requestor, FName RecipeRow, int32 Units)
{
	if (bHasActiveWork || Units <= 0) return;
	const FCraftingRecipe* R = FindRecipe(RecipeRow);
	if (!R) return;

	// 재료 소모
	UInventoryComponent* Inv = Requestor->GetInventoryComponent();
	if (!Inv) return;

	const int32 Max = UInventoryResourceProvider::ComputeMaxCraftable(Inv, R->RequiredMaterials);
	const int32 ToMake = FMath::Clamp(Units, 0, Max);

	int32 Made = 0;
	for (int32 i = 0; i < ToMake; ++i)
	{
		if (UInventoryResourceProvider::ConsumeItems(Inv, R->RequiredMaterials))
		{
			++Made;
		}
		else
		{
			break;
		}
	}
	if (Made <= 0)
	{
		return;
	}

	ActiveWork.RecipeRow = RecipeRow;
	ActiveWork.ResultItemID = R->ResultItemID;
	ActiveWork.ResultPerUnit = R->ResultCount;
	ActiveWork.WorkPerUnit = R->WorkRequired;
	ActiveWork.UnitsTotal = Made;
	ActiveWork.UnitsDone = 0;
	ActiveWork.WorkAccumulated = 0.f;
	bHasActiveWork = true;

	ForceNetUpdate();
	OnRep_ActiveWork();
}

void ACraftingStation::ServerAddWork_Implementation(float WorkDelta, class ASonheimPlayer* Worker)
{
	// 작업 입력마다 SFX 재생(서버 시간 기준 최소 간격으로 스로틀링)
	if (CraftWorkSFX)
	{
		const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
		if (Now - LastCraftSfxServerTime >= CraftSFXInterval)
		{
			LastCraftSfxServerTime = Now;
			Multicast_PlayCraftSfx();
		}
	}

	// 최근 작업 추가 시각 갱신(자동수령 쿨다운)
	LastWorkAddServerTime = GetWorld() ? GetWorld()->GetTimeSeconds() : LastWorkAddServerTime;

	ActiveWork.WorkAccumulated += WorkDelta;

	while (ActiveWork.WorkAccumulated >= ActiveWork.WorkPerUnit && bHasActiveWork)
	{
		ActiveWork.WorkAccumulated -= ActiveWork.WorkPerUnit;
		ActiveWork.UnitsDone++;
		CompletedToCollect += ActiveWork.ResultPerUnit;
		OnCompletedChanged.Broadcast();

		if (ActiveWork.UnitsDone >= ActiveWork.UnitsTotal)
		{
			// 작업 종료
			bHasActiveWork = false;
			ActiveWork.WorkAccumulated = 0.f;
		}
	}

	//ForceNetUpdate();
	OnRep_ActiveWork();
}

void ACraftingStation::ServerCollectAll_Implementation(ASonheimPlayer* Player)
{
	if (!HasAuthority() || !Player) return;
	if (CompletedToCollect <= 0) return;
	if (UInventoryComponent* Inv = Player->GetInventoryComponent())
	{
		if (CompletedToCollect > 0) Inv->AddItem(ActiveWork.ResultItemID, CompletedToCollect);
	}

	// 수령한만큼 아이템 차감
	const int32 PerUnit = FMath::Max(1, ActiveWork.ResultPerUnit);
	// 결과 수량 -> 작업 유닛 수로 환산
	const int32 CompletedUnits = CompletedToCollect / PerUnit;
	if (CompletedUnits > 0)
	{
		ActiveWork.UnitsDone = FMath::Max(0, ActiveWork.UnitsDone - CompletedUnits);
		ActiveWork.UnitsTotal = FMath::Max(0, ActiveWork.UnitsTotal - CompletedUnits);
	}

	CompletedToCollect = 0;

	if (!bHasActiveWork)
	{
		// 아이템 수령후 남은 작업이 없으면 작업 초기화
		ActiveWork.ResultItemID = 0;
		ActiveWork.ResultPerUnit = 0;
		ActiveWork.WorkPerUnit = 0;
		ActiveWork.UnitsTotal = 0;
		ActiveWork.UnitsDone = 0;
		ActiveWork.WorkAccumulated = 0.f;
	}

	ForceNetUpdate();
	OnCompletedChanged.Broadcast();

	// 수령 직후에는 홀드 연쇄를 막기 위해 클라 홀드 중단 요청
	Player->Client_StopInteractionHold(EHoldPurpose::Interact);

	// 최근 수령 시간 기록(동틱 UI 오픈 방지)
	LastCollectServerTime = GetWorld() ? GetWorld()->GetTimeSeconds() : LastCollectServerTime;
}

void ACraftingStation::ServerCancelUnfinished_Implementation(class ASonheimPlayer* Requestor)
{
	// 완료된 유닛 제외, 남은 유닛만 환불
	if (!bHasActiveWork) return;
	const FCraftingRecipe* R = FindRecipe(ActiveWork.RecipeRow);
	if (R)
	{
		const int32 RemainUnits = ActiveWork.UnitsTotal - ActiveWork.UnitsDone;
		if (RemainUnits > 0)
		{
			for (const auto& P : R->RequiredMaterials)
			{
				const int32 MatID = P.Key;
				const int32 RefundCount = P.Value * RemainUnits;
				if (UInventoryComponent* Inv = Requestor->GetInventoryComponent())
				{
					if (RefundCount > 0) Inv->AddItem(MatID, RefundCount);
				}
			}
		}
	}
	// 작업은 종료, CompletedToCollect는 그대로(이미 완료분 보관)
	bHasActiveWork = false;
	ActiveWork.WorkAccumulated = 0.f;
	ActiveWork.UnitsTotal = ActiveWork.UnitsDone; // 논리 일치
	ForceNetUpdate();
	OnRep_ActiveWork();
}

void ACraftingStation::ServerRequestOpenUI_Implementation(ASonheimPlayer* Player)
{
	if (!HasAuthority() || !Player) return;
	if (UIOwner && UIOwner != Player) return;
	UIOwner = Player;
	if (auto* PC = Cast<ASonheimPlayerController>(Player->GetController()))
	{
		PC->Client_OpenCraftingUI(this);
	}
	Player->Client_StopInteractionHold(EHoldPurpose::Interact);
}

void ACraftingStation::ServerReleaseUI_Implementation(ASonheimPlayer* Player)
{
	if (!HasAuthority() || !Player) return;
	if (Player != UIOwner) return;
	if (auto* PC = Cast<ASonheimPlayerController>(Player->GetController()))
	{
		PC->Client_CloseCraftingUI();
	}
	UIOwner = nullptr;
}

void ACraftingStation::ServerStartAssist_Implementation(ASonheimPlayer* Player)
{
	if (!HasAuthority() || !Player) return;
	Assistants.Add(Player);
}

void ACraftingStation::ServerStopAssist_Implementation(ASonheimPlayer* Player)
{
	if (!HasAuthority() || !Player) return;
	Assistants.Remove(Player);
}

void ACraftingStation::UpdateDetectWidgetText()
{
	if (!DetectWidget) return;
	if (UDetectWidget* DW = Cast<UDetectWidget>(DetectWidget->GetUserWidgetObject()))
	{
		DW->SetInteractionInfo(GetInteractionName_Implementation(), "");

		const bool bCancel = CanHoldCancel_Implementation();
		DW->SetCancelVisible(bCancel);
		if (bCancel)
		{
			DW->SetCancelInfo(FText::FromString(TEXT("취소")), FText::FromString(TEXT("C")));
			DW->UpdateCancelHoldProgress(0.f);
		}
	}
}

UDetectWidget* ACraftingStation::GetDetectWidget() const
{
	if (!DetectWidget) return nullptr;
	if (UUserWidget* W = DetectWidget->GetUserWidgetObject())
		return Cast<UDetectWidget>(W);
	return nullptr;
}

const FCraftingRecipe* ACraftingStation::FindRecipe(FName Row) const
{
	if (!RecipeTable) return nullptr;
	return RecipeTable->FindRow<FCraftingRecipe>(Row, TEXT("CraftingStation"));
}

// ===== SFX Helpers =====
void ACraftingStation::PlayCraftSfxOnce()
{
	if (!CraftWorkSFX) return;
	if (GetNetMode() == NM_DedicatedServer) return;
	UGameplayStatics::PlaySoundAtLocation(this, CraftWorkSFX, GetActorLocation());
}

void ACraftingStation::Multicast_PlayCraftSfx_Implementation()
{
	PlayCraftSfxOnce();
}

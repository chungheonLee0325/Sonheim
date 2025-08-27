// BaseItem.cpp
#include "BaseItem.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "Sonheim/Utilities/LogMacro.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"
#include "Sonheim/UI/Widget/DetectWidget.h"

ABaseItem::ABaseItem()
{
	PrimaryActorTick.bCanEverTick = true;

	// 메시 컴포넌트 (루트 및 물리 담당)
	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	RootComponent = ItemMesh;
	ItemMesh->SetRelativeScale3D(FVector(0.01f));
	ItemMesh->SetIsReplicated(true);

	// 메시가 물리를 담당
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ItemMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	ItemMesh->SetSimulatePhysics(false); // 기본적으로 비활성, 드롭 시 활성화

	// 임시 메시 설정
	ConstructorHelpers::FObjectFinder<UStaticMesh> tempMesh(
		TEXT("/Script/Engine.StaticMesh'/Game/_Resource/SurvivalGameKitV2/Meshes/Static/SM_TreasureBags02.SM_TreasureBags02'"));
	if (tempMesh.Succeeded())
	{
		ItemMesh->SetStaticMesh(tempMesh.Object);
	}

	// 콜리전 스피어 (자동 획득 감지용)
	CollectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollectionSphere"));
	CollectionSphere->SetupAttachment(ItemMesh);
	CollectionSphere->SetCollisionObjectType(ECC_Pawn);
	CollectionSphere->SetSphereRadius(50);
	CollectionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollectionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollectionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// 네트워크 설정
	bReplicates = true;
	SetReplicateMovement(true);
	SetNetUpdateFrequency(100.0f);
	SetMinNetUpdateFrequency(33.0f);

	// 감지 위젯 컴포넌트
	DetectWidgetComponent = CreateDefaultSubobject<UWidgetComponent>("DetectWidget");
	DetectWidgetComponent->SetVisibility(false);
	DetectWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	DetectWidgetComponent->SetPivot(FVector2D(0.5f, 0.8f));
	DetectWidgetComponent->SetupAttachment(ItemMesh);
	DetectWidgetComponent->SetRelativeLocation(FVector(0, 0, 0));

	ConstructorHelpers::FClassFinder<UUserWidget> detectWidgetClass(
		TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/_BluePrint/Widget/WBP_Detect.WBP_Detect_C'"));
	if (detectWidgetClass.Succeeded())
	{
		DetectWidgetClass = detectWidgetClass.Class;
	}
}

void ABaseItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABaseItem, m_ItemID);
	DOREPLIFETIME(ABaseItem, m_ItemValue);
	DOREPLIFETIME(ABaseItem, m_IsCollected);
	DOREPLIFETIME(ABaseItem, bRequireInteraction);
	DOREPLIFETIME(ABaseItem, InteractionType);
	DOREPLIFETIME(ABaseItem, HoldDuration);
	DOREPLIFETIME(ABaseItem, ItemRarity);
}

void ABaseItem::BeginPlay()
{
	Super::BeginPlay();

	m_GameInstance = Cast<USonheimGameInstance>(GetGameInstance());
	if (m_ItemID > 0)
	{
		dt_ItemData = m_GameInstance->GetDataItem(m_ItemID);
	}

	SetupComponents();
}

void ABaseItem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 타이머 정리
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(AutoPickupTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(LifeTimeTimerHandle);
	}
	
	Super::EndPlay(EndPlayReason);
}

void ABaseItem::InitializeItem(int32 InItemID, const FItemSpawnOptions& Options)
{
	m_ItemID = InItemID;
	m_ItemValue = Options.ItemCount;
	bRequireInteraction = Options.bRequireInteraction;
	InteractionType = Options.InteractionType;
	HoldDuration = Options.HoldDuration;

	// 데이터 테이블 로드
	if (m_GameInstance)
	{
		dt_ItemData = m_GameInstance->GetDataItem(m_ItemID);
	}

	SetupComponents();

	// 서버에서만 실행
	if (!HasAuthority()) return;
	
	// 자동 획득 지연
	if (Options.AutoPickupDelay > 0.0f && !bRequireInteraction)
	{
		// 처음엔 CollectionSphere 콜리전 비활성화
		CollectionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		// 지연 후 활성화
		GetWorld()->GetTimerManager().SetTimer(AutoPickupTimerHandle,
		                                       this, &ABaseItem::EnableAutoPickup, Options.AutoPickupDelay, false);
	}
	else if (!bRequireInteraction)
	{
		// 지연이 없으면 바로 활성화
		CollectionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}

	// 물리 적용
	if (Options.bApplyPhysicsOnDrop)
	{
		FVector RandomDirection = FMath::VRand();
		RandomDirection.Z = FMath::Abs(RandomDirection.Z); // 위쪽으로만
		FVector DropImpulse = RandomDirection * Options.DropForce;

		// 멀티캐스트로 모든 클라이언트에 물리 적용
		Multicast_OnDropped(GetActorLocation(), DropImpulse);
	}

	// 수명 설정
	if (Options.LifeTime > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(LifeTimeTimerHandle,
		                                       this, &ABaseItem::OnLifeTimeExpired, Options.LifeTime, false);
	}
}

void ABaseItem::InitializeAsDroppedItem(int32 InItemID, int32 ItemValue, float DropDelay)
{
	FItemSpawnOptions Options;
	Options.bRequireInteraction = false;
	Options.ItemCount = ItemValue;
	Options.AutoPickupDelay = DropDelay;
	Options.bApplyPhysicsOnDrop = true;
	Options.DropForce = 600.0f;
	Options.LifeTime = 300.0f; // 5분 후 사라짐

	InitializeItem(InItemID, Options);
}

void ABaseItem::InitializeAsInteractableItem(int32 InItemID, int32 ItemValue, EItemInteractionType Type)
{
	FItemSpawnOptions Options;
	Options.bRequireInteraction = true;
	Options.InteractionType = Type;
	Options.ItemCount = ItemValue;
	Options.HoldDuration = (Type == EItemInteractionType::Hold) ? 2.0f : 0.0f;

	InitializeItem(InItemID, Options);
}

void ABaseItem::SetupComponents()
{
	// 메쉬 설정
	if (dt_ItemData->ItemMesh)
	{
		ItemMesh->SetStaticMesh(dt_ItemData->ItemMesh);
	}
	
	// 콜리전 설정
	if (!bRequireInteraction)
	{
		// 자동 획득 아이템
		CollectionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		if (!CollectionSphere->OnComponentBeginOverlap.IsBound())
		{
			CollectionSphere->OnComponentBeginOverlap.AddDynamic(this, &ABaseItem::OnOverlapBegin);
		}
	}
	else
	{
		// 상호작용 필요 아이템
		CollectionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// 위젯 설정
	if (DetectWidgetClass && DetectWidgetComponent)
	{
		DetectWidgetComponent->SetWidgetClass(DetectWidgetClass);
		
		if (UUserWidget* Widget = DetectWidgetComponent->GetUserWidgetObject())
		{
			if (UDetectWidget* DetectWidget = Cast<UDetectWidget>(Widget))
			{
				FString ItemName = GetInteractionName_Implementation();
				DetectWidget->SetInteractionInfo(ItemName);
			}
		}
		
		DetectWidgetComponent->SetVisibility(false);
		DetectWidgetComponent->SetActive(true);
	}
}

void ABaseItem::EnableAutoPickup()
{
	if (HasAuthority() && !bRequireInteraction)
	{
		// 자동 획득 지연이 끝나면 CollectionSphere 활성화
		CollectionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
}

void ABaseItem::OnLifeTimeExpired()
{
	if (HasAuthority())
	{
		Destroy();
	}
}

void ABaseItem::Multicast_OnDropped_Implementation(FVector DropLocation, FVector DropImpulse)
{
	// 물리 시뮬레이션 활성화 (ItemMesh가 담당)
	if (ItemMesh)
	{
		ItemMesh->SetSimulatePhysics(true);
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		ItemMesh->AddImpulse(DropImpulse);

		// 일정 시간 후 물리 비활성화
		FTimerHandle PhysicsTimer;
		GetWorld()->GetTimerManager().SetTimer(PhysicsTimer, [this]()
		{
			if (IsValid(this))
			{
				if (ItemMesh)
				{
					ItemMesh->SetSimulatePhysics(false);
					// 물리 충돌은 유지 (땅바닥에 놓임)
					ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				}
			}
		}, 2.0f, false);
	}
}

void ABaseItem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 감지된 상태에서 거리에 따른 위젯 페이드
	if (bIsDetected && DetectWidgetComponent && DetectWidgetComponent->IsVisible())
	{
		if (ASonheimPlayer* Player = Cast<ASonheimPlayer>(GetWorld()->GetFirstPlayerController()->GetPawn()))
		{
			float Distance = FVector::Dist(GetActorLocation(), Player->GetActorLocation());

			if (UDetectWidget* DetectWidget = Cast<UDetectWidget>(DetectWidgetComponent->GetUserWidgetObject()))
			{
				DetectWidget->UpdateDistanceFade(Distance, 300.0f);
			}
		}
	}
}

FString ABaseItem::GetInteractionName_Implementation() const
{
	return dt_ItemData ? dt_ItemData->ItemName.ToString() : FString("아이템");
}

void ABaseItem::UpdateHoldProgress(float Progress)
{
	// 클라이언트 측 UI 업데이트
	if (DetectWidgetComponent)
	{
		if (UDetectWidget* DetectWidget = Cast<UDetectWidget>(DetectWidgetComponent->GetUserWidgetObject()))
		{
			DetectWidget->UpdateHoldProgress(Progress);
		}
	}
}

void ABaseItem::Multicast_OnCollected_Implementation()
{
	// 모든 클라이언트에서 수집 효과 재생
	if (CollectionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), CollectionSound, GetActorLocation());
	}
	if (CollectionEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), CollectionEffect, GetActorLocation());
	}
}

bool ABaseItem::CanBeCollectedBy(ASonheimPlayer* Player)
{
	return !m_IsCollected && Player != nullptr && HasAuthority();
}

void ABaseItem::OnCollected(ASonheimPlayer* Player)
{
	// 서버에서만 실행
	if (!HasAuthority()) return;

	if (!m_IsCollected)
	{
		m_IsCollected = true;

		// 아이템 효과 적용
		ApplyEffect(Player);

		// 타이머 정리
		GetWorld()->GetTimerManager().ClearTimer(AutoPickupTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(LifeTimeTimerHandle);

		// 아이템 제거
		Destroy();
	}
}

void ABaseItem::ApplyEffect(ASonheimPlayer* Player)
{
	// 서버에서 실행
	if (HasAuthority())
	{
		Player->Reward(m_ItemID, m_ItemValue);
	}
}

void ABaseItem::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent,
                               AActor* OtherActor,
                               UPrimitiveComponent* OtherComp,
                               int32 OtherBodyIndex,
                               bool bFromSweep,
                               const FHitResult& SweepResult)
{
	// 서버에서만 처리
	if (!HasAuthority()) return;

	// 자동 획득 아이템만 처리
	if (ASonheimPlayer* Player = Cast<ASonheimPlayer>(OtherActor))
	{
		if (CanBeCollectedBy(Player))
		{
			OnCollected(Player);
			Multicast_OnCollected();
		}
	}
}

// IInteractableInterface 구현
bool ABaseItem::CanInteract_Implementation() const
{
	// 서버와 클라이언트 모두에서 체크
	return !m_IsCollected;
}

void ABaseItem::OnDetected_Implementation(bool bDetected)
{
	// 클라이언트 측 시각 효과
	if (bIsDetected == bDetected)
		return;

	bIsDetected = bDetected;

	if (DetectWidgetComponent)
	{
		DetectWidgetComponent->SetVisibility(bDetected);

		if (UDetectWidget* DetectWidget = Cast<UDetectWidget>(DetectWidgetComponent->GetUserWidgetObject()))
		{
			if (bIsDetected)
			{
				DetectWidget->PlayShowAnimation();
			}
			else
			{
				DetectWidget->PlayHideAnimation();
			}
		}
	}

	// 하이라이트 효과
	if (ItemMesh && HighlightMaterial)
	{
		ItemMesh->SetOverlayMaterial(bIsDetected ? HighlightMaterial : nullptr);
	}
}

void ABaseItem::Interact_Implementation(ASonheimPlayer* Player)
{
	if (CanInteract_Implementation() && CanBeCollectedBy(Player))
	{
		OnCollected(Player);
		Multicast_OnCollected();
	}
}

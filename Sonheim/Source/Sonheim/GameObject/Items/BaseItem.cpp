// BaseItem.cpp
#include "BaseItem.h"

#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
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
	ItemMesh->SetRelativeScale3D(FVector(1.f));
	ItemMesh->SetIsReplicated(true);

	// 메시가 물리를 담당
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ItemMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	ItemMesh->SetSimulatePhysics(false); // 기본적으로 비활성, 드롭 시 활성화

	// 임시 메시 설정
	static ConstructorHelpers::FObjectFinder<UStaticMesh> tempMesh(
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

	static ConstructorHelpers::FClassFinder<UUserWidget> detectWidgetClass(
		TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/_BluePrint/Widget/WBP_Detect.WBP_Detect_C'"));
	if (detectWidgetClass.Succeeded())
	{
		DetectWidgetClass = detectWidgetClass.Class;
	}

	RarityVFXComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("RarityVFXComp"));
	RarityVFXComp->SetupAttachment(ItemMesh);
	RarityVFXComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RarityVFXComp->SetAutoActivate(false);
	RarityVFXComp->SetUsingAbsoluteRotation(true);
	RarityVFXComp->SetUsingAbsoluteScale(true);

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> collectionEffect_common(
		TEXT("NiagaraSystem'/Game/_Resource/FX/ItemDropFx/Fx/NS_Common_Item.NS_Common_Item'"));
	if (collectionEffect_common.Succeeded())
	{
		RarityVFXMap.Add(EItemRarity::Common, collectionEffect_common.Object);
	}
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> collectionEffect_uncommon(
		TEXT("NiagaraSystem'/Game/_Resource/FX/ItemDropFx/Fx/NS_Uncommon_Item.NS_Uncommon_Item'"));
	if (collectionEffect_uncommon.Succeeded())
	{
		RarityVFXMap.Add(EItemRarity::Uncommon, collectionEffect_uncommon.Object);
	}
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> collectionEffect_rare(
		TEXT("NiagaraSystem'/Game/_Resource/FX/ItemDropFx/Fx/NS_Rare_System.NS_Rare_System'"));
	if (collectionEffect_rare.Succeeded())
	{
		RarityVFXMap.Add(EItemRarity::Rare, collectionEffect_rare.Object);
	}
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> collectionEffect_epic(
		TEXT("NiagaraSystem'/Game/_Resource/FX/ItemDropFx/Fx/NS_Epic_System.NS_Epic_System'"));
	if (collectionEffect_epic.Succeeded())
	{
		RarityVFXMap.Add(EItemRarity::Epic, collectionEffect_epic.Object);
	}
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> collectionEffect_legendary(
		TEXT("NiagaraSystem'/Game/_Resource/FX/ItemDropFx/Fx/NS_Legendary_System.NS_Legendary_System'"));
	if (collectionEffect_legendary.Succeeded())
	{
		RarityVFXMap.Add(EItemRarity::Legendary, collectionEffect_legendary.Object);
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
	ApplyRarityVFX();
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

	ApplyRarityVFX();

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

	SetReplicates(true);
	SetReplicateMovement(true);
	SetNetDormancy(DORM_Awake);  // 처음엔 깨어있는 상태

	ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ItemMesh->SetSimulatePhysics(true);
	ItemMesh->SetEnableGravity(true);
	ItemMesh->SetNotifyRigidBodyCollision(true); // 필요 시 Hit 이벤트

	// 초기 튕김용 물리 값 (상태 기반 최적화로만 조절)
	ItemMesh->BodyInstance.bUseCCD = true;      // 빠르게 튈 때만 안전하게
	ItemMesh->SetLinearDamping(0.05f);
	ItemMesh->SetAngularDamping(0.05f);

	// 랜덤 임펄스(드랍 연출)
	if (Options.bApplyPhysicsOnDrop)
	{
		//Z축(위)을 중심으로 45도 각도의 원뿔 내에서 랜덤 방향 생성
		const FVector ConeDirection = FVector::UpVector;
		const float ConeHalfAngleRad = FMath::DegreesToRadians(45.0f);
		FVector RandomDirection = FMath::VRandCone(ConeDirection, ConeHalfAngleRad);
		FVector DropImpulse = RandomDirection * Options.DropForce;
		ItemMesh->AddImpulse(DropImpulse);
	}

	// 수면/각성 이벤트 구독 
	ItemMesh->OnComponentSleep.AddDynamic(this, &ABaseItem::OnMeshSleep);
	ItemMesh->OnComponentWake .AddDynamic(this, &ABaseItem::OnMeshWake);

	// 수명 설정
	if (Options.LifeTime > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(LifeTimeTimerHandle,
		                                       this, &ABaseItem::OnLifeTimeExpired, Options.LifeTime, false);
	}
}

void ABaseItem::SetupComponents()
{
	// 메쉬 설정
	if (dt_ItemData && dt_ItemData->ItemMesh)
	{
		// 동일 메쉬면 교체 생략 (중복 호출 방지)
		if (ItemMesh->GetStaticMesh() != dt_ItemData->ItemMesh)
		{
			ItemMesh->SetStaticMesh(dt_ItemData->ItemMesh);
			ItemMesh->SetRelativeScale3D(dt_ItemData->MeshScale);
		}
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

void ABaseItem::OnRep_ItemID()
{
	if (!m_GameInstance) m_GameInstance = Cast<USonheimGameInstance>(GetGameInstance());
	if (m_GameInstance)
		dt_ItemData = m_GameInstance->GetDataItem(m_ItemID);

	// 메시 등 외형도 갱신
	SetupComponents();
	// 희귀도 VFX 반영
	ApplyRarityVFX();
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

void ABaseItem::OnMeshSleep(UPrimitiveComponent* Comp, FName)
{
	if (!HasAuthority()) return;

	// 잠들면 네트워크 도먼시로 전환 → 네트워크 부하 0에 수렴
	SetNetDormancy(DORM_DormantAll);

	// 안정화 이후 무거운 옵션만 완화
	if (auto* Mesh = Cast<UPrimitiveComponent>(Comp))
	{
		Mesh->BodyInstance.bUseCCD = false; 
		Mesh->SetLinearDamping(0.2f);
		Mesh->SetAngularDamping(0.4f);
	}
}

void ABaseItem::OnMeshWake(UPrimitiveComponent* Comp, FName)
{
	if (!HasAuthority()) return;

	// 다시 움직이기 시작 → 즉시 네트워크 갱신 재개
	FlushNetDormancy();
	SetNetDormancy(DORM_Awake);

	if (auto* Mesh = Cast<UPrimitiveComponent>(Comp))
	{
		// 초기 튕김 수준까진 아니어도 CCD 재가동
		Mesh->BodyInstance.bUseCCD = true;
		Mesh->SetLinearDamping(0.05f);
		Mesh->SetAngularDamping(0.05f);
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

void ABaseItem::UpdateHoldProgressUI_Implementation(float Progress, EHoldPurpose Purpose)
{
	if (DetectWidgetComponent)
	{
		if (auto* DW = Cast<UDetectWidget>(DetectWidgetComponent->GetUserWidgetObject()))
		{
			// 아이템은 취소 모드 없음
			DW->UpdateInteractProgress(Progress);
		}
	}
}

bool ABaseItem::CanHoldCancel_Implementation() const
{
	return false;
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

void ABaseItem::ApplyRarityVFX()
{
	if (!RarityVFXComp) return;

	UNiagaraSystem* Sys = nullptr;
	if (!dt_ItemData) return;
	
	Sys = RarityVFXMap.FindRef(dt_ItemData->ItemRarity);

	RarityVFXComp->SetAsset(Sys);
	if (Sys) RarityVFXComp->Activate(true);
	else RarityVFXComp->DeactivateImmediate();
}
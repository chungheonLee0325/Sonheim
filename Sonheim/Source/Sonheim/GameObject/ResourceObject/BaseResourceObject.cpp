// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseResourceObject.h"

#include "Components/BoxComponent.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "Sonheim/GameManager/SonheimGameMode.h"
#include "Sonheim/GameObject/Items/BaseItem.h"
#include "Sonheim/UI/FloatingDamageActor.h"
#include "Sonheim/UI/Widget/FloatingDamageWidget.h"


// Sets default values
ABaseResourceObject::ABaseResourceObject()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));

	m_BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	m_BoxComponent->SetCollisionProfileName("BlockAllDynamic");
	RootComponent = m_BoxComponent;

	m_StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	m_StaticMeshComponent->SetCollisionProfileName("NoCollision");
	m_StaticMeshComponent->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void ABaseResourceObject::BeginPlay()
{
	Super::BeginPlay();

	if (m_ResourceObjectID == 0)
	{
		return;
	}
	// 데이터 초기화 & Game Instance Setting
	m_GameInstance = Cast<USonheimGameInstance>(GetGameInstance());
	dt_ResourceObject = m_GameInstance->GetDataResourceObject(m_ResourceObjectID);

	HealthComponent->InitHealth(dt_ResourceObject->HPMax);
	
	// GameMode Setting
	m_GameMode = Cast<ASonheimGameMode>(GetWorld()->GetAuthGameMode());
}

float ABaseResourceObject::GetWeaknessModifier(EAttackType AttackType) const
{
	if (dt_ResourceObject->WeaknessAttackMap.Contains(AttackType))
	{
		return dt_ResourceObject->WeaknessAttackMap.FindRef(AttackType);
	}
	return 1.0f;
}

// Called every frame
void ABaseResourceObject::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ABaseResourceObject::OnDestroy()
{
	CanHarvest = false;

	if (dt_ResourceObject->DestroySoundID != 0)
	{
		m_GameMode->PlayPositionalSound(dt_ResourceObject->DestroySoundID, GetActorLocation());
	}

	Destroy();
}

// 부분 자원 스폰 함수 (구간별 개별 처리)
void ABaseResourceObject::SpawnPartialResources(int32 SegmentsLost)
{
	// 각 구간별로 개별적으로 처리
	for (int32 SegmentIdx = 0; SegmentIdx < SegmentsLost; ++SegmentIdx)
	{
		// 구간별 약간 다른 위치에서 스폰 (위치 분산)
		FVector SegmentSpawnBaseLocation = GetActorLocation();
		SegmentSpawnBaseLocation.X += FMath::RandRange(-50.0f, 50.0f) * SegmentIdx;
		SegmentSpawnBaseLocation.Y += FMath::RandRange(-50.0f, 50.0f) * SegmentIdx;

		// 각 가능한 드롭 아이템에 대해 처리
		for (const auto& DropInfoPair : dt_ResourceObject->PossibleDropItemID)
		{
			// 드롭 확률에 따라 아이템 드롭 여부 결정
			if (DropInfoPair.Value < FMath::RandRange(0, 100))
			{
				return;
			}

			FItemData* const DropInfo = m_GameInstance->GetDataItem(DropInfoPair.Key);

			// 단일 구간에 대한 드롭 수량 결정
			// 아이템 스폰 위치 계산 (구간별 위치 + 랜덤 오프셋)
			// 랜덤 오프셋 생성 (반경 내에서)
			const float SpawnRadius = 100.0f;
			FVector RandomOffset(
				FMath::RandRange(-SpawnRadius, SpawnRadius),
				FMath::RandRange(-SpawnRadius, SpawnRadius),
				0.0f
			);

			// 스폰 위치 계산
			FVector SpawnLocation = SegmentSpawnBaseLocation + RandomOffset;
			SpawnLocation.Z += 50.0f + (10.0f * SegmentIdx); // 구간별로 약간 다른 높이

			// 스폰 회전 (랜덤)
			FRotator SpawnRotation(0.0f, FMath::RandRange(0.0f, 360.0f), 0.0f);
			FTransform SpawnTransform(SpawnRotation, SpawnLocation);

			if (ABaseItem* SpawnedItem = GetWorld()->SpawnActor<ABaseItem>(
				DropInfo->ItemClass, SpawnTransform))
			{
				// 아이템 초기화
				//SpawnedItem->InitializeItem(DropInfo.ItemID, 1);

				// 물리 시뮬레이션 활성화
				if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(SpawnedItem->GetRootComponent()))
				{
					PrimComp->SetSimulatePhysics(true);
					// 랜덤한 방향으로 약간 튀어오르게
					FVector RandomImpulse(
						FMath::RandRange(-50.0f, 50.0f),
						FMath::RandRange(-50.0f, 50.0f),
						FMath::RandRange(100.0f, 200.0f)
					);
					PrimComp->AddImpulse(RandomImpulse);
				}
			}
		}
	}
}


float ABaseResourceObject::TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator,
                                      AActor* DamageCauser)
{
	if (!CanHarvest)
		return 0.0f;

	FHitResult hitResult;
	FVector hitDir;
	FAttackData attackData;

	if (DamageEvent.IsOfType(FCustomDamageEvent::ClassID))
	{
		FCustomDamageEvent* const customDamageEvent = (FCustomDamageEvent*)&DamageEvent;
		attackData = customDamageEvent->AttackData;
		customDamageEvent->GetBestHitInfo(this, DamageCauser, hitResult, hitDir);
	}

	float ActualDamage = Damage;

	float damageCoefficient = GetWeaknessModifier(attackData.AttackType);
	ActualDamage = ActualDamage * damageCoefficient;

	// 데미지 적용 전 현재 체력 확인
	float PreviousHP = HealthComponent->GetHP();

	// 데미지 적용
	float CurrentHP = HealthComponent->DecreaseHP(ActualDamage);

	// 임계값 계산 (최대 체력의 10%)
	float ThresholdHP = HealthComponent->GetMaxHP() * dt_ResourceObject->DamageThresholdPct;

	// 체력 변화량 확인
	float HPChange = PreviousHP - CurrentHP;

	// 현재 체력 비율 계산
	float CurrentHPRatio = CurrentHP / HealthComponent->GetMaxHP();

	// 이전 체력 구간 계산 (10% 단위로)
	int32 PreviousHPSegment = FMath::FloorToInt(PreviousHP / ThresholdHP);

	// 현재 체력 구간 계산 (10% 단위로)
	int32 CurrentHPSegment = FMath::FloorToInt(CurrentHP / ThresholdHP);

	// 구간이 변경되었는지 확인 (10% 단위로 체력이 감소했는지)
	if (CurrentHPSegment < PreviousHPSegment)
	{
		// 10% 임계값을 넘어서 데미지를 입은 경우 일부 아이템 스폰
		SpawnPartialResources(PreviousHPSegment - CurrentHPSegment);
	}

	if (FMath::IsNearlyZero(CurrentHP))
	{
		if (CanHarvest)
		{
			OnDestroy();
		}
	}

	// Spawn floating damage

	//FVector SpawnLocation = GetActorLocation();
	FVector SpawnLocation = hitResult.Location;

	FTransform SpawnTransform(FRotator::ZeroRotator, SpawnLocation);

	if (AFloatingDamageActor* DamageActor = GetWorld()->SpawnActor<AFloatingDamageActor>(
		AFloatingDamageActor::StaticClass(), SpawnTransform))
	{
		// FloatingDamageType 계산
		EFloatingOutLineDamageType damageType = damageCoefficient > 1.0f
			                                        ? EFloatingOutLineDamageType::WeakPointDamage
			                                        : EFloatingOutLineDamageType::Normal;
		DamageActor->Initialize(ActualDamage, damageType);
	}

	// Spawn Harvest SFX
	if (dt_ResourceObject->HarvestSoundID != 0)
	{
		m_GameMode->PlayPositionalSound(dt_ResourceObject->HarvestSoundID, GetActorLocation());
	}

	return ActualDamage;
}

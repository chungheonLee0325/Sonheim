// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseResourceObject.h"

#include "NiagaraFunctionLibrary.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "Sonheim/GameManager/SonheimGameMode.h"
#include "Sonheim/GameObject/Items/BaseItem.h"
#include "Sonheim/UI/FloatingDamageActor.h"
#include "Sonheim/UI/FloatingDamagePool.h"
#include "Sonheim/UI/Widget/FloatingDamageWidget.h"
#include "Sonheim/Utilities/SonheimUtility.h"


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

	bReplicates = true;
	SetReplicateMovement(true);
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

	if (dt_ResourceObject && dt_ResourceObject->ResourceMesh)
	{
		m_StaticMeshComponent->SetStaticMesh(dt_ResourceObject->ResourceMesh);
		m_StaticMeshComponent->SetRelativeScale3D(dt_ResourceObject->MeshScale);
	}

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
	if (!HasAuthority()) return;
	
	CanHarvest = false;

	// 남은 자원 모두 드롭
	if (dt_ResourceObject->PossibleDropItemID.Num() > 0)
	{
		// 파괴 시 추가 보너스 드롭
		SpawnPartialResources(1);
	}

	if (dt_ResourceObject->DestroySoundID != 0)
	{
		m_GameMode->PlayPositionalSound(dt_ResourceObject->DestroySoundID, GetActorLocation());
	}

	Destroy();
}

// 부분 자원 스폰 함수 (구간별 개별 처리)
void ABaseResourceObject::SpawnPartialResources(int32 SegmentsLost) const
{
	if (!HasAuthority()) return;

	FItemSpawnOptions Opt;
	Opt.bRequireInteraction = false;
	Opt.ItemCount = 1;
	Opt.AutoPickupDelay = 1.0f;
	Opt.bApplyPhysicsOnDrop = true;
	Opt.DropForce = 1500.f;
	Opt.LifeTime = 300.f;

	const TMap<int32, int32>& DropChance = dt_ResourceObject->PossibleDropItemID;

	USonheimUtility::SpawnFromDropTableWithOptions(
		this,
		DropChance,
		GetActorLocation(),
		SegmentsLost,
		100.f,
		Opt
	);
}


float ABaseResourceObject::TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator,
                                      AActor* DamageCauser)
{
	if (GetLocalRole() != ROLE_Authority)
	{
		return 0.0f; // 클라이언트에서는 데미지 처리 안함
	}
	
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
	HealthComponent->ModifyHP(-ActualDamage);
	float CurrentHP = HealthComponent->GetHP();

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
			MulticastDestroyEffect();
			OnDestroy();
		}
	}

	MulticastDamageEffect(ActualDamage, hitResult.Location, DamageCauser, damageCoefficient, attackData);

	return ActualDamage;
}

void ABaseResourceObject::MulticastDamageEffect_Implementation(float Damage, FVector HitLocation, AActor* DamageCauser,
                                                               float DamageCoefficient,const FAttackData& AttackData)
{
	// Spawn floating damage
	FVector SpawnLocation = HitLocation;

	FTransform SpawnTransform(FRotator::ZeroRotator, SpawnLocation);

	// 풀 매니저를 통해 데미지 표시 요청
	if (AFloatingDamagePool* DamagePool = AFloatingDamagePool::GetInstance(GetWorld()))
	{
		FDamageNumberRequest Request;
		Request.Damage = Damage;
		Request.WorldLocation = HitLocation;
		Request.DamageCauser = DamageCauser;
		Request.DamagedActor = this;

		// 약점 타입 설정
		Request.WeakPointType = DamageCoefficient > 1.0f
			                        ? EFloatingOutLineDamageType::WeakPointDamage
			                        : EFloatingOutLineDamageType::Normal;

		// 속성 타입 설정
		Request.ElementAttributeType = EFloatingTextDamageType::Normal;

		DamagePool->RequestDamageNumber(Request);
	}

	// Spawn Harvest SFX
	if (dt_ResourceObject->HarvestSoundID != 0)
	{
		m_GameMode->PlayPositionalSound(dt_ResourceObject->HarvestSoundID, GetActorLocation());
	}
	// Spawn Harvest VFX
	if (dt_ResourceObject->HarvestEffect != nullptr)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), dt_ResourceObject->HarvestEffect, SpawnLocation);
	}

	// Spawn Hit SFX
	if (AttackData.HitSFX != nullptr)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), AttackData.HitSFX, HitLocation);
	}

	// Spawn Hit VFX
	if (AttackData.HitVFX_N != nullptr)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), AttackData.HitVFX_N, HitLocation,
													   FRotator::ZeroRotator, FVector(1.f) * AttackData.VFXScale);
	}
	else if (AttackData.HitVFX_P != nullptr)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), AttackData.HitVFX_P, HitLocation,
												 FRotator::ZeroRotator, FVector(1.f) * AttackData.VFXScale);
	}
}

void ABaseResourceObject::MulticastDestroyEffect_Implementation()
{
	// Spawn Destroy SFX
	if (dt_ResourceObject->DestroySoundID != 0)
	{
		m_GameMode->PlayPositionalSound(dt_ResourceObject->DestroySoundID, GetActorLocation());
	}
	// Spawn Harvest VFX
	if (dt_ResourceObject->DestroyEffect != nullptr)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), dt_ResourceObject->DestroyEffect, GetActorLocation());
	}
}
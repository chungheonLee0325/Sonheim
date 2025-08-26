// Fill out your copyright notice in the Description page of Project Settings.


#include "AreaObject.h"

#include "NiagaraFunctionLibrary.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Sonheim/AreaObject/Attribute/HealthComponent.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Sonheim/AreaObject/Attribute/LevelComponent.h"
#include "Sonheim/AreaObject/Skill/Base/BaseSkill.h"
#include "Sonheim/Utilities/LogMacro.h"
#include "Sonheim/AreaObject/Attribute/StaminaComponent.h"
#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/AreaObject/Utility/MoveUtilComponent.h"
#include "Sonheim/AreaObject/Utility/RotateUtilComponent.h"
#include "Sonheim/GameManager/SonheimGameMode.h"
#include "Sonheim/UI/FloatingDamageActor.h"
#include "Sonheim/Utilities/SonheimUtility.h"
#include "Net/UnrealNetwork.h"
#include "Sonheim/Animation/Common/AnimInstance/BaseAnimInstance.h"
#include "Sonheim/GameObject/ResourceObject/BaseResourceObject.h"
#include "Sonheim/UI/FloatingDamagePool.h"
#include "NiagaraComponent.h"

// Sets default values
AAreaObject::AAreaObject()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// 액터 리플리케이션 활성화
	bReplicates = true;
	bNetUseOwnerRelevancy = true;
	SetNetUpdateFrequency(66.0f);
	NetPriority = 1.0f;

	// Health Component 생성
	m_HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));

	// Stamina Component 생성
	m_StaminaComponent = CreateDefaultSubobject<UStaminaComponent>(TEXT("StaminaComponent"));

	// Condition Component 생성
	m_ConditionComponent = CreateDefaultSubobject<UConditionComponent>(TEXT("ConditionComponent"));

	// Level Component 생성
	m_LevelComponent = CreateDefaultSubobject<ULevelComponent>(TEXT("LevelComponent"));

	// Rotation Component 생성
	m_RotateUtilComponent = CreateDefaultSubobject<URotateUtilComponent>(TEXT("RotateUtilComponent"));

	// MoveUtil Component 생성
	m_MoveUtilComponent = CreateDefaultSubobject<UMoveUtilComponent>(TEXT("MoveUtilComponent"));

	//GetCapsuleComponent()->SetSimulatePhysics(true);
	GetCapsuleComponent()->SetNotifyRigidBodyCollision(true);

	// 서버가 렌더하지 않아도 본 업데이트/노티파이 진행
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	// 최적화 끄기(확인후 프레임 저하시 주석처리)
	GetMesh()->bEnableUpdateRateOptimizations = false;
}

void AAreaObject::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 사망 상태 리플리케이션
	DOREPLIFETIME(AAreaObject, bIsDead);
}

UBaseAnimInstance* AAreaObject::GetSAnimInstance() const
{
	return m_AnimInstance;
}

bool AAreaObject::CanAttack(AActor* TargetActor)
{
	// ToDo : Handle로 변경하는게 좋을듯... resource object 처리 핸들 , monster 처리 핸들 ....
	AAreaObject* targetAreaObject = Cast<AAreaObject>(TargetActor);
	if (targetAreaObject != nullptr)
	{
		if (targetAreaObject->HasCondition(EConditionBitsType::Dead) || targetAreaObject->
			HasCondition(EConditionBitsType::Invincible) || targetAreaObject->HasCondition(
				EConditionBitsType::Invincible))
		{
			return false;
		}
	}

	return true;
}

// Called when the game starts or when spawned
void AAreaObject::BeginPlay()
{
	Super::BeginPlay();

	// AreaObject ID 는 반드시 셋팅되어야 함!!
	if (m_AreaObjectID == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Area Object ID is 0!!!"));
		return;
	}

	// 데이터 초기화
	m_GameInstance = Cast<USonheimGameInstance>(GetGameInstance());
	dt_AreaObject = m_GameInstance->GetDataAreaObject(m_AreaObjectID);

	// Init Attribute By Data
	float hpMax = 1.0f;
	float maxStamina = 100.0f; // Assuming a default value, actual implementation needed
	float staminaRecoveryRate = 20.f;
	float groggyDuration = 5.f;
	float walkSpeed = 400.0f;

	if (dt_AreaObject != nullptr)
	{
		hpMax = dt_AreaObject->HPMax;
		m_OwnSkillIDSet = dt_AreaObject->SkillList;
		maxStamina = dt_AreaObject->StaminaMax;
		staminaRecoveryRate = dt_AreaObject->StaminaRecoveryRate;
		groggyDuration = dt_AreaObject->GroggyDuration;
		walkSpeed = dt_AreaObject->WalkSpeed;
	}

	// 서버에서만 초기화
	if (HasAuthority() && dt_AreaObject)
	{
		// Component 초기화
		m_HealthComponent->InitHealth(dt_AreaObject->HPMax);
		m_StaminaComponent->InitStamina(
			dt_AreaObject->StaminaMax, 
			dt_AreaObject->StaminaRecoveryRate, 
			dt_AreaObject->GroggyDuration
		);
		m_LevelComponent->InitLevel(this);
        
		GetCharacterMovement()->MaxWalkSpeed = dt_AreaObject->WalkSpeed;
	}

	// 스킬 인스턴스 생성
	for (auto& skill : m_OwnSkillIDSet)
	{
		if (FSkillData* skillData = m_GameInstance->GetDataSkill(skill))
		{
			FString SkillName = FString::Printf(TEXT("BaseSkill_%d"), skill);
			UBaseSkill* NewSkill = NewObject<UBaseSkill>(this, skillData->SkillClass, *SkillName);
			NewSkill->InitSkill(skillData);
			m_SkillInstanceMap.Add(skill, NewSkill);
		}
		else
		{
			LOG_SCREEN_MY(4.0f, FColor::Red, "%d 해당 아이디의 스킬이 존재하지 않습니다.", skill);
			UE_LOG(LogTemp, Error, TEXT("Skill ID is 0!!!"));
		}
	}

	// GameMode Setting
	m_GameMode = Cast<ASonheimGameMode>(GetWorld()->GetAuthGameMode());

	// AnimInstance Setting
	m_AnimInstance = Cast<UBaseAnimInstance>(GetMesh()->GetAnimInstance());
}

void AAreaObject::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

FName AAreaObject::DetermineDirection(const FVector& TargetPos) const
{
	FVector StartPos = GetActorLocation();
	// 공격 위치에서 방어자로의 방향 벡터
	FVector Direction = (StartPos - TargetPos).GetSafeNormal();

	// 방어자의 로컬 좌우, 상하 방향 계산
	FVector RightVector = FVector::CrossProduct(GetActorForwardVector(), FVector::UpVector);

	// 내적을 통해 방향 각도 계산
	float ForwardDot = FVector::DotProduct(Direction, GetActorForwardVector());
	float RightDot = FVector::DotProduct(Direction, RightVector);
	float UpDot = FVector::DotProduct(Direction, FVector::UpVector);

	// 각도 기반 방향 판정
	if (FMath::Abs(UpDot) > FMath::Abs(RightDot))
	{
		// 수직 방향이 더 강함
		//return (UpDot > 0) ? EReactionDirection::UP : EReactionDirection::DOWN;
		return (UpDot > 0) ? FName("UP") : FName("DOWN");
	}
	else
	{
		// 수평 방향이 더 강함
		//return (RightDot > 0) ? EReactionDirection::RIGHT : EReactionDirection::LEFT;
		return (RightDot > 0) ? FName("RIGHT") : FName("LEFT");
	}
}

float AAreaObject::HandleAttackDamageCalculation(float Damage)
{
	return Damage;
}

float AAreaObject::HandleDefenceDamageCalculation(float Damage)
{
	return Damage;
}

// Called every frame
void AAreaObject::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority() && m_CurrentSkill != nullptr)
	{
		m_CurrentSkill->OnCastTick(DeltaTime);
	}
}

// Called to bind functionality to input
void AAreaObject::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AAreaObject::CalcDamage(FAttackData& AttackData, AActor* Caster, AActor* Target, FHitResult& HitInfo)
{
	// IFF 확인후 공격 가능
	if (!CanAttack(Target))
		return;

	// 클라이언트는 서버에 요청
	if (!HasAuthority())
	{
		Server_CalcDamage(AttackData, Caster, Target, HitInfo);
		return;
	}

	// 서버는 직접 처리
	float Damage = FMath::RandRange(AttackData.HealthDamageAmountMin, AttackData.HealthDamageAmountMax);
	Damage = HandleAttackDamageCalculation(Damage);

	// Stab Damage Process - 포켓몬 자속기
	for (auto& elementAttribute : this->dt_AreaObject->DefenceElementalAttributes)
	{
		if (elementAttribute == AttackData.AttackElementalAttribute)
		{
			Damage *= 1.2f;
			break;
		}
	}

	if (!Target) return;

	FCustomDamageEvent DamageEvent;
	DamageEvent.AttackData = AttackData;
	DamageEvent.HitInfo = HitInfo;
	DamageEvent.Damage = Damage;

	Target->TakeDamage(Damage, DamageEvent, GetController(), this);
}


void AAreaObject::Server_CalcDamage_Implementation(FAttackData AttackData, AActor* Caster, AActor* Target,
												   FHitResult HitInfo)
{
	// 서버에서 실행할 데미지 계산 로직
	CalcDamage(AttackData, Caster, Target, HitInfo);
}

float AAreaObject::TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator,
                              AActor* DamageCauser)
{
// 서버에서만 처리
    if (!HasAuthority())
        return 0.0f;

    // IFF 및 상태 체크
    AAreaObject* damageCauser = Cast<AAreaObject>(DamageCauser);
	if (damageCauser->CanAttack(this) == false)
	{
		return 0.0f;
	}

    if (bIsDead || HasCondition(EConditionBitsType::Invincible) || HasCondition(EConditionBitsType::Hidden))
        return 0.0f;

	// Instigator 설정 - 경험치 보상에 사용 -> Aggro System으로 확장시 변경 예정
	SetInstigator(EventInstigator->GetPawn());
	
    // 데미지 계산
    float ActualDamage = HandleDefenceDamageCalculation(Damage);
    
    FHitResult hitResult;
    FVector hitDir;
    FAttackData attackData;
    bool bIsWeakPointHit = false;
    float elementDamageMultiplier = 1.0f;

    if (DamageEvent.IsOfType(FCustomDamageEvent::ClassID))
    {
        FCustomDamageEvent* const customDamageEvent = (FCustomDamageEvent*)&DamageEvent;
        attackData = customDamageEvent->AttackData;
        customDamageEvent->GetBestHitInfo(this, DamageCauser, hitResult, hitDir);

        bIsWeakPointHit = IsWeakPointHit(hitResult.Location);

        if (attackData.bEnableHitStop)
            ApplyHitStop(attackData.HitStopDuration);

        HandleKnockBack(DamageCauser->GetActorLocation(), attackData, m_KnockBackForceMultiplier);
    }

    // 약점 및 속성 계산
    ActualDamage = bIsWeakPointHit ? ActualDamage * 1.5f : ActualDamage;
    
    for (auto defectElementalAttribute : dt_AreaObject->DefenceElementalAttributes)
    {
        elementDamageMultiplier *= USonheimUtility::CalculateDamageMultiplier(
            defectElementalAttribute, attackData.AttackElementalAttribute);
    }
    ActualDamage *= elementDamageMultiplier;

    // HP 감소 (Component가 알아서 동기화)
    float CurrentHP = DecreaseHP(ActualDamage);
    
    // 사망 처리
    if (FMath::IsNearlyZero(CurrentHP) && !bIsDead)
    {
        bIsDead = true;  // Replicated 변수
    	OnRep_IsDead();
        OnDie();  // Multicast로 모두에게 전파
    }

    // 스태미나 감소
    DecreaseStamina(attackData.StaminaDamageAmount);

    // 시각 효과 전파
    FVector spawnLocation = hitResult.Location != FVector::ZeroVector ? hitResult.Location : GetActorLocation();
    MulticastDamageEffect(ActualDamage, spawnLocation, DamageCauser, bIsWeakPointHit, elementDamageMultiplier, attackData);

    return ActualDamage;
}


void AAreaObject::StopRotate() const
{
	m_RotateUtilComponent->StopRotation();
}

void AAreaObject::StopMove() const
{
	m_MoveUtilComponent->StopMovement();
}

void AAreaObject::StopAll()
{
	m_MoveUtilComponent->StopMovement();
	m_RotateUtilComponent->StopRotation();
	if (m_CurrentSkill != nullptr)
	{
		if(HasAuthority())
		{
			m_CurrentSkill->CancelCast();
		}
		ClearCurrentSkill();
	}
}

void AAreaObject::OnDie_Implementation()
{
	if (HasAuthority())
	{
		// 서버: 게임플레이 로직
		StopAll();
		AddCondition(EConditionBitsType::Dead);

		if (AAreaObject* Killer = Cast<AAreaObject>(GetInstigator()))
		{
			OnKill(Killer);
		}
        
		// 타이머로 제거 예약
		// GetWorld()->GetTimerManager().SetTimer(DeathTimerHandle, 
		// 	[this]() { 
		// 		if (IsValid(this)) 
		// 			Destroy(); 
		// 	}, 
		// 	DestroyDelayTime, false);
	}

	// 모든 곳: 시각적 효과
	if (UAnimInstance* AnimInst = GetMesh()->GetAnimInstance())
	{
		AnimInst->StopAllMontages(0.1f);
		if (dt_AreaObject->Die_AnimMontage)
		{
			AnimInst->Montage_Play(dt_AreaObject->Die_AnimMontage);
		}
	}

	// 죽음 이펙트
	if (DeathEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), DeathEffect, GetActorLocation());
	}
}

void AAreaObject::OnKill(AAreaObject* Killer)
{
	// 죽인 사람에게 Reward 처리
	if (Killer)
	{
		Killer->m_LevelComponent->AddExp(this->m_LevelComponent->RewardHuntExp());
	}
}

void AAreaObject::OnRevival()
{
	// Die Montage 종료
	StopAnimMontage();

	// condition 제거
	RemoveCondition(EConditionBitsType::Dead);
	//콜리전 활성화
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	m_HealthComponent->InitHealth(dt_AreaObject->HPMax);
	m_StaminaComponent->InitStamina(dt_AreaObject->StaminaMax, dt_AreaObject->StaminaRecoveryRate,
	                                dt_AreaObject->GroggyDuration);
}

UBaseSkill* AAreaObject::GetCurrentSkill()
{
	if (false == IsValid(m_CurrentSkill))
	{
		if (m_CurrentSkill != nullptr)
		{
			LOG_PRINT(TEXT("스킬 댕글링 포인터 문제발생!!!!"));
		}
		m_CurrentSkill = nullptr;
		return nullptr;
	}
	return m_CurrentSkill;
}

FAttackData* AAreaObject::GetCurrentSkillAttackData(int Index)
{
	if (false == IsValid(m_CurrentSkill))
	{
		LOG_PRINT(TEXT("스킬 댕글링 포인터 문제발생!!!!"));
		m_CurrentSkill = nullptr;
		return nullptr;
	}
	return m_CurrentSkill->GetAttackDataByIndex(Index);
}

void AAreaObject::UpdateCurrentSkill(UBaseSkill* NewSkill)
{
	if (!IsValid(NewSkill))
	{
		LOG_PRINT(TEXT("스킬 댕글링 포인터 문제발생!!!!"));
		return;
	}

	m_CurrentSkill = NewSkill;
}

UBaseSkill* AAreaObject::GetSkillByID(int SkillID)
{
	auto skillPointer = m_SkillInstanceMap.Find(SkillID);

	if (skillPointer == nullptr || !IsValid(*skillPointer))
	{
		//LOG_PRINT(TEXT("스킬 댕글링 포인터 문제발생!!!!"));
		return nullptr;
	}
	return *skillPointer;
}

bool AAreaObject::CanCastSkill(UBaseSkill* Skill, AAreaObject* Target)
{
	if (nullptr != m_CurrentSkill)
	{
		LOG_PRINT(TEXT("현재 스킬 사용중. m_CurrentSkill 초기화 후 사용"));
		return false;
	}

	if (Skill == nullptr) LOG_PRINT(TEXT("Skill is Empty"));
	if (Target == nullptr) LOG_PRINT(TEXT("Target is Empty"));

	return Skill && Skill->CanCast(this, Target);
}

bool AAreaObject::CanCastNextSkill(UBaseSkill* Skill, AAreaObject* Target)
{
	return bCanNextSkill && Skill && Skill->CanCast(this, Target);
}

bool AAreaObject::CastSkill(UBaseSkill* Skill, AAreaObject* Target)
{
	if (CanCastSkill(Skill, Target))
	{
		// 권한 체크
		// 클라이언트에서는 서버에 요청 보냄
		Server_CastSkill(Skill->GetSkillID(), Target);
		return true;
	}
	else
	{
		FString fail = UEnum::GetValueAsString(Skill->SkillFailCase);
		LOG_PRINT(TEXT("CastSkill Failed: %s"), *fail);
		return false;
	}
}

void AAreaObject::Server_CastSkill_Implementation(int SkillID, AAreaObject* Target)
{
	UBaseSkill* Skill = GetSkillByID(SkillID);
	Skill->OnCastStart(this, Target);
	MultiCast_CastSkill(SkillID, Target);
}

void AAreaObject::MultiCast_CastSkill_Implementation(int SkillID, AAreaObject* Target)
{
	if (m_GameInstance == nullptr) return;
	UBaseSkill* Skill = GetSkillByID(SkillID);
	FSkillData* SkillData = m_GameInstance->GetDataSkill(SkillID);

	if (!HasAuthority()) m_AnimInstance->Montage_Play(SkillData->Montage);
	UpdateCurrentSkill(Skill);
}


void AAreaObject::ClearCurrentSkill()
{
	if (m_CurrentSkill != nullptr)
	{
		m_CurrentSkill->CancelCast();
		m_CurrentSkill = nullptr;
	}
}

void AAreaObject::ClearThisCurrentSkill(UBaseSkill* Skill)
{
	if (m_CurrentSkill == Skill)
	{
		m_CurrentSkill = nullptr;
	}
}

bool AAreaObject::AddCondition(EConditionBitsType AddConditionType, float Duration)
{
	m_ConditionComponent->AddCondition(AddConditionType, Duration);
	return HasCondition(AddConditionType);
}

bool AAreaObject::RemoveCondition(EConditionBitsType RemoveConditionType) const
{
	m_ConditionComponent->RemoveCondition(RemoveConditionType);
	return HasCondition(RemoveConditionType);
}

bool AAreaObject::HasCondition(EConditionBitsType HasConditionType) const
{
	return m_ConditionComponent->HasCondition(HasConditionType);
}

bool AAreaObject::ExchangeDead() const
{
	m_ConditionComponent->ExchangeDead();
	return HasCondition(EConditionBitsType::Dead);
}

void AAreaObject::MoveActorTo(const FVector& TargetPosition, float Duration, EMovementInterpolationType InterpType,
                              bool bStickToGround)
{
	m_MoveUtilComponent->MoveActorTo(TargetPosition, Duration, InterpType, bStickToGround);
}

void AAreaObject::MoveActorToWithSpeed(const FVector& TargetPosition, float Speed,
                                       EMovementInterpolationType InterpType, bool bStickToGround)
{
	m_MoveUtilComponent->MoveActorToWithSpeed(TargetPosition, Speed, InterpType, bStickToGround);
}

void AAreaObject::LookAtLocation(const FVector& TargetLocation, EPMRotationMode Mode, float DurationOrSpeed,
                                 float Ratio,
                                 EMovementInterpolationType InterpType)
{
	m_RotateUtilComponent->LookAtLocation(TargetLocation, Mode, DurationOrSpeed, Ratio, InterpType);
}

void AAreaObject::LookAtLocationDirect(const FVector& TargetLocation) const
{
	m_RotateUtilComponent->LookAtLocationDirect(TargetLocation);
}

float AAreaObject::IncreaseHP(float Delta)
{
	m_HealthComponent->ModifyHP(Delta);
	return m_HealthComponent->GetHP();
}

float AAreaObject::DecreaseHP(float Delta)
{
	m_HealthComponent->ModifyHP(-Delta);
	return m_HealthComponent->GetHP();
}


void AAreaObject::SetHPByRate(float Rate)
{
	m_HealthComponent->SetHPByRate(Rate);
}

float AAreaObject::GetHP() const
{
	return m_HealthComponent->GetHP();
}

float AAreaObject::GetMaxHP() const
{
	return m_HealthComponent->GetMaxHP();
}

bool AAreaObject::IsMaxHP() const
{
	return FMath::IsNearlyEqual(GetHP(), GetMaxHP());
}

float AAreaObject::IncreaseStamina(float Delta)
{
	m_StaminaComponent->ModifyStamina(Delta);
	return m_StaminaComponent->GetStamina();
}

float AAreaObject::DecreaseStamina(float Delta, bool bIsDamaged)
{
	m_StaminaComponent->ModifyStamina(-Delta, bIsDamaged);
	return m_StaminaComponent->GetStamina();
}

float AAreaObject::GetStamina() const
{
	return m_StaminaComponent->GetStamina();
}

bool AAreaObject::CanUseStamina(float Cost) const
{
	return m_StaminaComponent->CanUseStamina(Cost);
}

void AAreaObject::ApplyHitStop(float Duration)
{
	// 월드 전체 시간 조절
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.008f);

	// 타이머로 원래 속도로 복구
	GetWorld()->GetTimerManager().SetTimer(
		HitStopTimerHandle,
		this,
		&AAreaObject::ResetTimeScale,
		Duration * 0.008f, // 실제 시간으로 변환
		false
	);
}

void AAreaObject::ResetTimeScale() const
{
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);
}

void AAreaObject::HandleKnockBack(const FVector& TargetPos, const FAttackData& AttackData, float ForceCoefficient)
{
	if (AttackData.KnockBackForce > 0.0f && m_KnockBackForceMultiplier > 0.0f)
	{
		FVector knockBackDir;
		if (AttackData.bUseCustomKnockBackDirection)
		{
			// knockBackDir = GetActorLocation() + AttackData.KnockBackDirection.GetSafeNormal() * AttackData.
			// 	KnockBackForce * m_KnockBackForceMultiplier * ForceCoefficient;
			knockBackDir = AttackData.KnockBackDirection.GetSafeNormal();

			//FQuat Rotation{FQuat(GetActorRotation())};
			knockBackDir = GetActorRotation().RotateVector(knockBackDir);
		}
		else
		{
			// 기본적으로 타격 방향으로 넉백
			knockBackDir = (GetActorLocation() - TargetPos).GetSafeNormal2D();
			//knockBackDir = (GetActorLocation() - hitResult.Location).GetSafeNormal2D();
		}
		FVector knockBackLocation = GetActorLocation() + knockBackDir * AttackData.KnockBackForce *
			m_KnockBackForceMultiplier * ForceCoefficient;

		ApplyKnockBack(knockBackLocation);
	}
}

void AAreaObject::ApplyKnockBack(const FVector& KnockBackForce)
{
	MoveActorTo(KnockBackForce, KnockBackDuration, EMovementInterpolationType::EaseOut);
}

bool AAreaObject::IsWeakPointHit(const FVector& HitLoc)
{
	return false;
}

void AAreaObject::PlayGlobalSound(int SoundID)
{
	if (m_GameMode == nullptr)
	{
		LOG_PRINT(TEXT("GameMode nullptr"));
	}
	m_GameMode->PlayGlobalSound(SoundID);
}

void AAreaObject::PlayPositionalSound(int SoundID, FVector Position)
{
	if (m_GameMode == nullptr)
	{
		LOG_PRINT(TEXT("GameMode nullptr"));
	}
	m_GameMode->PlayPositionalSound(SoundID, Position);
}

void AAreaObject::PlayBGM(int SoundID, bool bLoop)
{
	if (m_GameMode == nullptr)
	{
		LOG_PRINT(TEXT("GameMode nullptr"));
	}
	m_GameMode->PlayBGM(SoundID, bLoop);
}

void AAreaObject::StopBGM()
{
	if (m_GameMode == nullptr)
	{
		LOG_PRINT(TEXT("GameMode nullptr"));
	}
	m_GameMode->StopBGM();
}

float AAreaObject::GetCurrentSpeedRatio() const
{
	if (GetCharacterMovement())
	{
		const float MaxSpeed = GetCharacterMovement()->MaxWalkSpeed;
		if (MaxSpeed > 0.f)
		{
			// 현재 속도를 최대 속도로 나누어 0~1 사이의 값으로 정규화
			return GetVelocity().Size() / MaxSpeed;
		}
	}
	return 0.0f;
}

void AAreaObject::MulticastDamageEffect_Implementation(float Damage, FVector HitLocation, AActor* DamageCauser,
                                                       bool bWeakPoint, float ElementDamageMultiplier,
                                                       const FAttackData& AttackData)
{
    // 풀 매니저를 통해 데미지 표시 요청
    if (AFloatingDamagePool* DamagePool = AFloatingDamagePool::GetInstance(GetWorld()))
    {
        FDamageNumberRequest Request;
        Request.Damage = Damage;
        Request.WorldLocation = HitLocation;
        Request.DamageCauser = DamageCauser;
        Request.DamagedActor = this;
        
        // 약점 타입 설정
        Request.WeakPointType = bWeakPoint 
            ? EFloatingOutLineDamageType::WeakPointDamage 
            : EFloatingOutLineDamageType::Normal;
        
        // 속성 타입 설정
        if (ElementDamageMultiplier > 1.0f)
        {
            Request.ElementAttributeType = EFloatingTextDamageType::EffectiveElementDamage;
        }
        else if (ElementDamageMultiplier < 1.0f)
        {
            Request.ElementAttributeType = EFloatingTextDamageType::InefficientElementDamage;
        }
        else
        {
            Request.ElementAttributeType = EFloatingTextDamageType::Normal;
        }
        
        DamagePool->RequestDamageNumber(Request);
    }

    // === 사운드 및 VFX는 기존과 동일 ===
    
    // Spawn Hit SFX
    if (dt_AreaObject->HitSoundID != 0)
    {
        PlayPositionalSound(dt_AreaObject->HitSoundID, HitLocation);
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

void AAreaObject::OnRep_IsDead()
{
	// 사망 상태 변경 시 필요한 클라이언트 처리
	if (bIsDead)
	{
	}
}

void AAreaObject::Multicast_PlayNiagaraEffectAttached_Implementation(AActor* AttachTarget,
	UNiagaraSystem* NiagaraEffect, FRotator Rotator, float Duration)
{
	if (!AttachTarget || !NiagaraEffect)
	{	
		return;
	}

	UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
		NiagaraEffect,
		AttachTarget->GetRootComponent(),
		NAME_None,
		FVector::ZeroVector,
		Rotator,
		EAttachLocation::KeepRelativeOffset,
		true,
		true, // Auto Destroy
		ENCPoolMethod::AutoRelease,
		true // Auto Activate
	);

	if (NiagaraComp && Duration > 0.f)
	{
		FTimerHandle TimerHandle;
		// 타이머 람다에서 NiagaraComp를 안전하게 파괴
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, [NiagaraComp]() {
			if (IsValid(NiagaraComp))
			{
				NiagaraComp->DestroyComponent();
			}
		}, Duration, false);
	}
}

void AAreaObject::Multicast_PlayNiagaraEffectAtLocation_Implementation(FVector Location, UNiagaraSystem* NiagaraEffect,
	 FRotator Rotator, float Duration)
{
	if (!NiagaraEffect)
	{	
		return;
	}

	UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(),
		NiagaraEffect,
		Location,
		Rotator
	);

	if (NiagaraComp && Duration > 0.f)
	{
		FTimerHandle TimerHandle;
		// 타이머 람다에서 NiagaraComp를 안전하게 파괴
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, [NiagaraComp]() {
			if (IsValid(NiagaraComp))
			{
				NiagaraComp->DestroyComponent();
			}
		}, Duration, false);
	}
}

void AAreaObject::Multicast_PlaySoundAtLocation_Implementation(FVector Location, USoundBase* SoundEffect)
{
	if (!SoundEffect)
	{	
		return;
	}

	UGameplayStatics::PlaySoundAtLocation(GetWorld(), SoundEffect, Location);
}

void AAreaObject::Multicast_PlaySound_Implementation(USoundBase* SoundEffect)
{
	if (!SoundEffect)
	{	
		return;
	}

	UGameplayStatics::PlaySound2D(GetWorld(), SoundEffect);
}

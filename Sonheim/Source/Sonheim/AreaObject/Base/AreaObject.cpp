// Fill out your copyright notice in the Description page of Project Settings.


#include "AreaObject.h"
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

// Sets default values
AAreaObject::AAreaObject()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

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
	
	m_HealthComponent->InitHealth(hpMax);
	m_StaminaComponent->InitStamina(maxStamina, staminaRecoveryRate, groggyDuration);
	GetCharacterMovement()->MaxWalkSpeed = walkSpeed;
	m_LevelComponent->InitLevel(this);

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

// Called every frame
void AAreaObject::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (m_CurrentSkill != nullptr)
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
	float Damage = FMath::RandRange(AttackData.HealthDamageAmountMin, AttackData.HealthDamageAmountMax);

	// Stab Damage Process - 포켓몬 자속기
	for (auto& elementAttribute : this->dt_AreaObject->DefenceElementalAttributes)
	{
		if (elementAttribute == AttackData.AttackElementalAttribute)
		{
			Damage *= 1.2f;
			break;
		}
	}

	if (Target == nullptr)
	{
		return;
	}

	FCustomDamageEvent DamageEvent;
	DamageEvent.AttackData = AttackData;
	DamageEvent.HitInfo = HitInfo;
	DamageEvent.Damage = Damage;

	FLog::Log("Damage", Damage);
	
	Target->TakeDamage(Damage, DamageEvent, GetController(), this);
}

float AAreaObject::TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator,
                              AActor* DamageCauser)
{
	// IFF 처리
	// ToDo : 변경 예정
	AAreaObject* damageCauser = Cast<AAreaObject>(DamageCauser);
	if (damageCauser)
	{
		// 같은 타입의 AreaObject끼리는 데미지 x
		if (damageCauser->dt_AreaObject->AreaObjectType == this->dt_AreaObject->AreaObjectType)
		{
			return 0;
		}
	}


	// ToDo : Can Attack Logic 추가? -> 설인 만들면 추가해야할듯
	if (IsDie() || HasCondition(EConditionBitsType::Invincible))
		return 0.0f;

	float ActualDamage = Damage;

	FHitResult hitResult;
	FVector hitDir;
	FAttackData attackData;
	bool bIsWeakPointHit = false;

	if (DamageEvent.IsOfType(FCustomDamageEvent::ClassID))
	{
		FCustomDamageEvent* const customDamageEvent = (FCustomDamageEvent*)&DamageEvent;
		attackData = customDamageEvent->AttackData;
		customDamageEvent->GetBestHitInfo(this, DamageCauser, hitResult, hitDir);

		// Check for weak point hit
		bIsWeakPointHit = IsWeakPointHit(hitResult.Location);

		// HitStop 처리
		if (attackData.bEnableHitStop)
		{
			// 월드의 시간 스케일을 조절하거나
			// 캐릭터의 애니메이션만 일시 정지
			ApplyHitStop(attackData.HitStopDuration);
		}

		// 넉백 처리
		HandleKnockBack(DamageCauser->GetActorLocation(), attackData, m_KnockBackForceMultiplier);
	}

	ActualDamage = Super::TakeDamage(ActualDamage, DamageEvent, EventInstigator, DamageCauser);

	// damage multiply by weak point Hit
	ActualDamage = bIsWeakPointHit ? ActualDamage * 1.5f : ActualDamage;

	// damage multiply by Element Attribute Type
	float elementDamageMultiplier = 1.0f;
	for (auto defectElementalAttribute : dt_AreaObject->DefenceElementalAttributes)
	{
		elementDamageMultiplier *= USonheimUtility::CalculateDamageMultiplier(
			defectElementalAttribute, attackData.AttackElementalAttribute);
	}
	ActualDamage *= elementDamageMultiplier; 

	// apply actual hp damage
	float CurrentHP = DecreaseHP(ActualDamage);
	if (FMath::IsNearlyZero(CurrentHP))
	{
		if (true == ExchangeDead())
		{
			OnDie();
		}
	}

	// apply stamina damage
	DecreaseStamina(attackData.StaminaDamageAmount);

	// Spawn floating damage
	FVector SpawnLocation;
	if (hitResult.Location == FVector::ZeroVector)
	{
		// 캐릭터 중앙에서 생성
		SpawnLocation = GetActorLocation();
	}
	else
	{
		// Hit 위치에서 생성
		SpawnLocation = hitResult.Location;
	}

	FTransform SpawnTransform(FRotator::ZeroRotator, SpawnLocation);

	if (AFloatingDamageActor* DamageActor = GetWorld()->SpawnActor<AFloatingDamageActor>(
		AFloatingDamageActor::StaticClass(), SpawnTransform))
	{
		// FloatingDamageType 계산
		// 약점 계산
		EFloatingOutLineDamageType weakPointDamageType = bIsWeakPointHit
			                                 ? EFloatingOutLineDamageType::WeakPointDamage
			                                 : EFloatingOutLineDamageType::Normal;
		EFloatingTextDamageType elementAttributeDamageType = EFloatingTextDamageType::Normal;
		if ( elementDamageMultiplier > 1.0f)
		{
			elementAttributeDamageType= EFloatingTextDamageType::EffectiveElementDamage;
		}
		else if (elementDamageMultiplier < 1.0f)
		{
			elementAttributeDamageType= EFloatingTextDamageType::InefficientElementDamage;
		}
		DamageActor->Initialize(ActualDamage, weakPointDamageType, elementAttributeDamageType);
	}

	// Spawn Hit SFX
	if (dt_AreaObject->HitSoundID != 0)
	{
		PlayPositionalSound(dt_AreaObject->HitSoundID, hitResult.Location);
	}

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
		m_CurrentSkill->CancelCast();
		ClearCurrentSkill();
	}
}

void AAreaObject::OnDie()
{
	StopAll();

	UAnimInstance* animInstance = GetMesh()->GetAnimInstance();
	// 몽타주 정지
	animInstance->StopAllMontages(0.1f);
	if (UAnimMontage* montage = dt_AreaObject->Die_AnimMontage)
	{
		// 죽음 몽타주 재생
		animInstance->Montage_Play(montage);
	}
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	TWeakObjectPtr<AAreaObject> weakThis = this;
	GetWorld()->GetTimerManager().SetTimer(DeathTimerHandle, [weakThis]()
	{
		AAreaObject* strongThis = weakThis.Get(); // 콜리전 전환

		if (strongThis != nullptr)
		{
			// Death Effect
			if (strongThis->DeathEffect != nullptr)
			{
				UGameplayStatics::SpawnEmitterAtLocation(strongThis->GetWorld(), strongThis->DeathEffect,
				                                         strongThis->GetActorLocation());
			}
			//strongThis->Destroy();
		}
	}, DestroyDelayTime, false);
}

void AAreaObject::OnKill()
{
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

	if (!IsValid(*skillPointer))
	{
		LOG_PRINT(TEXT("스킬 댕글링 포인터 문제발생!!!!"));
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
		UpdateCurrentSkill(Skill);
		Skill->OnCastStart(this, Target);
		return true;
	}
	else
	{
		FString fail = UEnum::GetValueAsString(Skill->SkillFailCase);
		LOG_PRINT(TEXT("CastSkill Failed: %s"), *fail);
		//LOG_SCREEN_ERROR(this, "CastSkill Failed");
		return false;
	}
}

void AAreaObject::ClearCurrentSkill()
{
	m_CurrentSkill = nullptr;
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
	return m_ConditionComponent->AddCondition(AddConditionType, Duration);
}

bool AAreaObject::RemoveCondition(EConditionBitsType RemoveConditionType) const
{
	return m_ConditionComponent->RemoveCondition(RemoveConditionType);
}

bool AAreaObject::HasCondition(EConditionBitsType HasConditionType) const
{
	return m_ConditionComponent->HasCondition(HasConditionType);
}

bool AAreaObject::ExchangeDead() const
{
	return m_ConditionComponent->ExchangeDead();
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

float AAreaObject::IncreaseHP(float Delta) const
{
	return m_HealthComponent->IncreaseHP(Delta);
}

float AAreaObject::DecreaseHP(float Delta)
{
	return m_HealthComponent->DecreaseHP(Delta);
}

void AAreaObject::SetHPByRate(float Rate) const
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

float AAreaObject::IncreaseStamina(float Delta) const
{
	return m_StaminaComponent->IncreaseStamina(Delta);
}

float AAreaObject::DecreaseStamina(float Delta, bool bIsDamaged)
{
	return m_StaminaComponent->DecreaseStamina(Delta, bIsDamaged);
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
			knockBackDir = GetActorLocation() + AttackData.KnockBackDirection.GetSafeNormal() * AttackData.
				KnockBackForce * m_KnockBackForceMultiplier * ForceCoefficient;
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

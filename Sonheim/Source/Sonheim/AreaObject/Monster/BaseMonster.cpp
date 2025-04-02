// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseMonster.h"
#include "AI/Base/BaseAiFSM.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
//#include "Perception/AIPerceptionComponent.h"
//#include "Perception/AISenseConfig_Sight.h"
#include "AIController.h"
#include "BaseSkillRoulette.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Sonheim/AreaObject/Attribute/StaminaComponent.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/AreaObject/Skill/Base/BaseSkill.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "Sonheim/GameObject/ResourceObject/BaseResourceObject.h"
#include "Sonheim/UI/Widget/BaseStatusWidget.h"
#include "Sonheim/UI/Widget/Monster/MonsterStatusWidget.h"
#include "Sonheim/Utilities/CommonUtil.h"

// Sets default values
ABaseMonster::ABaseMonster()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetIsReplicated(true);

	SetNetUpdateFrequency(120.f);
	SetMinNetUpdateFrequency(50.f);

	GetCapsuleComponent()->SetNetAddressable();
	GetCapsuleComponent()->SetIsReplicated(true);

	GetCharacterMovement()->bOrientRotationToMovement = true;

	// AI Perception
	/*
	// AI Perception 컴포넌트 생성
	AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));

	// 시야 설정 생성 및 구성
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("Sight Config"));
	SightConfig->SightRadius = SightRadius;
	SightConfig->LoseSightRadius = LoseSightRadius;
	SightConfig->PeripheralVisionAngleDegrees = 90.0f;
	SightConfig->SetMaxAge(0.5f);
	SightConfig->AutoSuccessRangeFromLastSeenLocation = 0.0f;

	// 팀 설정 - 여기서는 모든 팀을 감지
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;

	// AI Perception 컴포넌트에 시야 설정 추가
	AIPerceptionComponent->ConfigureSense(*SightConfig);
	AIPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());

	// 감지 이벤트에 함수 바인딩
	AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &ABaseMonster::OnPerceptionUpdated);
	*/

	// HP UI
	HPWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HPUI"));
	HPWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	HPWidgetComponent->SetupAttachment(RootComponent);
	HPWidgetComponent->SetRelativeLocation(FVector(0.f, 0.f, HeightHPUI));
	HPWidgetComponent->SetIsReplicated(true);

	ConstructorHelpers::FClassFinder<UMonsterStatusWidget> monsterHPWidget(TEXT(
		"/Script/UMGEditor.WidgetBlueprint'/Game/_BluePrint/Widget/WB_MonsterStatusWidget.WB_MonsterStatusWidget_C'"));
	if (monsterHPWidget.Succeeded())
	{
		HPWidgetComponent->SetWidgetClass(monsterHPWidget.Class);
	}

	HeadVFXPoint = CreateDefaultSubobject<USceneComponent>(TEXT("HeadVFXPoint"));

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bReplicates = true;
	bNetUseOwnerRelevancy = true;
}

UBaseSkillRoulette* ABaseMonster::GetSkillRoulette() const
{
	return m_SkillRoulette;
}

float ABaseMonster::DecreaseHP(float Delta)
{
	MultiCastRPC_Show();
	return Super::DecreaseHP(Delta);
}

void ABaseMonster::MultiCastRPC_Show_Implementation()
{
	SetHPWidgetVisibilityByDuration(8.f);
}

float ABaseMonster::DecreaseStamina(float Delta, bool bIsDamaged)
{
	if (bIsDamaged)
	{
		//SetHPWidgetVisibilityByDuration(8.f);
	}
	return Super::DecreaseStamina(Delta, bIsDamaged);
}

void ABaseMonster::SetHPWidgetVisibility(bool IsVisible)
{
	if (dt_AreaObject->EnemyType != EEnemyType::Boss)
	{
		HPWidgetComponent->SetVisibility(IsVisible);
	}
}

void ABaseMonster::SetHPWidgetVisibilityByDuration(float Duration)
{
	SetHPWidgetVisibility(true);
	// 파트너 팰은 항상 UI 보이기
	if (PartnerOwner != nullptr)
		return;

	TWeakObjectPtr<ABaseMonster> weakThis = this;
	GetWorld()->GetTimerManager().SetTimer(HPWidgetVisibleTimer, [weakThis]() {
		ABaseMonster* strongThis = weakThis.Get();
		if (strongThis != nullptr)
		{
			strongThis->SetHPWidgetVisibility(false);
		}
	}, Duration, false);
}

UBaseSkillRoulette* ABaseMonster::CreateSkillRoulette()
{
	return CreateDefaultSubobject<UBaseSkillRoulette>(TEXT("SkillRouletteComponent"));
}

// Called when the game starts or when spawned
void ABaseMonster::BeginPlay()
{
	Super::BeginPlay();

	HPWidgetComponent->SetVisibility(false);

	WalkSpeed = dt_AreaObject->WalkSpeed;
	ForcedWalkSpeed = WalkSpeed * 5.f;

	AIController = Cast<AAIController>(GetController());

	// HP UI 위치, Visible Setting
	if (dt_AreaObject->EnemyType != EEnemyType::Boss)
	{
		HeightHPUI = GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 10;
		HPWidgetComponent->SetRelativeLocation(FVector(0.f, 0.f, HeightHPUI));
		InitializeHUD();
		//HPWidgetComponent->SetVisibility(false);
	}

	USonheimGameInstance* gameInstance = Cast<USonheimGameInstance>(GetGameInstance());
	int skillbagID = dt_AreaObject->SkillBagID;
	if (skillbagID != 0)
	{
		dt_SkillBag = gameInstance->GetDataSkillBag(skillbagID);
		if (m_SkillRoulette != nullptr)
		{
			m_SkillRoulette->InitFromSkillBag(dt_SkillBag);
		}
		else
		{
			LOG_PRINT(TEXT("스킬룰렛 없음"));
			LOG_SCREEN("SkillRoulette is nullptr. please set in construct");
		}
	}
	else
	{
		LOG_PRINT(TEXT("SkillBag is 0"));
	}

	m_SpawnLocation = GetActorLocation();

	if (m_AiFSM != nullptr)
	{
		m_AiFSM->InitStatePool();
		m_AiFSM->CheckIsValidAiStates();
	}
	else
	{
		LOG_PRINT(TEXT("FSM is nullptr"));
		LOG_SCREEN("FSM is nullptr. please set in construct");
	}

	if (PickaxeMesh)
	{
		PickaxeMesh->SetVisibility(false);
	}
}

// Called every frame
void ABaseMonster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void ABaseMonster::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ABaseMonster::OnBodyBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                      UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                                      const FHitResult& SweepResult)
{}

UBaseAiFSM* ABaseMonster::CreateFSM()
{
	return nullptr;
}

AAreaObject* ABaseMonster::GetAggroTarget() const
{
	return m_AggroTarget;
}

float ABaseMonster::GetDistToTarget()
{
	if (m_AggroTarget == nullptr)
	{
		LOG_PRINT(TEXT("AggroTarget is null."));
		return 0;
	}
	return GetDistanceTo(m_AggroTarget);
}

FVector ABaseMonster::GetDirToTarget()
{
	if (m_AggroTarget == nullptr)
	{
		LOG_PRINT(TEXT("AggroTarget is null."));
		return FVector::ZeroVector;
	}
	return m_AggroTarget->GetActorLocation() - GetActorLocation();
}

float ABaseMonster::GetNextSkillRange()
{
	return NextSkill == nullptr ? 0.f : NextSkill->GetSkillRange();
}

FVector ABaseMonster::GetSpawnLocation()
{
	return m_SpawnLocation;
}

float ABaseMonster::GetSightLength()
{
	return SightRadius;
}

ABaseResourceObject* ABaseMonster::GetResourceTarget() const
{
	return m_ResourceTarget;
}

void ABaseMonster::OnDie()
{
	Super::OnDie();
	// 죽는 애니메이션 하고
	IsDead = true;
	ChangeFace(EFaceType::Dead);

	// 굴러다니게
	GetCapsuleComponent()->SetSimulatePhysics(true);

	// UI Disable
	StatusWidget->RemoveFromParent();

	// TWeakObjectPtr<AAreaObject> weakThis = this;
	// GetWorld()->GetTimerManager().SetTimer(DeathTimerHandle, [weakThis]()
	// {
	// 	AAreaObject* strongThis = weakThis.Get(); // 콜리전 전환
	//
	// 	if (strongThis != nullptr)
	// 	{
	// 		strongThis->Destroy();
	// 	}
	// }, DestroyDelayTime, false);
	// FSM 정지
	if (m_AiFSM != nullptr)
		m_AiFSM->StopFSM();
	// 움직임 정지
	StopAll();
}


void ABaseMonster::InitializeHUD()
{
	StatusWidget = Cast<UMonsterStatusWidget>(HPWidgetComponent->GetWidget());

	if (HPWidgetComponent)
	{
		// HP 변경 이벤트 바인딩
		if (m_HealthComponent)
		{
			m_HealthComponent->OnHealthChanged.AddDynamic(StatusWidget, &UMonsterStatusWidget::UpdateHealth);
			// 초기값 설정
			StatusWidget->UpdateHealth(GetHP(), 0.0f, m_HealthComponent->GetMaxHP());
		}

		// ToDo 맞게 수정
		StatusWidget->InitMonsterStatusWidget(dt_AreaObject, true, 1);
	}
}

void ABaseMonster::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABaseMonster, m_AiFSM);
	DOREPLIFETIME(ABaseMonster, IsDead);
	DOREPLIFETIME(ABaseMonster, bActivateSkill);
	DOREPLIFETIME(ABaseMonster, IsCalled);
	DOREPLIFETIME(ABaseMonster, PartnerOwner);
	DOREPLIFETIME(ABaseMonster, bIsCanAttack);
	DOREPLIFETIME(ABaseMonster, bIsActive);
	DOREPLIFETIME(ABaseMonster, bIsCanCalled);
	DOREPLIFETIME(ABaseMonster, bIsAttach);
}

float ABaseMonster::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
                               class AController* EventInstigator, AActor* DamageCauser)
{
	float damage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	FLog::Log("TakeDamage", damage);
	if (damage > 0.f)
	{
		TArray<int> array = {1, 5, 10};
		int index = FMath::RandRange(0, 2);
		GetNearResourceObject(array[index]);

		// 어그로 설정
		AAreaObject* Player{Cast<AAreaObject>(DamageCauser)};
		if (Player)
		{
			m_AggroTarget = Player;
		}
	}

	return damage;
}

void ABaseMonster::ChangeFace(EFaceType Type)
{
	MulticastRPC_ChangeFace(Type);
}

void ABaseMonster::MulticastRPC_ChangeFace_Implementation(EFaceType Type)
{
	switch (Type)
	{
	case EFaceType::Default:
		if (EyeMat)
		{
			EyeMat->SetVectorParameterValue(TEXT("EyeVector"), FLinearColor(0, 0, 0, 1));
		}
		if (MouthMat)
		{
			MouthMat->SetVectorParameterValue(TEXT("MouseVector"), FLinearColor(0, 0, 0, 1));
		}
		break;
	case EFaceType::Smile:
		if (EyeMat)
		{
			EyeMat->SetVectorParameterValue(TEXT("EyeVector"), FLinearColor(0.5, 0, 0, 1));
		}
		if (MouthMat)
		{
			MouthMat->SetVectorParameterValue(TEXT("MouseVector"), FLinearColor(0.5, 0, 0, 1));
		}
		break;
	case EFaceType::Boring:
		if (EyeMat)
		{
			EyeMat->SetVectorParameterValue(TEXT("EyeVector"), FLinearColor(0, 0.25, 0, 1));
		}
		if (MouthMat)
		{
			MouthMat->SetVectorParameterValue(TEXT("MouseVector"), FLinearColor(0.5, 0.5, 0, 1));
		}
		break;
	case EFaceType::Angry:
		if (EyeMat)
		{
			EyeMat->SetVectorParameterValue(TEXT("EyeVector"), FLinearColor(0.5, 0.25, 0, 1));
		}
		if (MouthMat)
		{
			MouthMat->SetVectorParameterValue(TEXT("MouseVector"), FLinearColor(0, 0.5, 0, 1));
		}
		break;
	case EFaceType::Sleep:
		if (EyeMat)
		{
			EyeMat->SetVectorParameterValue(TEXT("EyeVector"), FLinearColor(0, 0.5, 0, 1));
		}
		if (MouthMat)
		{
			MouthMat->SetVectorParameterValue(TEXT("MouseVector"), FLinearColor(0.5, 0.5, 0, 1));
		}
		break;
	case EFaceType::Sad:
		if (EyeMat)
		{
			EyeMat->SetVectorParameterValue(TEXT("EyeVector"), FLinearColor(0.5, 0.5, 0, 1));
		}
		if (MouthMat)
		{
			MouthMat->SetVectorParameterValue(TEXT("MouseVector"), FLinearColor(0.5, 0.5, 0, 1));
		}
		break;
	case EFaceType::Exciting:
		if (EyeMat)
		{
			EyeMat->SetVectorParameterValue(TEXT("EyeVector"), FLinearColor(0, 0.75, 0, 1));
		}
		if (MouthMat)
		{
			MouthMat->SetVectorParameterValue(TEXT("MouseVector"), FLinearColor(0.5, 0, 0, 1));
		}
		break;
	case EFaceType::Dead:
		if (EyeMat)
		{
			EyeMat->SetVectorParameterValue(TEXT("EyeVector"), FLinearColor(0.5, 0.75, 0, 1));
		}
		if (MouthMat)
		{
			MouthMat->SetVectorParameterValue(TEXT("MouseVector"), FLinearColor(0, 0.5, 0, 1));
		}
		break;
	}
}

void ABaseMonster::OnRep_PartnerOwner()
{
	if (PartnerOwner != nullptr)
	{
		StatusWidget->SetPartnerPalHPWidget();
	}
}

void ABaseMonster::SetPartnerOwner(ASonheimPlayer* NewOwner)
{
	PartnerOwner = NewOwner;
	NewOwner->RegisterOwnPal(this);
	StatusWidget->SetPartnerPalHPWidget();
	DeactivateMonster();
}

void ABaseMonster::ActivateMonster()
{
	m_AiFSM->ChangeState(EAiStateType::SelectMode);
	RemoveCondition(EConditionBitsType::Hidden);
	// 렌더링 활성화
	SetActorHiddenInGame(false);

	// 물리 및 충돌 활성화
	SetActorEnableCollision(true);

	// UI 활성화
	SetHPWidgetVisibility(true);

	// AI 컨트롤러 활성화
	if (AIController)
	{
		AIController->SetActorTickEnabled(true);
	}
	else
	{
		// ???? 왜??? 확인 필요 
		AIController = Cast<AAIController>(GetController());
	}

	if (PartnerOwner != nullptr)
	{
		FVector newLocation = PartnerOwner->GetActorLocation() + PartnerOwner->GetActorRightVector() * 100.f;
		SetActorLocation(newLocation);
	}
}

void ABaseMonster::DeactivateMonster()
{
	m_AiFSM->StopFSM();

	// 숨김 처리(CanAttack 방지용)
	AddCondition(EConditionBitsType::Hidden);

	// 렌더링 비활성화
	SetActorHiddenInGame(true);

	// 물리 및 충돌 비활성화
	SetActorEnableCollision(false);

	// UI 비활성화
	SetHPWidgetVisibility(false);

	// AI 컨트롤러 비활성화
	if (AIController)
	{
		AIController->SetActorTickEnabled(false);
	}
	else
	{
		AIController = Cast<AAIController>(GetController());
	}
}

bool ABaseMonster::CanAttack(AActor* TargetActor)
{
	bool result = Super::CanAttack(TargetActor);
	if (!result)
		return false;

	// 자원 처리
	ABaseResourceObject* targetResourceObject = Cast<ABaseResourceObject>(TargetActor);
	if (targetResourceObject != nullptr)
	{
		return true;
	}

	// 몬스터 처리
	ABaseMonster* targetMonster = Cast<ABaseMonster>(TargetActor);
	if (targetMonster)
	{
		// 주인이 있으면
		if (PartnerOwner != nullptr)
		{
			// 다른 주인있는 팰 공격 x
			if (targetMonster->PartnerOwner != nullptr)
				return false;
				// 주인없는 팰은 공격 가능
			else
				return true;
		}
		else
		{
			// 주인있는 팰은 공격 가능
			if (targetMonster->PartnerOwner != nullptr)
				return true;
				// 다른 야생팰 공격 x
			else
				return false;
		}
	}

	// 플레이어 처리
	ASonheimPlayer* targetPlayer = Cast<ASonheimPlayer>(TargetActor);
	if (targetPlayer)
	{
		// 내가 주인이 있다면 공격 불가
		if (PartnerOwner != nullptr)
			return false;
		else
			return true;
	}

	return false;
}

// ToDo: 기회가 된다면 안쓰는 코드 장리
void ABaseMonster::OnRep_IsAttached()
{
	if (PartnerOwner && PartnerOwner->GetMesh())
	{
		if (bIsAttach)
		{
			// UE_LOG(LogTemp, Display, TEXT("호출 제한2: %s"),
			//        *FCommonUtil::GetClassEnumKeyAsString(GetLocalRole()))
			GetCapsuleComponent()->SetCollisionProfileName(TEXT("NoCollision"));
			GetMesh()->SetRelativeLocation(FVector::ZeroVector, false, nullptr, ETeleportType::TeleportPhysics);
			BaseTranslationOffset = FVector::ZeroVector;

			SetActorEnableCollision(false);
			GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Flying);
			GetCharacterMovement()->StopMovementImmediately();

			//UE_LOG(LogTemp, Display, TEXT("movement mode: %s"), *GetCharacterMovement()->GetMovementName());
			
			AttachToComponent(PartnerOwner->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			                  FName("PartnerWeapon"));
			PartnerOwner->SetUsePartnerSkill(true);
			Temp_Implementation();
		}
		else
		{
			DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
			GetCapsuleComponent()->SetCollisionProfileName(TEXT("Monster"));
			SetActorEnableCollision(true);
			GetMesh()->SetRelativeLocation(FVector(0, 0, -60));
			BaseTranslationOffset = FVector(0, 0, -60);
			PartnerOwner->SetUsePartnerSkill(false);
			
			GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
			GetCharacterMovement()->SetActive(true);
			//UE_LOG(LogTemp, Display, TEXT("movement mode: %s"), *GetCharacterMovement()->GetMovementName());
		}
	}
}

void ABaseMonster::Temp_Implementation()
{
	 UE_LOG(LogTemp, Display, TEXT("호출: %s"),
	        *FCommonUtil::GetClassEnumKeyAsString(GetLocalRole()))
	GetMesh()->SetRelativeLocation(FVector(0), false, nullptr, ETeleportType::TeleportPhysics);
	BaseTranslationOffset = FVector::ZeroVector;
}


void ABaseMonster::Surprise()
{
	if (bIsForced)
		return;
	bIsSurprise = true;
	// Ouch Face
	//ChangeFace(2);
}

void ABaseMonster::CalmDown()
{
	if (bIsForced)
		return;
	bIsSurprise = false;
	// Smile Face
	//ChangeFace(0);
}

void ABaseMonster::StartTransport()
{
	bIsTransporting = true;
}

void ABaseMonster::EndTransport()
{
	bIsTransporting = false;
}

void ABaseMonster::AIVoiceCommand(int ResourceID, bool IsForced)
{
	ABaseResourceObject* Target = nullptr;
	// Tree
	if (ResourceID == 1 || ResourceID == 5 || ResourceID == 10)
	{
		Target = GetNearResourceObject(ResourceID);
	}
	else
	{
		FLog::Log("Wrong ResourceID");
		return;
	}
	SetIsForced(IsForced);
	m_AiFSM->StopFSM();
	m_AiFSM->ChangeState(EAiStateType::SelectAction);
}

void ABaseMonster::SetIsForced(bool IsForced)
{
	if (IsForced == true)
	{
		//ChangeFace(3);
		GetCharacterMovement()->MaxWalkSpeed = ForcedWalkSpeed;
		this->bIsForced = IsForced;
		VFXSpwan(1);
		VFXSpwan(2);
		VFXSpwan(3);
	}
	else
	{
		//ChangeFace(0);
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
		this->bIsForced = IsForced;
	}
}

void ABaseMonster::VFXSpwan(int VFXID)
{
	FVector VFXLocation = GetActorLocation() + FVector::UpVector * GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	if (VFXID == 0)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), VFX_Exe, VFXLocation);
	}
	else if (VFXID == 1)
	{
		//UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), VFX_Question, VFXLocation);
	}
	else if (VFXID == 2)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), VFX_Sweet, VFXLocation);
	}
}

class ABaseResourceObject* ABaseMonster::GetNearResourceObject(int ResourceID)
{
	TArray<AActor*> TargetArr;
	ABaseResourceObject* Target = nullptr;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseResourceObject::StaticClass(), TargetArr);

	// 나중에 동적으로 받으면 지우기
	// GotResource = 5;

	FVector OwnerLocation{GetActorLocation()};
	float MinDistance = FLT_MAX;

	for (auto FindTarget : TargetArr)
	{
		auto BaseResourceTarget = Cast<ABaseResourceObject>(FindTarget);

		if (BaseResourceTarget->m_ResourceObjectID == ResourceID)
		{
			float Distance = FVector::Dist(OwnerLocation, BaseResourceTarget->GetActorLocation());

			if (Distance < MinDistance)
			{
				MinDistance = Distance;
				Target = BaseResourceTarget;
				SetResourceTarget(Target);
				GotResource = ResourceID;
			}
		}
	}
	return Target;
}

void ABaseMonster::RemoveSkillEntryByID(const int id)
{
	m_SkillRoulette->RemoveSkillEntryByID(id);
}

void ABaseMonster::AddSkillEntryByID(const int id)
{
	m_SkillRoulette->AddSkillEntryByID(id);
}


//void ABaseMonster::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
//{
//	// 감지된 액터가 유효한지 확인
//	if (Actor && Stimulus.WasSuccessfullySensed())
//	{
//		// 플레이어인지 확인 (플레이어 클래스로 캐스팅)
//		APlayer_Kazan* PlayerCharacter = Cast<APlayer_Kazan>(Actor);
//		if (PlayerCharacter)
//		{
//			// 플레이어가 감지되었으므로 현재 타겟으로 설정
//			m_AggroTarget = PlayerCharacter;
//		}
//	}
//	// 감지는 Perception으로, Aggro 해제는 fsm에서
//	/*
//	else if (Actor && !Stimulus.WasSuccessfullySensed())
//	{
//		// 액터를 더 이상 감지하지 못함
//		if (Actor == m_AggroTarget)
//		{
//			// 현재 타겟을 잃었으므로 리셋
//			m_AggroTarget = nullptr;
//		}
//	}
//	*/
//}

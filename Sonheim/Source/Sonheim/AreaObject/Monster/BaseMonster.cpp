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
#include "Net/UnrealNetwork.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/AreaObject/Skill/Base/BaseSkill.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "Sonheim/GameObject/Items/BaseItem.h"
#include "Sonheim/GameObject/ResourceObject/BaseResourceObject.h"
#include "Sonheim/UI/Widget/BaseStatusWidget.h"
#include "Sonheim/UI/Widget/Monster/MonsterStatusWidget.h"
#include "Sonheim/Utilities/CommonUtil.h"
#include "Sonheim/Utilities/SonheimUtility.h"

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

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bReplicates = true;
	bNetUseOwnerRelevancy = true;
}

UBaseSkillRoulette* ABaseMonster::GetSkillRoulette() const
{
	return m_SkillRoulette;
}

bool ABaseMonster::CanCapture() const
{
	// 죽지않고, 주인없고, 숨김 처리 아닐때만 포획가능
	if (!IsDie() && !PartnerOwner && !HasCondition(EConditionBitsType::Hidden))
		return true;
	return false;
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

void ABaseMonster::OnDie_Implementation()
{
	Super::OnDie_Implementation();
	// 죽는 애니메이션 하고
	IsDead = true;
	ChangeFace(EFaceType::Dead);

	if (HasAuthority())
	{
		if (dt_AreaObject && dt_AreaObject->PossibleDropItemID.Num() > 0)
		{
			FItemSpawnOptions Opt;
			Opt.bRequireInteraction = false;
			Opt.ItemCount = 1;
			Opt.AutoPickupDelay = 1.5f;
			Opt.bApplyPhysicsOnDrop = true;
			Opt.DropForce = 1500.f;
			Opt.LifeTime = 300.f;

			USonheimUtility::SpawnFromDropTableWithOptions(
				this,
				dt_AreaObject->PossibleDropItemID,
				GetActorLocation(),
				1,
				150.f,
				Opt
			);
		}
	}

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
        // 파트너가 도착했지만 bIsAttach가 이미 true로 복제된 경우, 실제 부착을 재시도
        if (bIsAttach)
        {
            OnRep_IsAttached();
        }
    }
}

void ABaseMonster::SetPartnerOwner(ASonheimPlayer* NewOwner)
{
	PartnerOwner = NewOwner;
 	IncreaseHP(10000);
	StatusWidget->SetPartnerPalHPWidget();
	//DeactivateMonster();
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

	// 활성화된 스킬 종료
	ClearCurrentSkill();

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
			
            // 안전한 소켓/부착 처리: 소켓 존재 확인 후 스냅(스케일 포함) + 상대변환 초기화
            static const FName PartnerSocket("PartnerWeapon");
            USkeletalMeshComponent* OwnerMesh = PartnerOwner->GetMesh();
            const bool bHasSocket = OwnerMesh && OwnerMesh->DoesSocketExist(PartnerSocket);
            const FName SocketToUse = bHasSocket ? PartnerSocket : NAME_None;

            const FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, true);
            if (SocketToUse != NAME_None)
            {
                AttachToComponent(OwnerMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, SocketToUse);
            }
            else
            {
                // 소켓이 없으면 루트에 부착 후 상대변환 0으로 초기화
                AttachToComponent(OwnerMesh, AttachRules);
            }
            GetRootComponent()->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator, false, nullptr, ETeleportType::TeleportPhysics);
            SetActorRelativeScale3D(FVector(1.f));
            // 본/소켓 업데이트가 한 프레임 늦는 케이스를 보정하기 위해 재스냅 예약
            FTimerHandle Tmp;
            GetWorld()->GetTimerManager().SetTimer(Tmp, this, &ABaseMonster::ResnapToPartnerSocket, 0.01f, false);
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
	bIsSurprise = true;
	// Ouch Face
	//ChangeFace(2);
}

void ABaseMonster::CalmDown()
{
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

void ABaseMonster::RemoveSkillEntryByID(const int id)
{
	m_SkillRoulette->RemoveSkillEntryByID(id);
}

void ABaseMonster::AddSkillEntryByID(const int id)
{
	m_SkillRoulette->AddSkillEntryByID(id);
}

void ABaseMonster::ResnapToPartnerSocket()
{
    if (!PartnerOwner || !PartnerOwner->GetMesh()) return;
    if (!bIsAttach) return;

    USkeletalMeshComponent* OwnerMesh = PartnerOwner->GetMesh();
    static const FName PartnerSocket("PartnerWeapon");
    const bool bHasSocket = OwnerMesh->DoesSocketExist(PartnerSocket);
    const FAttachmentTransformRules AttachRulesSnap(FAttachmentTransformRules::SnapToTargetIncludingScale);

    if (bHasSocket)
    {
        AttachToComponent(OwnerMesh, AttachRulesSnap, PartnerSocket);
    }
    else
    {
        AttachToComponent(OwnerMesh, AttachRulesSnap);
    }
    GetRootComponent()->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator, false, nullptr, ETeleportType::TeleportPhysics);
    SetActorRelativeScale3D(FVector(1.f));
}

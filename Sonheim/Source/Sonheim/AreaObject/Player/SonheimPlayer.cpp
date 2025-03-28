// Fill out your copyright notice in the Description page of Project Settings.


#include "SonheimPlayer.h"
#include "SonheimPlayerController.h"
#include "SonheimPlayerState.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Sonheim/Animation/Player/PlayerAniminstance.h"
#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/AreaObject/Skill/Base/BaseSkill.h"
#include "Sonheim/AreaObject/Utility/GhostTrail.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "Sonheim/GameObject/ResourceObject/BaseResourceObject.h"
#include "Sonheim/Utilities/LogMacro.h"
#include "Sonheim/UI/Widget/Player/PlayerStatusWidget.h"
#include "Utility/InventoryComponent.h"


class UEnhancedInputLocalPlayerSubsystem;
// Sets default values
ASonheimPlayer::ASonheimPlayer()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Set AreaObject ID
	m_AreaObjectID = 1;

	// Die Setting
	DestroyDelayTime = 3.0f;

	// Set Size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(30.f, 96.f);

	// Set Mesh
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> tempSkeletalMesh(
		TEXT("/Script/Engine.SkeletalMesh'/Game/_Resource/Player/Merchent/Merchant.Merchant'"));
	if (tempSkeletalMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(tempSkeletalMesh.Object);
		GetMesh()->SetRelativeLocationAndRotation(FVector(0, 0, -95.0F), FRotator(0, -90, 0));
		GetMesh()->SetRelativeScale3D(FVector(0.004f));
	}

	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	WeaponComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponComponent->SetupAttachment(GetMesh(),TEXT("Weapon_R"));
	WeaponComponent->ComponentTags.Add("WeaponMesh");

	PalSphereComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PalSphereMesh"));
	PalSphereComponent->SetupAttachment(GetMesh(),TEXT("Weapon_R"));
	PalSphereComponent->SetVisibility(false);


	// Set Animation Blueprint
	ConstructorHelpers::FClassFinder<UAnimInstance> TempABP(TEXT(
		"/Script/Engine.AnimBlueprint'/Game/_BluePrint/AreaObject/Player/ABP_Player.ABP_Player_C'"));

	if (TempABP.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(TempABP.Class);
	}

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	// Rotation Setting
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // at this rotation rate

	// Movement Setting
	GetCharacterMovement()->MaxWalkSpeed = MAX_WALK_SPEED;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.f;
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.7f;

	// Create Camera Boom
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetRelativeLocation({0, 0, 40});

	CameraBoom->TargetArmLength = NormalCameraBoomAramLength;
	// The Camera follows at this distance behind the character
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller
	// Camera Lagging
	//CameraBoom->bEnableCameraLag = true;
	//CameraBoom->CameraLagSpeed = 10.0f;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	// Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm
	FollowCamera->FieldOfView = 100;
}

// Called when the game starts or when spawned
void ASonheimPlayer::BeginPlay()
{
	Super::BeginPlay();

	// 무기 없을때 공격 바인드
	CommonAttack = GetSkillByID(10);

	S_PlayerAnimInstance = Cast<UPlayerAnimInstance>(GetMesh()->GetAnimInstance());

	if (S_PlayerState == nullptr)S_PlayerState = Cast<ASonheimPlayerState>(GetPlayerState());
	if (S_PlayerController == nullptr) S_PlayerController = Cast<ASonheimPlayerController>(GetController());
	if (S_PlayerController != nullptr) S_PlayerController->InitializeHUD(this);

	// 장비 변경 이벤트 바인드
	//S_PlayerState->m_InventoryComponent->OnEquipmentChanged.AddDynamic(this, &ASonheimPlayer::UpdateEquipWeapon);
	// 무기 변경 이벤트 바인드
	//S_PlayerState->m_InventoryComponent->OnWeaponChanged.AddDynamic(this, &ASonheimPlayer::UpdateSelectedWeapon);
	WeaponSkillMap.Add(EEquipmentSlotType::Weapon1, CommonAttack);
	WeaponSkillMap.Add(EEquipmentSlotType::Weapon2, CommonAttack);
	WeaponSkillMap.Add(EEquipmentSlotType::Weapon3, CommonAttack);
	WeaponSkillMap.Add(EEquipmentSlotType::Weapon4, CommonAttack);

	InitializeStateRestrictions();

	// 게임 시작 시 첫 위치를 체크포인트로 저장
	SaveCheckpoint(GetActorLocation(), GetActorRotation());

	//S_PlayerState->OnPlayerStatsChanged.AddDynamic(this, &ASonheimPlayer::StatChanged);
}

void ASonheimPlayer::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	S_PlayerController = Cast<ASonheimPlayerController>(NewController);
	S_PlayerState = Cast<ASonheimPlayerState>(GetPlayerState());

	// 장비 변경 이벤트 바인드
	S_PlayerState->m_InventoryComponent->OnEquipmentChanged.AddDynamic(this, &ASonheimPlayer::UpdateEquipWeapon);
	// 무기 변경 이벤트 바인드
	S_PlayerState->m_InventoryComponent->OnWeaponChanged.AddDynamic(this, &ASonheimPlayer::UpdateSelectedWeapon);

	S_PlayerState->OnPlayerStatsChanged.AddDynamic(this, &ASonheimPlayer::StatChanged);
}

void ASonheimPlayer::Server_OnDie_Implementation()
{
	Super::Server_OnDie_Implementation();
}

void ASonheimPlayer::Client_OnDie_Implementation()
{
	Super::Client_OnDie_Implementation();
	// 로컬 플레이어인 경우만 UI 제한
	if (IsLocallyControlled())
	{
		SetPlayerState(EPlayerState::DIE);
		auto widget = S_PlayerController->GetPlayerStatusWidget();
		widget->SetVisibility(ESlateVisibility::Hidden);
	}
}

void ASonheimPlayer::OnRevival()
{
	Super::OnRevival();
	//S_PlayerController->FailWidget->RemoveFromParent();
	S_PlayerController->GetPlayerStatusWidget()->SetVisibility(ESlateVisibility::Visible);
}

float ASonheimPlayer::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
                                 class AController* EventInstigator, AActor* DamageCauser)
{
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (!FMath::IsNearlyZero(ActualDamage))
	{
		bCanRecover = false;
		GetWorld()->GetTimerManager().SetTimer(RecoveryTimerHandle, [this] { bCanRecover = true; }, 2.f, false);
	}

	return ActualDamage;
}

float ASonheimPlayer::HandleAttackDamageCalculation(float Damage)
{
	// ToDo : 수정 예정!! 하드한 공식 - skill로 고도화 예정
	return Damage + m_Attack;
}

float ASonheimPlayer::HandleDefenceDamageCalculation(float Damage)
{
	// ToDo : 수정 예정!! 하드한 공식 - skill로 고도화 예정
	return FMath::Clamp(Damage - m_Defence, 1, Damage - m_Defence);
}

void ASonheimPlayer::OnRep_IsDead()
{
	Super::OnRep_IsDead();
	S_PlayerAnimInstance->bIsDead = true;
	// if (bIsDead && IsLocallyControlled())
	// {
	// 	SetPlayerState(EPlayerState::DIE);
	// 	S_PlayerController->GetPlayerStatusWidget()->SetVisibility(ESlateVisibility::Hidden);
	// }
}

void ASonheimPlayer::Reward(int ItemID, int ItemValue) const
{
	if (S_PlayerState == nullptr) return;
	S_PlayerState->m_InventoryComponent->AddItem(ItemID, ItemValue);
}

void ASonheimPlayer::UpdateSelectedWeapon(EEquipmentSlotType WeaponSlot, int ItemID)
{
	SelectedWeaponSlot = WeaponSlot;

	if (ItemID == 0)
	{
		WeaponComponent->SetSkeletalMesh(nullptr);
		S_PlayerAnimInstance->bIsMelee = false;
		return;
	}
	else
	{
		FItemData* ItemData = m_GameInstance->GetDataItem(ItemID);
		// 무기 메쉬 설정
		WeaponComponent->SetSkeletalMesh(ItemData->EquipmentData.EquipmentMesh);
		S_PlayerAnimInstance->bIsMelee = true;

		// 애니메이션 모드 설정
	}
}

void ASonheimPlayer::StatChanged(EAreaObjectStatType StatType, float StatValue)
{
	if (StatType == EAreaObjectStatType::HP)
	{
		m_HealthComponent->SetMaxHP(StatValue);
	}
	else if (StatType == EAreaObjectStatType::Attack)
	{
		m_Attack = StatValue;
	}
	else if (StatType == EAreaObjectStatType::Defense)
	{
		m_Defence = StatValue;
	}
}

void ASonheimPlayer::RegisterOwnPal(ABaseMonster* Pal)
{
	int palNum = m_OwnedPals.Num();
	if (palNum == PalMaxIndex)
	{
		FLog::Log("Pal Num is Max");
		return;
	}
	m_OwnedPals.Add(palNum, Pal);
	UpdateSelectedPal();
	S_PlayerController->GetPlayerStatusWidget()->AddOwnedPal(Pal->m_AreaObjectID, palNum);
	//SetSelectedPal(0);
}

void ASonheimPlayer::SetSelectedPal(int PalIndex)
{
	m_SelectedPal = m_OwnedPals[PalIndex];
}


void ASonheimPlayer::UpdateEquipWeapon(EEquipmentSlotType WeaponSlot, FInventoryItem Item)
{
	FItemData* ItemData = m_GameInstance->GetDataItem(Item.ItemID);
	if (WeaponSlot == EEquipmentSlotType::Weapon1 || WeaponSlot == EEquipmentSlotType::Weapon2 || WeaponSlot ==
		EEquipmentSlotType::Weapon3 || WeaponSlot == EEquipmentSlotType::Weapon4)
	{
		if (!WeaponSkillMap.IsEmpty())
		{
			WeaponSkillMap[WeaponSlot] = CommonAttack;
		}

		if (ItemData == nullptr)
		{
			if (SelectedWeaponSlot == WeaponSlot)
			{
				WeaponComponent->SetSkeletalMesh(nullptr);
				S_PlayerAnimInstance->bIsMelee = false;
			}
			return;
		}

		FSkillData* SkillData = m_GameInstance->GetDataSkill(ItemData->EquipmentData.SkillID);
		if (SkillData == nullptr)
		{
			WeaponSkillMap.Add(WeaponSlot, CommonAttack);
			return;
		}
		UBaseSkill* weaponSkill = NewObject<UBaseSkill>(this, SkillData->SkillClass);
		weaponSkill->InitSkill(SkillData);

		WeaponSkillMap.Add(WeaponSlot, weaponSkill);

		if (SelectedWeaponSlot == WeaponSlot)
		{
			WeaponComponent->SetSkeletalMesh(ItemData->EquipmentData.EquipmentMesh);
			S_PlayerAnimInstance->bIsMelee = true;
		}
	}
}

// Called every frame
void ASonheimPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// HP 자동 회복
	if (bCanRecover && !IsMaxHP() && !IsDie())
	{
		float recovery = m_RecoveryRate * DeltaTime;

		IncreaseHP(recovery);
	}

	// 
	if (m_SelectedPal != nullptr)
	{
		TArray<AActor*> TargetArr;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABaseMonster::StaticClass(), TargetArr);

		// 플레이어 위치 가져오기
		FVector PlayerLocation = GetActorLocation();

		TArray<ABaseMonster*> IdleMonster;
		for (auto FindTarget : TargetArr)
		{
			auto monster = Cast<ABaseMonster>(FindTarget);
			if (monster != m_SelectedPal)
			{
				IdleMonster.Add(monster);
			}
		}

		// 거리에 따라 정렬
		IdleMonster.Sort([PlayerLocation](const ABaseMonster& A, const ABaseMonster& B)
		{
			float DistanceA = FVector::DistSquared(A.GetActorLocation(), PlayerLocation);
			float DistanceB = FVector::DistSquared(B.GetActorLocation(), PlayerLocation);
			return DistanceA < DistanceB;
		});
		if (IdleMonster.Num() > 0)
		{
			if (FVector::Dist(IdleMonster[0]->GetActorLocation(), PlayerLocation) < 800.f)
			{
				m_SelectedPal->SetAggroTarget(IdleMonster[0]);
			}
		}
	}
}

void ASonheimPlayer::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	S_PlayerState = Cast<ASonheimPlayerState>(GetPlayerState());
}

void ASonheimPlayer::OnRep_Controller()
{
	Super::OnRep_Controller();
	S_PlayerController = Cast<ASonheimPlayerController>(GetController());
}

void ASonheimPlayer::InitializeStateRestrictions()
{
	// 일반 상태 - 모든 행동 가능
	FActionRestrictions NormalRestrictions;
	StateRestrictions.Add(EPlayerState::NORMAL, NormalRestrictions);

	// Only Rotate 상태 - 회전만 가능
	FActionRestrictions OnlyRotateRestrictions;
	OnlyRotateRestrictions.bCanMove = false;
	OnlyRotateRestrictions.bCanOnlyRotate = true;
	OnlyRotateRestrictions.bCanAction = false;
	StateRestrictions.Add(EPlayerState::ONLY_ROTATE, OnlyRotateRestrictions);

	// Action 상태 - Action 제한
	FActionRestrictions ActionRestrictions;
	//ActionRestrictions.bCanMove = false;
	ActionRestrictions.bCanAction = false;
	StateRestrictions.Add(EPlayerState::ACTION, ActionRestrictions);

	// Can Action 상태 - 이동, 제한
	FActionRestrictions CanActionRestrictions;
	CanActionRestrictions.bCanMove = false;
	StateRestrictions.Add(EPlayerState::CANACTION, CanActionRestrictions);

	// Die - 삭제할수도?
	FActionRestrictions DieRestrictions;
	DieRestrictions.bCanMove = false;
	DieRestrictions.bCanAction = false;
	StateRestrictions.Add(EPlayerState::DIE, DieRestrictions);

	SetPlayerState(EPlayerState::NORMAL);
}

bool ASonheimPlayer::CanPerformAction(EPlayerState State, FString ActionName)
{
	if (!StateRestrictions.Contains(State))
		return false;

	const FActionRestrictions& Restrictions = StateRestrictions[State];

	if (ActionName == "Move")
		return Restrictions.bCanMove;
	else if (ActionName == "Rotate")
		return Restrictions.bCanRotate;
	else if (ActionName == "OnlyRotate")
		return Restrictions.bCanOnlyRotate;
	else if (ActionName == "Look")
		return Restrictions.bCanLook;
	else if (ActionName == "Action")
		return Restrictions.bCanAction;

	return false;
}

// void ASonheimPlayer::Server_ToggleLockOn_Implementation(bool IsActive)
// {
// 	S_PlayerAnimInstance->bIsLockOn = IsActive; 
// }

void ASonheimPlayer::SetComboState(bool bCanCombo, int SkillID)
{
	CanCombo = bCanCombo;
	NextComboSkillID = SkillID;
}

void ASonheimPlayer::SetPlayerState(EPlayerState NewState)
{
	CurrentPlayerState = NewState;

	// 상태 변경에 따른 추가 처리
	const FActionRestrictions& NewRestrictions = StateRestrictions[NewState];

	// 이동 제한 적용 - Root Motion도 제한하므로 생각해야할듯..
	//if (!NewRestrictions.bCanMove)
	//	GetCharacterMovement()->DisableMovement();
	//else
	//	GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	// 회전 제한 적용
	GetCharacterMovement()->bOrientRotationToMovement = NewRestrictions.bCanRotate;
}

void ASonheimPlayer::Move(const FVector2D MovementVector)
{
	// input is a Vector2D
	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add Movement
		if (CanPerformAction(CurrentPlayerState, "Move"))
		{
			//S_PlayerAnimInstance->Montage_Stop(0.2f);
			AddMovementInput(ForwardDirection, MovementVector.Y);
			AddMovementInput(RightDirection, MovementVector.X);
		}
		// Rotate Only
		if (CanPerformAction(CurrentPlayerState, "OnlyRotate"))
		{
			FVector targetLocation = GetActorLocation() + RightDirection * MovementVector.X + ForwardDirection *
				MovementVector.Y;

			LookAtLocation(targetLocation, EPMRotationMode::Speed, 1000.f);
		}
	}
}

void ASonheimPlayer::Look(const FVector2D LookAxisVector)
{
	// input is a Vector2D
	if (Controller != nullptr && CanPerformAction(CurrentPlayerState, "Look"))
	{
		// add yaw and pitch input to controller
		// 상하 회전 제한 적용

		//float oldPitchAngle = GetControlRotation().Pitch;
		//float newPitchAngle = oldPitchAngle + (LookAxisVector.Y * LookSensitivityY);
		//newPitchAngle = FMath::ClampAngle(newPitchAngle, MinPitchAngle, MaxPitchAngle);
		//float pitchInput = newPitchAngle - oldPitchAngle;
		float newPitchAngle = CurrentPitchAngle + (LookAxisVector.Y * LookSensitivityY);

		//newPitchAngle = FMath::ClampAngle(newPitchAngle, MinPitchAngle, MaxPitchAngle);
		float pitchInput = newPitchAngle - CurrentPitchAngle;

		// 좌우 회전
		AddControllerYawInput(LookAxisVector.X * LookSensitivityX);
		// 상하 회전
		AddControllerPitchInput(pitchInput);

		CurrentPitchAngle = newPitchAngle;
	}
}

void ASonheimPlayer::LeftMouse_Pressed()
{
	if (bUsingPartnerSkill)
	{
		m_SelectedPal->PartnerSkillTrigger(true);
		return;
	}
}

void ASonheimPlayer::LeftMouse_Triggered()
{
	if (!CanPerformAction(CurrentPlayerState, "Action")) return;

	if (bUsingPartnerSkill)
	{
		//SelectedPal->PartnerSkillTrigger(true);
		return;
	}


	if (CanCombo && NextComboSkillID)
	{
		TObjectPtr<UBaseSkill> comboSkill = GetSkillByID(NextComboSkillID);
		if (CastSkill(comboSkill, this))
		{
			SetPlayerState(EPlayerState::ACTION);
			comboSkill->OnSkillComplete.BindUObject(this, &ASonheimPlayer::SetPlayerNormalState);
		}
	}

	auto skill = GetWeaponAttack();

	if (CastSkill(skill, this))
	{
		SetPlayerState(EPlayerState::ACTION);
		skill->OnSkillComplete.BindUObject(this, &ASonheimPlayer::SetPlayerNormalState);
	}
}

void ASonheimPlayer::LeftMouse_Released()
{
	if (bUsingPartnerSkill)
	{
		m_SelectedPal->PartnerSkillTrigger(false);
		return;
	}
}

void ASonheimPlayer::RightMouse_Pressed()
{
	if (IsDie())
	{
		return;
	}

	TWeakObjectPtr<ASonheimPlayer> weakThis = this;
	GetWorld()->GetTimerManager().SetTimer(LockOnCameraTimerHandle, [weakThis]
	{
		ASonheimPlayer* strongThis = weakThis.Get();
		if (strongThis != nullptr)
		{
			if (strongThis->CameraBoom->TargetArmLength == strongThis->RClickCameraBoomAramLength)
			{
				strongThis->GetWorld()->GetTimerManager().ClearTimer(strongThis->LockOnCameraTimerHandle);
			}
			float alpha = 0.f;
			alpha = FMath::FInterpTo(strongThis->CameraBoom->TargetArmLength, strongThis->RClickCameraBoomAramLength,
			                         0.01f, 5.f);

			strongThis->CameraBoom->TargetArmLength = alpha;
		}
	}, 0.01f, true);
	// 카메라 회전
	LookAtLocation(GetActorLocation() + GetFollowCamera()->GetForwardVector(), EPMRotationMode::Speed, 600);
	// 록온 모드
	bUseControllerRotationYaw = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	// Server_ToggleLockOn(true);
	S_PlayerAnimInstance->bIsLockOn = true;
}

void ASonheimPlayer::RightMouse_Triggered()
{
	//FLog::Log("RightMouse_Triggered");
}

void ASonheimPlayer::RightMouse_Released()
{
	GetWorld()->GetTimerManager().ClearTimer(LockOnCameraTimerHandle);
	//CameraBoom->TargetArmLength = NormalCameraBoomAramLength;
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	// Server_ToggleLockOn(false);
	S_PlayerAnimInstance->bIsLockOn = false;

	TWeakObjectPtr<ASonheimPlayer> weakThis = this;
	GetWorld()->GetTimerManager().SetTimer(LockOnCameraTimerHandle, [weakThis]
	{
		ASonheimPlayer* strongThis = weakThis.Get();
		if (strongThis != nullptr)
		{
			if (strongThis->CameraBoom->TargetArmLength == strongThis->NormalCameraBoomAramLength)
			{
				strongThis->GetWorld()->GetTimerManager().ClearTimer(strongThis->LockOnCameraTimerHandle);
			}
			float alpha = 0.f;
			alpha = FMath::FInterpTo(strongThis->CameraBoom->TargetArmLength, strongThis->NormalCameraBoomAramLength,
			                         0.01f, 8.f);

			strongThis->CameraBoom->TargetArmLength = alpha;
		}
	}, 0.01f, true);
}

void ASonheimPlayer::Reload_Pressed()
{
	FLog::Log("Reloading...");
}

void ASonheimPlayer::Dodge_Pressed()
{
	if (!CanPerformAction(CurrentPlayerState, "Action")) return;
	int dodgeSkillID = 2;

	TObjectPtr<UBaseSkill> skill = GetSkillByID(dodgeSkillID);
	if (CastSkill(skill, this))
	{
		SetPlayerState(EPlayerState::ACTION);
		skill->OnSkillComplete.BindUObject(this, &ASonheimPlayer::SetPlayerNormalState);
	}
}

void ASonheimPlayer::Sprint_Pressed()
{
	float SprintSpeed = dt_AreaObject->WalkSpeed * SprintSpeedRatio;
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
}

void ASonheimPlayer::Sprint_Triggered()
{
	DecreaseStamina(0.5f);
}

void ASonheimPlayer::Sprint_Released()
{
	GetCharacterMovement()->MaxWalkSpeed = dt_AreaObject->WalkSpeed;
}

void ASonheimPlayer::Jump_Pressed()
{
	Jump();
}

void ASonheimPlayer::Jump_Released()
{
	StopJumping();
}

void ASonheimPlayer::WeaponSwitch_Triggered()
{
}

void ASonheimPlayer::PartnerSkill_Pressed()
{
	if (m_SelectedPal == nullptr || m_SummonedPal == nullptr)
	{
		FLog::Log("Partner Pall Is Not Exist");
		return;
	}

	if (!bUsingPartnerSkill)
	{
		m_SummonedPal->PartnerSkillStart();
		//SetUsePartnerSkill(true);
	}
	else
	{
		m_SummonedPal->PartnerSkillEnd();
		//SetUsePartnerSkill(false);
	}
}

void ASonheimPlayer::PartnerSkill_Triggered()
{
}

void ASonheimPlayer::PartnerSkill_Released()
{
	if (m_SelectedPal == nullptr)
	{
		FLog::Log("Partner Pall Is Not Exist");
		return;
	}
	//SelectedPal->PartnerSkillEnd();
}

void ASonheimPlayer::SummonPal_Pressed()
{
	UpdateSelectedPal();
	if (m_SelectedPal == nullptr)
	{
		FLog::Log("Partner Pall Is Not Exist");
		return;
	}
	FLog::Log("SelectedPal Index : ", CurrentPalIndex);

	// TODo: 스킬로 이관예정
	{
		FOnMontageEnded EndDelegate;
		EndDelegate.BindLambda([this](UAnimMontage* Montage, bool bInterrupted)
		{
			PalSphereComponent->SetVisibility(false);
			// 내 우측에 생성 + 근처에 대상 존재하면 상호작용 or 앞으로 전진
			if (m_SelectedPal != m_SummonedPal)
			{
				if (m_SummonedPal != nullptr)
				{
					// ToDo : 고민해보자..
					SetUsePartnerSkill(false);

					m_SummonedPal->DeactivateMonster();
				}
				m_SelectedPal->ActivateMonster();
				m_SummonedPal = m_SelectedPal;
			}
			else
			{
				// ToDo : 고민해보자..
				SetUsePartnerSkill(false);

				m_SummonedPal->DeactivateMonster();
				m_SummonedPal = nullptr;
			}
		});
		PalSphereComponent->SetVisibility(true);
		PlayAnimMontage(SummonPalMontage);
		S_PlayerAnimInstance->Montage_SetEndDelegate(EndDelegate, SummonPalMontage);
	}

	// // 내 우측에 생성 + 근처에 대상 존재하면 상호작용 or 앞으로 전진
	// if (m_SelectedPal != m_SummonedPal)
	// {
	// 	if (m_SummonedPal != nullptr)
	// 	{
	// 		// ToDo : 고민해보자..
	// 		SetUsePartnerSkill(false);
	//
	// 		m_SummonedPal->DeactivateMonster();
	// 	}
	// 	m_SelectedPal->ActivateMonster();
	// 	m_SummonedPal = m_SelectedPal;
	// }
	// else
	// {
	// 	// ToDo : 고민해보자..
	// 	SetUsePartnerSkill(false);
	//
	// 	m_SummonedPal->DeactivateMonster();
	// 	m_SummonedPal = nullptr;
	// }
}

void ASonheimPlayer::SwitchPalSlot_Triggered(int Index)
{
	const int minIndex = 0;
	const int maxIndex = FMath::Min(PalMaxIndex, m_OwnedPals.Num());
	// divide by zero 방지
	if (maxIndex == minIndex) return;

	CurrentPalIndex = (CurrentPalIndex + Index) % maxIndex;
	if (CurrentPalIndex < minIndex) CurrentPalIndex += maxIndex;
	UpdateSelectedPal();

	FLog::Log("CurrentPalIndex : ", CurrentPalIndex);
}


void ASonheimPlayer::Menu_Pressed()
{
}

void ASonheimPlayer::ThrowPalSphere_Pressed()
{
	S_PlayerAnimInstance->bIsThrowPalSphere = true;
	PalSphereComponent->SetVisibility(true);
}

void ASonheimPlayer::ThrowPalSphere_Triggered()
{
}

void ASonheimPlayer::ThrowPalSphere_Released()
{
	PalSphereComponent->SetVisibility(false);
	S_PlayerAnimInstance->bIsThrowPalSphere = false;
	UBaseSkill* Skill = GetSkillByID(15);
	CastSkill(Skill, this);
}

void ASonheimPlayer::Restart_Pressed()
{
	if (!IsDie())
	{
		return;
	}
	// 
	RespawnAtCheckpoint();
}

bool ASonheimPlayer::CanAttack(AActor* TargetActor)
{
	bool result = Super::CanAttack(TargetActor);
	if (!result) return false;

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
		// 다른 주인있는 팰 공격 x
		if (targetMonster->PartnerOwner != nullptr) return false;
		// 주인없는 팰은 공격 가능
		else return true;
	}

	// 플레이어 처리 - IFF 추가로 변경될수도..
	ASonheimPlayer* targetPlayer = Cast<ASonheimPlayer>(TargetActor);
	if (targetPlayer)
	{
		return false;
	}

	return false;
}

void ASonheimPlayer::UpdateSelectedPal()
{
	if (m_OwnedPals.Num() == 0)
	{
		m_SelectedPal = nullptr;
		return;
	}

	// 팰 상자나 버림 등 다른 변수로 최신화 안된경우 대비..
	CurrentPalIndex = FMath::Min(CurrentPalIndex, FMath::Min(m_OwnedPals.Num() - 1, PalMaxIndex));
	m_SelectedPal = m_OwnedPals[CurrentPalIndex];

	// UI Update
	S_PlayerController->GetPlayerStatusWidget()->SwitchSelectedPalIndex(CurrentPalIndex);
}

void ASonheimPlayer::SaveCheckpoint(FVector Location, FRotator Rotation)
{
	LastCheckpointLocation = Location;
	LastCheckpointRotation = Rotation;
}

void ASonheimPlayer::RespawnAtCheckpoint()
{
	OnRevival();
	// 캐릭터 위치 및 회전 설정
	SetActorLocation(LastCheckpointLocation);
	SetActorRotation(LastCheckpointRotation);

	// ToDo : 리스폰 초기화
	SetPlayerState(EPlayerState::NORMAL);
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "SonheimPlayer.h"
#include "SonheimPlayerController.h"
#include "SonheimPlayerState.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Sonheim/Animation/Player/PlayerAniminstance.h"
#include "Sonheim/AreaObject/Skill/Base/BaseSkill.h"
#include "Sonheim/AreaObject/Utility/GhostTrail.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
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
	S_PlayerController = Cast<ASonheimPlayerController>(GetController());
	S_PlayerState = Cast<ASonheimPlayerState>(GetPlayerState());

	S_PlayerController->InitializeHUD();

	// 장비 변경 이벤트 바인드
	S_PlayerState->m_InventoryComponent->OnEquipmentChanged.AddDynamic(this, &ASonheimPlayer::UpdateEquipWeapon);
	// 무기 변경 이벤트 바인드
	S_PlayerState->m_InventoryComponent->OnWeaponChanged.AddDynamic(this, &ASonheimPlayer::UpdateSelectedWeapon);
	WeaponSkillMap.Add(EEquipmentSlotType::Weapon1, CommonAttack);
	WeaponSkillMap.Add(EEquipmentSlotType::Weapon2, CommonAttack);
	WeaponSkillMap.Add(EEquipmentSlotType::Weapon3, CommonAttack);
	WeaponSkillMap.Add(EEquipmentSlotType::Weapon4, CommonAttack);

	InitializeStateRestrictions();

	// 게임 시작 시 첫 위치를 체크포인트로 저장
	SaveCheckpoint(GetActorLocation(), GetActorRotation());
}

void ASonheimPlayer::OnDie()
{
	Super::OnDie();
	SetPlayerState(EPlayerState::DIE);
	S_PlayerController->FailWidget->AddToViewport();
	S_PlayerController->GetPlayerStatusWidget()->SetVisibility(ESlateVisibility::Hidden);
	// ToDo : TimerHandle 정리?

	//GetCharacterMovement()->SetMovementMode(MOVE_None);
}

void ASonheimPlayer::OnRevival()
{
	Super::OnRevival();
	S_PlayerController->FailWidget->RemoveFromParent();
	S_PlayerController->GetPlayerStatusWidget()->SetVisibility(ESlateVisibility::Visible);
}

void ASonheimPlayer::Reward(int ItemID, int ItemValue) const
{
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

void ASonheimPlayer::LeftMouse_Triggered()
{
	if (!CanPerformAction(CurrentPlayerState, "Action")) return;

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
	//int weakAttackID = 10;
	//TObjectPtr<UBaseSkill> skill = GetSkillByID(weakAttackID);
	if (CastSkill(skill, this))
	{
		SetPlayerState(EPlayerState::ACTION);
		skill->OnSkillComplete.BindUObject(this, &ASonheimPlayer::SetPlayerNormalState);
	}
}

void ASonheimPlayer::RightMouse_Pressed()
{
	// 카메라 변환
	//CameraBoom->TargetArmLength = RClickCameraBoomAramLength;
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
	S_PlayerAnimInstance->bIsLockOn = true;
	// 애니메이션
	// set Anim state lockon
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


void ASonheimPlayer::Menu_Pressed()
{
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

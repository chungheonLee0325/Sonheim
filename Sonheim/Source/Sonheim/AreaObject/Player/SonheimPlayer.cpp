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
#include "Net/UnrealNetwork.h"
#include "Sonheim/Animation/Player/PlayerAniminstance.h"
#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/AreaObject/Skill/Base/BaseSkill.h"
#include "Sonheim/AreaObject/Utility/GhostTrail.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "Sonheim/GameObject/Items/BaseItem.h"
#include "Sonheim/GameObject/ResourceObject/BaseResourceObject.h"
#include "Sonheim/Utilities/LogMacro.h"
#include "Sonheim/UI/Widget/Player/PlayerStatusWidget.h"
#include "Utility/InteractionComponent.h"
#include "Utility/InventoryComponent.h"
#include "Utility/PalCaptureComponent.h"
#include "Utility/PalInventoryComponent.h"
#include "Utility/PalPartnerSkillComponent.h"


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
	WeaponComponent->SetupAttachment(GetMesh(),TEXT("viewWeaponR"));
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
	GetCharacterMovement()->JumpZVelocity = 900.f;
	GetCharacterMovement()->AirControl = 0.2f;

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

	// 글라이더 메시 컴포넌트 생성
	GliderMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("GliderMesh"));
	GliderMeshComponent->SetupAttachment(GetMesh(),TEXT("Weapon_R"));
	GliderMeshComponent->SetRelativeLocation(FVector(14, -4.6f, 6));
	GliderMeshComponent->SetRelativeRotation(FRotator(-45, -90, 115));
	GliderMeshComponent->SetRelativeScale3D(FVector(0.006, 0, 0));
	GliderMeshComponent->SetVisibility(false);
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> GliderMeshFinder(
		TEXT("/Script/Engine.SkeletalMesh'/Game/_Resource/Item/Glider/Glider.Glider'"));
	if (GliderMeshFinder.Succeeded())
	{
		GliderMeshComponent->SetSkeletalMesh(GliderMeshFinder.Object);
	}

	PalCaptureComponent = CreateDefaultSubobject<UPalCaptureComponent>(TEXT("PalCapture"));
	PalPartnerSkillComponent = CreateDefaultSubobject<UPalPartnerSkillComponent>(TEXT("PalPartnerSkill"));
	InteractionComponent = CreateDefaultSubobject<UInteractionComponent>(TEXT("InteractionComponent"));
}

// Called when the game starts or when spawned
void ASonheimPlayer::BeginPlay()
{
	Super::BeginPlay();

	// 컴포넌트 초기화
	InitPlayer();
	InitializeStateRestrictions();
	SaveCheckpoint(GetActorLocation(), GetActorRotation());

	// 상호작용 이벤트 바인딩
	if (InteractionComponent && IsLocallyControlled())
	{
		// 가장 가까운 상호작용 가능 타겟 변경 시
		InteractionComponent->OnInteractableChanged.AddDynamic(this, &ASonheimPlayer::OnInteractableChanged);
	}

	if (PalCaptureComponent && IsLocallyControlled())
	{
		PalCaptureComponent->OnThrowingStateChanged.AddDynamic(this, &ASonheimPlayer::HandlePalThrowingStateChanged);
	}
}

void ASonheimPlayer::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	S_PlayerController = Cast<ASonheimPlayerController>(NewController);
	S_PlayerState = Cast<ASonheimPlayerState>(GetPlayerState());

	if (S_PlayerController != nullptr && IsLocallyControlled())
		S_PlayerController->InitializeHUD(this);

	InitializeByPlayerState();
	// 서버에서만 델리게이트 바인딩
	if (HasAuthority())
	{
		BindDelegates();
	}
}

void ASonheimPlayer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindDelegates();
	Super::EndPlay(EndPlayReason);
}

void ASonheimPlayer::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASonheimPlayer, bIsSprinting);
	DOREPLIFETIME(ASonheimPlayer, bIsLockOn);
	DOREPLIFETIME(ASonheimPlayer, bIsGliding);
	DOREPLIFETIME(ASonheimPlayer, SelectedWeaponSlot);
	DOREPLIFETIME(ASonheimPlayer, CurrentWeaponItemID);
	DOREPLIFETIME(ASonheimPlayer, bWeaponVisible);
	DOREPLIFETIME_CONDITION(ASonheimPlayer, CurrentWeaponType, COND_OwnerOnly);
}

void ASonheimPlayer::InitPlayer()
{
	// 무기 없을때 공격 바인드
	CommonAttack = GetSkillByID(10);

	S_PlayerAnimInstance = Cast<UPlayerAnimInstance>(GetMesh()->GetAnimInstance());

	if (S_PlayerState == nullptr)
		S_PlayerState = Cast<ASonheimPlayerState>(GetPlayerState());

	if (S_PlayerController == nullptr)
		S_PlayerController = Cast<ASonheimPlayerController>(GetController());

	// 기본 무기 스킬 맵 초기화
	WeaponSkillMap.Add(EEquipmentSlotType::Weapon1, CommonAttack);
	WeaponSkillMap.Add(EEquipmentSlotType::Weapon2, CommonAttack);
	WeaponSkillMap.Add(EEquipmentSlotType::Weapon3, CommonAttack);
	WeaponSkillMap.Add(EEquipmentSlotType::Weapon4, CommonAttack);
}

void ASonheimPlayer::BindDelegates()
{
	if (S_PlayerState && S_PlayerState->m_InventoryComponent)
	{
		// 기존 바인딩 제거
		S_PlayerState->m_InventoryComponent->OnEquipmentChanged.RemoveDynamic(this, &ASonheimPlayer::UpdateEquipWeapon);
		S_PlayerState->m_InventoryComponent->OnWeaponChanged.RemoveDynamic(this, &ASonheimPlayer::UpdateSelectedWeapon);

		// 새로 바인딩
		S_PlayerState->m_InventoryComponent->OnEquipmentChanged.AddDynamic(this, &ASonheimPlayer::UpdateEquipWeapon);
		S_PlayerState->m_InventoryComponent->OnWeaponChanged.AddDynamic(this, &ASonheimPlayer::UpdateSelectedWeapon);
	}

	if (S_PlayerState)
	{
		S_PlayerState->OnPlayerStatsChanged.RemoveDynamic(this, &ASonheimPlayer::StatChanged);
		S_PlayerState->OnPlayerStatsChanged.AddDynamic(this, &ASonheimPlayer::StatChanged);
	}
}

void ASonheimPlayer::UnbindDelegates()
{
	if (S_PlayerState && S_PlayerState->m_InventoryComponent)
	{
		// 기존 바인딩 제거
		S_PlayerState->m_InventoryComponent->OnEquipmentChanged.RemoveDynamic(this, &ASonheimPlayer::UpdateEquipWeapon);
		S_PlayerState->m_InventoryComponent->OnWeaponChanged.RemoveDynamic(this, &ASonheimPlayer::UpdateSelectedWeapon);
	}

	if (S_PlayerState)
	{
		S_PlayerState->OnPlayerStatsChanged.RemoveDynamic(this, &ASonheimPlayer::StatChanged);
	}
}

void ASonheimPlayer::OnRep_IsDead()
{
	Super::OnRep_IsDead();
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

void ASonheimPlayer::Reward(int ItemID, int ItemValue)
{
	if (S_PlayerState && S_PlayerState->m_InventoryComponent)
	{
		S_PlayerState->m_InventoryComponent->AddItem(ItemID, ItemValue);
	}
}

void ASonheimPlayer::UpdateSelectedWeapon(EEquipmentSlotType WeaponSlot, int ItemID)
{
	if (HasAuthority())
	{
		Server_UpdateSelectedWeapon(WeaponSlot, ItemID);
	}
}

void ASonheimPlayer::Server_UpdateSelectedWeapon_Implementation(EEquipmentSlotType WeaponSlot, int ItemID)
{
	SelectedWeaponSlot = WeaponSlot;
	CurrentWeaponItemID = ItemID;
	// 서버 자신 반영
	OnRep_SelectedWeapon();
}

void ASonheimPlayer::OnRep_SelectedWeapon()
{
	if (CurrentWeaponItemID == 0)
	{
		ClearWeaponMesh();
	}
	else
	{
		UpdateWeaponMesh(CurrentWeaponItemID);
	}
}

void ASonheimPlayer::UpdateWeaponMesh(int ItemID)
{
	if (!m_GameInstance) return;

	FItemData* ItemData = m_GameInstance->GetDataItem(ItemID);
	if (ItemData && ItemData->EquipmentData.EquipmentMesh)
	{
		WeaponComponent->SetSkeletalMesh(ItemData->EquipmentData.EquipmentMesh);
		if (ItemData->EquipmentData.EquipmentAnim)
		{
			WeaponComponent->SetAnimInstanceClass(ItemData->EquipmentData.EquipmentAnim->GetClass());
		}

		if (S_PlayerAnimInstance)
		{
			S_PlayerAnimInstance->bIsMelee = (ItemData->EquipmentData.WeaponType == EWeaponType::Melee);
			S_PlayerAnimInstance->bIsShotgun = (ItemData->EquipmentData.WeaponType == EWeaponType::ShotGun);
		}
		
		CurrentWeaponType = ItemData->EquipmentData.WeaponType;
		OnRep_CurrentWeaponType();
	}
}

void ASonheimPlayer::ClearWeaponMesh()
{
	WeaponComponent->SetSkeletalMesh(nullptr);

	if (S_PlayerAnimInstance)
	{
		S_PlayerAnimInstance->bIsMelee = false;
		S_PlayerAnimInstance->bIsShotgun = false;
	}

	CurrentWeaponType = EWeaponType::None;
	OnRep_CurrentWeaponType();
}

void ASonheimPlayer::Server_SetWeaponVisible_Implementation(bool bNew)
{
	if (bNew != bWeaponVisible)
	{
		bWeaponVisible = bNew;
		OnRep_WeaponVisible();
	}
}

void ASonheimPlayer::OnRep_WeaponVisible()
{
	if (USkeletalMeshComponent* Weapon = GetWeaponMesh())
	{
		Weapon->SetVisibility(bWeaponVisible);
	}
}

void ASonheimPlayer::UpdateEquipWeapon(EEquipmentSlotType WeaponSlot, FInventoryItem Item)
{
	// 무기 슬롯인지 확인
	if (WeaponSlot < EEquipmentSlotType::Weapon1 || WeaponSlot > EEquipmentSlotType::Weapon4)
		return;

	// 아이템 데이터 가져오기
	FItemData* ItemData = nullptr;
	FSkillData* SkillData = nullptr;

	if (Item.ItemID > 0 && m_GameInstance)
	{
		ItemData = m_GameInstance->GetDataItem(Item.ItemID);
		if (ItemData)
		{
			SkillData = m_GameInstance->GetDataSkill(ItemData->EquipmentData.SkillID);
		}
	}

	// 기존 스킬 제거
	if (WeaponSkillMap.Contains(WeaponSlot))
	{
		UBaseSkill* OldSkill = WeaponSkillMap[WeaponSlot];
		if (OldSkill && OldSkill != CommonAttack)
		{
			m_SkillInstanceMap.Remove(OldSkill->GetSkillID());
		}
	}

	// 새 스킬 설정
	if (ItemData && SkillData && SkillData->SkillClass)
	{
		// 무기 스킬 생성
		UBaseSkill* NewWeaponSkill = NewObject<UBaseSkill>(this, SkillData->SkillClass);
		NewWeaponSkill->InitSkill(SkillData);

		// 스킬 등록
		WeaponSkillMap[WeaponSlot] = NewWeaponSkill;
		m_SkillInstanceMap.Add(SkillData->SkillID, NewWeaponSkill);
	}
	else
	{
		// 기본 스킬로 복귀
		WeaponSkillMap[WeaponSlot] = CommonAttack;
		if (CommonAttack)
		{
			m_SkillInstanceMap.Add(CommonAttack->GetSkillID(), CommonAttack);
		}
	}
}

void ASonheimPlayer::OnRep_CurrentWeaponType()
{
	if (IsLocallyControlled() && S_PlayerController)
	{
		if (UPlayerStatusWidget* StatusWidget = S_PlayerController->GetPlayerStatusWidget())
		{
			StatusWidget->SetCrosshairType(CurrentWeaponType);
		}
	}
}


void ASonheimPlayer::StatChanged(EAreaObjectStatType StatType, float StatValue)
{
	// 서버에서만 실제 값 적용
	if (!HasAuthority()) return;

	switch (StatType)
	{
	case EAreaObjectStatType::HP:
		if (m_HealthComponent)
		{
			m_HealthComponent->SetMaxHP(StatValue);
		}
		break;

	case EAreaObjectStatType::Attack:
		m_Attack = StatValue;
		break;

	case EAreaObjectStatType::Defense:
		m_Defence = StatValue;
		break;

	case EAreaObjectStatType::RunSpeed:
		if (GetCharacterMovement())
		{
			GetCharacterMovement()->MaxWalkSpeed = StatValue;
		}
		break;

	case EAreaObjectStatType::JumpHeight:
		if (GetCharacterMovement())
		{
			GetCharacterMovement()->JumpZVelocity = StatValue;
		}
		break;
	}
}

void ASonheimPlayer::WeaponSwitch_Triggered(int Index)
{
	if (S_PlayerState && S_PlayerState->m_InventoryComponent)
	{
		// InventoryComponent에서 처리 (이미 네트워크 동기화됨)
		S_PlayerState->m_InventoryComponent->SwitchWeaponSlot(Index);
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
	Server_RegisterOwnPal(Pal);
}

void ASonheimPlayer::Server_RegisterOwnPal_Implementation(ABaseMonster* Pal)
{
	Multicast_RegisterOwnPal(Pal);
}

void ASonheimPlayer::Multicast_RegisterOwnPal_Implementation(ABaseMonster* Pal)
{
	int palNum = m_OwnedPals.Num();
	// if (palNum == PalMaxIndex)
	// {
	// 	FLog::Log("Pal Num is Max");
	// 	return;
	// }
	m_OwnedPals.Add(palNum, Pal);
	UpdateSelectedPal();
	if (IsLocallyControlled())
	{
		S_PlayerController->GetPlayerStatusWidget()->AddOwnedPal(Pal->m_AreaObjectID, palNum);
	}
}

void ASonheimPlayer::RefreshWeaponSkillToSkillInstanceMap()
{
	for (auto& pair : WeaponSkillMap)
	{
		int skillID = pair.Value->GetSkillID();
		m_SkillInstanceMap.Remove(skillID);
		m_SkillInstanceMap.Add(skillID, pair.Value);
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

	// 글라이딩 상태일 때 업데이트
	if (bIsGliding)
	{
		UpdateGliding(DeltaTime);
	}
}

void ASonheimPlayer::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	S_PlayerState = Cast<ASonheimPlayerState>(GetPlayerState());

	if (S_PlayerController != nullptr && IsLocallyControlled())
		S_PlayerController->InitializeHUD(this);

	InitializeByPlayerState();
}

void ASonheimPlayer::OnRep_Controller()
{
	Super::OnRep_Controller();
	S_PlayerController = Cast<ASonheimPlayerController>(GetController());
	S_PlayerController->InitializeWithPlayer(this);
}

void ASonheimPlayer::InitializeByPlayerState()
{
	PalCaptureComponent->InitializeWithPlayerState(S_PlayerState);
	PalPartnerSkillComponent->InitializeWithPlayerState(S_PlayerState);

	//if(S_PlayerState->m_InventoryComponent && IsLocallyControlled())
	if (S_PlayerState->m_InventoryComponent && HasAuthority())
	{
		//S_PlayerState->m_InventoryComponent->OnItemAdded.AddDynamic(S_PlayerController->GetPlayerStatusWidget(), &UPlayerStatusWidget::OnItemAdded);
		S_PlayerState->m_InventoryComponent->OnItemAdded.AddDynamic(this, &ASonheimPlayer::Client_OnItemAdded);
	}
}

void ASonheimPlayer::Client_OnItemAdded_Implementation(int ItemID, int ItemCount)
{
	S_PlayerController->GetPlayerStatusWidget()->OnItemAdded(ItemID, ItemCount);
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

	// 글라이딩 상태 추가 - 회전과 특정 입력만 허용
	FActionRestrictions GlidingRestrictions;
	GlidingRestrictions.bCanMove = false; // 이동은 글라이더 로직에서 처리
	GlidingRestrictions.bCanAction = false; // 액션 불가
	StateRestrictions.Add(EPlayerState::GLIDING, GlidingRestrictions);

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

USkeletalMeshComponent* ASonheimPlayer::GetPalSphereComponent()
{
	return PalSphereComponent;
}

bool ASonheimPlayer::IsLockOn() const
{
	return S_PlayerAnimInstance->bIsLockOn;
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

// 마우스 왼쪽 클릭 처리
void ASonheimPlayer::LeftMouse_Pressed()
{
	Server_LeftMouse_Pressed();
}

void ASonheimPlayer::Server_LeftMouse_Pressed_Implementation()
{
	if (PalPartnerSkillComponent && PalPartnerSkillComponent->IsUsingPartnerSkill())
	{
		PalPartnerSkillComponent->SetPartnerSkillTrigger(true);
		return;
	}

	MultiCast_LeftMouse_Pressed();
}

void ASonheimPlayer::MultiCast_LeftMouse_Pressed_Implementation()
{
	// 멀티캐스트로 처리할 동작 (예: 애니메이션 설정 등)
}

void ASonheimPlayer::LeftMouse_Triggered()
{
	if (!CanPerformAction(CurrentPlayerState, "Action")) return;

	if (PalPartnerSkillComponent && PalPartnerSkillComponent->IsUsingPartnerSkill()) return;

	Server_LeftMouse_Triggered();
}

void ASonheimPlayer::Server_LeftMouse_Triggered_Implementation()
{
	if (!CanPerformAction(CurrentPlayerState, "Action")) return;
	//if (bUsingPartnerSkill) return;

	if (CanCombo && NextComboSkillID)
	{
		TObjectPtr<UBaseSkill> comboSkill = GetSkillByID(NextComboSkillID);
		if (CastSkill(comboSkill, this))
		{
			SetPlayerState(EPlayerState::ACTION);
			comboSkill->OnSkillComplete.BindUObject(this, &ASonheimPlayer::SetPlayerNormalState);
			MultiCast_LeftMouse_Triggered();
		}
	}
	else
	{
		auto skill = GetSkillByID(GetWeaponAttack()->GetSkillID());
		//auto skill = GetCurrentSkill();
		if (CastSkill(skill, this))
		{
			SetPlayerState(EPlayerState::ACTION);
			skill->OnSkillComplete.BindUObject(this, &ASonheimPlayer::SetPlayerNormalState);
			MultiCast_LeftMouse_Triggered();
		}
	}
}

void ASonheimPlayer::MultiCast_LeftMouse_Triggered_Implementation()
{
}

void ASonheimPlayer::LeftMouse_Released()
{
	Server_LeftMouse_Released();
}

void ASonheimPlayer::Server_LeftMouse_Released_Implementation()
{
	if (PalPartnerSkillComponent && PalPartnerSkillComponent->IsUsingPartnerSkill())
	{
		PalPartnerSkillComponent->SetPartnerSkillTrigger(false);
		return;
	}

	MultiCast_LeftMouse_Released();
}

void ASonheimPlayer::MultiCast_LeftMouse_Released_Implementation()
{
	// 멀티캐스트로 처리할 동작
}

void ASonheimPlayer::AdjustCameraForLockOn(bool IsLockOn)
{
	if (IsLockOn)
	{
		// 카메라 회전
		LookAtLocation(GetActorLocation() + GetFollowCamera()->GetForwardVector(), EPMRotationMode::Speed, 600);

		// 로컬 플레이어만 수행하는 카메라 처리 (줌인)
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
	}
	else
	{
		// 로컬 플레이어만 수행하는 카메라 처리
		GetWorld()->GetTimerManager().ClearTimer(LockOnCameraTimerHandle);

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
}

// 마우스 오른쪽 클릭 처리
void ASonheimPlayer::RightMouse_Pressed()
{
	if (IsDie())
	{
		return;
	}

	Server_SetLockOn(true);

	AdjustCameraForLockOn(true);
}

void ASonheimPlayer::RightMouse_Released()
{
	Server_SetLockOn(false);

	AdjustCameraForLockOn(false);
}

void ASonheimPlayer::OnRep_IsLockOn()
{
	// 회전/애님만 동기화
	bUseControllerRotationYaw = bIsLockOn;
	GetCharacterMovement()->bOrientRotationToMovement = !bIsLockOn;

	if (S_PlayerAnimInstance)
		S_PlayerAnimInstance->bIsLockOn = bIsLockOn;
}

void ASonheimPlayer::Server_SetLockOn_Implementation(bool bNew)
{
	bIsLockOn = bNew;
	// 서버 자신도 반영
	OnRep_IsLockOn();
}

void ASonheimPlayer::Jump_Pressed()
{
	// 글라이딩 중에는 점프를 무시하고 글라이더 해제
	if (bIsGliding)
	{
		DeactivateGlider();
		return;
	}

	Jump();
}

void ASonheimPlayer::Jump_Released()
{
	StopJumping();
}

void ASonheimPlayer::Sprint_Pressed()
{
	Server_SetSprinting(true);
	if (IsLocallyControlled()) ApplySprinting(true); // 체감 반응성
}

void ASonheimPlayer::Sprint_Released()
{
	Server_SetSprinting(false);
	if (IsLocallyControlled()) ApplySprinting(false);
}

void ASonheimPlayer::ApplySprinting(bool bNew)
{
	const float Base = dt_AreaObject ? dt_AreaObject->WalkSpeed : GetCharacterMovement()->MaxWalkSpeed;
	const float NewSpeed = bNew ? Base * SprintSpeedRatio : Base;
	GetCharacterMovement()->MaxWalkSpeed = NewSpeed;
}

void ASonheimPlayer::OnRep_IsSprinting()
{
	ApplySprinting(bIsSprinting);
}

void ASonheimPlayer::Server_SetSprinting_Implementation(bool bNew)
{
	bIsSprinting = bNew;
	ApplySprinting(bIsSprinting);
}

void ASonheimPlayer::Dodge_Pressed()
{
	if (!CanPerformAction(CurrentPlayerState, "Action")) return;
	Server_Dodge_Pressed();
}

void ASonheimPlayer::Server_Dodge_Pressed_Implementation()
{
	int dodgeSkillID = 2;
	TObjectPtr<UBaseSkill> skill = GetSkillByID(dodgeSkillID);

	if (skill && CastSkill(skill, this))
	{
		SetPlayerState(EPlayerState::ACTION);
		skill->OnSkillComplete.BindUObject(this, &ASonheimPlayer::SetPlayerNormalState);
		MultiCast_Dodge_Pressed();
	}
}

void ASonheimPlayer::MultiCast_Dodge_Pressed_Implementation()
{
}

void ASonheimPlayer::Reload_Pressed()
{
	Server_Reload_Pressed();
}

void ASonheimPlayer::Server_Reload_Pressed_Implementation()
{
	MultiCast_Reload_Pressed();
}

void ASonheimPlayer::MultiCast_Reload_Pressed_Implementation()
{
	FLog::Log("Reloading...");
}

void ASonheimPlayer::PartnerSkill_Pressed()
{
	if (PalPartnerSkillComponent)
	{
		PalPartnerSkillComponent->TogglePartnerSkill();
	}
}

// 파트너 스킬 트리거 처리
void ASonheimPlayer::PartnerSkill_Triggered()
{
	// 파트너 스킬 사용 중이면 트리거 입력 처리
	// if (PalPartnerSkillComponent && PalPartnerSkillComponent->IsUsingPartnerSkill())
	// {
	// 	PalPartnerSkillComponent->SetPartnerSkillTrigger(true);
	// }
}

void ASonheimPlayer::PartnerSkill_Released()
{
	if (PalPartnerSkillComponent)
	{
		PalPartnerSkillComponent->TogglePartnerSkill();
	}
	// if (PalPartnerSkillComponent && PalPartnerSkillComponent->IsUsingPartnerSkill())
	// {
	// 	PalPartnerSkillComponent->SetPartnerSkillTrigger(false);
	// }
}

void ASonheimPlayer::SummonPal_Pressed()
{
	if (PalPartnerSkillComponent)
	{
		PalPartnerSkillComponent->TogglePalSummon();
	}
}

void ASonheimPlayer::SwitchPalSlot_Triggered(int Index)
{
	if (S_PlayerState)
	{
		if (UPalInventoryComponent* PalInventory = S_PlayerState->FindComponentByClass<UPalInventoryComponent>())
		{
			PalInventory->ServerRPC_SwitchPalSlot(Index);
		}
	}
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

void ASonheimPlayer::SetUsePartnerSkill(bool UsePartnerSkill)
{
	PalPartnerSkillComponent->SetPartnerSkillState(UsePartnerSkill);
	// this->bUsingPartnerSkill = UsePartnerSkill;
	//
	// if (UsePartnerSkill)
	// {
	// 	S_PlayerAnimInstance->bUsingPartnerSkill = true;
	// }
	// else
	// {
	// 	S_PlayerAnimInstance->bUsingPartnerSkill = false;
	// }
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
	if (IsLocallyControlled())
	{
		S_PlayerController->GetPlayerStatusWidget()->SwitchSelectedPalIndex(CurrentPalIndex);
	}
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

void ASonheimPlayer::ThrowPalSphere_Pressed()
{
	if (PalCaptureComponent)
	{
		PalCaptureComponent->StartThrowPalSphere();
	}
}

void ASonheimPlayer::ThrowPalSphere_Triggered()
{
}

void ASonheimPlayer::ThrowPalSphere_Released()
{
	if (PalCaptureComponent)
	{
		PalCaptureComponent->ThrowPalSphere();
	}
}

void ASonheimPlayer::ThrowPalSphere_Canceled()
{
	if (PalCaptureComponent)
	{
		PalCaptureComponent->CancelThrowPalSphere();
	}
}

// 마우스 오른쪽 트리거 처리
void ASonheimPlayer::RightMouse_Triggered()
{
}

// 스프린트 트리거 처리
void ASonheimPlayer::Sprint_Triggered()
{
	Server_Sprint_Triggered();
}

void ASonheimPlayer::Server_Sprint_Triggered_Implementation()
{
	if (HasAuthority() && bIsSprinting)
	{
		// 최소 스태미너 보다 적을 경우 달리기 취소
		if (DecreaseStamina(0.5f) < 5.0f)
		{
			bIsSprinting = false;
			ApplySprinting(false);
		}
	}
}


void ASonheimPlayer::ActivateGlider()
{
	// 이미 글라이딩 중이거나 땅에 있거나 죽었으면 리턴
	if (bIsGliding || GetCharacterMovement()->IsMovingOnGround() || IsDie())
		return;

	Server_SetGliding(true);
}

void ASonheimPlayer::DeactivateGlider()
{
	if (!bIsGliding)
		return;

	Server_SetGliding(false);
}

void ASonheimPlayer::Multicast_PlayWeaponMontage_Implementation(UAnimMontage* Montage)
{
	if (Montage)
	{
		WeaponComponent->GetAnimInstance()->Montage_Play(Montage);
	}
}

void ASonheimPlayer::UpdateGliding(float DeltaTime)
{
	if (!bIsGliding)
		return;

	UCharacterMovementComponent* MovementComp = GetCharacterMovement();

	// // 지면에 닿으면 글라이더 비활성화
	// if (MovementComp->IsMovingOnGround())
	// {
	// 	DeactivateGlider();
	// 	return;
	// }

	// 카메라 방향으로 이동
	FVector ForwardVector = GetFollowCamera()->GetForwardVector();
	ForwardVector.Z = 0; // 수평 방향만 유지
	ForwardVector.Normalize();

	// 속도 조절
	FVector NewVelocity = ForwardVector * GliderForwardSpeed;
	NewVelocity.Z = -GliderFallSpeed; // 천천히 하강

	// 캐릭터 속도 설정
	MovementComp->Velocity = NewVelocity;

	// 캐릭터 회전 - 진행 방향으로
	if (!ForwardVector.IsNearlyZero())
	{
		FRotator NewRotation = ForwardVector.Rotation();
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), NewRotation, DeltaTime, 5.0f));
	}
}

void ASonheimPlayer::Landed(const FHitResult& Hit)
{
	// 착륙 시 글라이더가 활성화되어 있으면 비활성화
	if (bIsGliding)
	{
		DeactivateGlider();
	}
}

void ASonheimPlayer::Server_SetGliding_Implementation(bool bNew)
{
	// 플레이어 상태 검증 - 글라이더 해제는 어느때나 가능함!
	if (bNew && !CanPerformAction(CurrentPlayerState, "Action"))
	{
		return;
	}

	// 서버 검증
	if (bNew)
	{
		if (GetCharacterMovement()->IsMovingOnGround() || IsDie()) return;
	}
	bIsGliding = bNew;

	ApplyGliderState(bIsGliding);
}

void ASonheimPlayer::ApplyGliderState(bool bNew)
{
	if (bNew)
	{
		if (GliderMeshComponent)
		{
			GliderMeshComponent->SetVisibility(true);
		}

		SetPlayerState(EPlayerState::GLIDING);


		// 무기 숨김
		bWeaponVisible = false;
		OnRep_WeaponVisible();

		if (GliderOpenMontage)
		{
			PlayAnimMontage(GliderOpenMontage);
		}

		UCharacterMovementComponent* Move = GetCharacterMovement();
		Move->SetMovementMode(MOVE_Falling);
		Move->GravityScale = 0.1f;
		Move->FallingLateralFriction = 2.0f;
		Move->AirControl = 1.0f;
		Move->Velocity = FVector(Move->Velocity.X, Move->Velocity.Y, -GliderFallSpeed);

		if (S_PlayerAnimInstance)
		{
			S_PlayerAnimInstance->bIsGliding = true;
		}
	}
	else
	{
		if (GliderCloseMontage)
		{
			PlayAnimMontage(GliderCloseMontage);
		}

		if (GliderMeshComponent)
		{
			GliderMeshComponent->SetVisibility(false);
		}

		SetPlayerState(EPlayerState::NORMAL);

		// 무기 보이기
		bWeaponVisible = true;
		OnRep_WeaponVisible();

		UCharacterMovementComponent* Move = GetCharacterMovement();
		Move->GravityScale = 1.0f;
		Move->FallingLateralFriction = 0.0f;
		Move->AirControl = 0.2f;

		if (S_PlayerAnimInstance)
		{
			S_PlayerAnimInstance->bIsGliding = false;
		}
	}
}

void ASonheimPlayer::OnRep_IsGliding()
{
	ApplyGliderState(bIsGliding);
}

ABaseMonster* ASonheimPlayer::GetSummonedPal() const
{
	if (PalPartnerSkillComponent)
	{
		return PalPartnerSkillComponent->GetSummonedPal();
	}
	return nullptr;
}

void ASonheimPlayer::Client_UpdatePalUI_Implementation(int32 PalID, int32 Index, bool bIsAdd)
{
	if (ASonheimPlayerController* PC = Cast<ASonheimPlayerController>(GetController()))
	{
		if (UPlayerStatusWidget* StatusWidget = PC->GetPlayerStatusWidget())
		{
			if (bIsAdd)
			{
				StatusWidget->AddOwnedPal(PalID, Index);
			}
			else
			{
				//StatusWidget->RemoveOwnedPal(Index);
			}
		}
	}
}

void ASonheimPlayer::Client_UpdateSelectedPalIndex_Implementation(int32 NewIndex)
{
	if (ASonheimPlayerController* PC = Cast<ASonheimPlayerController>(GetController()))
	{
		if (UPlayerStatusWidget* StatusWidget = PC->GetPlayerStatusWidget())
		{
			StatusWidget->SwitchSelectedPalIndex(NewIndex);
		}
	}
}

void ASonheimPlayer::OnInteractableChanged(AActor* NewInteractable)
{
}

void ASonheimPlayer::Interaction_Pressed() const
{
	InteractionComponent->StartHoldInteraction();
}

void ASonheimPlayer::Interaction_Released() const
{
	InteractionComponent->StopHoldInteraction();
}

void ASonheimPlayer::SetWeaponVisible(bool bNew, bool bLocalPreview)
{
	// 1) 체감용 로컬 미리보기 (소유 클라만)
	if (bLocalPreview && IsLocallyControlled())
	{
		if (USkeletalMeshComponent* weaponMesh = GetWeaponMesh())
		{
			weaponMesh->SetVisibility(bNew);
		}
	}

	if (HasAuthority())
	{
		if (bWeaponVisible != bNew)
		{
			bWeaponVisible = bNew;
			OnRep_WeaponVisible();
		}
	}
	else
	{
		Server_SetWeaponVisible(bNew);
	}
}

void ASonheimPlayer::HandlePalThrowingStateChanged(bool bIsPreparing)
{
	if (InteractionComponent)
	{
		InteractionComponent->RequestDetectionUpdate();
	}

	// 로컬 플레이어의 UI만 변경
	if (IsLocallyControlled())
	{
		ASonheimPlayerController* PC = Cast<ASonheimPlayerController>(GetController());
		if (PC)
		{
			UPlayerStatusWidget* StatusWidget = PC->GetPlayerStatusWidget();
			if (StatusWidget)
			{
				if (bIsPreparing)
				{
					// PalSphere를 들었을 때는 무조건 일반 크로스헤어로 변경
					StatusWidget->SetCrosshairType(EWeaponType::None); 
				}
				else
				{
					// PalSphere를 내렸을 때는 현재 장착한 무기 타입의 크로스헤어로 복원
					StatusWidget->SetCrosshairType(CurrentWeaponType);
				}
			}
		}
	}	
}
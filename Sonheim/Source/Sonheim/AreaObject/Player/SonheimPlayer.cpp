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

void ASonheimPlayer::UpdateEquipWeapon(EEquipmentSlotType WeaponSlot, FInventoryItem Item)
{
	if (WeaponSlot == EEquipmentSlotType::Weapon1 || WeaponSlot == EEquipmentSlotType::Weapon2 || WeaponSlot ==
		EEquipmentSlotType::Weapon3 || WeaponSlot == EEquipmentSlotType::Weapon4)
	{
		FItemData* ItemData = m_GameInstance->GetDataItem(Item.ItemID);
		FSkillData* SkillData = ItemData == nullptr
			                        ? nullptr
			                        : m_GameInstance->GetDataSkill(ItemData->EquipmentData.SkillID);

		// 아이템 해제 or invalid skill
		if (ItemData == nullptr || SkillData == nullptr)
		{
			// 기존 스킬 지우기
			m_SkillInstanceMap.Remove(WeaponSkillMap[WeaponSlot]->GetSkillID());
			// 기본 스킬(주먹 평타) 등록
			WeaponSkillMap[WeaponSlot] = CommonAttack;
			// 목록 최신화
			RefreshWeaponSkillToSkillInstanceMap();

			if (SelectedWeaponSlot == WeaponSlot)
			{
				WeaponComponent->SetSkeletalMesh(nullptr);
				S_PlayerAnimInstance->bIsMelee = false;
			}
			return;
		}

		// 새로운 무기 장착
		// 무기 스킬 생성
		UBaseSkill* weaponSkill = NewObject<UBaseSkill>(this, SkillData->SkillClass);
		weaponSkill->InitSkill(SkillData);
		// 무기 스킬 등록
		WeaponSkillMap[WeaponSlot] = weaponSkill;
		// Skill Instance Map 최신화
		RefreshWeaponSkillToSkillInstanceMap();
		// m_SkillInstanceMap.Remove(SkillData->SkillID);
		// m_SkillInstanceMap.Add(SkillData->SkillID,weaponSkill);
		//WeaponSkillMap.Add(WeaponSlot, weaponSkill);

		// 외형 최신화
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

	// 글라이딩 상태일 때 업데이트
	if (bIsGliding)
	{
		UpdateGliding(DeltaTime);
	}

	// ToDo : 별도의 지시사항으로 Enable Disable
	if (m_SummonedPal != nullptr)
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
	if (bUsingPartnerSkill)
	{
		m_SelectedPal->PartnerSkillTrigger(true);
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

	if (bUsingPartnerSkill) return;

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
	if (bUsingPartnerSkill)
	{
		m_SelectedPal->PartnerSkillTrigger(false);
		return;
	}

	MultiCast_LeftMouse_Released();
}

void ASonheimPlayer::MultiCast_LeftMouse_Released_Implementation()
{
	// 멀티캐스트로 처리할 동작
}

// 마우스 오른쪽 클릭 처리
void ASonheimPlayer::RightMouse_Pressed()
{
	if (IsDie())
	{
		return;
	}

	Server_RightMouse_Pressed();

	// 로컬 플레이어만 수행하는 카메라 처리
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

void ASonheimPlayer::Server_RightMouse_Pressed_Implementation()
{
	MultiCast_RightMouse_Pressed();
}

void ASonheimPlayer::MultiCast_RightMouse_Pressed_Implementation()
{
	// 카메라 회전
	LookAtLocation(GetActorLocation() + GetFollowCamera()->GetForwardVector(), EPMRotationMode::Speed, 600);
	// 록온 모드
	bUseControllerRotationYaw = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	S_PlayerAnimInstance->bIsLockOn = true;

	if (S_PlayerController && S_PlayerController->GetPlayerStatusWidget())
	{
		S_PlayerController->GetPlayerStatusWidget()->SetEnableCrossHair(true);
	}
}

void ASonheimPlayer::RightMouse_Released()
{
	Server_RightMouse_Released();

	// 로컬 플레이어만 수행하는 UI 처리
	if (S_PlayerController && S_PlayerController->GetPlayerStatusWidget())
	{
		S_PlayerController->GetPlayerStatusWidget()->SetEnableCrossHair(false);
	}

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

void ASonheimPlayer::Server_RightMouse_Released_Implementation()
{
	MultiCast_RightMouse_Released();
}

void ASonheimPlayer::MultiCast_RightMouse_Released_Implementation()
{
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	if (S_PlayerAnimInstance != nullptr)
	{
		S_PlayerAnimInstance->bIsLockOn = false;
	}
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

void ASonheimPlayer::WeaponSwitch_Triggered()
{
}

void ASonheimPlayer::Sprint_Pressed()
{
	Server_Sprint_Pressed();
}

void ASonheimPlayer::Server_Sprint_Pressed_Implementation()
{
	MultiCast_Sprint_Pressed();
}

void ASonheimPlayer::MultiCast_Sprint_Pressed_Implementation()
{
	float SprintSpeed = dt_AreaObject->WalkSpeed * SprintSpeedRatio;
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
}

void ASonheimPlayer::Sprint_Released()
{
	Server_Sprint_Released();
}

void ASonheimPlayer::Server_Sprint_Released_Implementation()
{
	MultiCast_Sprint_Released();
}

void ASonheimPlayer::MultiCast_Sprint_Released_Implementation()
{
	GetCharacterMovement()->MaxWalkSpeed = dt_AreaObject->WalkSpeed;
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
	if (m_SelectedPal == nullptr || m_SummonedPal == nullptr)
	{
		FLog::Log("Partner Pal Is Not Exist");
		return;
	}

	Server_PartnerSkill_Pressed();
}

void ASonheimPlayer::Server_PartnerSkill_Pressed_Implementation()
{
	//if (m_SelectedPal == nullptr || m_SummonedPal == nullptr) return;
	if (m_SummonedPal == nullptr) return;

	if (!bUsingPartnerSkill)
	{
		m_SummonedPal->PartnerSkillStart();
		MultiCast_PartnerSkill_Pressed();
	}
	else
	{
		m_SummonedPal->PartnerSkillEnd();
		MultiCast_PartnerSkill_Released();
	}
}

void ASonheimPlayer::MultiCast_PartnerSkill_Pressed_Implementation()
{
	//SetUsePartnerSkill(true);
}

void ASonheimPlayer::PartnerSkill_Released()
{
	if (m_SelectedPal == nullptr)
	{
		FLog::Log("Partner Pall Is Not Exist");
		return;
	}

	Server_PartnerSkill_Released();
}

void ASonheimPlayer::Server_PartnerSkill_Released_Implementation()
{
	if (m_SelectedPal == nullptr) return;

	MultiCast_PartnerSkill_Released();
}

void ASonheimPlayer::MultiCast_PartnerSkill_Released_Implementation()
{
	SetUsePartnerSkill(false);
}

void ASonheimPlayer::SummonPal_Pressed()
{
	UpdateSelectedPal();
	if (m_SelectedPal == nullptr)
	{
		FLog::Log("Partner Pal Is Not Exist");
		return;
	}

	FLog::Log("SelectedPal Index : ", CurrentPalIndex);
	Server_SummonPal_Pressed();
}

void ASonheimPlayer::Server_SummonPal_Pressed_Implementation()
{
	if (m_SelectedPal == nullptr) return;
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
	MultiCast_SummonPal_Pressed();
}

void ASonheimPlayer::MultiCast_SummonPal_Pressed_Implementation()
{
	if (HasAuthority()) return;

	FOnMontageEnded EndDelegate;
	EndDelegate.BindLambda([this](UAnimMontage* Montage, bool bInterrupted)
	{
		PalSphereComponent->SetVisibility(false);
		if (m_SelectedPal != m_SummonedPal)
		{
			if (m_SummonedPal != nullptr)
			{
				SetUsePartnerSkill(false);
			}
			m_SummonedPal = m_SelectedPal;
		}
		else
		{
			SetUsePartnerSkill(false);
			m_SummonedPal = nullptr;
		}
	});
	PalSphereComponent->SetVisibility(true);
	PlayAnimMontage(SummonPalMontage);
	S_PlayerAnimInstance->Montage_SetEndDelegate(EndDelegate, SummonPalMontage);
}

void ASonheimPlayer::SwitchPalSlot_Triggered(int Index)
{
	Server_SwitchPalSlot_Triggered(Index);
}

void ASonheimPlayer::Server_SwitchPalSlot_Triggered_Implementation(int Index)
{
	MultiCast_SwitchPalSlot_Triggered(Index);
}

void ASonheimPlayer::MultiCast_SwitchPalSlot_Triggered_Implementation(int Index)
{
	const int minIndex = 0;
	const int maxIndex = FMath::Min(PalMaxIndex, m_OwnedPals.Num());
	// divide by zero 방지
	if (maxIndex == minIndex) return;

	CurrentPalIndex = (CurrentPalIndex + Index) % maxIndex;
	if (CurrentPalIndex < minIndex) CurrentPalIndex += maxIndex;
	FLog::Log("CurrentPalIndex : ", CurrentPalIndex);
	UpdateSelectedPal();
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
	this->bUsingPartnerSkill = UsePartnerSkill;

	if (UsePartnerSkill)
	{
		S_PlayerAnimInstance->bUsingPartnerSkill = true;
	}
	else
	{
		S_PlayerAnimInstance->bUsingPartnerSkill = false;
	}
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
	Server_ThrowPalSphere_Pressed();
}

void ASonheimPlayer::Server_ThrowPalSphere_Pressed_Implementation()
{
	MultiCast_ThrowPalSphere_Pressed();
}

void ASonheimPlayer::MultiCast_ThrowPalSphere_Pressed_Implementation()
{
	S_PlayerAnimInstance->bIsThrowPalSphere = true;
	PalSphereComponent->SetVisibility(true);
}

void ASonheimPlayer::ThrowPalSphere_Triggered()
{
}

void ASonheimPlayer::ThrowPalSphere_Released()
{
	Server_ThrowPalSphere_Released();
}

void ASonheimPlayer::Server_ThrowPalSphere_Released_Implementation()
{
	UBaseSkill* Skill = GetSkillByID(15);
	CastSkill(Skill, this);
	Multicast_ThrowPalSphere_Released();
}

void ASonheimPlayer::Multicast_ThrowPalSphere_Released_Implementation()
{
	PalSphereComponent->SetVisibility(false);
	S_PlayerAnimInstance->bIsThrowPalSphere = false;
}

// 마우스 오른쪽 트리거 처리
void ASonheimPlayer::RightMouse_Triggered()
{
	Server_RightMouse_Triggered();
}

void ASonheimPlayer::Server_RightMouse_Triggered_Implementation()
{
	MultiCast_RightMouse_Triggered();
}

void ASonheimPlayer::MultiCast_RightMouse_Triggered_Implementation()
{
	// 필요한 경우 추가 동작 구현
}

// 스프린트 트리거 처리
void ASonheimPlayer::Sprint_Triggered()
{
	Server_Sprint_Triggered();
}

void ASonheimPlayer::Server_Sprint_Triggered_Implementation()
{
	MultiCast_Sprint_Triggered();
}

void ASonheimPlayer::MultiCast_Sprint_Triggered_Implementation()
{
	DecreaseStamina(0.5f);
}

// 파트너 스킬 트리거 처리
void ASonheimPlayer::PartnerSkill_Triggered()
{
	if (m_SelectedPal == nullptr || m_SummonedPal == nullptr) return;

	Server_PartnerSkill_Triggered();
}

void ASonheimPlayer::Server_PartnerSkill_Triggered_Implementation()
{
	if (m_SelectedPal == nullptr || m_SummonedPal == nullptr) return;

	MultiCast_PartnerSkill_Triggered();
}

void ASonheimPlayer::MultiCast_PartnerSkill_Triggered_Implementation()
{
	// 필요한 경우 추가 동작 구현
}

void ASonheimPlayer::Server_ThrowPalSphere_Triggered_Implementation()
{
	MultiCast_ThrowPalSphere_Triggered();
}

void ASonheimPlayer::MultiCast_ThrowPalSphere_Triggered_Implementation()
{
	// 필요한 경우 추가 동작 구현
}

void ASonheimPlayer::ActivateGlider()
{
	// 이미 글라이딩 중이거나 땅에 있거나 죽었으면 리턴
	if (bIsGliding || GetCharacterMovement()->IsMovingOnGround() || IsDie())
		return;

	Server_ActivateGlider();
}

void ASonheimPlayer::Server_ActivateGlider_Implementation()
{
	// 서버 측 검증
	if (GetCharacterMovement()->IsMovingOnGround() || IsDie())
		return;

	MultiCast_ActivateGlider();
}

void ASonheimPlayer::MultiCast_ActivateGlider_Implementation()
{
	bIsGliding = true;

	// 글라이더 메시 표시
	GliderMeshComponent->SetVisibility(true);

	// 애니메이션 재생
	if (GliderOpenMontage)
	{
		PlayAnimMontage(GliderOpenMontage);
	}

	// 상태 변경
	SetPlayerState(EPlayerState::GLIDING);

	// 무브먼트 컴포넌트 설정 변경
	UCharacterMovementComponent* MovementComp = GetCharacterMovement();
	MovementComp->SetMovementMode(MOVE_Falling);
	MovementComp->GravityScale = 0.1f; // 중력 감소
	MovementComp->AirControl = 1.0f; // 공중 제어 최대화
	MovementComp->FallingLateralFriction = 2.0f; // 측면 마찰력 증가

	// 애니메이션 설정
	if (S_PlayerAnimInstance)
	{
		S_PlayerAnimInstance->bIsGliding = true;
	}
}

void ASonheimPlayer::DeactivateGlider()
{
	if (!bIsGliding)
		return;

	Server_DeactivateGlider();
}

void ASonheimPlayer::Server_DeactivateGlider_Implementation()
{
	MultiCast_DeactivateGlider();
}

void ASonheimPlayer::MultiCast_DeactivateGlider_Implementation()
{
	bIsGliding = false;

	// 글라이더 닫는 애니메이션 재생
	if (GliderCloseMontage)
	{
		PlayAnimMontage(GliderCloseMontage);
	}

	// 애니메이션 이벤트로 메시를 숨기도록 설정하거나 여기서 타이머로 처리
	GliderMeshComponent->SetVisibility(false);

	// 상태 초기화
	SetPlayerState(EPlayerState::NORMAL);

	// 무브먼트 컴포넌트 설정 복원
	UCharacterMovementComponent* MovementComp = GetCharacterMovement();
	MovementComp->SetMovementMode(MOVE_Falling);
	MovementComp->GravityScale = 1.0f;
	MovementComp->AirControl = 0.2f;
	MovementComp->FallingLateralFriction = 0.0f;

	// 애니메이션 설정
	if (S_PlayerAnimInstance)
	{
		S_PlayerAnimInstance->bIsGliding = false;
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

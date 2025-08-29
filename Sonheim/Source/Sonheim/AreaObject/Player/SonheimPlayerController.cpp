// Fill out your copyright notice in the Description page of Project Settings.


#include "SonheimPlayerController.h"
#include "InputMappingContext.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "SonheimPlayer.h"
#include "SonheimPlayerState.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Sonheim/AreaObject/Attribute/LevelComponent.h"
#include "Sonheim/AreaObject/Attribute/StaminaComponent.h"
#include "Sonheim/GameObject/Buildings/Storage/BaseContainer.h"
#include "Sonheim/GameObject/Buildings/Crafting/CraftingStation.h"
#include "Sonheim/GameObject/Buildings/Utility/ContainerComponent.h"
#include "Sonheim/UI/Widget/GameObject/ContainerInteractionWidget.h"
#include "Sonheim/UI/Widget/GameObject/Crafting/CraftingWidget.h"
#include "Sonheim/UI/Widget/Player/PlayerStatusWidget.h"
#include "Sonheim/UI/Widget/Player/Inventory/InventoryWidget.h"
#include "Sonheim/UI/Widget/Player/Inventory/PlayerStatWidget.h"
#include "Utility/InventoryComponent.h"

ASonheimPlayerController::ASonheimPlayerController()
{
	// Enhanced Input Setting
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> tempInputMapping(
		TEXT(
			"/Script/EnhancedInput.InputMappingContext'/Game/_BluePrint/AreaObject/Player/Input/IMC_Default.IMC_Default'"));
	if (tempInputMapping.Succeeded())
	{
		DefaultMappingContext = tempInputMapping.Object;
	}
	static ConstructorHelpers::FObjectFinder<UInputAction> tempMoveAction(
		TEXT("/Script/EnhancedInput.InputAction'/Game/_BluePrint/AreaObject/Player/Input/Actions/IA_Move.IA_Move'"));
	if (tempMoveAction.Succeeded())
	{
		MoveAction = tempMoveAction.Object;
	}
	static ConstructorHelpers::FObjectFinder<UInputAction> tempLookAction(
		TEXT("/Script/EnhancedInput.InputAction'/Game/_BluePrint/AreaObject/Player/Input/Actions/IA_Look.IA_Look'"));
	if (tempLookAction.Succeeded())
	{
		LookAction = tempLookAction.Object;
	}
	static ConstructorHelpers::FObjectFinder<UInputAction> tempLeftMouseAction(
		TEXT(
			"/Script/EnhancedInput.InputAction'/Game/_BluePrint/AreaObject/Player/Input/Actions/IA_LeftButton.IA_LeftButton'"));
	if (tempLeftMouseAction.Succeeded())
	{
		LeftMouseAction = tempLeftMouseAction.Object;
	}
	static ConstructorHelpers::FObjectFinder<UInputAction> tempRightMouseAction(
		TEXT(
			"/Script/EnhancedInput.InputAction'/Game/_BluePrint/AreaObject/Player/Input/Actions/IA_RightButton.IA_RightButton'"));
	if (tempRightMouseAction.Succeeded())
	{
		RightMouseAction = tempRightMouseAction.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> tempJumpAction(
		TEXT("/Script/EnhancedInput.InputAction'/Game/_BluePrint/AreaObject/Player/Input/Actions/IA_Jump.IA_Jump'"));
	if (tempJumpAction.Succeeded())
	{
		JumpAction = tempJumpAction.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> tempSprintAction(
		TEXT(
			"/Script/EnhancedInput.InputAction'/Game/_BluePrint/AreaObject/Player/Input/Actions/IA_Sprint.IA_Sprint'"));
	if (tempSprintAction.Succeeded())
	{
		SprintAction = tempSprintAction.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> tempEvadeAction(
		TEXT("/Script/EnhancedInput.InputAction'/Game/_BluePrint/AreaObject/Player/Input/Actions/IA_Dodge.IA_Dodge'"));
	if (tempEvadeAction.Succeeded())
	{
		EvadeAction = tempEvadeAction.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> tempReload(
		TEXT(
			"/Script/EnhancedInput.InputAction'/Game/_BluePrint/AreaObject/Player/Input/Actions/IA_Reload.IA_Reload'"));
	if (tempReload.Succeeded())
	{
		ReloadAction = tempReload.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> tempSwitchWeapon(
		TEXT(
			"/Script/EnhancedInput.InputAction'/Game/_BluePrint/AreaObject/Player/Input/Actions/IA_SwtichWeapon.IA_SwtichWeapon'"));
	if (tempSwitchWeapon.Succeeded())
	{
		SwitchWeaponAction = tempSwitchWeapon.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> tempPartnerSkill(
		TEXT(
			"/Script/EnhancedInput.InputAction'/Game/_BluePrint/AreaObject/Player/Input/Actions/IA_PartnerSkill.IA_PartnerSkill'"));
	if (tempPartnerSkill.Succeeded())
	{
		PartnerSkillAction = tempPartnerSkill.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> tempSwitchPal(
		TEXT(
			"/Script/EnhancedInput.InputAction'/Game/_BluePrint/AreaObject/Player/Input/Actions/IA_SwitchPal.IA_SwitchPal'"));
	if (tempSwitchPal.Succeeded())
	{
		SwitchPalAction = tempSwitchPal.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> tempSummonPal(
		TEXT(
			"/Script/EnhancedInput.InputAction'/Game/_BluePrint/AreaObject/Player/Input/Actions/IA_SummonPal.IA_SummonPal'"));
	if (tempSummonPal.Succeeded())
	{
		SummonPalSlotAction = tempSummonPal.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> tempThrowPalSphere(
		TEXT(
			"/Script/EnhancedInput.InputAction'/Game/_BluePrint/AreaObject/Player/Input/Actions/IA_ThrowPalSphere.IA_ThrowPalSphere'"));
	if (tempThrowPalSphere.Succeeded())
	{
		ThrowPalSphereAction = tempThrowPalSphere.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> tempMenu(
		TEXT("/Script/EnhancedInput.InputAction'/Game/_BluePrint/AreaObject/Player/Input/Actions/IA_Menu.IA_Menu'"));
	if (tempMenu.Succeeded())
	{
		MenuAction = tempMenu.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> tempGliderAction(
		TEXT(
			"/Script/EnhancedInput.InputAction'/Game/_BluePrint/AreaObject/Player/Input/Actions/IA_Glider.IA_Glider'"));
	if (tempGliderAction.Succeeded())
	{
		GliderAction = tempGliderAction.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> tempFAction(
		TEXT(
			"/Script/EnhancedInput.InputAction'/Game/_BluePrint/AreaObject/Player/Input/Actions/IA_FInput.IA_FInput'"));
	if (tempFAction.Succeeded())
	{
		FKeyAction = tempFAction.Object;
	}

	m_Player = nullptr;

	// UI 클래스 설정
	static ConstructorHelpers::FClassFinder<UPlayerStatusWidget> WidgetClassFinder(
		TEXT(
			"/Script/UMGEditor.WidgetBlueprint'/Game/_BluePrint/Widget/WB_PlayerStatusWidget.WB_PlayerStatusWidget_C'"));
	if (WidgetClassFinder.Succeeded())
	{
		StatusWidgetClass = WidgetClassFinder.Class;
	}

	static ConstructorHelpers::FClassFinder<UInventoryWidget> inventoryWidgetClassFinder(
		TEXT(
			"/Script/UMGEditor.WidgetBlueprint'/Game/_BluePrint/Widget/Player/WBP_Inventory.WBP_Inventory_C'"));
	if (inventoryWidgetClassFinder.Succeeded())
	{
		InventoryWidgetClass = inventoryWidgetClassFinder.Class;
	}

	static ConstructorHelpers::FClassFinder<UPlayerStatWidget> pStatWidgetClassFinder(
		TEXT(
			"/Script/UMGEditor.WidgetBlueprint'/Game/_BluePrint/Widget/Player/WBP_PlayerStat.WBP_PlayerStat_C'"));
	if (pStatWidgetClassFinder.Succeeded())
	{
		PlayerStatWidgetClass = pStatWidgetClassFinder.Class;
	}

	// 상자 UI 클래스 설정
	static ConstructorHelpers::FClassFinder<UContainerInteractionWidget> ContainerWidgetFinder(
		TEXT(
			"/Script/UMGEditor.WidgetBlueprint'/Game/_BluePrint/Widget/GameObject/WB_ContainerInteraction.WB_ContainerInteraction_C'"));
	if (ContainerWidgetFinder.Succeeded())
	{
		ContainerInteractionWidgetClass = ContainerWidgetFinder.Class;
	}
	static ConstructorHelpers::FClassFinder<UUserWidget> WBPClass(
		TEXT(
			"/Script/UMGEditor.WidgetBlueprint'/Game/_BluePrint/Widget/GameObject/WB_CraftingWidget.WB_CraftingWidget_C'"));
	if (WBPClass.Succeeded()) CraftingWidgetClass = WBPClass.Class;
}

void ASonheimPlayerController::BeginPlay()
{
	Super::BeginPlay();
	m_Player = Cast<ASonheimPlayer>(GetPawn());
	m_PlayerState = GetPlayerState<ASonheimPlayerState>();

	if (!IsLocalController()) return;

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(
		this->GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}

	// UI 초기화
	//InitializeHUD();
}

void ASonheimPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	ASonheimPlayer* player = Cast<ASonheimPlayer>(InPawn);
	if (m_Player == nullptr) m_Player = player;
	m_PlayerState = GetPlayerState<ASonheimPlayerState>();
	if (m_PlayerState != nullptr)
	{
		m_PlayerState->InitPlayerState();
	}
}

void ASonheimPlayerController::InitializeWithPlayer(ASonheimPlayer* NewPlayer)
{
	if (m_Player == nullptr) m_Player = NewPlayer;
	m_PlayerState = m_Player->GetSPlayerState();
	if (m_PlayerState != nullptr)
	{
		m_PlayerState->InitPlayerState();
	}
}

UPlayerStatusWidget* ASonheimPlayerController::GetPlayerStatusWidget() const
{
	return StatusWidget;
}

void ASonheimPlayerController::InitializeHUD_Implementation(ASonheimPlayer* NewPlayer)
{
	//if (!IsLocalController()) return;
	//LOG_SCREEN("ASonheimPlayerController::InitializeHUD");
	if (m_Player == nullptr) m_Player = NewPlayer;
	if (m_PlayerState == nullptr) m_PlayerState = Cast<ASonheimPlayerState>(PlayerState);
	if (!StatusWidgetClass || !m_Player) return;

	// UI 위젯 생성
	StatusWidget = CreateWidget<UPlayerStatusWidget>(this, StatusWidgetClass);
	if (StatusWidget)
	{
		StatusWidget->AddToViewport();

		// HP 변경 이벤트 바인딩
		if (m_Player->m_HealthComponent)
		{
			m_Player->m_HealthComponent->OnHealthChanged.AddDynamic(StatusWidget, &UPlayerStatusWidget::UpdateHealth);
			// 초기값 설정
			StatusWidget->UpdateHealth(m_Player->GetHP(), 0.0f, m_Player->m_HealthComponent->GetMaxHP());
		}

		// Stamina 변경 이벤트 바인딩
		if (m_Player->m_StaminaComponent)
		{
			m_Player->m_StaminaComponent->OnStaminaChanged.
			          AddDynamic(StatusWidget, &UPlayerStatusWidget::UpdateStamina);
			// 초기값 설정
			StatusWidget->UpdateStamina(m_Player->GetStamina(), 0.0f, m_Player->m_StaminaComponent->GetMaxStamina());
		}
		if (m_Player->m_LevelComponent)
		{
			m_Player->m_LevelComponent->OnLevelChanged.AddDynamic(StatusWidget, &UPlayerStatusWidget::UpdateLevel);
			StatusWidget->UpdateLevel(m_Player->m_LevelComponent->GetCurrentLevel(),
			                          m_Player->m_LevelComponent->GetCurrentLevel(), true);
			m_Player->m_LevelComponent->OnExperienceChanged.AddDynamic(StatusWidget, &UPlayerStatusWidget::UpdateExp);
			StatusWidget->UpdateExp(0, 1, 0);
		}
		StatusWidget->SetEnableCrossHair(false);
	}
}

void ASonheimPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (!IsLocalController()) return;
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this,
		                                   &ASonheimPlayerController::OnMove);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this,
		                                   &ASonheimPlayerController::OnLook);

		// Attack
		EnhancedInputComponent->BindAction(LeftMouseAction, ETriggerEvent::Started, this,
		                                   &ASonheimPlayerController::On_Mouse_Left_Pressed);
		EnhancedInputComponent->BindAction(LeftMouseAction, ETriggerEvent::Completed, this,
		                                   &ASonheimPlayerController::On_Mouse_Left_Released);
		EnhancedInputComponent->BindAction(LeftMouseAction, ETriggerEvent::Triggered, this,
		                                   &ASonheimPlayerController::On_Mouse_Left_Triggered);
		EnhancedInputComponent->BindAction(RightMouseAction, ETriggerEvent::Started, this,
		                                   &ASonheimPlayerController::On_Mouse_Right_Pressed);
		EnhancedInputComponent->BindAction(RightMouseAction, ETriggerEvent::Triggered, this,
		                                   &ASonheimPlayerController::On_Mouse_Right_Triggered);
		EnhancedInputComponent->BindAction(RightMouseAction, ETriggerEvent::Completed, this,
		                                   &ASonheimPlayerController::On_Mouse_Right_Released);

		// Jump
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this,
		                                   &ASonheimPlayerController::On_Jump_Pressed);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this,
		                                   &ASonheimPlayerController::On_Jump_Released);

		// Sprint
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this,
		                                   &ASonheimPlayerController::On_Sprint_Pressed);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Triggered, this,
		                                   &ASonheimPlayerController::On_Sprint_Triggered);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this,
		                                   &ASonheimPlayerController::On_Sprint_Released);

		// Evade
		EnhancedInputComponent->BindAction(EvadeAction, ETriggerEvent::Started, this,
		                                   &ASonheimPlayerController::On_Dodge_Pressed);

		// Reload
		EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this,
		                                   &ASonheimPlayerController::On_Reload_Pressed);

		// SwitchWeapon
		EnhancedInputComponent->BindAction(SwitchWeaponAction, ETriggerEvent::Triggered, this,
		                                   &ASonheimPlayerController::On_WeaponSwitch_Triggered);

		// PartnerSkill
		EnhancedInputComponent->BindAction(PartnerSkillAction, ETriggerEvent::Started, this,
		                                   &ASonheimPlayerController::On_PartnerSkill_Pressed);
		EnhancedInputComponent->BindAction(PartnerSkillAction, ETriggerEvent::Triggered, this,
		                                   &ASonheimPlayerController::On_PartnerSkill_Triggered);
		EnhancedInputComponent->BindAction(PartnerSkillAction, ETriggerEvent::Completed, this,
		                                   &ASonheimPlayerController::On_PartnerSkill_Released);

		// Summon Pal
		EnhancedInputComponent->BindAction(SummonPalSlotAction, ETriggerEvent::Started, this,
		                                   &ASonheimPlayerController::On_SummonPal_Pressed);

		// Switch Pal
		EnhancedInputComponent->BindAction(SwitchPalAction, ETriggerEvent::Triggered, this,
		                                   &ASonheimPlayerController::On_SwitchPalSlot_Triggered);

		// ThrowPalSphere
		EnhancedInputComponent->BindAction(ThrowPalSphereAction, ETriggerEvent::Started, this,
		                                   &ASonheimPlayerController::On_ThrowPalSphere_Pressed);
		EnhancedInputComponent->BindAction(ThrowPalSphereAction, ETriggerEvent::Triggered, this,
		                                   &ASonheimPlayerController::On_ThrowPalSphere_Triggered);
		EnhancedInputComponent->BindAction(ThrowPalSphereAction, ETriggerEvent::Completed, this,
		                                   &ASonheimPlayerController::On_ThrowPalSphere_Released);

		// Menu
		EnhancedInputComponent->BindAction(MenuAction, ETriggerEvent::Started, this,
		                                   &ASonheimPlayerController::On_Menu_Pressed);
		EnhancedInputComponent->BindAction(MenuAction, ETriggerEvent::Completed, this,
		                                   &ASonheimPlayerController::On_Menu_Released);

		// Restart
		EnhancedInputComponent->BindAction(RestartAction, ETriggerEvent::Started, this,
		                                   &ASonheimPlayerController::On_Menu_Pressed);

		// Glider (점프 버튼 두 번 누름으로도 활성화)
		EnhancedInputComponent->BindAction(GliderAction, ETriggerEvent::Started, this,
		                                   &ASonheimPlayerController::On_Glider_Pressed);
		EnhancedInputComponent->BindAction(GliderAction, ETriggerEvent::Completed, this,
		                                   &ASonheimPlayerController::On_Glider_Released);

		// F (상호작용 키)
		EnhancedInputComponent->BindAction(FKeyAction, ETriggerEvent::Started, this,
		                                   &ASonheimPlayerController::On_FKey_Pressed);
		EnhancedInputComponent->BindAction(FKeyAction, ETriggerEvent::Completed, this,
		                                   &ASonheimPlayerController::On_FKey_Released);
	}
	else
	{
		UE_LOG(LogTemp, Error,
		       TEXT(
			       "'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."
		       ), *GetNameSafe(this));
	}
}

void ASonheimPlayerController::OnMove(const FInputActionValue& Value)
{
	if (IsMenuActivate || IsContainerActivate) return;
	m_Player->Move(Value.Get<FVector2D>());
}

void ASonheimPlayerController::OnLook(const FInputActionValue& Value)
{
	if (IsMenuActivate || IsContainerActivate) return;
	m_Player->Look(Value.Get<FVector2D>());
}

void ASonheimPlayerController::On_Mouse_Left_Pressed(const FInputActionValue& InputActionValue)
{
	if (IsMenuActivate || IsContainerActivate) return;
	m_Player->LeftMouse_Pressed();
}

void ASonheimPlayerController::On_Mouse_Left_Released(const FInputActionValue& InputActionValue)
{
	if (IsMenuActivate || IsContainerActivate) return;
	m_Player->LeftMouse_Released();
}

void ASonheimPlayerController::On_Mouse_Left_Triggered(const FInputActionValue& InputActionValue)
{
	if (IsMenuActivate || IsContainerActivate) return;
	m_Player->LeftMouse_Triggered();
}

void ASonheimPlayerController::On_Mouse_Right_Pressed(const FInputActionValue& InputActionValue)
{
	if (IsMenuActivate || IsContainerActivate) return;

	// 팰스피어 던지는 중이면 캔슬하기
	if (m_Player->IsThrowingPalSphere())
	{
		m_Player->ThrowPalSphere_Canceled();
		GetPlayerStatusWidget()->SetEnableCrossHair(false);
		GetPlayerStatusWidget()->SetEnableKeyGuide(false, EUIKeyGuide::None);
		m_Player->SetWeaponVisible(true, true);
		m_Player->RightMouse_Released();
		return;;
	}

	m_Player->RightMouse_Pressed();
	GetPlayerStatusWidget()->SetEnableCrossHair(true);
}

void ASonheimPlayerController::On_Mouse_Right_Triggered(const FInputActionValue& InputActionValue)
{
	if (IsMenuActivate || IsContainerActivate) return;
	m_Player->RightMouse_Triggered();
}

void ASonheimPlayerController::On_Sprint_Pressed(const FInputActionValue& InputActionValue)
{
	if (IsMenuActivate || IsContainerActivate) return;
	m_Player->Sprint_Pressed();
}

void ASonheimPlayerController::On_Sprint_Triggered(const FInputActionValue& InputActionValue)
{
	if (IsMenuActivate || IsContainerActivate) return;
	m_Player->Sprint_Triggered();
}

void ASonheimPlayerController::On_Sprint_Released(const FInputActionValue& InputActionValue)
{
	if (IsMenuActivate || IsContainerActivate) return;
	m_Player->Sprint_Released();
}

void ASonheimPlayerController::On_Mouse_Right_Released(const FInputActionValue& InputActionValue)
{
	if (IsMenuActivate || IsContainerActivate) return;
	m_Player->RightMouse_Released();
	GetPlayerStatusWidget()->SetEnableCrossHair(false);
}

void ASonheimPlayerController::On_Dodge_Pressed(const FInputActionValue& InputActionValue)
{
	if (IsMenuActivate || IsContainerActivate) return;
	m_Player->Dodge_Pressed();
}

void ASonheimPlayerController::On_Jump_Pressed(const FInputActionValue& InputActionValue)
{
	if (IsMenuActivate || IsContainerActivate) return;

	float CurrentTime = GetWorld()->GetTimeSeconds();

	// 첫 번째 점프 후 짧은 시간 내에 두 번째 점프가 입력되면 글라이더 활성화
	if (CurrentTime - LastJumpTime < DoubleJumpTimeThreshold)
	{
		JumpCount++;

		// 공중에 있을 때만 글라이더 작동
		if (JumpCount >= 2 && m_Player && !m_Player->GetCharacterMovement()->IsMovingOnGround())
		{
			// 글라이더 활성화
			m_Player->ActivateGlider();
			JumpCount = 0;
		}
	}
	else
	{
		// 첫 번째 점프
		JumpCount = 1;
		m_Player->Jump_Pressed();
	}

	LastJumpTime = CurrentTime;
}

void ASonheimPlayerController::On_Jump_Released(const FInputActionValue& InputActionValue)
{
	if (IsMenuActivate || IsContainerActivate) return;
	m_Player->Jump_Released();
}

void ASonheimPlayerController::On_Reload_Pressed(const FInputActionValue& Value)
{
	if (IsMenuActivate || IsContainerActivate) return;
	m_Player->Reload_Pressed();
}

void ASonheimPlayerController::On_WeaponSwitch_Triggered(const FInputActionValue& Value)
{
	const int32 SwitchData = FMath::Sign(Value.Get<float>());
	if (SwitchData != 0)
	{
		m_PlayerState->m_InventoryComponent->SwitchWeaponSlot(SwitchData);
	}
}

void ASonheimPlayerController::On_PartnerSkill_Pressed(const FInputActionValue& InputActionValue)
{
	m_Player->PartnerSkill_Pressed();
}

void ASonheimPlayerController::On_PartnerSkill_Triggered(const FInputActionValue& InputActionValue)
{
	m_Player->PartnerSkill_Triggered();
}

void ASonheimPlayerController::On_PartnerSkill_Released(const FInputActionValue& InputActionValue)
{
	m_Player->PartnerSkill_Released();
}

void ASonheimPlayerController::On_SummonPal_Pressed(const FInputActionValue& Value)
{
	m_Player->SummonPal_Pressed();
}

void ASonheimPlayerController::On_SwitchPalSlot_Triggered(const FInputActionValue& Value)
{
	const int32 SwitchData = FMath::Sign(Value.Get<float>());
	if (SwitchData != 0)
	{
		m_Player->SwitchPalSlot_Triggered(SwitchData);
	}
}

void ASonheimPlayerController::On_ThrowPalSphere_Pressed(const FInputActionValue& InputActionValue)
{
	if (IsMenuActivate || IsContainerActivate) return;
	m_Player->RightMouse_Pressed();
	GetPlayerStatusWidget()->SetEnableCrossHair(true);
	GetPlayerStatusWidget()->SetEnableKeyGuide(true, EUIKeyGuide::RButton, "취소");
	m_Player->ThrowPalSphere_Pressed();
}

void ASonheimPlayerController::On_ThrowPalSphere_Triggered(const FInputActionValue& InputActionValue)
{
	m_Player->ThrowPalSphere_Triggered();
}

void ASonheimPlayerController::On_ThrowPalSphere_Released(const FInputActionValue& InputActionValue)
{
	if (IsMenuActivate || IsContainerActivate) return;
	GetPlayerStatusWidget()->SetEnableCrossHair(false);
	GetPlayerStatusWidget()->SetEnableKeyGuide(false, EUIKeyGuide::None);
	m_Player->RightMouse_Released();
	m_Player->ThrowPalSphere_Released();
}

void ASonheimPlayerController::On_Menu_Pressed(const FInputActionValue& Value)
{
	if (IsContainerActivate) return;
	if (!IsMenuActivate)
	{
		IsMenuActivate = true;

		m_Player->Menu_Pressed();
		InventoryWidget = CreateWidget<UInventoryWidget>(this, InventoryWidgetClass);
		InventoryWidget->AddToViewport(0);
		InventoryWidget->SetInventoryComponent(m_PlayerState->m_InventoryComponent);
		m_PlayerState->m_InventoryComponent->OnInventoryChanged.AddDynamic(InventoryWidget,
		                                                                   &UInventoryWidget::UpdateInventoryFromData);
		m_PlayerState->m_InventoryComponent->OnEquipmentChanged.AddDynamic(InventoryWidget,
		                                                                   &UInventoryWidget::UpdateEquipmentFromData);
		PlayerStatWidget = CreateWidget<UPlayerStatWidget>(this, PlayerStatWidgetClass);
		PlayerStatWidget->AddToViewport(0);
		PlayerStatWidget->InitializePlayerStatWidget(m_PlayerState);
		SetShowMouseCursor(true);
	}
	else
	{
		IsMenuActivate = false;
		SetShowMouseCursor(false);

		m_PlayerState->m_InventoryComponent->OnInventoryChanged.RemoveDynamic(
			InventoryWidget, &UInventoryWidget::UpdateInventoryFromData);
		m_PlayerState->m_InventoryComponent->OnEquipmentChanged.RemoveDynamic(InventoryWidget,
		                                                                      &UInventoryWidget::
		                                                                      UpdateEquipmentFromData);
		InventoryWidget->RemoveFromParent();
		PlayerStatWidget->RemoveFromParent();
		InventoryWidget = nullptr;
		PlayerStatWidget = nullptr;
	}
}

void ASonheimPlayerController::On_Menu_Released(const FInputActionValue& Value)
{
	//if (!IsLocalController()) return;
}

void ASonheimPlayerController::On_Glider_Pressed(const FInputActionValue& InputActionValue)
{
	if (IsMenuActivate || IsContainerActivate) return;
	if (m_Player && !m_Player->GetCharacterMovement()->IsMovingOnGround())
	{
		m_Player->ActivateGlider();
	}
}

void ASonheimPlayerController::On_Glider_Released(const FInputActionValue& InputActionValue)
{
	if (IsMenuActivate || IsContainerActivate) return;
	if (m_Player)
	{
		m_Player->DeactivateGlider();
	}
}

void ASonheimPlayerController::On_FKey_Pressed(const FInputActionValue& InputActionValue)
{
	if (IsMenuActivate || IsContainerActivate) return;
	if (m_Player)
	{
		m_Player->Interaction_Pressed();
	}
}

void ASonheimPlayerController::On_FKey_Released(const FInputActionValue& InputActionValue)
{
	if (IsMenuActivate || IsContainerActivate) return;
	if (m_Player)
	{
		m_Player->Interaction_Released();
	}
}

void ASonheimPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	m_PlayerState = Cast<ASonheimPlayerState>(PlayerState);
}

void ASonheimPlayerController::Client_OpenContainerUI_Implementation(ABaseContainer* Container)
{
	if (!Container || !ContainerInteractionWidgetClass)
		return;

	// 기존 상자 UI가 열려있으면 닫기
	if (ContainerInteractionWidget)
	{
		ContainerInteractionWidget->CloseContainer();
		ContainerInteractionWidget = nullptr;
	}

	// 새 상자 UI 생성
	ContainerInteractionWidget = CreateWidget<UContainerInteractionWidget>(this, ContainerInteractionWidgetClass);
	if (ContainerInteractionWidget)
	{
		ContainerInteractionWidget->AddToViewport(1); // 다른 UI보다 위에 표시
		m_PlayerState->m_InventoryComponent->OnInventoryChanged.AddDynamic(
			ContainerInteractionWidget->GetPlayerInventoryWidget(),
			&UInventoryWidget::UpdateInventoryFromData);
		m_PlayerState->m_InventoryComponent->OnEquipmentChanged.AddDynamic(
			ContainerInteractionWidget->GetPlayerInventoryWidget(),
			&UInventoryWidget::UpdateEquipmentFromData);
		ContainerInteractionWidget->OpenContainer(Container);
		IsContainerActivate = true;
	}
}

void ASonheimPlayerController::Client_CloseContainerUI_Implementation()
{
	if (ContainerInteractionWidget)
	{
		ContainerInteractionWidget->CloseContainer();
		m_PlayerState->m_InventoryComponent->OnInventoryChanged.RemoveDynamic(
			ContainerInteractionWidget->GetPlayerInventoryWidget(),
			&UInventoryWidget::UpdateInventoryFromData);
		m_PlayerState->m_InventoryComponent->OnEquipmentChanged.RemoveDynamic(
			ContainerInteractionWidget->GetPlayerInventoryWidget(),
			&UInventoryWidget::
			UpdateEquipmentFromData);
		ContainerInteractionWidget = nullptr;
		IsContainerActivate = false;
	}
}

void ASonheimPlayerController::Server_ContainerOperation_Implementation(
	ABaseContainer* Container,
	EContainerOperation Operation,
	int32 Param1,
	int32 Param2)
{
	// 기본 검증
	if (!Container || !ValidateContainerAccess(Container))
	{
		UE_LOG(LogTemp, Warning, TEXT("Container validation failed"));
		return;
	}

	UContainerComponent* ContainerComp = Container->GetContainerComponent();
	if (!ContainerComp)
		return;

	// Operation별 처리
	switch (Operation)
	{
	case EContainerOperation::Open:
		{
			// 이미 다른 사람이 사용 중인지 확인
			if (Container->Execute_CanInteract(Container))
			{
				UE_LOG(LogTemp, Warning, TEXT("Container already in use"));
				return;
			}
			Container->Interact(m_Player);
			break;
		}

	case EContainerOperation::Close:
		{
			// 현재 사용자가 맞는지 확인
			if (!Container->CanBeInteractedByPlayer(m_Player))
			{
				UE_LOG(LogTemp, Warning, TEXT("Not the current user of container"));
				return;
			}
			Container->CloseContainer();
			break;
		}

	case EContainerOperation::AddItem:
		{
			// Param1: ItemID, Param2: Count
			if (ValidateItemOperation(Param1, Param2))
			{
				ContainerComp->AddItem(Param1, Param2);
			}
			break;
		}

	case EContainerOperation::RemoveItem:
		{
			// Param1: ItemID, Param2: Count
			if (ValidateItemOperation(Param1, Param2))
			{
				ContainerComp->RemoveItem(Param1, Param2);
			}
			break;
		}

	case EContainerOperation::RemoveItemByIndex:
		{
			// Param1: Index
			if (Param1 >= 0 && Param1 < ContainerComp->GetMaxSlots())
			{
				ContainerComp->RemoveItemByIndex(Param1);
			}
			break;
		}

	case EContainerOperation::SwapItems:
		{
			// Param1: FromIndex, Param2: ToIndex
			if (Param1 >= 0 && Param2 >= 0 &&
				Param1 < ContainerComp->GetMaxSlots() &&
				Param2 < ContainerComp->GetMaxSlots())
			{
				ContainerComp->SwapItems(Param1, Param2);
			}
			break;
		}

	default:
		UE_LOG(LogTemp, Warning, TEXT("Unhandled container operation: %d"), (int32)Operation);
		break;
	}
}

void ASonheimPlayerController::Server_PlayerContainerTransfer_Implementation(
	ABaseContainer* Container,
	bool bFromContainerToPlayer,
	int32 ItemID,
	int32 Count,
	int32 SlotIndex)
{
	// 검증
	if (!Container || !ValidateContainerAccess(Container))
		return;

	if (!m_PlayerState || !m_PlayerState->m_InventoryComponent)
		return;

	if (!ValidateItemOperation(ItemID, Count))
		return;

	UContainerComponent* ContainerComp = Container->GetContainerComponent();
	UInventoryComponent* PlayerInv = m_PlayerState->m_InventoryComponent;

	if (!ContainerComp || !PlayerInv)
		return;

	if (bFromContainerToPlayer)
	{
		// 상자 -> 플레이어
		bool bRemoved = false;

		if (SlotIndex >= 0)
		{
			// 특정 슬롯에서 제거
			bRemoved = ContainerComp->RemoveItemByIndex(SlotIndex);
		}
		else
		{
			// ItemID와 Count로 제거
			bRemoved = ContainerComp->RemoveItem(ItemID, Count);
		}

		if (bRemoved)
		{
			PlayerInv->AddItem(ItemID, Count);

			// 로그
			UE_LOG(LogTemp, Log, TEXT("Transferred %d x%d from container to player"), ItemID, Count);
		}
	}
	else
	{
		// 플레이어 -> 상자
		bool bRemoved = false;

		if (SlotIndex >= 0)
		{
			bRemoved = PlayerInv->RemoveItemByIndex(SlotIndex);
		}
		else
		{
			bRemoved = PlayerInv->RemoveItem(ItemID, Count);
		}

		if (bRemoved)
		{
			ContainerComp->AddItem(ItemID, Count);

			// 로그
			UE_LOG(LogTemp, Log, TEXT("Transferred %d x%d from player to container"), ItemID, Count);
		}
	}
}

bool ASonheimPlayerController::ValidateContainerAccess(ABaseContainer* Container) const
{
	if (!Container || !m_Player)
		return false;

	// 거리 검증
	if (!ValidateDistance(Container))
		return false;

	// 상자가 사용 가능한지 확인
	if (!Container->CanBeInteractedByPlayer(m_Player))
	{
		UE_LOG(LogTemp, Warning, TEXT("Container cannot be interacted with"));
		return false;
	}

	return true;
}

bool ASonheimPlayerController::ValidateItemOperation(int32 ItemID, int32 Count) const
{
	// 아이템 ID 유효성
	if (ItemID <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid ItemID: %d"), ItemID);
		return false;
	}

	// 수량 유효성
	if (Count <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid item count: %d"), Count);
		return false;
	}

	// 최대 스택 크기 확인
	const int32 MaxStackSize = 9999;
	if (Count > MaxStackSize)
	{
		UE_LOG(LogTemp, Warning, TEXT("Item count exceeds max stack size: %d"), Count);
		return false;
	}

	return true;
}

bool ASonheimPlayerController::ValidateDistance(AActor* Target, float MaxDistance) const
{
	if (!Target || !m_Player)
		return false;

	float Distance = FVector::Dist(m_Player->GetActorLocation(), Target->GetActorLocation());

	if (Distance > MaxDistance)
	{
		UE_LOG(LogTemp, Warning, TEXT("Target too far: %.2f > %.2f"), Distance, MaxDistance);
		return false;
	}

	return true;
}

void ASonheimPlayerController::Client_OpenCraftingUI_Implementation(ACraftingStation* Station)
{
	if (!IsLocalController() || !Station) return;
	if (!CraftingWidget)
	{
		if (CraftingWidgetClass)
		{
			CraftingWidget = CreateWidget<UCraftingWidget>(this, CraftingWidgetClass);
		}
	}
	if (CraftingWidget && !CraftingWidget->IsInViewport())
	{
		CraftingWidget->AddToViewport();
		CraftingWidget->Initialise(Station);
		bShowMouseCursor = true;
		SetInputMode(FInputModeUIOnly());
	}
}

void ASonheimPlayerController::Client_CloseCraftingUI_Implementation()
{
	if (!IsLocalController()) return;
	if (CraftingWidget)
	{
		CraftingWidget->RemoveFromParent();
		CraftingWidget = nullptr;
	}
	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());
}

void ASonheimPlayerController::ServerStartWork_Implementation(
	ACraftingStation* Station, FName Row, int32 Quantity)
{
	if (!Station) return;
	ASonheimPlayer* P = m_Player ? m_Player : Cast<ASonheimPlayer>(GetPawn());
	Station->ServerStartWork(P, Row, Quantity);
}

void ASonheimPlayerController::Server_Crafting_RequestCollectAll_Implementation(ACraftingStation* Station)
{
	if (!Station) return;
	ASonheimPlayer* P = m_Player ? m_Player : Cast<ASonheimPlayer>(GetPawn());
	Station->ServerCollectAll(P);
}

void ASonheimPlayerController::Server_Crafting_ReleaseUI_Implementation(ACraftingStation* Station)
{
	if (!Station) return;
	ASonheimPlayer* P = m_Player ? m_Player : Cast<ASonheimPlayer>(GetPawn());
	Station->ServerReleaseUI(P);
}

void ASonheimPlayerController::Server_Crafting_AssistStart_Implementation(ACraftingStation* Station)
{
	if (!Station) return;
	ASonheimPlayer* P = m_Player ? m_Player : Cast<ASonheimPlayer>(GetPawn());
	Station->ServerStartAssist(P);
}

void ASonheimPlayerController::Server_Crafting_AssistStop_Implementation(ACraftingStation* Station)
{
	if (!Station) return;
	ASonheimPlayer* P = m_Player ? m_Player : Cast<ASonheimPlayer>(GetPawn());
	Station->ServerStopAssist(P);
}


void ASonheimPlayerController::Server_Crafting_CancelUnfinished_Implementation(class ACraftingStation* Station)
{
	if (IsValid(Station))
	{
		Station->ServerCancelUnfinished(Cast<ASonheimPlayer>(GetPawn()));
	}
}

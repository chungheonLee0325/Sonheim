// Fill out your copyright notice in the Description page of Project Settings.


#include "SonheimPlayerController.h"
#include "InputMappingContext.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "SonheimPlayer.h"
#include "SonheimPlayerState.h"
#include "Sonheim/AreaObject/Attribute/LevelComponent.h"
#include "Sonheim/AreaObject/Attribute/StaminaComponent.h"
#include "Sonheim/UI/Widget/Player/PlayerStatusWidget.h"
#include "Sonheim/UI/Widget/Player/Inventory/InventoryWidget.h"
#include "Sonheim/Utilities/LogMacro.h"
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

	static ConstructorHelpers::FObjectFinder<UInputAction> tempMenu(
		TEXT("/Script/EnhancedInput.InputAction'/Game/_BluePrint/AreaObject/Player/Input/Actions/IA_Menu.IA_Menu'"));
	if (tempMenu.Succeeded())
	{
		MenuAction = tempMenu.Object;
	}

	// static ConstructorHelpers::FObjectFinder<UInputAction> tempRestart(
	// TEXT("/Script/EnhancedInput.InputAction'/Game/_Input/IA_Restart.IA_Restart'"));
	// if (tempRestart.Succeeded())
	// {
	// 	RestartAction = tempRestart.Object;
	// }

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
			"/Script/UMGEditor.WidgetBlueprint'/Game/_BluePrint/Widget/Player/BP_Inventory.BP_Inventory_C'"));
	if (inventoryWidgetClassFinder.Succeeded())
	{
		InventoryWidgetClass = inventoryWidgetClassFinder.Class;
	}

	//ConstructorHelpers::FClassFinder<UUserWidget> missionFailWidget(
	//	TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/_BluePrints/Widget/WB_KazanHasFallen.WB_KazanHasFallen_C'"));
	//if (missionFailWidget.Succeeded())
	//{
	//	MissionFailClass = missionFailWidget.Class;
	//}
}

void ASonheimPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(
		this->GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}
	m_Player = Cast<ASonheimPlayer>(GetPawn());
	m_PlayerState = GetPlayerState<ASonheimPlayerState>();

	// UI 초기화
	//InitializeHUD();
}

UPlayerStatusWidget* ASonheimPlayerController::GetPlayerStatusWidget() const
{
	return StatusWidget;
}

void ASonheimPlayerController::InitializeHUD()
{
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
			StatusWidget->UpdateExp(m_Player->m_LevelComponent->GetCurrentExp(),
			                        m_Player->m_LevelComponent->GetExpToNextLevel(), 0);
		}
		StatusWidget->SetEnableCrossHair(false);
	}

	//FailWidget = CreateWidget<UUserWidget>(this, MissionFailClass);
}

void ASonheimPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
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

		// Menu
		EnhancedInputComponent->BindAction(MenuAction, ETriggerEvent::Started, this,
		                                   &ASonheimPlayerController::On_Menu_Pressed);
		EnhancedInputComponent->BindAction(MenuAction, ETriggerEvent::Completed, this,
		                                   &ASonheimPlayerController::On_Menu_Released);

		// Restart
		EnhancedInputComponent->BindAction(RestartAction, ETriggerEvent::Started, this,
		                                   &ASonheimPlayerController::On_Menu_Pressed);
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
	m_Player->Move(Value.Get<FVector2D>());
}

void ASonheimPlayerController::OnLook(const FInputActionValue& Value)
{
	m_Player->Look(Value.Get<FVector2D>());
}

void ASonheimPlayerController::On_Mouse_Left_Triggered(const FInputActionValue& InputActionValue)
{
	m_Player->LeftMouse_Triggered();
}

void ASonheimPlayerController::On_Mouse_Right_Pressed(const FInputActionValue& InputActionValue)
{
	m_Player->RightMouse_Pressed();
	GetPlayerStatusWidget()->SetEnableCrossHair(true);
}

void ASonheimPlayerController::On_Mouse_Right_Triggered(const FInputActionValue& InputActionValue)
{
	m_Player->RightMouse_Triggered();
}

void ASonheimPlayerController::On_Sprint_Pressed(const FInputActionValue& InputActionValue)
{
	m_Player->Sprint_Pressed();
}

void ASonheimPlayerController::On_Sprint_Triggered(const FInputActionValue& InputActionValue)
{
	m_Player->Sprint_Triggered();
}

void ASonheimPlayerController::On_Sprint_Released(const FInputActionValue& InputActionValue)
{
	m_Player->Sprint_Released();
}

void ASonheimPlayerController::On_Mouse_Right_Released(const FInputActionValue& InputActionValue)
{
	m_Player->RightMouse_Released();
	GetPlayerStatusWidget()->SetEnableCrossHair(false);
}

void ASonheimPlayerController::On_Dodge_Pressed(const FInputActionValue& InputActionValue)
{
	m_Player->Dodge_Pressed();
}

void ASonheimPlayerController::On_Jump_Pressed(const FInputActionValue& InputActionValue)
{
	m_Player->Jump_Pressed();
}

void ASonheimPlayerController::On_Jump_Released(const FInputActionValue& InputActionValue)
{
	m_Player->Jump_Released();
}

void ASonheimPlayerController::On_Reload_Pressed(const FInputActionValue& Value)
{
	m_Player->Reload_Pressed();
}

void ASonheimPlayerController::On_WeaponSwitch_Triggered(const FInputActionValue& Value)
{
	m_Player->WeaponSwitch_Triggered();
}

void ASonheimPlayerController::On_Menu_Pressed(const FInputActionValue& Value)
{
	if (!IsMenuActivate)
	{
		IsMenuActivate = true;

		m_Player->Menu_Pressed();
		InventoryWidget = CreateWidget<UInventoryWidget>(this, InventoryWidgetClass);
		InventoryWidget->AddToViewport(0);
		m_PlayerState->m_InventoryComponent->OnInventoryChanged.AddDynamic(InventoryWidget,
		                                                                   &UInventoryWidget::UpdateInventoryFromData);
		InventoryWidget->UpdateInventoryFromData(m_PlayerState->m_InventoryComponent->GetInventory());
		m_PlayerState->m_InventoryComponent->OnEquipmentChanged.AddDynamic(InventoryWidget,
		                                                                   &UInventoryWidget::UpdateEquipmentFromData);
		auto mapData = m_PlayerState->m_InventoryComponent->GetEquippedItems();
		for (auto pair : mapData)
		{
			InventoryWidget->UpdateEquipmentFromData(pair.Key, pair.Value);
		}
	}
	else
	{
		IsMenuActivate = false;

		m_PlayerState->m_InventoryComponent->OnInventoryChanged.RemoveDynamic(
			InventoryWidget, &UInventoryWidget::UpdateInventoryFromData);
		m_PlayerState->m_InventoryComponent->OnEquipmentChanged.RemoveDynamic(InventoryWidget,
		                                                                      &UInventoryWidget::
		                                                                      UpdateEquipmentFromData);
		InventoryWidget->RemoveFromViewport();
		InventoryWidget = nullptr;
	}
}

void ASonheimPlayerController::On_Menu_Released(const FInputActionValue& Value)
{
}

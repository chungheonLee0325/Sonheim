// Fill out your copyright notice in the Description page of Project Settings.


#include "SonheimPlayerController.h"
#include "InputMappingContext.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "SonheimPlayer.h"
#include "Sonheim/AreaObject/Attribute/LevelComponent.h"
#include "Sonheim/AreaObject/Attribute/StaminaComponent.h"
#include "Sonheim/UI/Widget/Player/PlayerStatusWidget.h"
#include "Sonheim/Utilities/LogMacro.h"

ASonheimPlayerController::ASonheimPlayerController()
{
	// Enhanced Input Setting
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> tempInputMapping(
		TEXT("/Script/EnhancedInput.InputMappingContext'/Game/_BluePrint/AreaObject/Player/Input/IMC_Default.IMC_Default'"));
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
		TEXT("/Script/EnhancedInput.InputAction'/Game/_BluePrint/AreaObject/Player/Input/Actions/IA_LeftButton.IA_LeftButton'"));
	if (tempLeftMouseAction.Succeeded())
	{
		LeftMouseAction = tempLeftMouseAction.Object;
	}
	static ConstructorHelpers::FObjectFinder<UInputAction> tempRightMouseAction(
		TEXT("/Script/EnhancedInput.InputAction'/Game/_BluePrint/AreaObject/Player/Input/Actions/IA_RightButton.IA_RightButton'"));
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

	static ConstructorHelpers::FObjectFinder<UInputAction> tempEvadeAction(
		TEXT("/Script/EnhancedInput.InputAction'/Game/_BluePrint/AreaObject/Player/Input/Actions/IA_Dodge.IA_Dodge'"));
	if (tempEvadeAction.Succeeded())
	{
		EvadeAction = tempEvadeAction.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> tempRestart(
		TEXT("/Script/EnhancedInput.InputAction'/Game/_Input/IA_Restart.IA_Restart'"));
	if (tempRestart.Succeeded())
	{
		RestartAction = tempRestart.Object;
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

	ConstructorHelpers::FClassFinder<UUserWidget> missionFailWidget(
		TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/_BluePrints/Widget/WB_KazanHasFallen.WB_KazanHasFallen_C'"));
	if (missionFailWidget.Succeeded())
	{
		MissionFailClass = missionFailWidget.Class;
	}
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
			StatusWidget->UpdateLevel(m_Player->m_LevelComponent->GetCurrentLevel(),m_Player->m_LevelComponent->GetCurrentLevel(),true);
			m_Player->m_LevelComponent->OnExperienceChanged.AddDynamic(StatusWidget, &UPlayerStatusWidget::UpdateExp);
			StatusWidget->UpdateExp(m_Player->m_LevelComponent->GetCurrentExp(),m_Player->m_LevelComponent->GetExpToNextLevel(),0);
		}
		
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
		EnhancedInputComponent->BindAction(LeftMouseAction, ETriggerEvent::Started, this,
		                                   &ASonheimPlayerController::On_Mouse_Left_Pressed);
		EnhancedInputComponent->BindAction(RightMouseAction, ETriggerEvent::Started, this,
		                                   &ASonheimPlayerController::On_Mouse_Right_Pressed);

		// Jump
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this,
										   &ASonheimPlayerController::On_Jump_Pressed);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this,
										   &ASonheimPlayerController::On_Jump_Released);
		
		// Evade
		EnhancedInputComponent->BindAction(EvadeAction, ETriggerEvent::Started, this,
		                                   &ASonheimPlayerController::On_Dodge_Pressed);

		// Restart
		EnhancedInputComponent->BindAction(RestartAction, ETriggerEvent::Started, this,
		                                   &ASonheimPlayerController::On_Restart_Pressed);
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

void ASonheimPlayerController::On_Mouse_Left_Pressed(const FInputActionValue& InputActionValue)
{
	m_Player->LeftMouse_Pressed();
}

void ASonheimPlayerController::On_Mouse_Right_Pressed(const FInputActionValue& InputActionValue)
{
	m_Player->RightMouse_Pressed();
	//LOG_PRINT(TEXT("Press"));
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

void ASonheimPlayerController::On_Restart_Pressed(const FInputActionValue& Value)
{
	m_Player->Restart_Pressed();
}


//void ASonheimPlayerController::AddCurrency(ECurrencyType CurrencyType, int CurrencyValue)
//{
//	if (CurrencyValue < 0) return;
//	CurrencyValues[CurrencyType] += CurrencyValue;
//	this->OnCurrencyChange.Broadcast(CurrencyType, CurrencyValues[CurrencyType], CurrencyValue);
//}
//
//void ASonheimPlayerController::RemoveCurrency(ECurrencyType CurrencyType, int CurrencyValue)
//{
//	if (CurrencyValue < 0) return;
//	// 스태미나 감소
//	float oldCurrency = CurrencyValues[CurrencyType];
//	CurrencyValues[CurrencyType] = FMath::Max(CurrencyValues[CurrencyType] - CurrencyValue, 0);
//	this->OnCurrencyChange.Broadcast(CurrencyType, CurrencyValues[CurrencyType],
//	                                 -(oldCurrency - CurrencyValues[CurrencyType]));
//}
//
//int ASonheimPlayerController::GetCurrencyValue(ECurrencyType CurrencyType)
//{
//	return CurrencyValues[CurrencyType];
//}

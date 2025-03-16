// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SonheimPlayer.h"
#include "GameFramework/PlayerController.h"
#include "SonheimPlayerController.generated.h"

struct FInputActionValue;
//DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCurrencyChangeDelegate, ECurrencyType, CurrencyType, int,
//                                               CurrencyValue, int, Delta);

/**
 * 
 */
UCLASS()
class SONHEIM_API ASonheimPlayerController : public APlayerController
{
	GENERATED_BODY()
	virtual void SetupInputComponent() override;

public:
	ASonheimPlayerController();

	virtual void BeginPlay() override;

	// UI 초기화 및 바인딩 - Player에서 Component 모두 초기화 후 호출
	void InitializeHUD();

	UPROPERTY()
	class UUserWidget* FailWidget;

	class UPlayerStatusWidget* GetPlayerStatusWidget() const;

private:
	// Input Action
	/** Called for movement input */
	void OnMove(const FInputActionValue& Value);
	/** Called for looking input */
	void OnLook(const FInputActionValue& Value);
	/** Called for attack input */
	void On_Mouse_Left_Pressed(const FInputActionValue& InputActionValue);
	void On_Mouse_Right_Pressed(const FInputActionValue& InputActionValue);

	/** Called for Dodge input */
	void On_Dodge_Pressed(const FInputActionValue& InputActionValue);
	/** Called for run input */
	void On_Jump_Pressed(const FInputActionValue& InputActionValue);
	void On_Jump_Released(const FInputActionValue& InputActionValue);
	/** Called for HPRecover input */
	void On_Restart_Pressed(const FInputActionValue& Value);

	// Owner
	UPROPERTY(VisibleAnywhere)
	ASonheimPlayer* m_Player;

	// 재화 관련 데이터
	// TMap<ECurrencyType, int> CurrencyValues;

	// UI 관련
	UPROPERTY()
	class UPlayerStatusWidget* StatusWidget;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UPlayerStatusWidget> StatusWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UUserWidget> MissionFailClass;

	// Input Setting
	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	/** Attack_C Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LeftMouseAction;

	/** Attack_S Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* RightMouseAction;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Evade Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* EvadeAction;

	/** Restart Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* RestartAction;
};

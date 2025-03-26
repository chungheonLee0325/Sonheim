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

	virtual void OnPossess(APawn* InPawn) override;
	// UI 초기화 및 바인딩 - Player에서 Component 모두 초기화 후 호출
	UFUNCTION(Client, Reliable)
	void InitializeHUD(ASonheimPlayer* NewPlayer);

	UPROPERTY()
	class UUserWidget* FailWidget;

	class UPlayerStatusWidget* GetPlayerStatusWidget() const;

	bool GetIsMenuActivate() { return IsMenuActivate; }

private:
	// Input Action
	/** Called for movement input */
	void OnMove(const FInputActionValue& Value);
	/** Called for looking input */
	void OnLook(const FInputActionValue& Value);
	/** Called for mouse input */
	void On_Mouse_Left_Pressed(const FInputActionValue& InputActionValue);
	void On_Mouse_Left_Triggered(const FInputActionValue& InputActionValue);
	void On_Mouse_Left_Released(const FInputActionValue& InputActionValue);
	void On_Mouse_Right_Pressed(const FInputActionValue& InputActionValue);
	void On_Mouse_Right_Triggered(const FInputActionValue& InputActionValue);
	void On_Mouse_Right_Released(const FInputActionValue& InputActionValue);

	/** Called for Dodge input */
	void On_Dodge_Pressed(const FInputActionValue& InputActionValue);
	/** Called for Dodge input */
	void On_Sprint_Pressed(const FInputActionValue& InputActionValue);
	void On_Sprint_Triggered(const FInputActionValue& InputActionValue);
	void On_Sprint_Released(const FInputActionValue& InputActionValue);
	/** Called for Jump input */
	void On_Jump_Pressed(const FInputActionValue& InputActionValue);
	void On_Jump_Released(const FInputActionValue& InputActionValue);
	/** Called for Reload input */
	void On_Reload_Pressed(const FInputActionValue& Value);
	/** Called for Weapon Switch input */
	void On_WeaponSwitch_Triggered(const FInputActionValue& Value);
	/** Called for PartnerSkill input */
	void On_PartnerSkill_Pressed(const FInputActionValue& InputActionValue);
	void On_PartnerSkill_Triggered(const FInputActionValue& InputActionValue);
	void On_PartnerSkill_Released(const FInputActionValue& InputActionValue);
	/** Called for Menu input */
	void On_Menu_Pressed(const FInputActionValue& Value);
	void On_Menu_Released(const FInputActionValue& Value);

	// Owner
	UPROPERTY(VisibleAnywhere)
	ASonheimPlayer* m_Player;

	//UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_PlayerState)
	UPROPERTY(VisibleAnywhere)
	ASonheimPlayerState* m_PlayerState;

	virtual void OnRep_PlayerState() override;
	// 재화 관련 데이터
	// TMap<ECurrencyType, int> CurrencyValues;

	// UI 관련
	UPROPERTY()
	class UPlayerStatusWidget* StatusWidget;
	UPROPERTY()
	class UInventoryWidget* InventoryWidget;
	UPROPERTY()
	class UPlayerStatWidget* PlayerStatWidget;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UPlayerStatusWidget> StatusWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UInventoryWidget> InventoryWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UPlayerStatWidget> PlayerStatWidgetClass;

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

	/** Sprint Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* SprintAction;

	/** Evade Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* EvadeAction;

	/** Attack_S Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ReloadAction;

	/** Switch Weapon Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* SwitchWeaponAction;

	/** PartnerSkill Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* PartnerSkillAction;

	/** Menu Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MenuAction;

	/** Menu Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* RestartAction;

	bool IsMenuActivate = false;

public:
	// UFUNCTION(Server, Reliable)
	// void ServerRPC_ChangeState(UBaseAiFSM* FSM, EAiStateType StateType);
};

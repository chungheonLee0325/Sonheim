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
	void InitializeWithPlayer(ASonheimPlayer* NewPlayer);
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
	/** Called for Summon Pal input */
	void On_SummonPal_Pressed(const FInputActionValue& Value);
	/** Called for Switch Pal input */
	void On_SwitchPalSlot_Triggered(const FInputActionValue& Value);
	/** Called for ThrowPalSphere input */
	void On_ThrowPalSphere_Pressed(const FInputActionValue& InputActionValue);
	void On_ThrowPalSphere_Triggered(const FInputActionValue& InputActionValue);
	void On_ThrowPalSphere_Released(const FInputActionValue& InputActionValue);
	/** Called for Menu input */
	void On_Menu_Pressed(const FInputActionValue& Value);
	void On_Menu_Released(const FInputActionValue& Value);

	/** Called for Glider input */
	void On_Glider_Pressed(const FInputActionValue& InputActionValue);
	void On_Glider_Released(const FInputActionValue& InputActionValue);

	/** Called for Glider input */
	void On_FKey_Pressed(const FInputActionValue& InputActionValue);
	void On_FKey_Released(const FInputActionValue& InputActionValue);
	
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

	/** SummonPal Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* SummonPalSlotAction;

	/** SwitchPal Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* SwitchPalAction;

	/** ThrowPalSphere Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ThrowPalSphereAction;
	
	/** Menu Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MenuAction;

	/** Menu Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* RestartAction;

	/** Glider Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* GliderAction;

	/** FKey Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* FKeyAction;
	
	bool IsMenuActivate = false;
	bool IsContainerActivate = false;

	// 점프 횟수 추적을 위한 변수
	float LastJumpTime = 0.0f;
	int JumpCount = 0;
	const float DoubleJumpTimeThreshold = 0.5f; // 더블 점프 인식 시간

public:
	// === 상자 UI 관리 ===
	UFUNCTION(Client, Reliable)
	void Client_OpenContainerUI(ABaseContainer* Container);
	UFUNCTION(Client, Reliable)
	void Client_CloseContainerUI();
	
	// ===== Container 중계 함수들 =====
	// 기본 상자 작업
	UFUNCTION(Server, Reliable)
	void Server_ContainerOperation(
		class ABaseContainer* Container, 
		EContainerOperation Operation, 
		int32 Param1 = 0, 
		int32 Param2 = 0
	);
    
	// 플레이어-컨테이너 간 전송
	UFUNCTION(Server, Reliable)
	void Server_PlayerContainerTransfer(
		class ABaseContainer* Container,
		bool bFromContainerToPlayer,
		int32 ItemID,
		int32 Count,
		int32 SlotIndex = -1
	);

	// === Crafting UI 관리 ===
	UFUNCTION(Client, Reliable)
	void Client_OpenCraftingUI(class ACraftingStation* Station);
	UFUNCTION(Client, Reliable)
	void Client_CloseCraftingUI();

	// ==== Crafting 중계 함수 ====
	UFUNCTION(Server, Reliable)
	void ServerStartWork(class ACraftingStation* Station, FName Row, int32 Quantity);
	UFUNCTION(Server, Reliable)
	void Server_Crafting_RequestCollectAll(class ACraftingStation* Station);
	UFUNCTION(Server, Reliable)
	void Server_Crafting_ReleaseUI(class ACraftingStation* Station);
	UFUNCTION(Server, Reliable)
	void Server_Crafting_AssistStart(class ACraftingStation* Station);
	UFUNCTION(Server, Reliable)
	void Server_Crafting_AssistStop(class ACraftingStation* Station);
	UFUNCTION(Server, Reliable)
	void Server_Crafting_CancelUnfinished(class ACraftingStation* Station);

private:
	// 상자 UI 위젯
	UPROPERTY()
	class UContainerInteractionWidget* ContainerInteractionWidget;
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UContainerInteractionWidget> ContainerInteractionWidgetClass;

	// Crafting UI
	UPROPERTY()
	class UCraftingWidget* CraftingWidget;
	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<class UCraftingWidget> CraftingWidgetClass;

	// 검증 헬퍼 함수들
	bool ValidateContainerAccess(class ABaseContainer* Container) const;
	bool ValidateItemOperation(int32 ItemID, int32 Count) const;
	bool ValidateDistance(AActor* Target, float MaxDistance = 500.0f) const;
};

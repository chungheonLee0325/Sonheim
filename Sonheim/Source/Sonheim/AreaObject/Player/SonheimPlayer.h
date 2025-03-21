// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sonheim/Animation/Player/PlayerAniminstance.h"
#include "Sonheim/AreaObject/Base/AreaObject.h"
#include "SonheimPlayer.generated.h"

class ASonheimPlayerState;
class ULockOnComponent;
class ASonheimPlayerController;
class ABaseItem;
class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

// 플레이어의 상태를 정의하는 열거형
UENUM(BlueprintType)
enum class EPlayerState : uint8
{
	// 일반 상태
	NORMAL,
	// 회전만 가능한 상태 (공격 Casting 중 1tick)
	ONLY_ROTATE,
	// Action 상태
	ACTION,
	// Action 사용 가능한 상태
	CANACTION,
	// 사망 상태
	DIE,
};

// 액션 제한을 관리하는 구조체
USTRUCT(BlueprintType)
struct FActionRestrictions
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCanLook = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCanMove = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCanRotate = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCanOnlyRotate = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCanAction = true;
};

UCLASS()
class SONHEIM_API ASonheimPlayer : public AAreaObject
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ASonheimPlayer();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int checkpoint = 0;

	void SetPlayerState(EPlayerState NewState);
	void SetPlayerNormalState() { SetPlayerState(EPlayerState::NORMAL); }
	void SetComboState(bool bCanCombo, int SkillID);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void OnDie() override;

	virtual void OnRevival() override;

	virtual float HandleAttackDamageCalculation(float Damage) override;
	virtual float HandleDefenceDamageCalculation(float Damage) override;

public:
	// Movement
	/** Called for movement input */
	void Move(FVector2D MovementVector);

	// Camera Rotation
	/** Called for looking input */
	void Look(FVector2D LookAxisVector);

	/** Called for attack input */
	void LeftMouse_Triggered();
	void RightMouse_Pressed();
	void RightMouse_Triggered();
	void RightMouse_Released();
	void Reload_Pressed();

	/** Called for evade input */
	void Dodge_Pressed();

	/** Called for sprint input */
	void Sprint_Pressed();
	void Sprint_Triggered();
	void Sprint_Released();

	/** Called for run input */
	void Jump_Pressed();
	void Jump_Released();

	/** Called for WeaponSwitch input */
	void WeaponSwitch_Triggered();

	/** Called for Menu input */
	void Menu_Pressed();

	/** Called for Restart input */
	void Restart_Pressed();

	UFUNCTION(BlueprintCallable, Category = "Checkpoint")
	void SaveCheckpoint(FVector Location, FRotator Rotation);

	UFUNCTION(BlueprintCallable, Category = "Checkpoint")
	void RespawnAtCheckpoint();

	void Reward(int ItemID, int ItemValue) const;

	ASonheimPlayerState* GetSPlayerState() const {return S_PlayerState;};

	UFUNCTION(BlueprintCallable)
	void UpdateEquipWeapon(EEquipmentSlotType WeaponSlot, FInventoryItem Item);

	UFUNCTION(BlueprintCallable)
	void UpdateSelectedWeapon(EEquipmentSlotType WeaponSlot, int ItemID);

	UFUNCTION(BlueprintCallable)
	void StatChanged(EAreaObjectStatType StatType, float StatValue);
	//// 장비 시각화 관련 함수 추가
	//UFUNCTION(BlueprintCallable, Category = "Equipment")
	//void EquipVisualItem(EEquipmentSlotType SlotType, int ItemID);
	//
	//UFUNCTION(BlueprintCallable, Category = "Equipment")
	//void UnequipVisualItem(EEquipmentSlotType SlotType);
	//
	//// 장비 관련 컴포넌트 추가
	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Equipment, meta = (AllowPrivateAccess = "true"))
	//USkeletalMeshComponent* HeadMesh;
	//
	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Equipment, meta = (AllowPrivateAccess = "true"))
	//USkeletalMeshComponent* BodyMesh;

	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Equipment, meta = (AllowPrivateAccess = "true"))
	//USkeletalMeshComponent* SubWeaponMesh;

	// 현재 무기 타입
	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	//EWeaponType CurrentWeaponType;

	// 무기 타입별 스킬
	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	//TMap<EWeaponType, TArray<int32>> WeaponTypeSkills;

	// 무기 타입 설정
	//UFUNCTION(BlueprintCallable, Category = "Equipment")
	//void SetWeaponType(EWeaponType NewWeaponType);

	// // 무기 스킬 추가
	// UFUNCTION(BlueprintCallable, Category = "Equipment")
	// void AddWeaponSkill(int32 SkillID);
	//
	// // 무기 스킬 클리어
	// UFUNCTION(BlueprintCallable, Category = "Equipment")
	// void ClearWeaponSkills();

	// 현재 무기에 따른 공격 처리
	UFUNCTION(BlueprintCallable, Category = "Combat")
	UBaseSkill* GetWeaponAttack() { return WeaponSkillMap[SelectedWeaponSlot]; };

	UPROPERTY()
	TMap<EEquipmentSlotType, UBaseSkill*> WeaponSkillMap;

	UPROPERTY()
	EEquipmentSlotType SelectedWeaponSlot = EEquipmentSlotType::Weapon1;

	//// 현재 도구에 따른 상호작용 처리
	//UFUNCTION(BlueprintCallable, Category = "Interaction")
	//bool TryToolInteraction(AActor* TargetActor);

	//// 특수 능력 관리자
	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	//UAbilityManagerComponent* AbilityManager;

	//// 특수 능력 관리자 반환
	//UFUNCTION(BlueprintCallable, Category = "Abilities")
	//UAbilityManagerComponent* GetAbilityManager() const { return AbilityManager; }

private:
	// Weapon Setting
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* WeaponComponent;

	// Camera Setting
	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;
	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	// 카메라 민감도 설정
	UPROPERTY(EditAnywhere, Category = "Camera|Controls")
	float LookSensitivityX = 0.25f;
	UPROPERTY(EditAnywhere, Category = "Camera|Controls")
	float LookSensitivityY = 0.15f;
	// 수직 각도 제한
	UPROPERTY(EditAnywhere, Category = "Camera|Limits")
	float MinPitchAngle = -10.0f; // 위쪽 제한
	UPROPERTY(EditAnywhere, Category = "Camera|Limits")
	float MaxPitchAngle = 40.0f; // 아래쪽 제한
	// 현재 피치 각도를 추적
	float CurrentPitchAngle = 0.0f;

	UPROPERTY()
	UPlayerAnimInstance* S_PlayerAnimInstance;
	UPROPERTY()
	ASonheimPlayerController* S_PlayerController;
	UPROPERTY()
	ASonheimPlayerState* S_PlayerState;

	// 플레이어 상태 관리
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	EPlayerState CurrentPlayerState;

	UPROPERTY(EditDefaultsOnly, Category = "State")
	TMap<EPlayerState, FActionRestrictions> StateRestrictions;

	void InitializeStateRestrictions();
	bool CanPerformAction(EPlayerState State, FString ActionName);

	// Data
	const float MAX_WALK_SPEED = 500.f;

	bool CanCombo;
	int NextComboSkillID = 0;

	bool IsRotateCameraWithSpeed;
	FRotator RotateCameraTarget;
	float CameraInterpSpeed = 10.f;
	FTimerHandle RotateCameraTimerHandle;

	bool IsZoomCameraWithSpeed;
	float TargetFieldOfView;
	float ZoomInterpSpeed;
	FTimerHandle ZoomCameraTimerHandle;

	// 마지막 체크포인트 위치
	UPROPERTY(VisibleAnywhere, Category = "Checkpoint")
	FVector LastCheckpointLocation = FVector::ZeroVector;

	// 마지막 체크포인트 회전
	UPROPERTY(VisibleAnywhere, Category = "Checkpoint")
	FRotator LastCheckpointRotation = FRotator::ZeroRotator;

	int NoItemAttackID = 10;
	int AttackID = 10;

	FTimerHandle LockOnCameraTimerHandle;

	float NormalCameraBoomAramLength = 180.f;
	float RClickCameraBoomAramLength = 90.f;

	UPROPERTY()
	UBaseSkill* CommonAttack = nullptr;


	// ToDO : 이관 예정!!!!
	float m_Attack = 10.f;
	float m_Defence = 10.f;
	
};

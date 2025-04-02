// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sonheim/Animation/Player/PlayerAniminstance.h"
#include "Sonheim/AreaObject/Base/AreaObject.h"
#include "SonheimPlayer.generated.h"

class ABaseMonster;
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
	// 글라이딩 상태
	GLIDING,
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

	virtual void PossessedBy(AController* NewController) override;


	virtual void Server_OnDie_Implementation() override;
	virtual void Client_OnDie_Implementation() override;

	virtual void OnRevival() override;

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	virtual float HandleAttackDamageCalculation(float Damage) override;
	virtual float HandleDefenceDamageCalculation(float Damage) override;
public:
	// Movement
	/** Called for movement input */
	void Move(FVector2D MovementVector);

	// Camera Rotation
	/** Called for looking input */
	void Look(FVector2D LookAxisVector);

	// 마우스 왼쪽 입력 처리
	void LeftMouse_Pressed();
	void LeftMouse_Triggered();
	void LeftMouse_Released();
	
	UFUNCTION(Server, Reliable)
	void Server_LeftMouse_Pressed();
	UFUNCTION(NetMulticast, Reliable)
	void MultiCast_LeftMouse_Pressed();
	UFUNCTION(Server, Reliable)
	void Server_LeftMouse_Triggered();
	UFUNCTION(NetMulticast, Reliable)
	void MultiCast_LeftMouse_Triggered();
	UFUNCTION(Server, Reliable)
	void Server_LeftMouse_Released();
	UFUNCTION(NetMulticast, Reliable)
	void MultiCast_LeftMouse_Released();
	
	// 마우스 오른쪽 입력 처리
	void RightMouse_Pressed();
	void RightMouse_Triggered();
	void RightMouse_Released();
	
	UFUNCTION(Server, Reliable)
	void Server_RightMouse_Pressed();
	UFUNCTION(NetMulticast, Reliable)
	void MultiCast_RightMouse_Pressed();
	UFUNCTION(Server, Reliable)
	void Server_RightMouse_Triggered();
	UFUNCTION(NetMulticast, Reliable)
	void MultiCast_RightMouse_Triggered();
	UFUNCTION(Server, Reliable)
	void Server_RightMouse_Released();
	UFUNCTION(NetMulticast, Reliable)
	void MultiCast_RightMouse_Released();
	
	// 재장전 입력 처리
	void Reload_Pressed();
	
	UFUNCTION(Server, Reliable)
	void Server_Reload_Pressed();
	UFUNCTION(NetMulticast, Reliable)
	void MultiCast_Reload_Pressed();
	
	// 회피 입력 처리
	void Dodge_Pressed();
	
	UFUNCTION(Server, Reliable)
	void Server_Dodge_Pressed();
	UFUNCTION(NetMulticast, Reliable)
	void MultiCast_Dodge_Pressed();
	
	// 달리기 입력 처리
	void Sprint_Pressed();
	void Sprint_Triggered();
	void Sprint_Released();
	
	UFUNCTION(Server, Reliable)
	void Server_Sprint_Pressed();
	UFUNCTION(NetMulticast, Reliable)
	void MultiCast_Sprint_Pressed();
	UFUNCTION(Server, Reliable)
	void Server_Sprint_Triggered();
	UFUNCTION(NetMulticast, Reliable)
	void MultiCast_Sprint_Triggered();
	UFUNCTION(Server, Reliable)
	void Server_Sprint_Released();
	UFUNCTION(NetMulticast, Reliable)
	void MultiCast_Sprint_Released();
	
	// 점프 입력 처리
	void Jump_Pressed();
	void Jump_Released();
	
	// 무기 전환 입력 처리
	void WeaponSwitch_Triggered();
	
	// 파트너 스킬 입력 처리
	void PartnerSkill_Pressed();
	void PartnerSkill_Triggered();
	void PartnerSkill_Released();
	
	UFUNCTION(Server, Reliable)
	void Server_PartnerSkill_Pressed();
	UFUNCTION(NetMulticast, Reliable)
	void MultiCast_PartnerSkill_Pressed();
	UFUNCTION(Server, Reliable)
	void Server_PartnerSkill_Triggered();
	UFUNCTION(NetMulticast, Reliable)
	void MultiCast_PartnerSkill_Triggered();
	UFUNCTION(Server, Reliable)
	void Server_PartnerSkill_Released();
	UFUNCTION(NetMulticast, Reliable)
	void MultiCast_PartnerSkill_Released();
	
	// 팔 소환 입력 처리
	void SummonPal_Pressed();
	
	UFUNCTION(Server, Reliable)
	void Server_SummonPal_Pressed();
	UFUNCTION(NetMulticast, Reliable)
	void MultiCast_SummonPal_Pressed();
	
	// 팔 슬롯 전환 입력 처리
	void SwitchPalSlot_Triggered(int Index);

	UFUNCTION(Server, Reliable)
	void Server_SwitchPalSlot_Triggered(int Index);
	UFUNCTION(NetMulticast, Reliable)
	void MultiCast_SwitchPalSlot_Triggered(int Index);
	
	// 메뉴 입력 처리
	void Menu_Pressed();
	
	// 팔 구체 던지기 입력 처리
	void ThrowPalSphere_Pressed();
	void ThrowPalSphere_Triggered();
	void ThrowPalSphere_Released();
	
	UFUNCTION(Server, Reliable)
	void Server_ThrowPalSphere_Pressed();
	UFUNCTION(NetMulticast, Reliable)
	void MultiCast_ThrowPalSphere_Pressed();
	UFUNCTION(Server, Reliable)
	void Server_ThrowPalSphere_Triggered();
	UFUNCTION(NetMulticast, Reliable)
	void MultiCast_ThrowPalSphere_Triggered();
	UFUNCTION(Server, Reliable)
	void Server_ThrowPalSphere_Released();
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_ThrowPalSphere_Released();
	
	// 재시작 입력 처리
	void Restart_Pressed();

	UFUNCTION(BlueprintCallable, Category = "Checkpoint")
	void SaveCheckpoint(FVector Location, FRotator Rotation);

	UFUNCTION(BlueprintCallable, Category = "Checkpoint")
	void RespawnAtCheckpoint();

	void Reward(int ItemID, int ItemValue) const;

	UFUNCTION(BlueprintCallable)
	ASonheimPlayerState* GetSPlayerState() const {return S_PlayerState;};

	// 같은 무기 중복 사용으로 인한 오류 방지.. 전부 최신화
	void RefreshWeaponSkillToSkillInstanceMap();
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

	UFUNCTION(BlueprintCallable)
	void RegisterOwnPal(ABaseMonster* Pal);
	UFUNCTION(Server, Reliable)
	void Server_RegisterOwnPal(ABaseMonster* Pal);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_RegisterOwnPal(ABaseMonster* Pal);

	// Glider
	UFUNCTION(BlueprintCallable, Category = "Movement|Glider")
	void ActivateGlider();
	
	UFUNCTION(BlueprintCallable, Category = "Movement|Glider")
	void DeactivateGlider();
	
	UFUNCTION(Server, Reliable)
	void Server_ActivateGlider();
	
	UFUNCTION(NetMulticast, Reliable)
	void MultiCast_ActivateGlider();
	
	UFUNCTION(Server, Reliable)
	void Server_DeactivateGlider();
	
	UFUNCTION(NetMulticast, Reliable)
	void MultiCast_DeactivateGlider();
	
	// 글라이더 상태 확인
	UFUNCTION(BlueprintPure, Category = "Movement|Glider")
	bool IsGliding() const { return bIsGliding; }

private:
	// Weapon Setting
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Equipment, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* WeaponComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Equipment, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* PalSphereComponent;

	// ToDo : Skill 로 이관 예정.. 타이밍 등 적용
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Montage, meta = (AllowPrivateAccess = "true"))
	UAnimMontage* SummonPalMontage;

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
	virtual void OnRep_PlayerState() override;
	virtual void OnRep_Controller() override;

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

	bool bCanRecover = true;
	float m_RecoveryRate = 5.0f;
	FTimerHandle RecoveryTimerHandle;
	
	bool bUsingPartnerSkill = false;

public:
	void SetUsePartnerSkill(bool UsePartnerSkill);
	
	virtual bool CanAttack(AActor* TargetActor) override;

	UFUNCTION(BlueprintImplementableEvent)
	void RestoreStair(int ItemID, int ItemCount);

private:
	// UFUNCTION(Server, Reliable)
	// void Server_ToggleLockOn(bool IsActive);
	void UpdateSelectedPal();
	
	UPROPERTY(EditDefaultsOnly, Category = "Pals")
	int PalMaxIndex = 5;
	int CurrentPalIndex = 0;
	UPROPERTY(VisibleAnywhere, Category = "Pals")
	TMap<int, ABaseMonster*> m_OwnedPals;
	UPROPERTY(VisibleAnywhere, Category = "Pals")	
	ABaseMonster* m_SelectedPal = nullptr;
	UPROPERTY(VisibleAnywhere, Category = "Pals")
	ABaseMonster* m_SummonedPal = nullptr;

	// Glider Variable
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Glider", meta = (AllowPrivateAccess = "true"))
	bool bIsGliding = false;
	
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Glider")
	float GliderFallSpeed = 200.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Glider")
	float GliderForwardSpeed = 800.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Glider")
	UAnimMontage* GliderOpenMontage;
	
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Glider")
	UAnimMontage* GliderLoopMontage;
	
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Glider")
	UAnimMontage* GliderCloseMontage;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Glider", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* GliderMeshComponent;
	
	// Glider Helper Function
	void UpdateGliding(float DeltaTime);

	virtual void Landed(const FHitResult& Hit) override;
	
};

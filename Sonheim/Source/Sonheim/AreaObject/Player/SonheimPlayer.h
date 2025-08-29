// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SonheimPlayerState.h"
#include "Sonheim/Animation/Player/PlayerAniminstance.h"
#include "Sonheim/AreaObject/Base/AreaObject.h"
#include "Utility/InteractionComponent.h"
#include "Utility/PalCaptureComponent.h"
#include "SonheimPlayer.generated.h"

class UInventoryComponent;
class UMonsterDetectionComponent;
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
class ABaseItem;

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

	bool IsLockOn() const;
	void SetPlayerState(EPlayerState NewState);
	void SetPlayerNormalState() { SetPlayerState(EPlayerState::NORMAL); }
	void SetComboState(bool bCanCombo, int SkillID);
	USkeletalMeshComponent* GetPalSphereComponent();
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void PossessedBy(AController* NewController) override;
	void EndPlay(EEndPlayReason::Type EndPlayReason);

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void OnRep_IsDead() override;

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
	
	// 록온(우클릭 카메라 처리)
	void AdjustCameraForLockOn(bool IsLockOn);

	UFUNCTION(Server, Reliable)
	void Server_SetLockOn(bool bNew);
	
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
	void Server_Sprint_Triggered();
	
	// 점프 입력 처리
	void Jump_Pressed();
	void Jump_Released();
	
	// 파트너 스킬 입력 처리
	void PartnerSkill_Pressed();
	void PartnerSkill_Triggered();
	void PartnerSkill_Released();
	
	// 팔 소환 입력 처리
	void SummonPal_Pressed();
	
	// 팔 슬롯 전환 입력 처리
	void SwitchPalSlot_Triggered(int Index);
	
	// 메뉴 입력 처리
	void Menu_Pressed();
	
	// 팔 구체 던지기 입력 처리
	void ThrowPalSphere_Pressed();
	void ThrowPalSphere_Triggered();
	void ThrowPalSphere_Released();
	void ThrowPalSphere_Canceled();
	
	// 재시작 입력 처리
	void Restart_Pressed();

	UFUNCTION(BlueprintCallable, Category = "Checkpoint")
	void SaveCheckpoint(FVector Location, FRotator Rotation);

	UFUNCTION(BlueprintCallable, Category = "Checkpoint")
	void RespawnAtCheckpoint();

	UFUNCTION(BlueprintCallable)
	ASonheimPlayerState* GetSPlayerState() const {return S_PlayerState;};
	UFUNCTION(BlueprintCallable)
	UInventoryComponent* GetInventoryComponent() const {return S_PlayerState->m_InventoryComponent;};

	// 아이템 획득 
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void Reward(int ItemID, int ItemValue);

	// 장비 업데이트 
	UFUNCTION()
	void UpdateEquipWeapon(EEquipmentSlotType WeaponSlot, FInventoryItem Item);
    
	UFUNCTION()
	void UpdateSelectedWeapon(EEquipmentSlotType WeaponSlot, int ItemID);
    
	UFUNCTION(Server, Reliable)
	void Server_UpdateSelectedWeapon(EEquipmentSlotType WeaponSlot, int ItemID);
    
	// 스탯 변경 - 서버 권한 추가
	UFUNCTION()
	void StatChanged(EAreaObjectStatType StatType, float StatValue);

	// 무기 전환
	void WeaponSwitch_Triggered(int Index);
	
	// 같은 무기 중복 사용으로 인한 오류 방지.. 전부 최신화
	void RefreshWeaponSkillToSkillInstanceMap();

	// 현재 무기에 따른 공격 처리
	UFUNCTION(BlueprintCallable, Category = "Combat")
	UBaseSkill* GetWeaponAttack() { return WeaponSkillMap[SelectedWeaponSlot]; };

	UPROPERTY()
	TMap<EEquipmentSlotType, UBaseSkill*> WeaponSkillMap;

	UPROPERTY(Replicated) 
	EEquipmentSlotType SelectedWeaponSlot = EEquipmentSlotType::Weapon1;

	UFUNCTION(BlueprintCallable)
	void RegisterOwnPal(ABaseMonster* Pal);
	UFUNCTION(Server, Reliable)
	void Server_RegisterOwnPal(ABaseMonster* Pal);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_RegisterOwnPal(ABaseMonster* Pal);

	UFUNCTION(Client, Reliable)
	void Client_UpdatePalUI(int32 PalID, int32 Index, bool bIsAdd);

	UFUNCTION(Client, Reliable)
	void Client_UpdateSelectedPalIndex(int32 NewIndex);

	// Glider
	UFUNCTION(BlueprintCallable, Category = "Movement|Glider")
	void ActivateGlider();
	
	UFUNCTION(BlueprintCallable, Category = "Movement|Glider")
	void DeactivateGlider();
	
	// 글라이더 상태 확인
	UFUNCTION(BlueprintPure, Category = "Movement|Glider")
	bool IsGliding() const { return bIsGliding; }


	// 상호작용 키
	void Interaction_Pressed() const;
	
	void Interaction_Released() const;
	
	// 무기 메쉬 
	USkeletalMeshComponent* GetWeaponMesh() const {return WeaponComponent;};
	
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayWeaponMontage(UAnimMontage* Montage);
	
	// Skill 로 이관 예정.. 타이밍 등 적용
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Montage, meta = (AllowPrivateAccess = "true"))
	UAnimMontage* SummonPalMontage;
	
	UFUNCTION(BlueprintPure, Category = "Interaction")
	UInteractionComponent* GetInteractionComponent() const { return InteractionComponent; }

	void SetWeaponVisible(bool bNew, bool bLocalPreview = true);
	
	UFUNCTION()
	void SetWeaponMeshVisible() { SetWeaponVisible(true); }
	UFUNCTION()
	void SetWeaponMeshInvisible() { SetWeaponVisible(false); }
private:
	// Weapon Setting
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Equipment, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* WeaponComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Equipment, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* PalSphereComponent;

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
	void InitializeByPlayerState();

	UFUNCTION(Client, Reliable)
	void Client_OnItemAdded(int ItemID, int ItemCount);

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
	
	// 중복 제거를 위한 초기화 함수
	void InitPlayer();
	void BindDelegates();
	void UnbindDelegates();

	// 무기 메시 업데이트 헬퍼 함수
	void UpdateWeaponMesh(int ItemID);
	void ClearWeaponMesh();

	// 현재 장착된 무기 아이템 ID 추적
	UPROPERTY(ReplicatedUsing=OnRep_SelectedWeapon)
	int32 CurrentWeaponItemID = 0;

	// 상호작용 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	class UInteractionComponent* InteractionComponent;

	// 상호작용 대상 변경
	UFUNCTION()
	void OnInteractableChanged(AActor* NewInteractable);

public:
	void SetUsePartnerSkill(bool UsePartnerSkill);
	
	virtual bool CanAttack(AActor* TargetActor) override;

	UFUNCTION(BlueprintImplementableEvent)
	void RestoreStair(int ItemID, int ItemCount);

	UFUNCTION(BlueprintCallable)
	bool IsThrowingPalSphere() {return PalCaptureComponent->IsThrowingPalSphere();};

private:
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
	UPROPERTY(ReplicatedUsing=OnRep_IsGliding, VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Glider", meta = (AllowPrivateAccess = "true"))
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
	ABaseMonster* GetSummonedPal() const;

	UPROPERTY()
	class UPalPartnerSkillComponent* PalPartnerSkillComponent;
	UPROPERTY()
	class UPalCaptureComponent* PalCaptureComponent;

	// === Sprint ===
	UPROPERTY(ReplicatedUsing=OnRep_IsSprinting)
	bool bIsSprinting = false;

	UFUNCTION()
	void OnRep_IsSprinting();
	void ApplySprinting(bool bNew);
	UFUNCTION(Server, Reliable)
	void Server_SetSprinting(bool bNew);

	// === LockOn ===
	UPROPERTY(ReplicatedUsing=OnRep_IsLockOn)
	bool bIsLockOn = false;
	UFUNCTION()
	void OnRep_IsLockOn();


	// === Weapon Select & Visibility ===
	UFUNCTION()
	void OnRep_SelectedWeapon();
	
	UPROPERTY(ReplicatedUsing=OnRep_WeaponVisible)
	bool bWeaponVisible = true;
	
	UFUNCTION()
	void OnRep_WeaponVisible();
	
	UFUNCTION(Server, Reliable)
	void Server_SetWeaponVisible(bool bNew);

	// === Glider ===
	UFUNCTION()
	void OnRep_IsGliding();
	void ApplyGliderState(bool bNew);
	UFUNCTION(Server, Reliable)
	void Server_SetGliding(bool bNew);

	UPROPERTY(ReplicatedUsing=OnRep_CurrentWeaponType)
	EWeaponType CurrentWeaponType = EWeaponType::None;

	UFUNCTION()
	void OnRep_CurrentWeaponType();
	
	UFUNCTION()
	void HandlePalThrowingStateChanged(bool bIsPreparing);
};

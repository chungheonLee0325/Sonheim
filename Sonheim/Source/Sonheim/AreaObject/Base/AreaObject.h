// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Sonheim/AreaObject/Attribute/ConditionComponent.h"
#include "Sonheim/AreaObject/Attribute/HealthComponent.h"
#include "Sonheim/AreaObject/Utility/RotateUtilComponent.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "Sonheim/UI/Widget/FloatingDamageWidget.h"
#include "AreaObject.generated.h"

class USonheimGameInstance;
class UMoveUtilComponent;
class ASonheimGameMode;

UCLASS()
class SONHEIM_API AAreaObject : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AAreaObject();

	UPROPERTY(EditAnywhere, Category = "Debug Setting")
	bool bShowDebug = false;

	// Attribute
	UPROPERTY(BlueprintReadWrite)
	UHealthComponent* m_HealthComponent;
	UPROPERTY(BlueprintReadWrite)
	UConditionComponent* m_ConditionComponent;
	UPROPERTY(BlueprintReadWrite)
	class UStaminaComponent* m_StaminaComponent;
	UPROPERTY(BlueprintReadWrite)
	class ULevelComponent* m_LevelComponent;

	// Utility
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	URotateUtilComponent* m_RotateUtilComponent;
	UPROPERTY(BlueprintReadWrite)
	UMoveUtilComponent* m_MoveUtilComponent;

	UPROPERTY(EditDefaultsOnly, Category = "AreaObject Data Setting")
	int m_AreaObjectID;
	UPROPERTY(EditDefaultsOnly, Category = "TakeDamage")
	FVector AdjustKnockBackForce;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// 초기화 로직
	virtual void PostInitializeComponents() override;

	// Skill System
	UPROPERTY(EditAnywhere, Category = "Skill")
	TSet<int> m_OwnSkillIDSet;

	UPROPERTY(EditAnywhere, Category = "Skill")
	TMap<int, TObjectPtr<UBaseSkill>> m_SkillInstanceMap;

	UPROPERTY()
	TObjectPtr<UBaseSkill> m_CurrentSkill;

	// Death Setting
	// 죽음 후 destroy 지연 시간
	UPROPERTY(EditDefaultsOnly, Category = "Death Settings")
	float DestroyDelayTime = 3.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Death Settings")
	UParticleSystem* DeathEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Death Settings")
	FTimerHandle DeathTimerHandle;

	// ToDo : BluePrintFunction Library
	FName DetermineDirection(const FVector& TargetPos) const;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintCallable)
	virtual void CalcDamage(FAttackData& AttackData, AActor* Caster, AActor* Target, FHitResult& HitInfo);
	UFUNCTION(BlueprintCallable)
	virtual float TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator,
	                         AActor* DamageCauser) override;

	UFUNCTION(BlueprintCallable)
	virtual void OnDie();
	UFUNCTION(BlueprintCallable)
	virtual void OnKill();
	UFUNCTION(BlueprintCallable)
	virtual void OnRevival();

	bool IsDie() const { return m_ConditionComponent->IsDead(); }

	// Health 기능 퍼사드 제공
	UFUNCTION(BlueprintCallable, Category = "HP")
	float IncreaseHP(float Delta) const;
	UFUNCTION(BlueprintCallable, Category = "HP")
	virtual float DecreaseHP(float Delta);
	UFUNCTION(BlueprintCallable, Category = "HP")
	void SetHPByRate(float Rate) const;
	UFUNCTION(BlueprintCallable, Category = "HP")
	float GetHP() const;
	float GetMaxHP() const;
	bool IsMaxHP() const;

	// Stamina 기능 퍼사드 제공
	UFUNCTION(BlueprintCallable, Category = "Stamina")
	float IncreaseStamina(float Delta) const;
	UFUNCTION(BlueprintCallable, Category = "Stamina")
	virtual float DecreaseStamina(float Delta, bool bIsDamaged = true);
	UFUNCTION(BlueprintCallable, Category = "Stamina")
	float GetStamina() const;
	UFUNCTION(BlueprintCallable, Category = "Stamina")
	bool CanUseStamina(float Cost) const;

	// Condition 기능 퍼사드 제공
	UFUNCTION(BlueprintCallable, Category = "Condition")
	bool AddCondition(EConditionBitsType AddConditionType, float Duration = 0.0f);
	UFUNCTION(BlueprintCallable, Category = "Condition")
	bool RemoveCondition(EConditionBitsType RemoveConditionType) const;
	UFUNCTION(BlueprintCallable, Category = "Condition")
	bool HasCondition(EConditionBitsType HasConditionType) const;
	UFUNCTION(BlueprintCallable, Category = "Condition")
	bool ExchangeDead() const;

	// Move Rotate Interface
	void StopRotate() const;
	void StopMove() const;
	void StopAll();

	// Move 기능 퍼사드 제공
	void MoveActorTo(const FVector& TargetPosition, float Duration,
	                 EMovementInterpolationType InterpType = EMovementInterpolationType::Linear,
	                 bool bStickToGround = false);

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void MoveActorToWithSpeed(const FVector& TargetPosition, float Speed,
	                          EMovementInterpolationType InterpType = EMovementInterpolationType::Linear,
	                          bool bStickToGround = false);

	// Rotate 기능 퍼사드 제공
	UFUNCTION(BlueprintCallable, Category = "Rotation")
	void LookAtLocation(const FVector& TargetLocation, EPMRotationMode Mode, float DurationOrSpeed,
	                    float Ratio = 1.0f, EMovementInterpolationType InterpType = EMovementInterpolationType::Linear);
	UFUNCTION(BlueprintCallable, Category = "Rotation")
	void LookAtLocationDirect(const FVector& TargetLocation) const;

	// Skill Interface
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual UBaseSkill* GetCurrentSkill();
	virtual FAttackData* GetCurrentSkillAttackData(int Index);
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual bool CanCastSkill(UBaseSkill* Skill, AAreaObject* Target);
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual bool CanCastNextSkill(UBaseSkill* Skill, AAreaObject* Target);
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual bool CastSkill(UBaseSkill* Skill, AAreaObject* Target);
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void UpdateCurrentSkill(UBaseSkill* NewSkill);
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual UBaseSkill* GetSkillByID(int SkillID);

	// ToDo : 스킬 사용후 이동, 공격 가능 기능 추가 구현
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void ClearCurrentSkill();
	virtual void ClearThisCurrentSkill(UBaseSkill* Skill);

	// Sound Interface 
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void PlayGlobalSound(int SoundID);
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void PlayPositionalSound(int SoundID, FVector Position);
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void PlayBGM(int SoundID, bool bLoop = true);
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void StopBGM();

#pragma region DamageSystem
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	bool bCanNextSkill = false;

	// HitStop 관련
	void ApplyHitStop(float Duration);
	void ResetTimeScale() const;
	FTimerHandle HitStopTimerHandle;

	// 넉백 관련
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void HandleKnockBack(const FVector& TargetPos, const FAttackData& AttackData,
	                     float KnockBackForceMultiplier = 1.0f);
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void ApplyKnockBack(const FVector& KnockBackForce);
	float KnockBackDuration = 0.1f;
	// 넉백 거리 배율
	float m_KnockBackForceMultiplier = 1.0f;

#pragma endregion DamageSystem
	//약점에 맞았는지
	virtual bool IsWeakPointHit(const FVector& HitLoc);

	FAreaObjectData* dt_AreaObject;

private:
	UPROPERTY()
	USonheimGameInstance* m_GameInstance = nullptr;
	UPROPERTY()
	ASonheimGameMode* m_GameMode = nullptr;
};

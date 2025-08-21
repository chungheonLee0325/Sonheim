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

class UBaseAnimInstance;
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

	// === Data ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AreaObject Data Setting")
	int m_AreaObjectID;
	
	// === General ===
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// 복제 속성 설정
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "AreaObject")
	FAreaObjectData GetAreaObjectData() const {return *dt_AreaObject; };
protected:
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;
	
public:
	// === Facade Pattern ===
	// Health 기능 퍼사드 제공
	UFUNCTION(BlueprintCallable, Category = "HP")
	float IncreaseHP(float Delta);
	UFUNCTION(BlueprintCallable, Category = "HP")
	virtual float DecreaseHP(float Delta);
	UFUNCTION(BlueprintCallable, Category = "HP")
	void SetHPByRate(float Rate);
	
	UFUNCTION(BlueprintCallable, Category = "HP")
	float GetHP() const;
	float GetMaxHP() const;
	bool IsMaxHP() const;

	// Stamina 기능 퍼사드 제공
	UFUNCTION(BlueprintCallable, Category = "Stamina")
	float IncreaseStamina(float Delta);
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

	// Others
	UBaseAnimInstance* GetSAnimInstance() const;

	
	// === Attribute ===
	UPROPERTY(BlueprintReadWrite)
	UHealthComponent* m_HealthComponent;
	UPROPERTY(BlueprintReadWrite)
	UConditionComponent* m_ConditionComponent;
	UPROPERTY(BlueprintReadWrite)
	class UStaminaComponent* m_StaminaComponent;
	UPROPERTY(BlueprintReadWrite)
	class ULevelComponent* m_LevelComponent;

	// === Utility Component ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	URotateUtilComponent* m_RotateUtilComponent;
	UPROPERTY(BlueprintReadWrite)
	UMoveUtilComponent* m_MoveUtilComponent;

#pragma region DamageSystem
	// === Combat ===
	UFUNCTION(BlueprintCallable)
	virtual void CalcDamage(FAttackData& AttackData, AActor* Caster, AActor* Target, FHitResult& HitInfo);
	
	virtual bool CanAttack(AActor* TargetActor);
	
protected:
	// 서버에서 데미지 적용
	UFUNCTION(Server, Reliable)
	void Server_CalcDamage(FAttackData AttackData, AActor* Caster, AActor* Target, FHitResult HitInfo);
	
	UFUNCTION(BlueprintCallable)
	virtual float TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator,
							 AActor* DamageCauser) override;

	// 클라이언트에게 데미지 효과 적용 (VFX, SFX 등)
	UFUNCTION(NetMulticast, Reliable)
	void MulticastDamageEffect(float Damage, FVector HitLocation, AActor* DamageCauser, bool bWeakPoint, float ElementDamageMultiplier, const FAttackData& AttackData);

	// 피격 방향 적용
	FName DetermineDirection(const FVector& TargetPos) const;

	// 공격력 계산 공식
	virtual float HandleAttackDamageCalculation(float Damage);
	// 방어력 계산 공식
	virtual float HandleDefenceDamageCalculation(float Damage);

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

	//약점에 맞았는지
	virtual bool IsWeakPointHit(const FVector& HitLoc);
#pragma endregion DamageSystem
	
	// === Death Setting ===
public:
	//UFUNCTION(BlueprintCallable)
	UFUNCTION(BlueprintCallable, NetMulticast, reliable)
	virtual void OnDie();
	
	UFUNCTION(BlueprintCallable)
	virtual void OnKill(AAreaObject* Killer);
	UFUNCTION(BlueprintCallable)
	virtual void OnRevival();

	bool IsDie() const { return m_ConditionComponent->IsDead(); }

protected:
	// 죽음 후 destroy 지연 시간
	UPROPERTY(EditDefaultsOnly, Category = "Death Settings")
	float DestroyDelayTime = 3.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Death Settings")
	UParticleSystem* DeathEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Death Settings")
	FTimerHandle DeathTimerHandle;

	// 리플리케이션된 데이터를 처리하기 위한 함수들
	UPROPERTY(ReplicatedUsing = OnRep_IsDead)
	bool bIsDead = false;
	
	UFUNCTION()
	virtual void OnRep_IsDead();

	
	// === Animation ===
	UPROPERTY()
	class UBaseAnimInstance* m_AnimInstance; 
	UPROPERTY()
	USonheimGameInstance* m_GameInstance = nullptr;
	UPROPERTY()
	ASonheimGameMode* m_GameMode = nullptr;


public:
	// === Skill System ===
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual UBaseSkill* GetCurrentSkill();
	virtual FAttackData* GetCurrentSkillAttackData(int Index);
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual bool CanCastSkill(UBaseSkill* Skill, AAreaObject* Target);
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual bool CanCastNextSkill(UBaseSkill* Skill, AAreaObject* Target);
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual bool CastSkill(UBaseSkill* Skill, AAreaObject* Target);
	UFUNCTION(Server, Reliable)
	virtual void Server_CastSkill(int SkillID, AAreaObject* Target);
	UFUNCTION(NetMulticast, Reliable)
	virtual void MultiCast_CastSkill(int SkillID, AAreaObject* Target);
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void UpdateCurrentSkill(UBaseSkill* NewSkill);
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual UBaseSkill* GetSkillByID(int SkillID);
	
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void ClearCurrentSkill();
	virtual void ClearThisCurrentSkill(UBaseSkill* Skill);
protected:
	UPROPERTY(EditAnywhere, Category = "Skill")
	TSet<int> m_OwnSkillIDSet;

	UPROPERTY(EditAnywhere, Category = "Skill")
	TMap<int, TObjectPtr<UBaseSkill>> m_SkillInstanceMap;

	UPROPERTY()
	TObjectPtr<UBaseSkill> m_CurrentSkill;

public:
	// VFX / SFX Network Interface
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayNiagaraEffectAttached(AActor* AttachTarget, UNiagaraSystem* NiagaraEffect, FRotator Rotator = FRotator::ZeroRotator, float Duration = 0.f);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayNiagaraEffectAtLocation(FVector Location, UNiagaraSystem* NiagaraEffect, FRotator Rotator = FRotator::ZeroRotator, float Duration = 0.f);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlaySoundAtLocation(FVector Location, USoundBase* SoundEffect);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlaySound(USoundBase* SoundEffect);
	
	// Sound Interface 
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void PlayGlobalSound(int SoundID);
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void PlayPositionalSound(int SoundID, FVector Position);
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void PlayBGM(int SoundID, bool bLoop = true);
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void StopBGM();

	FAreaObjectData* dt_AreaObject;

	float SprintSpeedRatio = 2.0f;

	// 현재 이동 속도 비율을 반환 (0.0 ~ 1.0)
	UFUNCTION(BlueprintPure, Category = "Movement")
	float GetCurrentSpeedRatio() const;
};

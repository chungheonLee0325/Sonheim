#pragma once

#include "CoreMinimal.h"
#include "Sonheim/AreaObject/Base/AreaObject.h"
#include "Containers/Queue.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "Sonheim/Utilities/LogMacro.h"
#include "BaseMonster.generated.h"

class UWidgetComponent;
class USkillBag;
class UBaseAiFSM;
class UBaseSkill;
//struct FAIStimulus;

UCLASS()
class SONHEIM_API ABaseMonster : public AAreaObject
{
	GENERATED_BODY()

public:
	ABaseMonster();
	// Skill
	FSkillBagData* dt_SkillBag;

	UPROPERTY(BlueprintReadWrite)
	UBaseSkill* NextSkill;

	UFUNCTION()
	UBaseSkillRoulette* GetSkillRoulette() const;
	
	UPROPERTY(EditAnywhere, Category = "UI")
	UWidgetComponent* HPWidgetComponent;
	
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	class UMonsterStatusWidget* StatusWidget;
	
	UPROPERTY(EditAnywhere, Category = "UI")
	float HeightHPUI = 160.0f;
	
	UPROPERTY()
	FTimerHandle OnDieHandle;

	virtual float DecreaseHP(float Delta) override;
	virtual float DecreaseStamina(float Delta, bool bIsDamaged = true) override;
	void SetHPWidgetVisibility(bool IsVisible);
	void SetHPWidgetVisibilityByDuration(float Duration);
	FTimerHandle HPWidgetVisibleTimer;
protected:
	// Combat System
	UPROPERTY()
	AAreaObject* m_AggroTarget;
	UPROPERTY()
	FVector m_SpawnLocation;
	UPROPERTY(VisibleAnywhere)
	UBaseAiFSM* m_AiFSM;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Skill")
	UBaseSkillRoulette* m_SkillRoulette;

	UPROPERTY(EditDefaultsOnly, Category = "Sight")
	float SightRadius = 1500.f;
	UPROPERTY(EditDefaultsOnly, Category = "Sight")
	float LoseSightRadius = 1500.f;
	
private:
	UPROPERTY()
	AActor* m_CurrentTarget;

public:
	// Core Functions
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintCallable)
	virtual void OnBodyBeginOverlap(UPrimitiveComponent* OverlappedComponent,
	                                AActor* OtherActor,
	                                UPrimitiveComponent* OtherComp,
	                                int32 OtherBodyIndex,
	                                bool bFromSweep,
	                                const FHitResult& SweepResult);

	// Combat Interface
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual AAreaObject* GetAggroTarget() const;

	// Combat System
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void SetAggroTarget(AAreaObject* NewTarget) { m_AggroTarget = NewTarget; }

	UFUNCTION(BlueprintCallable, Category = "Combat")
	float GetDistToTarget();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	FVector GetDirToTarget();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	float GetNextSkillRange();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	FVector GetSpawnLocation();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	float GetSightLength();

	// Skill
	void RemoveSkillEntryByID(const int id);
	void AddSkillEntryByID(const int id);

	/*
	// AI Perception 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	class UAIPerceptionComponent* AIPerceptionComponent;

	// 시야 설정
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	class UAISenseConfig_Sight* SightConfig;

	// 감지 이벤트를 처리할 함수
	//UFUNCTION()
	//void OnPerceptionUpdated(AActor* Actor, struct FAIStimulus Stimulus);
*/
protected:
	// 가상 팩토리 함수
	UFUNCTION(BlueprintCallable)
	virtual UBaseAiFSM* CreateFSM();
	UFUNCTION(BlueprintCallable)
	virtual UBaseSkillRoulette* CreateSkillRoulette();

	virtual void OnDie() override;

	virtual void InitializeHUD();
};

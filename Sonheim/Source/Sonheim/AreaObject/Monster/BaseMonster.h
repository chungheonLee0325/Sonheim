#pragma once

#include "CoreMinimal.h"
#include "Sonheim/AreaObject/Base/AreaObject.h"
#include "Containers/Queue.h"
#include "Sonheim/GameManager/SonheimGameInstance.h"
#include "Sonheim/Utilities/LogMacro.h"
#include "BaseMonster.generated.h"

class UNiagaraSystem;
class UNiagaraEmitter;
class UWidgetComponent;
class USkillBag;
class UBaseAiFSM;
class UBaseSkill;
class ABaseResourceObject;
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
	UFUNCTION(NetMulticast, Reliable)
	void MultiCastRPC_Show();
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

	// GameJam으로 추가
	// ToDo: 필요한 기능들 위로 올리기
public:

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	class AAIController* AIController;

	// 몬스터 속도 변화
	bool bIsWarning{false};

	// 놀라기
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	bool bIsSurprise{false};

	// Resource
	int32 GotResource{};

	// 운반 중?
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	bool bIsTransporting{false};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UStaticMeshComponent* PickaxeMesh;

	UPROPERTY(VisibleAnywhere, Replicated)
	UBaseAiFSM* m_AiFSM;

	UPROPERTY()
	ABaseResourceObject* m_ResourceTarget;
	// Resource
	UFUNCTION(BlueprintCallable, Category = "Resource")
	virtual ABaseResourceObject* GetResourceTarget() const;

	UFUNCTION(BlueprintCallable, Category = "Resource")
	virtual void SetResourceTarget(ABaseResourceObject* NewTarget)
	{
		m_ResourceTarget = NewTarget;
		if (m_ResourceTarget != nullptr) { IsWorked = true; }
	}

	// 놀라기
	void Surprise();
	void CalmDown();

	// 짐 들기
	void StartTransport();
	void EndTransport();

	// 얼굴 변화 ( ai 게임 잼 때 쓴 거 )
	// UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	// void ChangeFace(int32 Feel);

	// AI Voice Command
	UFUNCTION(BlueprintCallable)
	void AIVoiceCommand(int ResourceID, bool IsForced = false);

	UFUNCTION(BlueprintCallable)
	class ABaseResourceObject* GetNearResourceObject(int ResourceID);

	void SetIsForced(bool IsForced);
	bool bIsForced = false;
	void VFXSpwan(int VFXID);
	bool IsWorked = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UNiagaraSystem* VFX_Exe;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UNiagaraSystem* VFX_Question;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UNiagaraSystem* VFX_Sweet;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USceneComponent* HeadVFXPoint;


	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
	                         class AController* EventInstigator, AActor* DamageCauser) override;

	float WalkSpeed = 400.f;
	float ForcedWalkSpeed = 1200.f;

	UFUNCTION()
	void ChangeFace(EFaceType Type);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastRPC_ChangeFace(EFaceType Type);
	
	UPROPERTY()
	class UMaterialInstanceDynamic* EyeMat{nullptr};
	UPROPERTY()
	class UMaterialInstanceDynamic* MouthMat{nullptr};

	UPROPERTY(Replicated)
	bool IsDead{false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite,Replicated)
	bool IsCalled{false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	bool bActivateSkill{false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	class ASonheimPlayer* PartnerOwner;

	// ToDo : Begin Play에서 호출하는것 변경 예정
	UFUNCTION(BlueprintCallable)
	void SetPartnerOwner(ASonheimPlayer* NewOwner);

	// ToDo : @@도윤 인터페이스화 할것
	void PartnerSkillStart() { IsCalled = true; }
	// ToDo : 추가로 수정 예정 - 현재 : true일때 발사 false 발사중지 보내주기 -> trigger로 쏴주기
	void PartnerSkillTrigger(bool IsTrigger) { bActivateSkill = IsTrigger; };
	void PartnerSkillEnd() { IsCalled = false; }

	bool bIsActive = true;

	UFUNCTION(BlueprintCallable)
	void ActivateMonster();
	UFUNCTION(BlueprintCallable)
	void DeactivateMonster();
};

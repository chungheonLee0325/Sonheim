// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "BaseAiFSM.generated.h"

class ABaseMonster;
class UBaseAiState;

UCLASS(Abstract, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SONHEIM_API UBaseAiFSM : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UBaseAiFSM();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	void AddState(EAiStateType StateType, UBaseAiState* State);

	void ChangeState(EAiStateType StateType);

	bool IsExistState(EAiStateType StateType) const;

	virtual void UpdateState(float dt);

	virtual void InitStatePool() PURE_VIRTUAL(UBaseAiFSM::InitStatePool,);

	void StopFSM() { m_CurrentState = nullptr; }

	// AiState 검증(Builder Pattern 간 누락 방지)
	void CheckIsValidAiStates() const;

	// Template 를 이용한 동적 state 생성 -> Builder Pattern을 사용하기 위해 적용
	template <typename T>
	static T* CreateState(UObject* Outer, ABaseMonster* Owner, EAiStateType NextState = EAiStateType::None,
	                      EAiStateType SuccessState = EAiStateType::None, EAiStateType FailState = EAiStateType::None)
	{
		static_assert(std::is_base_of_v<UBaseAiState, T>, "T must derive from UAiState");

		T* State = NewObject<T>(Outer, T::StaticClass());
		State->SetOwner(Owner);
		State->InitState();
		State->SetNextState(NextState);
		State->SetSuccessState(SuccessState);
		State->SetFailState(FailState);
		// ToDo : 논의 후 AddState도 여기에 넣기
		return State;
	}

protected:
	UPROPERTY()
	ABaseMonster* m_Owner = nullptr;

private:
	UPROPERTY()
	TMap<EAiStateType, UBaseAiState*> m_AiStates;

	UPROPERTY(VisibleAnywhere)
	UBaseAiState* m_CurrentState = nullptr;
	UPROPERTY(VisibleAnywhere)
	UBaseAiState* m_PreviousState = nullptr;
};

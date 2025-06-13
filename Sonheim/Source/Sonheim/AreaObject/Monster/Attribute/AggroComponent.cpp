#include "AggroComponent.h"
#include "Sonheim/AreaObject/Base/AreaObject.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "Sonheim/Utilities/LogMacro.h"

UAggroComponent::UAggroComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f; // 성능을 위해 매 프레임이 아닌 0.1초마다 업데이트
	SetIsReplicatedByDefault(true);
	
	// 우선순위 가중치 초기화
	PriorityWeights.Add(EAggroPriority::None, 0.0f);
	PriorityWeights.Add(EAggroPriority::Low, 0.5f);
	PriorityWeights.Add(EAggroPriority::Medium, 1.0f);
	PriorityWeights.Add(EAggroPriority::High, 2.0f);
	PriorityWeights.Add(EAggroPriority::Highest, 5.0f);
}

void UAggroComponent::BeginPlay()
{
	Super::BeginPlay();
	
	OwnerAreaObject = Cast<AAreaObject>(GetOwner());
}

void UAggroComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (!bIsAggroEnabled || !GetOwner()->HasAuthority())
		return;
	
	// 위협 감소
	DecayThreats(DeltaTime);
	
	// 무효한 엔트리 정리
	CleanupInvalidEntries();
	
	// 타겟 업데이트
	LastUpdateTime += DeltaTime;
	if (LastUpdateTime >= ThreatUpdateInterval)
	{
		UpdateCurrentTarget();
		LastUpdateTime = 0.0f;
	}
}

void UAggroComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(UAggroComponent, ThreatTable);
	DOREPLIFETIME(UAggroComponent, CurrentTarget);
}

void UAggroComponent::AddThreat(AActor* Target, float ThreatValue, EAggroPriority Priority)
{
	if (!Target || !bIsAggroEnabled || ThreatValue <= 0.0f)
		return;
	
	if (GetOwner()->HasAuthority())
	{
		Server_AddThreat(Target, ThreatValue, Priority);
	}
	else
	{
		Server_AddThreat(Target, ThreatValue, Priority);
	}
}

void UAggroComponent::Server_AddThreat_Implementation(AActor* Target, float ThreatValue, EAggroPriority Priority)
{
	if (!Target || !bIsAggroEnabled || ThreatValue <= 0.0f)
		return;
	
	// 유효한 타겟인지 확인
	if (!IsValidTarget(Target))
		return;
	
	// 기존 엔트리 찾기
	FAggroEntry* ExistingEntry = nullptr;
	for (FAggroEntry& Entry : ThreatTable)
	{
		if (Entry.Target == Target)
		{
			ExistingEntry = &Entry;
			break;
		}
	}
	
	// 거리 및 우선순위 수정자 계산
	float DistanceModifier = CalculateDistanceModifier(Target);
	float PriorityModifier = CalculatePriorityModifier(Priority);
	float FinalThreat = ThreatValue * DistanceModifier * PriorityModifier;
	
	if (ExistingEntry)
	{
		// 기존 위협 업데이트
		ExistingEntry->ThreatValue += FinalThreat;
		ExistingEntry->LastUpdateTime = GetWorld()->GetTimeSeconds();
		ExistingEntry->Priority = FMath::Max(ExistingEntry->Priority, Priority);
	}
	else
	{
		// 새 엔트리 추가
		FAggroEntry NewEntry;
		NewEntry.Target = Target;
		NewEntry.ThreatValue = FinalThreat;
		NewEntry.LastUpdateTime = GetWorld()->GetTimeSeconds();
		NewEntry.Priority = Priority;
		ThreatTable.Add(NewEntry);
	}
	
	OnThreatAdded.Broadcast(Target, FinalThreat);
	
	// 즉시 타겟 업데이트
	UpdateCurrentTarget();
}

void UAggroComponent::RemoveThreat(AActor* Target)
{
	if (!Target)
		return;
	
	if (GetOwner()->HasAuthority())
	{
		Server_RemoveThreat(Target);
	}
	else
	{
		Server_RemoveThreat(Target);
	}
}

void UAggroComponent::Server_RemoveThreat_Implementation(AActor* Target)
{
	if (!Target)
		return;
	
	ThreatTable.RemoveAll([Target](const FAggroEntry& Entry)
	{
		return Entry.Target == Target;
	});
	
	OnThreatRemoved.Broadcast(Target);
	
	// 현재 타겟이 제거되었으면 업데이트
	if (CurrentTarget == Target)
	{
		UpdateCurrentTarget();
	}
}

void UAggroComponent::ClearAllThreats()
{
	if (GetOwner()->HasAuthority())
	{
		Server_ClearAllThreats();
	}
	else
	{
		Server_ClearAllThreats();
	}
}

void UAggroComponent::Server_ClearAllThreats_Implementation()
{
	ThreatTable.Empty();
	
	AActor* OldTarget = CurrentTarget;
	CurrentTarget = nullptr;
	
	if (OldTarget)
	{
		MultiCast_OnTargetChanged(OldTarget, nullptr);
	}
}

void UAggroComponent::ModifyThreat(AActor* Target, float DeltaThreat)
{
	if (!Target || !bIsAggroEnabled)
		return;
	
	// 기존 위협값 가져오기
	for (FAggroEntry& Entry : ThreatTable)
	{
		if (Entry.Target == Target)
		{
			float NewThreat = FMath::Max(0.0f, Entry.ThreatValue + DeltaThreat);
			Entry.ThreatValue = NewThreat;
			Entry.LastUpdateTime = GetWorld()->GetTimeSeconds();
			
			if (NewThreat <= 0.0f)
			{
				RemoveThreat(Target);
			}
			else
			{
				UpdateCurrentTarget();
			}
			break;
		}
	}
}

void UAggroComponent::SetThreatMultiplier(AActor* Target, float Multiplier)
{
	if (!Target || !bIsAggroEnabled)
		return;
	
	for (FAggroEntry& Entry : ThreatTable)
	{
		if (Entry.Target == Target)
		{
			Entry.ThreatValue *= Multiplier;
			UpdateCurrentTarget();
			break;
		}
	}
}

AActor* UAggroComponent::GetHighestThreatTarget() const
{
	if (ThreatTable.Num() == 0)
		return nullptr;
	
	// 도발 대상이 있으면 우선 반환
	if (IsValid(TauntTarget))
		return TauntTarget;
	
	const FAggroEntry* HighestEntry = nullptr;
	float HighestThreat = 0.0f;
	
	for (const FAggroEntry& Entry : ThreatTable)
	{
		if (IsValid(Entry.Target.Get()) && Entry.ThreatValue > HighestThreat)
		{
			HighestThreat = Entry.ThreatValue;
			HighestEntry = &Entry;
		}
	}
	
	return HighestEntry ? HighestEntry->Target.Get() : nullptr;
}

TArray<AActor*> UAggroComponent::GetThreatList() const
{
	TArray<AActor*> Result;
	
	for (const FAggroEntry& Entry : ThreatTable)
	{
		if (IsValid(Entry.Target.Get()))
		{
			Result.Add(Entry.Target.Get());
		}
	}
	
	// 위협값 기준으로 정렬
	Result.Sort([this](const AActor& A, const AActor& B)
	{
		float ThreatA = GetThreatValue(const_cast<AActor*>(&A));
		float ThreatB = GetThreatValue(const_cast<AActor*>(&B));
		return ThreatA > ThreatB;
	});
	
	return Result;
}

float UAggroComponent::GetThreatValue(AActor* Target) const
{
	if (!Target)
		return 0.0f;
	
	for (const FAggroEntry& Entry : ThreatTable)
	{
		if (Entry.Target == Target)
		{
			return Entry.ThreatValue;
		}
	}
	
	return 0.0f;
}

bool UAggroComponent::HasThreat(AActor* Target) const
{
	if (!Target)
		return false;
	
	for (const FAggroEntry& Entry : ThreatTable)
	{
		if (Entry.Target == Target)
		{
			return Entry.ThreatValue > 0.0f;
		}
	}
	
	return false;
}

void UAggroComponent::Taunt(AActor* Taunter, float Duration)
{
	if (!Taunter || !bIsAggroEnabled)
		return;
	
	// 기존 도발 타이머 취소
	if (TauntTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(TauntTimerHandle);
	}
	
	// 도발 대상 설정
	TauntTarget = Taunter;
	
	// 즉시 타겟 변경
	AActor* OldTarget = CurrentTarget;
	CurrentTarget = TauntTarget;
	
	if (OldTarget != CurrentTarget)
	{
		MultiCast_OnTargetChanged(OldTarget, CurrentTarget);
	}
	
	// 도발 지속시간 타이머
	GetWorld()->GetTimerManager().SetTimer(TauntTimerHandle, [this]()
	{
		TauntTarget = nullptr;
		UpdateCurrentTarget();
	}, Duration, false);
}

void UAggroComponent::DropAggro(float Percentage)
{
	if (!bIsAggroEnabled)
		return;
	
	Percentage = FMath::Clamp(Percentage, 0.0f, 1.0f);
	
	for (FAggroEntry& Entry : ThreatTable)
	{
		Entry.ThreatValue *= (1.0f - Percentage);
	}
	
	UpdateCurrentTarget();
}

void UAggroComponent::MultiCast_OnTargetChanged_Implementation(AActor* OldTarget, AActor* NewTarget)
{
	OnAggroTargetChanged.Broadcast(OldTarget, NewTarget);
}

void UAggroComponent::OnRep_CurrentTarget(AActor* OldTarget)
{
	OnAggroTargetChanged.Broadcast(OldTarget, CurrentTarget);
}

void UAggroComponent::UpdateCurrentTarget()
{
	AActor* NewTarget = GetHighestThreatTarget();
	
	if (NewTarget != CurrentTarget)
	{
		AActor* OldTarget = CurrentTarget;
		CurrentTarget = NewTarget;
		MultiCast_OnTargetChanged(OldTarget, CurrentTarget);
	}
}

void UAggroComponent::CleanupInvalidEntries()
{
	ThreatTable.RemoveAll([this](const FAggroEntry& Entry)
	{
		return !IsValid(Entry.Target.Get()) || !IsValidTarget(Entry.Target.Get());
	});
}

void UAggroComponent::DecayThreats(float DeltaTime)
{
	if (ThreatDecayRate <= 0.0f)
		return;
	
	float CurrentTime = GetWorld()->GetTimeSeconds();
	
	for (FAggroEntry& Entry : ThreatTable)
	{
		// 최근에 업데이트되지 않은 위협만 감소
		if (CurrentTime - Entry.LastUpdateTime > 1.0f)
		{
			Entry.ThreatValue = FMath::Max(0.0f, Entry.ThreatValue - (ThreatDecayRate * DeltaTime));
		}
	}
	
	// 위협이 0이 된 엔트리 제거
	ThreatTable.RemoveAll([](const FAggroEntry& Entry)
	{
		return Entry.ThreatValue <= 0.0f;
	});
}

bool UAggroComponent::IsValidTarget(AActor* Target) const
{
	if (!Target || !OwnerAreaObject)
		return false;
	
	// 자기 자신은 타겟이 될 수 없음
	if (Target == GetOwner())
		return false;
	
	// AreaObject인지 확인
	AAreaObject* TargetAreaObject = Cast<AAreaObject>(Target);
	if (!TargetAreaObject)
		return false;
	
	// 죽었거나 숨겨진 대상은 제외
	if (TargetAreaObject->IsDie() || TargetAreaObject->HasCondition(EConditionBitsType::Hidden))
		return false;
	
	// 공격 가능한 대상인지 확인
	if (!OwnerAreaObject->CanAttack(Target))
		return false;
	
	return true;
}

float UAggroComponent::CalculateDistanceModifier(AActor* Target) const
{
	if (!Target || !GetOwner())
		return 1.0f;
	
	float Distance = FVector::Dist(GetOwner()->GetActorLocation(), Target->GetActorLocation());
	
	if (Distance > MaxThreatDistance)
	{
		return OutOfRangeThreatMultiplier;
	}
	
	// 거리가 가까울수록 위협 증가
	float DistanceRatio = 1.0f - (Distance / MaxThreatDistance);
	return FMath::Lerp(0.5f, 1.5f, DistanceRatio);
}

float UAggroComponent::CalculatePriorityModifier(EAggroPriority Priority) const
{
	if (const float* Weight = PriorityWeights.Find(Priority))
	{
		return *Weight;
	}
	return 1.0f;
}
// SonheimSkillComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/UnrealNetwork.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "SonheimSkillComponent.generated.h"

class UBaseSkill;
class AAreaObject;
class USonheimGameInstance;

USTRUCT(BlueprintType)
struct FSonheimSkillSpecItem : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 SkillId = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Level = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bIsCasting = false;

	// 서버 기준 종료시각 (World Time Seconds)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float CooldownEndTime = 0.0f;
};

USTRUCT(BlueprintType)
struct FSonheimSkillSpecContainer : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FSonheimSkillSpecItem> Items;

	UPROPERTY(NotReplicated)
	class USonheimSkillComponent* Owner = nullptr;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FSonheimSkillSpecItem, FSonheimSkillSpecContainer>(
			Items, DeltaParms, *this);
	}
};

template <>
struct TStructOpsTypeTraits<FSonheimSkillSpecContainer> : public TStructOpsTypeTraitsBase2<FSonheimSkillSpecContainer>
{
	enum { WithNetDeltaSerializer = true };
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SONHEIM_API USonheimSkillComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USonheimSkillComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 초기 보유 스킬을 등록 (서버에서만)
	void InitializeOwnedSkills(const TSet<int32>& SkillIds);

	// 서버: 스킬 시작/쿨다운 상태 반영
	void OnServerSkillActivated(int32 SkillId);
	void OnServerSkillCooldownStart(int32 SkillId, float CooldownEndTime);
	void OnServerSkillCancelled(int32 SkillId);

	// 조회
	//UFUNCTION(BlueprintCallable, Category="Skill")
	const FSonheimSkillSpecItem* GetSpec(int32 SkillId) const;

	// 전체 컨테이너(읽기)
	UFUNCTION(BlueprintCallable, Category="Skill")
	const FSonheimSkillSpecContainer& GetSpecs() const { return SkillSpecs; }

	// 활성 상태 조회(컴포넌트 기준)
	UFUNCTION(BlueprintCallable, Category="Skill")
	bool IsAnySkillCasting() const;

	UFUNCTION(BlueprintCallable, Category="Skill")
	int32 GetActiveSkillId() const;

	// 캐스팅(클라에서 호출하면 서버 RPC로 위임)
	UFUNCTION(BlueprintCallable, Category="Skill")
	bool TryCastSkillById(int32 SkillId, AAreaObject* Target);

	// 서버 알림: 애님노티파이에서 발사/완료 타이밍
	UFUNCTION(Server, Reliable)
	void Server_NotifySkillFire(int32 SkillId);
	UFUNCTION(Server, Reliable)
	void Server_NotifySkillComplete(int32 SkillId);

	// 서버: 캐스팅 시도
	UFUNCTION(Server, Reliable)
	void Server_TryCastSkill(int32 SkillId, AAreaObject* Target);

	// 스킬 인스턴스 관리
	UFUNCTION(BlueprintCallable, Category="Skill")
	UBaseSkill* EnsureSkillInstance(int32 SkillId);

	UFUNCTION(BlueprintCallable, Category="Skill")
	UBaseSkill* GetSkillById(int32 SkillId) const;

	// Grant API (장비/스킬트리/버프 등 외부 소스 스킬 부여/회수)
	UFUNCTION(BlueprintCallable, Category="Skill")
	FGuid GrantSkills(const TArray<int32>& SkillIds);
	UFUNCTION(BlueprintCallable, Category="Skill")
	void RevokeByGrantId(const FGuid& GrantId);
	// 기존 Grant를 새 스킬셋으로 원자적으로 교체(GrantId가 유효하지 않으면 새로 발급)
	UFUNCTION(BlueprintCallable, Category="Skill")
	void ReplaceGrant(FGuid& InOutGrantId, const TArray<int32>& NewSkillIds);

	// 복제를 위한 조회자
	const TMap<int32, TObjectPtr<UBaseSkill>>& GetSkillInstances() const { return SkillInstances; }

protected:
	UPROPERTY(Replicated)
	FSonheimSkillSpecContainer SkillSpecs;

	UPROPERTY()
	TMap<int32, int32> IndexBySkillId;

	int32 FindOrAddSpecIndex(int32 SkillId);
	int32 FindSpecIndex(int32 SkillId) const;

	// 스킬 유효 검사: 기본 보유 또는 그랜트된 스킬이면 허용
	bool IsSkillAllowed(int32 SkillId) const;

	// 인스턴스/슬롯 매핑
	UPROPERTY()
	TMap<int32, TObjectPtr<UBaseSkill>> SkillInstances;

	// 소유 스킬 집합(데이터→컴포넌트 복사, 비복제)
	UPROPERTY()
	TSet<int32> OwnedSkillIds;

	// Grant 저장 및 참조 카운트
	//UPROPERTY()
	TMap<FGuid, TArray<int32>> GrantsById;
	UPROPERTY()
	TMap<int32, int32> GrantRefCounts;
};

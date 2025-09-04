// SonheimSkillComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/UnrealNetwork.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "SonheimSkillComponent.generated.h"

class UBaseSkill;

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
	void InitializeOwnedSkills(const TSet<int32>& OwnedSkillIds);

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

protected:
	UPROPERTY(Replicated)
	FSonheimSkillSpecContainer SkillSpecs;

	UPROPERTY()
	TMap<int32, int32> IndexBySkillId;

	int32 FindOrAddSpecIndex(int32 SkillId);
	int32 FindSpecIndex(int32 SkillId) const;
};

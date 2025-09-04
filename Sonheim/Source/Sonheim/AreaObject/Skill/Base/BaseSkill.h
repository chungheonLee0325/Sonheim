// BaseSkill.h

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameFramework/Actor.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "BaseSkill.generated.h"

class AAreaObject;

UENUM(BlueprintType)
enum class ESkillFailCase : uint8
{
	OutRange,
	Null,
	NotReady,
	OutStamina,
	None
};

UENUM(BlueprintType)
enum class ESkillPhase : uint8
{
	Ready, // 스킬 사용 가능
	Casting, // 시전 중
	PostCasting, // 시전 후딜레이
	CoolTime // 쿨타임
};

UCLASS()
class SONHEIM_API UBaseSkill : public UObject
{
	GENERATED_BODY()

public:
	UBaseSkill();
	// 스킬 초기화 - 데이터 초기화
	virtual void InitSkill(FSkillData* SkillData);

	// 스킬 완료 델리게이트 -> 상태머신에서 사용(다음 상태 전이용)
	DECLARE_DELEGATE(FOnSkillComplete)
	FOnSkillComplete OnSkillComplete;
	DECLARE_DELEGATE(FOnSkillCancel)
	FOnSkillCancel OnSkillCancel;

	// 쿨타임 타이머 핸들
	FTimerHandle CoolTimeTimerHandle;

	virtual bool CanCast(class AAreaObject* Caster, const AAreaObject* Target) const;
	// 서버 전용: 스킬 활성화(스태미나 소모/상태 전환/초기화)
	virtual void Activate(class AAreaObject* Caster, AAreaObject* Target);
	// 서버 전용: 스킬 틱(필요한 스킬만 오버라이드)
	virtual void Tick(float DeltaTime);
	// 서버 전용: 스킬 완료(몽타주 종료/Notify 시 호출)
	virtual void Complete();
	// 서버 전용: 스킬 취소(중단 처리)
	virtual void Cancel();

	// 서버 전용: 투사체/판정 스폰 등 실제 효과 발동(Notify로 타이밍 수신)
	UFUNCTION(BlueprintCallable)
	virtual void Fire();

	// 코스메틱: 몽타주 끝/블렌드아웃을 감지해 Complete/Cancel 트리거 연결(서버도 바인딩됨)
	void BindMontageDelegates(class UAnimInstance* AnimInstance, class UAnimMontage* Montage);

	// Getters
	// 현재 진행 페이즈 반환
	UFUNCTION(BlueprintCallable, Category = "Skill")
	ESkillPhase GetCurrentPhase() const { return m_CurrentPhase; }

	// Attack Data 반환
	FAttackData* GetAttackDataByIndex(int Index) const;

	// 쿨타임 진행률 반환 함수
	UFUNCTION(BlueprintCallable, Category = "Skill")
	float GetCooldownProgress() const;

	const FSkillData* GetSkillData() const { return m_SkillData; }
	float GetSkillRange() const { return m_SkillData->CastRange; }
	int GetSkillID() const { return m_SkillData->SkillID; }

	UFUNCTION(BlueprintCallable, Category = "Skill")
	bool IsInRange(const AAreaObject* Caster, const AAreaObject* Target) const;

	UFUNCTION(BlueprintCallable, Category = "Skill")
	void SkillLogPrint();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skill")
	mutable ESkillFailCase SkillFailCase = ESkillFailCase::None;

	void SetNextSkillID(int NextSkillID);
	virtual void ResetNextSkillByBHit();

protected:
	UFUNCTION()
	void AdjustCoolTime();

	// === Replicated State ===
	// 스킬 현재 페이즈 (서버 권한에서만 변경, 클라에 복제)
	UPROPERTY(ReplicatedUsing=OnRep_SkillState)
	ESkillPhase m_CurrentPhase;

	// Caster
	UPROPERTY()
	AAreaObject* m_Caster;

	// Target
	UPROPERTY()
	AAreaObject* m_Target;

	// 서버에서 갱신되는 타겟 위치(코스메틱/검증용). 클라에 복제
	UPROPERTY(Replicated)
	FVector m_TargetPos;

	UPROPERTY()
	TSubclassOf<UBaseSkill> m_NextSkillClass;

	FSkillData* m_SkillData;

	// 다음 연계 스킬 ID (서버에서 셋, 클라 표시용 복제)
	UPROPERTY(Replicated)
	int m_NextSkillID;

	//OnCastEnd에서 사용할 몽타주 종료시 블렌드
	float MontageBlendTime = 0.1f;

private:
	// 남은 쿨타임(서버에서만 감소, 클라로 복제하여 UI에서 사용)
	UPROPERTY(Replicated)
	float m_CurrentCoolTime;

	FOnMontageEnded EndDelegate;
	FOnMontageBlendingOutStarted CompleteDelegate;

public:
	// UObject 네트워킹 지원 설정
	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual bool IsNameStableForNetworking() const override { return true; }
	// 복제 프로퍼티 등록
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UFUNCTION()
	void OnRep_SkillState();

	UFUNCTION()
	void OnMontageEnded(class UAnimMontage* Montage, bool bInterrupted);
	UFUNCTION()
	void OnMontageBlendOut(class UAnimMontage* Montage, bool bInterrupted);
};

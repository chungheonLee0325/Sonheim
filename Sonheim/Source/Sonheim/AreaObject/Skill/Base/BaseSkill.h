// BaseSkill.h

// [네트/스킬 설계 원칙]
// - 서버 권한에서만 게임에 영향을 주는 로직을 실행합니다.
//   (Activate / Tick / Fire / Complete / Cancel 모두 서버 전용)
// - 클라이언트는 코스메틱(애니/사운드/VFX)만 처리하며, 상태는 복제된 값으로 표시합니다.
// - 몽타주 종료/블렌드아웃은 스킬 객체에 델리게이트로 바인딩하여
//   Complete/Cancel을 정확히 1회만 호출하도록 보장합니다.
// - 핵심 상태(Phase/쿨타임/타겟 위치/NextSkillID)만 복제하여 JIP/지연 환경에서도 일관성을 확보합니다.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameFramework/Actor.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "BaseSkill.generated.h"

class AAreaObject;
class USonheimSkillComponent;

UENUM(BlueprintType)
enum class ESkillFailCase : uint8
{
	OutRange,
	Null,
	NotReady,
	OutStamina,
	OutItem,
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
    // 생성/초기화
    UBaseSkill();
    virtual void InitSkill(FSkillData* SkillData);

    // 델리게이트(상태머신 등에서 사용)
    DECLARE_DELEGATE(FOnSkillComplete)
    FOnSkillComplete OnSkillComplete;
    DECLARE_DELEGATE(FOnSkillCancel)
    FOnSkillCancel OnSkillCancel;

    // 서버 전용 실행 경계
    virtual bool CanCast(class AAreaObject* Caster, const AAreaObject* Target) const;
    virtual bool Activate(class AAreaObject* Caster, AAreaObject* Target);
    virtual void Tick(float DeltaTime);
    virtual bool Complete();
    virtual void Cancel();

    // 서버 전용: 실제 효과(투사체/판정 등)
    UFUNCTION(BlueprintCallable)
    virtual bool Fire();

    // 코스메틱: 몽타주 종료/블렌드아웃 바인딩
    void BindMontageDelegates(class UAnimInstance* AnimInstance, class UAnimMontage* Montage);

    // 조회/유틸
    UFUNCTION(BlueprintCallable, Category = "Skill")
    ESkillPhase GetCurrentPhase() const { return m_CurrentPhase; }
    FAttackData* GetAttackDataByIndex(int Index) const;
    UFUNCTION(BlueprintCallable, Category = "Skill")
    float GetCooldownProgress() const;
    const FSkillData* GetSkillData() const { return m_SkillData; }
    float GetSkillRange() const { return m_SkillData->CastRange; }
    int GetSkillID() const { return m_SkillData->SkillID; }
    UFUNCTION(BlueprintCallable, Category = "Skill")
    bool IsInRange(const AAreaObject* Caster, const AAreaObject* Target) const;
    UFUNCTION(BlueprintCallable, Category = "Skill")
    void SkillLogPrint();
    void SetNextSkillID(int NextSkillID);
    virtual void ResetNextSkillByBHit();

	// === 비용 처리 유틸 (스태미나 + ItemCosts) ===
	// Phase 시점의 '검사' (소모는 안함). 부족 아이템을 OutMissingItems로 반환.
	bool CheckCosts(class AAreaObject* Caster, ESkillCostPhase Phase, bool * bOutStaminaLack = nullptr, TArray<int32>* OutMissingItems = nullptr) const;
	// Phase 시점의 '실소모' (서버 권한 전제). 실패 시 false.
	bool ApplyCosts(class AAreaObject* Caster, ESkillCostPhase Phase);
	// RefKind에 따라 실제 ItemID 해석
	int32 ResolveItemID(const FSkillItemCost& Cost, const class AAreaObject* Caster) const;
	

    // 편의: 캐스터의 SkillComponent 가져오기
    UFUNCTION(BlueprintPure, Category = "Skill")
    USonheimSkillComponent* GetSkillComponent() const;

    // 네트워킹 지원/복제 등록
    virtual bool IsSupportedForNetworking() const override { return true; }
    virtual bool IsNameStableForNetworking() const override { return true; }
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 스킬 실패 사유
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skill")
	mutable ESkillFailCase SkillFailCase = ESkillFailCase::None;
protected:
    // 내부 쿨타임 처리
    UFUNCTION()
    void AdjustCoolTime();

    // === 복제 상태 ===
    // 현재 페이즈(서버에서만 변경, 클라 복제)
    UPROPERTY(ReplicatedUsing=OnRep_SkillState)
    ESkillPhase m_CurrentPhase;
    // 캐스터/타겟
    UPROPERTY()
    AAreaObject* m_Caster;
    UPROPERTY()
    AAreaObject* m_Target;
    // 코스메틱/검증용 타겟 위치(복제)
    UPROPERTY(Replicated)
    FVector m_TargetPos;
    // 다음 스킬 정보
    UPROPERTY()
    TSubclassOf<UBaseSkill> m_NextSkillClass;
    FSkillData* m_SkillData;
    UPROPERTY(Replicated)
    int m_NextSkillID;
    // 몽타주 정지 블렌드 시간
    float MontageBlendTime = 0.1f;

    // RepNotify/델리게이트
    UFUNCTION()
    void OnRep_SkillState();
    UFUNCTION()
    void OnMontageEnded(class UAnimMontage* Montage, bool bInterrupted);
    UFUNCTION()
    void OnMontageBlendOut(class UAnimMontage* Montage, bool bInterrupted);
private:
    // 남은 쿨타임(서버만 감소, 클라 복제)
    UPROPERTY(Replicated)
    float m_CurrentCoolTime;
    // 몽타주 델리게이트
    FOnMontageEnded EndDelegate;
    FOnMontageBlendingOutStarted CompleteDelegate;
    // 타이머 핸들
    FTimerHandle CoolTimeTimerHandle;
};

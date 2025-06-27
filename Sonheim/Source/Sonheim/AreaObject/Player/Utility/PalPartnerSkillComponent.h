#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PalPartnerSkillComponent.generated.h"

class ASonheimPlayerState;
class ABaseMonster;
class ASonheimPlayer;
class UPalInventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPalSummoned, ABaseMonster*, SummonedPal);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPalDismissed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPartnerSkillStateChanged, bool, bIsUsing);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SONHEIM_API UPalPartnerSkillComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPalPartnerSkillComponent();

    void InitializeWithPlayerState(ASonheimPlayerState* PlayerState);
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // Pal 소환/해제
    UFUNCTION(BlueprintCallable, Category = "Pal Partner")
    void TogglePalSummon();

    // 파트너 스킬 사용(장착 시작)
    UFUNCTION(BlueprintCallable, Category = "Pal Partner")
    void TogglePartnerSkill();

    // 파트너 스킬 상태 변경(장착 콜백)
    UFUNCTION()
    void SetPartnerSkillState(bool bIsUsing);

    // 파트너 스킬 사용 트리거
    UFUNCTION(BlueprintCallable, Category = "Pal Partner")
    void SetPartnerSkillTrigger(bool bTrigger);

    // 현재 소환된 Pal 가져오기
    UFUNCTION(BlueprintCallable, Category = "Pal Partner")
    ABaseMonster* GetSummonedPal() const { return SummonedPal; }

    UFUNCTION(BlueprintCallable, Category = "Pal Partner")
    bool IsUsingPartnerSkill() const { return bUsingPartnerSkill; }

    // 델리게이트
    UPROPERTY(BlueprintAssignable)
    FOnPalSummoned OnPalSummoned;

    UPROPERTY(BlueprintAssignable)
    FOnPalDismissed OnPalDismissed;

    UPROPERTY(BlueprintAssignable)
    FOnPartnerSkillStateChanged OnPartnerSkillStateChanged;

    // 소환 몽타주
    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    UAnimMontage* SummonPalMontage;

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void OnRep_SummonedPal();

    UFUNCTION()
    void OnRep_UsingPartnerSkill();

private:
    UPROPERTY()
    ASonheimPlayer* OwnerPlayer;

    UPROPERTY()
    UPalInventoryComponent* PalInventory;

    // 현재 소환된 Pal
    UPROPERTY(ReplicatedUsing = OnRep_SummonedPal)
    ABaseMonster* SummonedPal;

    // 파트너 스킬 사용 중 여부
    UPROPERTY(ReplicatedUsing = OnRep_UsingPartnerSkill)
    bool bUsingPartnerSkill = false;
    
    UPROPERTY()
    ABaseMonster* PendingPal;
    
    bool bPendingDismiss = false;
    
    void ProcessPendingSummon();

    // Pal 찾기용 (디버그)
    void UpdateNearestEnemy();

    UFUNCTION(Server, Reliable)
    void Server_TogglePalSummon();

    UFUNCTION(Server, Reliable)
    void Server_TogglePartnerSkill();

    UFUNCTION(Server, Reliable)
    void Server_SetPartnerSkillTrigger(bool bTrigger);

    UFUNCTION(NetMulticast, Reliable)
    void MultiCast_PlaySummonAnimation();
    
    UFUNCTION(NetMulticast, Reliable)
    void MultiCast_SetPartnerSkillState(bool bIsUsing);
};
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "PalManagementComponent.generated.h"

class ABaseMonster;
class ASonheimPlayer;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPalRegistered, ABaseMonster*, Pal, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPalSwitched, int32, OldIndex, int32, NewIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPalSummoned, ABaseMonster*, Pal);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPalRecalled, ABaseMonster*, Pal);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SONHEIM_API UPalManagementComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UPalManagementComponent();

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// === 팰 등록/해제 ===
	UFUNCTION(BlueprintCallable, Category = "Pal Management")
	bool RegisterPal(ABaseMonster* NewPal);
	
	UFUNCTION(BlueprintCallable, Category = "Pal Management")
	bool UnregisterPal(int32 SlotIndex);
	
	UFUNCTION(BlueprintCallable, Category = "Pal Management")
	bool UnregisterPalByRef(ABaseMonster* Pal);

	// === 팰 소환/회수 ===
	UFUNCTION(BlueprintCallable, Category = "Pal Management")
	void SummonPal();
	
	UFUNCTION(BlueprintCallable, Category = "Pal Management")
	void RecallPal();
	
	UFUNCTION(BlueprintCallable, Category = "Pal Management")
	void TogglePalSummon();

	// === 팰 전환 ===
	UFUNCTION(BlueprintCallable, Category = "Pal Management")
	void SwitchPalSlot(int32 Direction);
	
	UFUNCTION(BlueprintCallable, Category = "Pal Management")
	void SelectPalSlot(int32 SlotIndex);

	// === 파트너 스킬 ===
	UFUNCTION(BlueprintCallable, Category = "Pal Management")
	void StartPartnerSkill();
	
	UFUNCTION(BlueprintCallable, Category = "Pal Management")
	void TriggerPartnerSkill(bool bTrigger);
	
	UFUNCTION(BlueprintCallable, Category = "Pal Management")
	void EndPartnerSkill();

	// === Getters ===
	UFUNCTION(BlueprintPure, Category = "Pal Management")
	ABaseMonster* GetSelectedPal() const { return m_SelectedPal; }
	
	UFUNCTION(BlueprintPure, Category = "Pal Management")
	ABaseMonster* GetSummonedPal() const { return m_SummonedPal; }
	
	UFUNCTION(BlueprintPure, Category = "Pal Management")
	TArray<ABaseMonster*> GetOwnedPals() const;
	
	UFUNCTION(BlueprintPure, Category = "Pal Management")
	int32 GetCurrentPalIndex() const { return CurrentPalIndex; }
	
	UFUNCTION(BlueprintPure, Category = "Pal Management")
	int32 GetMaxPalSlots() const { return MaxPalSlots; }
	
	UFUNCTION(BlueprintPure, Category = "Pal Management")
	bool IsUsingPartnerSkill() const { return bUsingPartnerSkill; }

	// === Events ===
	UPROPERTY(BlueprintAssignable, Category = "Pal Management")
	FOnPalRegistered OnPalRegistered;
	
	UPROPERTY(BlueprintAssignable, Category = "Pal Management")
	FOnPalSwitched OnPalSwitched;
	
	UPROPERTY(BlueprintAssignable, Category = "Pal Management")
	FOnPalSummoned OnPalSummoned;
	
	UPROPERTY(BlueprintAssignable, Category = "Pal Management")
	FOnPalRecalled OnPalRecalled;

protected:
	// === Server RPCs ===
	UFUNCTION(Server, Reliable)
	void Server_RegisterPal(ABaseMonster* NewPal);
	
	UFUNCTION(Server, Reliable)
	void Server_UnregisterPal(int32 SlotIndex);
	
	UFUNCTION(Server, Reliable)
	void Server_SummonPal();
	
	UFUNCTION(Server, Reliable)
	void Server_RecallPal();
	
	UFUNCTION(Server, Reliable)
	void Server_SwitchPalSlot(int32 Direction);
	
	UFUNCTION(Server, Reliable)
	void Server_StartPartnerSkill();
	
	UFUNCTION(Server, Reliable)
	void Server_TriggerPartnerSkill(bool bTrigger);
	
	UFUNCTION(Server, Reliable)
	void Server_EndPartnerSkill();

	// === Multicast RPCs ===
	UFUNCTION(NetMulticast, Reliable)
	void MultiCast_OnPalRegistered(ABaseMonster* Pal, int32 SlotIndex);
	
	UFUNCTION(NetMulticast, Reliable)
	void MultiCast_OnPalSwitched(int32 OldIndex, int32 NewIndex);
	
	UFUNCTION(NetMulticast, Reliable)
	void MultiCast_OnPalSummoned(ABaseMonster* Pal);
	
	UFUNCTION(NetMulticast, Reliable)
	void MultiCast_OnPalRecalled(ABaseMonster* Pal);

private:
	// === 내부 함수 ===
	void UpdateSelectedPal();
	int32 FindEmptySlot() const;
	bool IsValidSlotIndex(int32 Index) const;
	void SetPartnerSkillState(bool bActive);
	FVector GetSummonLocation() const;

private:
	// === 팰 저장소 ===
	UPROPERTY(Replicated, VisibleAnywhere, Category = "Pal Management")
	TArray<FPalSlot> OwnedPals;
	
	UPROPERTY(Replicated, VisibleAnywhere, Category = "Pal Management")
	ABaseMonster* m_SelectedPal;
	
	UPROPERTY(Replicated, VisibleAnywhere, Category = "Pal Management")
	ABaseMonster* m_SummonedPal;

	// === 설정 ===
	UPROPERTY(EditDefaultsOnly, Category = "Pal Management")
	int32 MaxPalSlots = 5;
	
	UPROPERTY(EditDefaultsOnly, Category = "Pal Management")
	float SummonDistance = 200.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Pal Management")
	UAnimMontage* SummonPalMontage;

	// === 상태 ===
	UPROPERTY(Replicated)
	int32 CurrentPalIndex = 0;
	
	UPROPERTY(Replicated)
	bool bUsingPartnerSkill = false;

	// === 참조 ===
	UPROPERTY()
	ASonheimPlayer* OwnerPlayer;
};
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "PalInventoryComponent.generated.h"

class ASonheimPlayerState;
class ABaseMonster;
class ASonheimPlayer;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPalAdded, ABaseMonster*, Pal, int32, Index);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPalRemoved, int32, Index);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSelectedPalChanged, int32, NewIndex);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SONHEIM_API UPalInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPalInventoryComponent();

	void InitializeWithPlayer(ASonheimPlayer* Player);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Pal 관리 함수들
	UFUNCTION(BlueprintCallable, Category = "Pal Inventory")
	bool AddPal(ABaseMonster* NewPal);

	UFUNCTION(BlueprintCallable, Category = "Pal Inventory")
	bool RemovePal(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Pal Inventory")
	ABaseMonster* GetPal(int32 Index) const;

	UFUNCTION(BlueprintCallable, Category = "Pal Inventory")
	ABaseMonster* GetSelectedPal() const;

	UFUNCTION(BlueprintCallable, Category = "Pal Inventory")
	void SwitchPalSlot(int32 Direction);

	UFUNCTION(Server, Reliable, Category = "Pal Inventory")
	void ServerRPC_SwitchPalSlot(int32 Direction);
	
	UFUNCTION(Client, Reliable, Category = "Pal Inventory")
	void ClientRPC_SwitchPalSlot(int32 Direction);


	UFUNCTION(BlueprintCallable, Category = "Pal Inventory")
	int32 GetOwnedPalCount() const { return OwnedPals.Num(); }

	UFUNCTION(BlueprintCallable, Category = "Pal Inventory")
	int32 GetCurrentPalIndex() const { return CurrentPalIndex; }

	UFUNCTION(BlueprintCallable, Category = "Pal Inventory")
	TArray<ABaseMonster*> GetAllOwnedPals() const { return OwnedPals; }

	// 델리게이트
	UPROPERTY(BlueprintAssignable)
	FOnPalAdded OnPalAdded;

	UPROPERTY(BlueprintAssignable)
	FOnPalRemoved OnPalRemoved;

	UPROPERTY(BlueprintAssignable)
	FOnSelectedPalChanged OnSelectedPalChanged;

	// 최대 Pal 수
	UPROPERTY(EditDefaultsOnly, Category = "Pal Inventory")
	int32 MaxPalCount = 5;

protected:
	virtual void BeginPlay() override;
	void BindDelegates();
	UFUNCTION()
	void HandlePalAdded(ABaseMonster* Pal, int32 Index);
	UFUNCTION()
	void HandlePalRemoved(int32 Index);
	UFUNCTION()
	void HandleSelectedPalChanged(int32 NewIndex);

	UFUNCTION()
	void OnRep_OwnedPals();
	UFUNCTION()
	void OnRep_CurrentPalIndex();

	void RefreshAllPalUI();

private:
	// 소유한 Pal 목록
	UPROPERTY(ReplicatedUsing = OnRep_OwnedPals)
	TArray<ABaseMonster*> OwnedPals;

	// 현재 선택된 Pal 인덱스
	UPROPERTY(ReplicatedUsing = OnRep_CurrentPalIndex)
	int32 CurrentPalIndex = 0;

	UPROPERTY()
	ASonheimPlayer* OwnerPlayer;
};

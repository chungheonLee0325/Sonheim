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

	// 초기 동기화 상태 확인용 (클라 UI가 초깃값 채우기 애니메이션을 억제할 때 사용)
	UFUNCTION(BlueprintPure, Category = "Pal Inventory")
	bool IsInitialSyncInProgress() const { return bInitialSyncInProgress; }

	UFUNCTION(BlueprintPure, Category = "Pal Inventory")
	bool IsRepOwnedPalsInitialized() const { return bRepOwnedPalsInitialized; }

	// Pal 관리 함수들
	UFUNCTION(BlueprintCallable, Category = "Pal Inventory")
	bool AddPal(ABaseMonster* NewPal);

	UFUNCTION(BlueprintCallable, Category = "Pal Inventory")
	bool RemovePal(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Pal Inventory")
	ABaseMonster* GetPal(int32 Index) const;

	UFUNCTION(BlueprintCallable, Category = "Pal Inventory")
	ABaseMonster* GetSelectedPal() const;

	// 현재 보유 목록에서 Pal의 인덱스를 찾기. 없으면 -1 반환
	UFUNCTION(BlueprintCallable, Category = "Pal Inventory")
	int32 FindPalIndex(ABaseMonster* Pal) const;

	UFUNCTION(BlueprintCallable, Category = "Pal Inventory")
	void SwitchPalSlot(int32 Direction);

	UFUNCTION(Server, Reliable, Category = "Pal Inventory")
	void ServerRPC_SwitchPalSlot(int32 Direction);


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

	UFUNCTION()
	void OnRep_OwnedPals();
	UFUNCTION()
	void OnRep_CurrentPalIndex();

private:
	// 소유한 Pal 목록
	UPROPERTY(ReplicatedUsing = OnRep_OwnedPals)
	TArray<ABaseMonster*> OwnedPals;

	// 현재 선택된 Pal 인덱스
	UPROPERTY(ReplicatedUsing = OnRep_CurrentPalIndex)
	int32 CurrentPalIndex = 0;

	UPROPERTY()
	ASonheimPlayer* OwnerPlayer;

	// 클라이언트에서 소유 팔 목록의 이전 상태를 추적하여 OnRep 시 변경분만 브로드캐스트
	TArray<TWeakObjectPtr<ABaseMonster>> PrevOwnedPals;
	bool bRepOwnedPalsInitialized = false;
	bool bInitialSyncInProgress = false;
};

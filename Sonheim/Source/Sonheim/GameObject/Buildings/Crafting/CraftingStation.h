#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"
#include "Sonheim/GameObject/InteractableInterface.h"
#include "CraftingStation.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class UWidgetComponent;
class UDataTable;
class UInventoryComponent;
class USoundBase;
class ASonheimPlayer;
class ASonheimPlayerController;
class UCraftingQueueWidget;
class UDetectWidget;

USTRUCT(BlueprintType)
struct FCraftingRecipe : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 ResultItemID = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 ResultCount = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 WorkRequired = 100;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<int32, int32> RequiredMaterials;

	// Build
	// 최종 액터 클래스. 설정 시 현장 액터가 공사 중/완료 상태 전환에 사용.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Build|Product")
	TSoftClassPtr<AActor> ProductActorClass;
	// 공사용 머티리얼. 설정 시 공사 중에 최종 액터 메시에 적용.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Build|Product")
	TSoftObjectPtr<UMaterialInterface> ConstructionMaterial;
};

// 단일 작업 상태(레시피 한 종류만 N개)
USTRUCT(BlueprintType)
struct FActiveCraftWork
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName RecipeRow;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 ResultItemID = 0;
	// 총 제작 개수
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 UnitsTotal = 0;
	// 완료된 개수
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 UnitsDone = 0;
	// 1개 완료 시 지급 수량
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 ResultPerUnit = 1;
	// 1개에 필요한 작업치
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 WorkPerUnit = 100;
	// 현재 유닛의 누적 작업치
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float WorkAccumulated = 0.f;
};

USTRUCT(BlueprintType)
struct FCompletedItem
{
	GENERATED_BODY()
	;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 ItemID = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Count = 0;
};

DECLARE_MULTICAST_DELEGATE(FOnWorkChanged);
DECLARE_MULTICAST_DELEGATE(FOnCompletedChanged);

UCLASS()
class SONHEIM_API ACraftingStation : public AActor, public IInteractableInterface
{
    GENERATED_BODY()

public:
    ACraftingStation();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Interactable
	virtual bool CanInteract_Implementation() const override;
	virtual void OnDetected_Implementation(bool bDetected) override;
	virtual void Interact_Implementation(ASonheimPlayer* Player) override;
	virtual FString GetInteractionName_Implementation() const override;
	virtual EInteractableType GetInteractableType_Implementation() const override { return EInteractableType::Object; }
	virtual float GetHoldDuration_Implementation() const override { return HoldToCraftDuration; }
	virtual void UpdateHoldProgressUI_Implementation(float Progress, EHoldPurpose Purpose) override;
	virtual bool CanHoldCancel_Implementation() const override { return bHasActiveWork; }
	virtual void ExecuteCancel_Implementation(ASonheimPlayer* Player) override;
	virtual float GetCancelHoldDuration_Implementation() const override { return 0.8f; }

	UFUNCTION(BlueprintCallable)
	void SetSelectedRecipe(FName Row) { SelectedRecipe = Row; }

	// ==== Server ==== 
	// 제작
	UFUNCTION(Server, Reliable)
	void ServerStartWork(ASonheimPlayer* Requestor, FName RecipeRow, int32 Units);
	// 진행
	UFUNCTION(Server, Reliable)
	void ServerAddWork(float WorkDelta, class ASonheimPlayer* Worker);
	// 취소
	UFUNCTION(Server, Reliable)
	void ServerCancelUnfinished(class ASonheimPlayer* Requestor);
	// 수령
	UFUNCTION(Server, Reliable)
	void ServerCollectAll(class ASonheimPlayer* Requestor);

	// UI
	UFUNCTION(Server, Reliable)
	void ServerRequestOpenUI(ASonheimPlayer* Player);
	UFUNCTION(Server, Reliable)
	void ServerReleaseUI(ASonheimPlayer* Player);

	// 작업 서버 틱 누적용
	UFUNCTION(Server, Reliable)
	void ServerStartAssist(ASonheimPlayer* Player);
	UFUNCTION(Server, Reliable)
	void ServerStopAssist(ASonheimPlayer* Player);

	// UI
	UFUNCTION(BlueprintPure)
	ASonheimPlayer* GetUIOwner() const { return UIOwner; }

    UFUNCTION(BlueprintCallable)
    float GetCurrentProgress() const;

    // 인터페이스: 상호작용 모드 코드
    virtual int32 GetInteractionModeCode_Implementation() const override;

	UFUNCTION(BlueprintPure, Category="Crafting|Data")
	UDataTable* GetRecipeTable() const { return RecipeTable; }

	// 활성화 작업
	UPROPERTY(ReplicatedUsing=OnRep_ActiveWork)
	FActiveCraftWork ActiveWork;
	// 활성화된 작업이 있는지
	UPROPERTY(Replicated)
	bool bHasActiveWork = false;
	// 완료/수령 대기 수량(해당 ResultItemID만 누적)
	UPROPERTY(ReplicatedUsing=OnRep_CompletedToCollect)
	int32 CompletedToCollect = 0;

	// Events
	FOnWorkChanged OnWorkChanged;
	FOnCompletedChanged OnCompletedChanged;

protected:
	UFUNCTION()
	void OnRep_ActiveWork();
	UFUNCTION()
	void OnRep_CompletedToCollect();

	// === Detect Widget ===
	void UpdateDetectWidgetText();
	UDetectWidget* GetDetectWidget() const;
	
	// 테이블로부터 레시피 조회
	const FCraftingRecipe* FindRecipe(FName Row) const;
	// 플레이어 작업속도 가져오기
	float ResolvePlayerWorkSpeed_Internal(ASonheimPlayer* Player) const;

	// Components
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* StationMesh;
	UPROPERTY(VisibleAnywhere)
	USphereComponent* InteractionSphere;

	// 디텍트 위젯
	UPROPERTY(VisibleAnywhere)
	UWidgetComponent* DetectWidget;
	UPROPERTY(EditDefaultsOnly, Category="Interaction")
	TSubclassOf<class UUserWidget> DetectWidgetClass;

	// 제작 큐 위젯
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UWidgetComponent* QueueWidgetComp = nullptr;
	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<class UCraftingQueueWidget> QueueWidgetClass;

	// Data
	UPROPERTY(EditAnywhere, Category="Crafting|Data")
	UDataTable* RecipeTable = nullptr;

	// Interaction
	UPROPERTY(EditAnywhere, Category="Crafting|Interaction")
	float HoldToCraftDuration = 0.8f;

	// Work
	UPROPERTY(EditAnywhere, Category="Crafting|Work")
	float DefaultWorkSpeed = 1.f;
	UPROPERTY(EditAnywhere, Category="Crafting|Work")
	float StationBaseWorkPerSecond = 0.f;

    // SFX
    UPROPERTY(EditDefaultsOnly, Category="Crafting|SFX")
    USoundBase* CraftWorkSFX = nullptr;
	// 제작 중 주기 재생 간격
    UPROPERTY(EditDefaultsOnly, Category="Crafting|SFX", meta=(ClampMin="0.05", ClampMax="5.0"))
    float CraftSFXInterval = 0.9f;

	// State
	UPROPERTY(Replicated, VisibleAnywhere, Category="Crafting|State")
	FName SelectedRecipe;

	UPROPERTY(Replicated, VisibleAnywhere, Category="Crafting|State")
	ASonheimPlayer* UIOwner = nullptr;

    // Server-only
    TSet<TWeakObjectPtr<ASonheimPlayer>> Assistants;
    bool bIsDetected = false;

private:
    // SFX control (작업 추가 시 1회 재생, 최소 간격 제한)
    float LastCraftSfxServerTime = -1000.f;
    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_PlayCraftSfx();
    void PlayCraftSfxOnce();

    // 자동수령 방지용: 최근 작업 추가 서버시간
    float LastWorkAddServerTime = -1000.f;

    // 최근 작업 직후 수령 허용까지의 최소 지연(초) — 홀드 중 자동수령 방지
    UPROPERTY(EditDefaultsOnly, Category="Crafting|Interaction", meta=(ClampMin="0.0", ClampMax="2.0"))
    float ManualCollectDelay = 0.35f;

    // 최근 수령 직후 UI 오픈까지의 최소 지연(초) — 수령→UI 같은 틱 연쇄 방지
    UPROPERTY(EditDefaultsOnly, Category="Crafting|Interaction", meta=(ClampMin="0.0", ClampMax="1.0"))
    float ManualOpenUIDelay = 0.1f;
    // 최근 수령 서버 시간
    float LastCollectServerTime = -1000.f;
};

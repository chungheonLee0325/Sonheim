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
class ASonheimPlayer;
class ASonheimPlayerController;
class UWidgetComponent;
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

	// 인벤토리로부터 제작 재료 소비
	static bool TryConsumeMaterialsForOne(UInventoryComponent* Inv, const FCraftingRecipe& R);
	// 최대 제작 가능 갯수 점검(서버 권위 검증용)
	static int32 ComputeMaxCraftable(UInventoryComponent* Inv, const FCraftingRecipe& R);
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

	// State
	UPROPERTY(Replicated, VisibleAnywhere, Category="Crafting|State")
	FName SelectedRecipe;

	UPROPERTY(Replicated, VisibleAnywhere, Category="Crafting|State")
	ASonheimPlayer* UIOwner = nullptr;

	// Server-only
	TSet<TWeakObjectPtr<ASonheimPlayer>> Assistants;
	bool bIsDetected = false;
};

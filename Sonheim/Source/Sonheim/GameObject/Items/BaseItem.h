// BaseItem.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Sonheim/AreaObject/Player/SonheimPlayer.h"
#include "Sonheim/GameObject/InteractableInterface.h"
#include "BaseItem.generated.h"

class USphereComponent;

// 아이템 상호작용 타입
UENUM(BlueprintType)
enum class EItemInteractionType : uint8
{
	Instant		UMETA(DisplayName = "Instant"),      // 즉시 획득
	Hold		UMETA(DisplayName = "Hold")           // 길게 눌러서 획득
};

// 아이템 생성 옵션 구조체
USTRUCT(BlueprintType)
struct FItemSpawnOptions
{
	GENERATED_BODY()

	// 상호작용 필요 여부
	UPROPERTY(BlueprintReadWrite)
	bool bRequireInteraction = false;

	// 상호작용 타입
	UPROPERTY(BlueprintReadWrite)
	EItemInteractionType InteractionType = EItemInteractionType::Instant;

	// 홀드 시간 (Hold 타입일 때만)
	UPROPERTY(BlueprintReadWrite)
	float HoldDuration = 1.0f;

	// 아이템 개수
	UPROPERTY(BlueprintReadWrite)
	int32 ItemCount = 1;

	// 자동 획득 지연 시간 (자원에서 드롭 시)
	UPROPERTY(BlueprintReadWrite)
	float AutoPickupDelay = 0.0f;

	// 드롭 시 물리 적용
	UPROPERTY(BlueprintReadWrite)
	bool bApplyPhysicsOnDrop = false;

	// 드롭 시 튕기는 힘
	UPROPERTY(BlueprintReadWrite)
	float DropForce = 300.0f;

	// 수명 (0이면 무한)
	UPROPERTY(BlueprintReadWrite)
	float LifeTime = 0.0f;
};

UCLASS()
class SONHEIM_API ABaseItem : public AActor, public IInteractableInterface
{
	GENERATED_BODY()

public:
	ABaseItem();

protected:
	// 초기화 메서드들
	UFUNCTION(BlueprintCallable, Category = "Item")
	void InitializeItem(int32 InItemID, const FItemSpawnOptions& Options);
public:
	UFUNCTION(BlueprintCallable, Category = "Item")
	void InitializeAsDroppedItem(int32 InItemID, int32 ItemValue, float DropDelay = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "Item")
	void InitializeAsInteractableItem(int32 InItemID, int32 ItemValue, EItemInteractionType Type = EItemInteractionType::Instant);

	UFUNCTION(BlueprintCallable, Category = "Item")
	void SetItemValue(int32 ItemValue) { m_ItemValue = ItemValue; }

	virtual void Tick(float DeltaTime) override;
	
	// IInteractableInterface 구현
	virtual bool CanInteract_Implementation() const override;
	virtual void OnDetected_Implementation(bool bDetected) override;
	virtual void Interact_Implementation(ASonheimPlayer* Player) override;
	virtual FString GetInteractionName_Implementation() const override;
	virtual EInteractableType GetInteractableType_Implementation() const override { return EInteractableType::Item; }
	virtual float GetHoldDuration_Implementation() const override { return InteractionType == EItemInteractionType::Hold ? HoldDuration : 0.0f; }

	// 상호작용 처리 함수들
	void UpdateHoldProgress(float Progress);

	// 네트워크 함수들
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnCollected();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnDropped(FVector DropLocation, FVector DropImpulse);

	// Getter
	bool CanBeCollectedBy(ASonheimPlayer* Player);
	USphereComponent* GetCollectionSphere() const { return CollectionSphere; }

	// Properties - Replicated
	UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadWrite, Category = "Data")
	int32 m_ItemID = 0;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	bool bRequireInteraction = false;

	UPROPERTY(Replicated, EditAnywhere, Category = "Interaction")
	EItemInteractionType InteractionType = EItemInteractionType::Instant;

	UPROPERTY(Replicated, EditAnywhere, Category = "Interaction", meta = (EditCondition = "InteractionType == EItemInteractionType::Hold"))
	float HoldDuration = 1.0f;

	// 데이터 테이블 참조
	FItemData* dt_ItemData = nullptr;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void ApplyEffect(class ASonheimPlayer* Player);
	virtual void OnCollected(ASonheimPlayer* Player);

	// 자동 획득 활성화
	UFUNCTION()
	void EnableAutoPickup();

	// 수명 종료
	UFUNCTION()
	void OnLifeTimeExpired();

	// 컴포넌트
	UPROPERTY(EditAnywhere, Category = "Collection")
	class UStaticMeshComponent* ItemMesh;

	UPROPERTY(EditAnywhere, Category = "Collection")
	class USphereComponent* CollectionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	class UWidgetComponent* DetectWidgetComponent;

	// 감지 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	TSubclassOf<UUserWidget> DetectWidgetClass;

	// 효과
	UPROPERTY(EditAnywhere, Category = "Collection")
	USoundBase* CollectionSound;

	UPROPERTY(EditAnywhere, Category = "Collection")
	UParticleSystem* CollectionEffect;

	// 하이라이트
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	UMaterialInterface* HighlightMaterial;

	// 아이템 속성
	UPROPERTY(Replicated, EditDefaultsOnly, Category = "Item")
	EItemRarity ItemRarity = EItemRarity::Common;

	// 오버랩 이벤트
	UFUNCTION()
	virtual void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent,
								AActor* OtherActor,
								UPrimitiveComponent* OtherComp,
								int32 OtherBodyIndex,
								bool bFromSweep,
								const FHitResult& SweepResult);

private:
	UPROPERTY(Replicated)
	bool m_IsCollected = false;
	
	UPROPERTY(Replicated)
	int32 m_ItemValue = 1;
	
	bool bIsDetected = false;

	// 타이머 핸들
	FTimerHandle AutoPickupTimerHandle;
	FTimerHandle LifeTimeTimerHandle;

	UPROPERTY()
	USonheimGameInstance* m_GameInstance;

	// 초기화 헬퍼 함수
	void SetupComponents();
};
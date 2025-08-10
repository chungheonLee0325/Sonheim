#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Sonheim/GameObject/InteractableInterface.h"
#include "Sonheim/GameObject/Buildings/Utility/ContainerComponent.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "BaseContainer.generated.h"

UCLASS()
class SONHEIM_API ABaseContainer : public AActor, public IInteractableInterface
{
	GENERATED_BODY()

public:
	ABaseContainer();

	UContainerComponent* GetContainerComponent() const {return ContainerComponent;};

	UFUNCTION(BlueprintCallable, Category = "Container")
	void CloseContainer();
	bool CanBeInteractedByPlayer(const ASonheimPlayer* Player) const;
	
protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// IInteractableInterface 구현
	virtual bool CanInteract_Implementation() const override;
	virtual void OnDetected_Implementation(bool bDetected) override;
	virtual void Interact_Implementation(class ASonheimPlayer* Player) override;
	virtual FString GetInteractionName_Implementation() const override;
	virtual EInteractableType GetInteractableType_Implementation() const override { return EInteractableType::Object; }
	virtual float GetHoldDuration_Implementation() const override;

	// 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Container")
	class UStaticMeshComponent* ContainerMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Container")
	class UContainerComponent* ContainerComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	class UWidgetComponent* DetectWidgetComponent;

	// 상자 설정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Container")
	int32 ContainerDataID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Container")
	int32 OverrideSlotCount = 0; // 0이면 데이터테이블 값 사용

	// 상태
	UPROPERTY(ReplicatedUsing = OnRep_IsOpen, BlueprintReadOnly, Category = "Container")
	bool bIsOpen = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Container")
	class ASonheimPlayer* CurrentUser = nullptr;

	UFUNCTION()
	void OnRep_IsOpen();

	// 상자 열기/닫기
	UFUNCTION(BlueprintCallable, Category = "Container")
	void OpenContainer(class ASonheimPlayer* Player);

private:
	UPROPERTY()
	class USonheimGameInstance* GameInstance;

	FContainerData* ContainerData;

	// 감지 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	TSubclassOf<class UUserWidget> DetectWidgetClass;

	bool bIsDetected = false;

	// 사운드
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	class USoundBase* DefaultOpenSound;

	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	class USoundBase* DefaultCloseSound;
};
#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DungeonClientBridgeComponent.generated.h"
UCLASS()
class SONHEIM_API UDungeonClientBridgeComponent : public UActorComponent
{
	GENERATED_BODY()
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};

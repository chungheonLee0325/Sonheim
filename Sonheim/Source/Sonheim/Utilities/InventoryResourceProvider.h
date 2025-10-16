#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "InventoryResourceProvider.generated.h"

class UInventoryComponent;

/**
 * 인벤토리 리소스 프로바이더: 재료 보유/소모 유틸
 */
UCLASS()
class SONHEIM_API UInventoryResourceProvider : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure, Category="Inventory|Resource")
    static bool HasItems(UInventoryComponent* Inv, const TMap<int32,int32>& Required);

    UFUNCTION(BlueprintCallable, Category="Inventory|Resource")
    static bool ConsumeItems(UInventoryComponent* Inv, const TMap<int32,int32>& Required);

    // 재료 맵 기준 최대 제작 가능 수량 계산
    UFUNCTION(BlueprintPure, Category="Inventory|Resource")
    static int32 ComputeMaxCraftable(UInventoryComponent* Inv, const TMap<int32,int32>& Required);
};

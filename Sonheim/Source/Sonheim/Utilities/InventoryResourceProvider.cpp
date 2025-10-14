#include "InventoryResourceProvider.h"

#include "Sonheim/AreaObject/Player/Utility/InventoryComponent.h"

bool UInventoryResourceProvider::HasItems(UInventoryComponent* Inv, const TMap<int32, int32>& Required)
{
    if (!Inv) return false;
    for (const auto& P : Required)
    {
        if (!Inv->HasItem(P.Key, P.Value)) return false;
    }
    return true;
}

bool UInventoryResourceProvider::ConsumeItems(UInventoryComponent* Inv, const TMap<int32, int32>& Required)
{
    if (!Inv) return false;
    // 선검증
    if (!HasItems(Inv, Required)) return false;
    for (const auto& P : Required)
    {
        Inv->RemoveItem(P.Key, P.Value);
    }
    return true;
}


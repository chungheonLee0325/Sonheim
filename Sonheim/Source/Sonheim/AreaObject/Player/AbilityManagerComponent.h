//#pragma once
//
//#include "CoreMinimal.h"
//#include "Components/ActorComponent.h"
//#include "Sonheim/ResourceManager/SonheimGameType.h"
//#include "AbilityManagerComponent.generated.h"
//
//// 특수 능력 열거형
//UENUM(BlueprintType)
//enum class ESpecialAbility : uint8
//{
//    None UMETA(DisplayName = "None"),
//    WaterWalking UMETA(DisplayName = "Water Walking"),
//    HighJump UMETA(DisplayName = "High Jump"),
//    Gliding UMETA(DisplayName = "Gliding"),
//    FastSwimming UMETA(DisplayName = "Fast Swimming"),
//    NightVision UMETA(DisplayName = "Night Vision"),
//    FireResistance UMETA(DisplayName = "Fire Resistance")
//};
//
//UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
//class SONHEIM_API UAbilityManagerComponent : public UActorComponent
//{
//    GENERATED_BODY()
//
//public:    
//    UAbilityManagerComponent();
//    
//    // 능력 활성화
//    UFUNCTION(BlueprintCallable, Category = "Abilities")
//    void AddAbility(ESpecialAbility Ability, int32 SourceID);
//    
//    // 능력 비활성화
//    UFUNCTION(BlueprintCallable, Category = "Abilities")
//    void RemoveAbility(ESpecialAbility Ability, int32 SourceID);
//    
//    // 능력 활성화 상태 확인
//    UFUNCTION(BlueprintPure, Category = "Abilities")
//    bool HasAbility(ESpecialAbility Ability) const;
//    
//    // 특정 소스에서 제공한 모든 능력 제거
//    UFUNCTION(BlueprintCallable, Category = "Abilities")
//    void RemoveAllAbilitiesFromSource(int32 SourceID);
//
//private:
//    // 활성화된 능력 및 소스 추적
//    TMap<ESpecialAbility, TArray<int32>> ActiveAbilities;
//}; 
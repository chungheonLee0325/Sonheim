#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Sonheim/UI/Widget/FloatingDamageWidget.h"
#include "FloatingDamagePool.generated.h"

class AFloatingDamageActor;

USTRUCT()
struct FDamageNumberRequest
{
    GENERATED_BODY()

    float Damage = 0.0f;
    FVector WorldLocation = FVector::ZeroVector;
    EFloatingOutLineDamageType WeakPointType = EFloatingOutLineDamageType::Normal;
    EFloatingTextDamageType ElementAttributeType = EFloatingTextDamageType::Normal;
    AActor* DamageCauser = nullptr;
    AActor* DamagedActor = nullptr;
    
    FDamageNumberRequest() {}
};

UCLASS()
class SONHEIM_API AFloatingDamagePool : public AActor
{
    GENERATED_BODY()

public:
    AFloatingDamagePool();

    // 싱글톤 접근
    static AFloatingDamagePool* GetInstance(UWorld* World);

    void ReturnToPool(AFloatingDamageActor* Actor);
    // 데미지 표시 요청
    void RequestDamageNumber(const FDamageNumberRequest& Request);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    // 풀 관리
    void InitializePool();
    AFloatingDamageActor* GetPooledActor();

    // 가시성 체크
    bool ShouldShowDamageForPlayer(const FVector& DamageLocation, APlayerController* PlayerController) const;

    UPROPERTY()
    TArray<AFloatingDamageActor*> Pool;

    UPROPERTY()
    TArray<AFloatingDamageActor*> ActiveActors;

    // 풀 설정
    UPROPERTY(EditDefaultsOnly, Category = "Pool Settings")
    int32 InitialPoolSize = 50;

    UPROPERTY(EditDefaultsOnly, Category = "Pool Settings")
    int32 MaxPoolSize = 200;

    // 가시성 설정
    UPROPERTY(EditDefaultsOnly, Category = "Visibility")
    float MaxVisibleDistance = 3000.0f;

    // 싱글톤 인스턴스
    static AFloatingDamagePool* Instance;

    UPROPERTY(EditDefaultsOnly, Category = "Widget")
    TSubclassOf<AFloatingDamageActor> FloatingDamageActorClass;
};
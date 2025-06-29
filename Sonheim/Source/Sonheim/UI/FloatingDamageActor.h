#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Sonheim/UI/Widget/FloatingDamageWidget.h"
#include "FloatingDamageActor.generated.h"

class AFloatingDamagePool;

UCLASS()
class SONHEIM_API AFloatingDamageActor : public AActor
{
    GENERATED_BODY()

public:
    AFloatingDamageActor();

    // 초기화 (풀에서 가져올 때 호출)
    void Initialize(float Damage, 
        EFloatingOutLineDamageType WeakPointType = EFloatingOutLineDamageType::Normal,  
        EFloatingTextDamageType ElementAttributeType = EFloatingTextDamageType::Normal, 
        float Duration = 2.0f, 
        float RiseSpeed = 10.0f,
        float ScaleMultiplier = 1.0f);

    // 풀로 반환하기 전 리셋
    void Reset();

    // 풀 설정
    void SetPool(AFloatingDamagePool* InPool) { Pool = InPool; }

protected:
    virtual void Tick(float DeltaTime) override;
    virtual void BeginPlay() override;

private:
    UPROPERTY(VisibleAnywhere)
    class UWidgetComponent* DamageWidget;

    // 풀 참조
    UPROPERTY()
    AFloatingDamagePool* Pool = nullptr;

    // 애니메이션 파라미터
    float LifeTime = 2.0f;
    float CurrentLifeTime = 0.0f;
    float MovementSpeed = 50.0f;
    float BaseScale = 1.0f;
    
    // 크리티컬/특수 효과용
    bool bIsCritical = false;
    float CriticalPulseSpeed = 10.0f;
    float CriticalPulseAmount = 0.2f;

    UPROPERTY(EditDefaultsOnly, Category = "Movement")
    float RandomOffsetRange = 30.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Movement")
    UCurveFloat* MovementCurve = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Critical")
    UCurveFloat* CriticalScaleCurve = nullptr;
    
    FVector MovementDirection;
    FVector InitialLocation;

    // 애니메이션 업데이트
    void UpdateMovement(float DeltaTime);
    void UpdateCriticalEffect(float DeltaTime);
    void ReturnToPool();
};
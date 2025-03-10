#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Sonheim/UI/Widget/FloatingDamageWidget.h"
#include "FloatingDamageActor.generated.h"

UCLASS()
class SONHEIM_API AFloatingDamageActor : public AActor
{
    GENERATED_BODY()

public:
    AFloatingDamageActor();

    void Initialize(float Damage, EFloatingOutLineDamageType WeakPointType = EFloatingOutLineDamageType::Normal,  EFloatingTextDamageType ElementAttributeType = EFloatingTextDamageType::Normal, 
                   float Duration = 2.0f, float RiseSpeed = 10.0f);

protected:
    virtual void Tick(float DeltaTime) override;

private:
    UPROPERTY(VisibleAnywhere)
    class UWidgetComponent* DamageWidget;

    float LifeTime = 10.0f;
    float CurrentLifeTime;
    float MovementSpeed = 50.f;

    UPROPERTY(EditDefaultsOnly, Category = "Movement")
    float RandomOffsetRange = 30.0f;  // 랜덤 오프셋 범위
    
    FVector MovementDirection;  // 이동 방향 (랜덤 오프셋 포함)
}; 
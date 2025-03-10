#include "FloatingDamageActor.h"
#include "Components/WidgetComponent.h"
#include "Sonheim/UI/Widget/FloatingDamageWidget.h"

AFloatingDamageActor::AFloatingDamageActor()
{
    PrimaryActorTick.bCanEverTick = true;

    DamageWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("DamageWidget"));
    RootComponent = DamageWidget;

    DamageWidget->SetWidgetSpace(EWidgetSpace::Screen);
    DamageWidget->SetDrawAtDesiredSize(true);

    // UI 클래스 설정
    static ConstructorHelpers::FClassFinder<UFloatingDamageWidget> WidgetClassFinder(TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/_BluePrint/Widget/WB_FloatingDamageWidget.WB_FloatingDamageWidget_C'"));
    if (WidgetClassFinder.Succeeded())
    {
        DamageWidget->SetWidgetClass(WidgetClassFinder.Class);
    }
}

void AFloatingDamageActor::Initialize(float Damage, EFloatingOutLineDamageType WeakPointType,  EFloatingTextDamageType ElementAttributeType, float Duration, float RiseSpeed)
{
    LifeTime = Duration;
    CurrentLifeTime = 0.0f;
    MovementSpeed = RiseSpeed;

    // 랜덤 오프셋 계산 및 적용
    float RandomX = FMath::RandRange(-RandomOffsetRange, RandomOffsetRange);
    float RandomY = FMath::RandRange(-RandomOffsetRange, RandomOffsetRange);
    float RandomZ = FMath::RandRange(-RandomOffsetRange, RandomOffsetRange);
    AddActorWorldOffset(FVector(RandomX, RandomY, RandomZ));
    
    MovementDirection = FVector(0,0,1);

    if (UFloatingDamageWidget* Widget = Cast<UFloatingDamageWidget>(DamageWidget->GetUserWidgetObject()))
    {
        Widget->SetDamageInfo(Damage, WeakPointType, ElementAttributeType);
        Widget->PlayFadeAnimation();
    }
}

void AFloatingDamageActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 설정된 방향으로 이동
    FVector NewLocation = GetActorLocation() + (MovementDirection * MovementSpeed * DeltaTime);
    SetActorLocation(NewLocation);

    CurrentLifeTime += DeltaTime;
    if (CurrentLifeTime >= LifeTime)
    {
        Destroy();
    }
} 
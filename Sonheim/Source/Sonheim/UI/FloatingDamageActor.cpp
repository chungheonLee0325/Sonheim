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
    static ConstructorHelpers::FClassFinder<UFloatingDamageWidget> WidgetClassFinder(TEXT("/Script/UMGEditor.WidgetBlueprint'/Game/_BluePrints/Widget/WB_FloatingDamageWidget.WB_FloatingDamageWidget_C'"));
    if (WidgetClassFinder.Succeeded())
    {
        DamageWidget->SetWidgetClass(WidgetClassFinder.Class);
    }
}

void AFloatingDamageActor::Initialize(float Damage, EFloatingDamageType DamageType, float Duration, float RiseSpeed)
{
    LifeTime = Duration;
    CurrentLifeTime = 0.0f;
    MovementSpeed = RiseSpeed;

    // 랜덤 오프셋 계산
    float RandomX = FMath::RandRange(-RandomOffsetRange, RandomOffsetRange);
    float RandomY = FMath::RandRange(-RandomOffsetRange, RandomOffsetRange);
    
    // 기본적으로 위로 향하는 방향에 랜덤성 추가
    MovementDirection = FVector(RandomX, RandomY, MovementSpeed).GetSafeNormal();

    if (UFloatingDamageWidget* Widget = Cast<UFloatingDamageWidget>(DamageWidget->GetUserWidgetObject()))
    {
        Widget->SetDamageInfo(Damage, DamageType);
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
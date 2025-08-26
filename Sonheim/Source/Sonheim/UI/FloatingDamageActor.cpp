#include "FloatingDamageActor.h"
#include "FloatingDamagePool.h"
#include "Components/WidgetComponent.h"
#include "Sonheim/UI/Widget/FloatingDamageWidget.h"
#include "Sonheim/Utilities/LogMacro.h"

AFloatingDamageActor::AFloatingDamageActor()
{
	PrimaryActorTick.bCanEverTick = true;

	DamageWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("DamageWidget"));
	RootComponent = DamageWidget;

	DamageWidget->SetWidgetSpace(EWidgetSpace::Screen);
	DamageWidget->SetDrawAtDesiredSize(true);

	// UI 클래스 설정 (하드코딩 유지)
	static ConstructorHelpers::FClassFinder<UFloatingDamageWidget> WidgetClassFinder(
		TEXT(
			"/Script/UMGEditor.WidgetBlueprint'/Game/_BluePrint/Widget/WB_FloatingDamageWidget.WB_FloatingDamageWidget_C'"));
	if (WidgetClassFinder.Succeeded())
	{
		DamageWidget->SetWidgetClass(WidgetClassFinder.Class);
	}

	// 네트워크 비활성화 (로컬 전용)
	SetReplicates(false);
}

void AFloatingDamageActor::BeginPlay()
{
	Super::BeginPlay();

	// 위젯 생성 강제 (풀링을 위해)
	if (DamageWidget && DamageWidget->GetWidgetClass())
	{
		DamageWidget->InitWidget();
	}
}

void AFloatingDamageActor::Initialize(float Damage,
                                      EFloatingOutLineDamageType WeakPointType,
                                      EFloatingTextDamageType ElementAttributeType,
                                      float Duration,
                                      float RiseSpeed,
                                      float ScaleMultiplier)
{
	LifeTime = Duration;
	CurrentLifeTime = 0.0f;
	MovementSpeed = RiseSpeed;
	BaseScale = ScaleMultiplier;

	// 랜덤 오프셋 계산
	float RandomX = FMath::RandRange(-RandomOffsetRange, RandomOffsetRange);
	float RandomY = FMath::RandRange(-RandomOffsetRange, RandomOffsetRange);
	float RandomZ = FMath::RandRange(0.f, RandomOffsetRange * 0.5f);

	InitialLocation = GetActorLocation();
	FVector OffsetLocation = InitialLocation + FVector(RandomX, RandomY, RandomZ);
	SetActorLocation(OffsetLocation);

	// 이동 방향 설정 (약간의 랜덤 각도 추가)
	float RandomAngle = FMath::RandRange(-15.0f, 15.0f);
	MovementDirection = FVector(
		FMath::Sin(FMath::DegreesToRadians(RandomAngle)),
		0,
		FMath::Cos(FMath::DegreesToRadians(RandomAngle))
	);

	// 크리티컬 여부 확인
	bIsCritical = (WeakPointType == EFloatingOutLineDamageType::CriticalDamaged ||
		WeakPointType == EFloatingOutLineDamageType::WeakPointDamage);

	// 위젯 업데이트
	if (UFloatingDamageWidget* Widget = Cast<UFloatingDamageWidget>(DamageWidget->GetUserWidgetObject()))
	{
		Widget->SetDamageInfo(Damage, WeakPointType, ElementAttributeType);

		Widget->StopAllAnimations();
		Widget->SetRenderOpacity(1.0f);

		// 초기화 시점문제로 간헐적으로 유실 문제 방지
		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(TimerHandle, [Widget](){ Widget->PlayFadeAnimation();}, 0.1f, false);

		// 크리티컬인 경우 초기 크기 증가
		if (bIsCritical)
		{
			Widget->SetRenderScale(FVector2D(BaseScale * 1.2f));
		}
		else
		{
			Widget->SetRenderScale(FVector2D(BaseScale));
		}
	}
}

void AFloatingDamageActor::Reset()
{
	CurrentLifeTime = 0.0f;
	bIsCritical = false;
	BaseScale = 1.0f;

	if (UFloatingDamageWidget* Widget = Cast<UFloatingDamageWidget>(DamageWidget->GetUserWidgetObject()))
	{
		Widget->SetRenderScale(FVector2D(1.0f));
		Widget->SetRenderOpacity(1.0f);
	}
}

void AFloatingDamageActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateMovement(DeltaTime);

	if (bIsCritical)
	{
		UpdateCriticalEffect(DeltaTime);
	}

	CurrentLifeTime += DeltaTime;
	if (CurrentLifeTime >= LifeTime)
	{
		ReturnToPool();
	}
}

void AFloatingDamageActor::UpdateMovement(float DeltaTime)
{
	FVector NewLocation = GetActorLocation();

	if (MovementCurve)
	{
		// 커브 기반 이동
		float Alpha = CurrentLifeTime / LifeTime;
		float CurveValue = MovementCurve->GetFloatValue(Alpha);
		NewLocation += MovementDirection * MovementSpeed * CurveValue * DeltaTime;
	}
	else
	{
		// 선형 이동 (점점 느려짐)
		float SpeedMultiplier = FMath::Lerp(1.0f, 0.2f, CurrentLifeTime / LifeTime);
		NewLocation += MovementDirection * MovementSpeed * SpeedMultiplier * DeltaTime;
	}

	SetActorLocation(NewLocation);
}

void AFloatingDamageActor::UpdateCriticalEffect(float DeltaTime)
{
	if (!DamageWidget) return;

	UFloatingDamageWidget* Widget = Cast<UFloatingDamageWidget>(DamageWidget->GetUserWidgetObject());
	if (!Widget) return;

	if (CriticalScaleCurve)
	{
		// 커브 기반 스케일 애니메이션
		float Alpha = CurrentLifeTime / LifeTime;
		float ScaleValue = CriticalScaleCurve->GetFloatValue(Alpha);
		Widget->SetRenderScale(FVector2D(BaseScale * ScaleValue));
	}
	else
	{
		// 기본 펄스 효과
		float PulseValue = FMath::Sin(CurrentLifeTime * CriticalPulseSpeed) * CriticalPulseAmount;
		float CurrentScale = BaseScale * (1.2f + PulseValue);
		Widget->SetRenderScale(FVector2D(CurrentScale));
	}
}

void AFloatingDamageActor::ReturnToPool()
{
	if (Pool)
	{
		Pool->ReturnToPool(this);
	}
	else
	{
		// 풀이 없으면 기존처럼 제거
		Destroy();
	}
}

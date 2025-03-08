// Fill out your copyright notice in the Description page of Project Settings.


#include "GhostTrail.h"
#include "Components/PoseableMeshComponent.h"


AGhostTrail::AGhostTrail()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	PoseableMesh = CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("POSEABLEMESH"));
	RootComponent = PoseableMesh;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> tempGhostTail(
		TEXT("/Script/Engine.Material'/Game/_Resource/Materials/GhostMaterial.GhostMaterial'"));
	if (tempGhostTail.Succeeded())
	{
		GhostMaterial = tempGhostTail.Object;
	}
}


void AGhostTrail::BeginPlay()
{
	Super::BeginPlay();
}

void AGhostTrail::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (IsSpawned)
	{
		if (!IsEnabled)
		{
			EnableCountDown -= DeltaTime;
			if (EnableCountDown <= 0.0f)
			{
				IsEnabled = true;
				// 초기 Opacity 설정
				for (auto* Material : DynamicMaterials)
				{
					if (Material)
					{
						Material->SetScalarParameterValue("Opacity", InitialOpacity);
					}
				}
			}
			else
			{
				// 활성화되기 전까지는 투명하게 유지
				for (auto* Material : DynamicMaterials)
				{
					if (Material)
					{
						Material->SetScalarParameterValue("Opacity", 0.0f);
					}
				}
			}
			return;
		}

		FadeCountDown -= DeltaTime;
		if (FadeCountDown < 0)
		{
			Destroy();
			return;
		}

		float CurrentOpacity = FMath::Max(FadeCountDown / FadeOutTime, 0.2f);
		
		// 모든 머티리얼의 투명도 업데이트
		for (auto* Material : DynamicMaterials)
		{
			if (Material)
			{
				Material->SetScalarParameterValue("Opacity", CurrentOpacity);
			}
		}
	}
}

void AGhostTrail::Init(USkeletalMeshComponent* Pawn, float FadeOutDuration, float EnableDelay)
{
	PoseableMesh->SetSkinnedAssetAndUpdate(Pawn->GetSkeletalMeshAsset());
	PoseableMesh->CopyPoseFromSkeletalComponent(Pawn);
	PoseableMesh->SetRelativeScale3D(Pawn->GetRelativeScale3D());

	int32 MatCount = Pawn->GetNumMaterials();
	for (int32 i = 0; i < MatCount; i++)
	{
		if (UMaterialInterface* OriginalMaterial = Pawn->GetMaterial(i))
		{
			UMaterialInstanceDynamic* DynMaterial = UMaterialInstanceDynamic::Create(OriginalMaterial, this);
			if (DynMaterial)
			{
				DynMaterial->SetScalarParameterValue("Opacity", 0.0f);  // 초기에는 투명하게 시작
				DynamicMaterials.Add(DynMaterial);
				PoseableMesh->SetMaterial(i, DynMaterial);
			}
		}
	}

	FadeOutTime = FadeOutDuration;
	FadeCountDown = FadeOutDuration;
	EnableCountDown = EnableDelay;
	IsSpawned = true;
	IsEnabled = false;
}

void AGhostTrail::InitByMaterials(USkeletalMeshComponent* Pawn, float FadeOutDuration, float EnableDelay)
{
	PoseableMesh->SetSkinnedAssetAndUpdate(Pawn->GetSkeletalMeshAsset());
	PoseableMesh->CopyPoseFromSkeletalComponent(Pawn);
	PoseableMesh->SetRelativeScale3D(Pawn->GetRelativeScale3D());

	// 기존 마테리얼 초기화
	int32 OriginMatCount = Pawn->GetNumMaterials();
	for (int32 i = 0; i < OriginMatCount; i++)
	{
		UMaterialInstanceDynamic* DynMaterial = UMaterialInstanceDynamic::Create(GhostMaterial, this);
		if (DynMaterial)
		{
			DynMaterial->SetScalarParameterValue("Opacity", 0.0f);  // 초기에는 투명하게 시작
			DynamicMaterials.Add(DynMaterial);
			PoseableMesh->SetMaterial(i, DynMaterial);
		}
	}

	FadeOutTime = FadeOutDuration;
	FadeCountDown = FadeOutDuration;
	EnableCountDown = EnableDelay;
	IsSpawned = true;
	IsEnabled = false;
}

/*
void AGhostTrail::InitByMaterials2(USkeletalMeshComponent* Pawn, const TArray<UMaterialInterface*>& Materials)
{
	PoseableMesh->SetSkinnedAssetAndUpdate(Pawn->GetSkeletalMeshAsset());
	PoseableMesh->CopyPoseFromSkeletalComponent(Pawn);
	PoseableMesh->SetRelativeScale3D(Pawn->GetRelativeScale3D());

	// 기존 마테리얼 초기화
	int32 OriginMatCount = Pawn->GetNumMaterials();
	for (int32 i = 0; i < OriginMatCount; i++)
	{
		PoseableMesh->SetMaterial(i, nullptr);
	}

	int32 MatCount = Materials.Num();
	for (int32 i = 0; i < MatCount; i++)
	{
		if (UMaterialInterface* OriginalMaterial = Materials[i])
		{
			// 원본 머티리얼의 동적 인스턴스 생성
			UMaterialInstanceDynamic* DynMaterial = UMaterialInstanceDynamic::Create(OriginalMaterial, this);
			if (DynMaterial)
			{
				DynMaterial->SetScalarParameterValue("Opacity", 0.7f);
				DynamicMaterials.Add(DynMaterial);
				PoseableMesh->SetMaterial(i, DynMaterial);
			}
		}
	}

	FadeCountDown = FadeOutTime;
	IsSpawned = true;
}
 */
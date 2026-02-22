#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SonheimVariantData.generated.h"

class UParticleSystem;
class USoundBase;
class UStaticMesh;
class UStringTable;
class UTexture2D;
class UUserWidget;

UCLASS(BlueprintType)
class SONHEIM_API USonheimAreaObjectVariantData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Variant")
	TSoftObjectPtr<UTexture2D> AreaObjectIcon = nullptr;
};

UCLASS(BlueprintType)
class SONHEIM_API USonheimItemVariantData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Variant")
	TSoftObjectPtr<UTexture2D> ItemIcon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Variant")
	TSoftObjectPtr<UStaticMesh> ItemMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Variant")
	FVector MeshScale = FVector(1.0f);
};

UCLASS(BlueprintType)
class SONHEIM_API USonheimResourceObjectVariantData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Variant")
	TSoftObjectPtr<UParticleSystem> HarvestEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Variant")
	TSoftObjectPtr<UParticleSystem> DestroyEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Variant")
	TSoftObjectPtr<UStaticMesh> ResourceMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Variant")
	FVector MeshScale = FVector(1.0f);
};

UCLASS(BlueprintType)
class SONHEIM_API USonheimContainerVariantData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Variant")
	TSoftObjectPtr<UStaticMesh> ContainerMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Variant")
	TSoftObjectPtr<USoundBase> OpenSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Variant")
	TSoftObjectPtr<USoundBase> CloseSound = nullptr;
};

UCLASS(BlueprintType)
class SONHEIM_API USonheimMonsterDexVariantData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Variant")
	TSoftObjectPtr<UTexture2D> Icon = nullptr;
};

UCLASS(BlueprintType)
class SONHEIM_API USonheimUIWidgetDefVariantData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Variant")
	TSoftClassPtr<UUserWidget> WidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Variant")
	bool bPreloadWidgetClass = false;
};

UCLASS(BlueprintType)
class SONHEIM_API USonheimUIPresetVariantData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Variant")
	TSoftObjectPtr<UStringTable> StringTableAsset = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Variant")
	FName DefaultTitleKey = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Variant")
	FName DefaultBodyKey = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Variant")
	FName PrimaryButtonKey = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Variant")
	FName SecondaryButtonKey = NAME_None;
};

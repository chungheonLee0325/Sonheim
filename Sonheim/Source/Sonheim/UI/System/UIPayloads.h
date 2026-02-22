#pragma once

#include "CoreMinimal.h"

#include "UIPayloads.generated.h"

USTRUCT(BlueprintType)
struct FModalPayload
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Body;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText PrimaryText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText SecondaryText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 IconItemID = 0;
};

USTRUCT(BlueprintType)
struct FToastPayload
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Text;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 IconItemID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="10.0"))
	float DurationSeconds = 2.0f;
};

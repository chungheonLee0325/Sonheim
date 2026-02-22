#include "MonsterDexRewardWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Sonheim/AreaObject/Player/Utility/MonsterDexComponent.h"

void UMonsterDexRewardWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (BtnClaim)
	{
		BtnClaim->OnClicked.AddDynamic(this, &UMonsterDexRewardWidget::HandleClaim);
	}
}

void UMonsterDexRewardWidget::Setup(UMonsterDexComponent* InDexComponent, const FMonsterDexRewardTierData& InData)
{
	DexComponent = InDexComponent;
	MonsterID = InData.MonsterID;
	TierIndex = InData.TierIndex;

	if (TxtRequirement)
	{
		TxtRequirement->SetText(FText::FromString(
			FString::Printf(TEXT("K:%d C:%d"), InData.RequiredKillCount, InData.RequiredCaptureCount)));
	}
	if (TxtReward)
	{
		TxtReward->SetText(InData.RewardText);
	}
	if (TxtStatus)
	{
		TxtStatus->SetText(InData.bClaimed ? FText::FromString(TEXT("Claimed")) :
			(InData.bCanClaim ? FText::FromString(TEXT("Ready")) : FText::GetEmpty()));
	}

	if (BtnClaim)
	{
		BtnClaim->SetIsEnabled(InData.bCanClaim && !InData.bClaimed);
	}

	ApplyRewardData(InData);
}

void UMonsterDexRewardWidget::HandleClaim()
{
	if (DexComponent.IsValid())
	{
		DexComponent->ServerClaimReward(MonsterID, TierIndex);
	}
}

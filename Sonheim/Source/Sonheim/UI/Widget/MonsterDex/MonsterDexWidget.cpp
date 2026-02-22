#include "MonsterDexWidget.h"

#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Sonheim/MonsterDex/MonsterDexData.h"
#include "Sonheim/AreaObject/Player/Utility/MonsterDexComponent.h"
#include "Sonheim/UI/Widget/MonsterDex/MonsterDexEntryWidget.h"
#include "Sonheim/UI/Widget/MonsterDex/MonsterDexRewardWidget.h"
#include "Sonheim/Utilities/LogMacro.h"
#include "Sonheim/Utilities/TableManagerHelper.h"

void UMonsterDexWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindTableManagerReady();
}

void UMonsterDexWidget::NativeDestruct()
{
	if (TableManager.IsValid() && TableReadyHandle.IsValid())
	{
		TableManager->OnReady().Remove(TableReadyHandle);
		TableReadyHandle.Reset();
	}

	if (DexComponent.IsValid())
	{
		DexComponent->OnDexChanged.RemoveDynamic(this, &UMonsterDexWidget::HandleDexChanged);
	}

	Super::NativeDestruct();
}

void UMonsterDexWidget::BindDexComponent(UMonsterDexComponent* InDexComponent)
{
	if (DexComponent.Get() == InDexComponent) return;

	if (DexComponent.IsValid())
	{
		DexComponent->OnDexChanged.RemoveDynamic(this, &UMonsterDexWidget::HandleDexChanged);
	}

	DexComponent = InDexComponent;

	if (DexComponent.IsValid())
	{
		DexComponent->OnDexChanged.AddDynamic(this, &UMonsterDexWidget::HandleDexChanged);
	}

	HandleDexChanged();
}

void UMonsterDexWidget::BindTableManagerReady()
{
	USonheimTableManagerSubsystem* NewTableManager = Sonheim::TableManager::Get(this);
	if (TableManager.Get() != NewTableManager)
	{
		if (TableManager.IsValid() && TableReadyHandle.IsValid())
		{
			TableManager->OnReady().Remove(TableReadyHandle);
		}

		TableManager = NewTableManager;
		TableReadyHandle.Reset();
	}

	if (!TableManager.IsValid() || TableManager->IsReady() || TableReadyHandle.IsValid())
	{
		return;
	}

	TableReadyHandle = TableManager->OnReady().AddUObject(this, &UMonsterDexWidget::HandleTableManagerReady);
}

void UMonsterDexWidget::HandleTableManagerReady()
{
	if (TableManager.IsValid() && TableReadyHandle.IsValid())
	{
		TableManager->OnReady().Remove(TableReadyHandle);
		TableReadyHandle.Reset();
	}

	HandleDexChanged();
}

void UMonsterDexWidget::HandleDexChanged()
{
	BindTableManagerReady();

	OnDexDataChanged();
	RebuildMonsterList();
	if (SelectedMonsterID != 0)
	{
		UpdateDetailPanel(SelectedMonsterID);
	}
}

void UMonsterDexWidget::RebuildMonsterList()
{
	if (!EntryWidgetClass)
	{
		EntryWidgetClass = LoadClass<UMonsterDexEntryWidget>(
			nullptr,
			TEXT("/Game/_BluePrint/Widget/MonsterDex/WBP_MonsterDexEntry.WBP_MonsterDexEntry_C"));
		if (!EntryWidgetClass)
		{
			UE_LOG(SONHEIM, Warning, TEXT("MonsterDexWidget: failed to resolve default entry widget class."));
		}
	}

	if (!MonsterListBox || !EntryWidgetClass || !DexComponent.IsValid())
	{
		return;
	}

	MonsterListBox->ClearChildren();

	int32 FirstId = 0;
	bool bHasSelected = false;

	BindTableManagerReady();
	USonheimTableManagerSubsystem* ActiveTableManager = TableManager.Get();
	if (!ActiveTableManager || !ActiveTableManager->IsReady())
	{
		return;
	}

	const TArray<FMonsterDexEntry>& Entries = DexComponent->GetDexEntries();
	auto FindEntryById = [&Entries](int32 Id) -> const FMonsterDexEntry*
	{
		for (const FMonsterDexEntry& E : Entries)
		{
			if (E.MonsterID == Id) return &E;
		}
		return nullptr;
	};

	const TMap<int32, FMonsterDexData>* DexMap = ActiveTableManager ? &ActiveTableManager->GetMonsterDexDataMap() : nullptr;
	if (DexMap && DexMap->Num() > 0)
	{
		for (const TPair<int32, FMonsterDexData>& Pair : *DexMap)
		{
			const int32 MonsterId = Pair.Key;
			const FMonsterDexData& Def = Pair.Value;

			const FMonsterDexEntry* Entry = FindEntryById(MonsterId);
			const int32 KillCount = Entry ? Entry->KillCount : 0;
			const int32 CaptureCount = Entry ? Entry->CaptureCount : 0;
			const bool bDiscovered = (KillCount > 0 || CaptureCount > 0);

			FText Name = Def.Name;
			if (Def.bHiddenUntilDiscovered && !bDiscovered)
			{
				Name = FText::FromString(TEXT("???"));
			}

			if (FirstId == 0)
			{
				FirstId = MonsterId;
			}

			FMonsterDexEntryData Data;
			Data.MonsterID = MonsterId;
			Data.Name = Name;
			Data.KillCount = KillCount;
			Data.CaptureCount = CaptureCount;
			Data.bDiscovered = bDiscovered;

			if (UMonsterDexEntryWidget* W = CreateWidget<UMonsterDexEntryWidget>(this, EntryWidgetClass))
			{
				W->Setup(Data);
				W->OnClicked.AddDynamic(this, &UMonsterDexWidget::HandleEntryClicked);
				MonsterListBox->AddChild(W);
			}

			if (SelectedMonsterID != 0 && SelectedMonsterID == MonsterId)
			{
				bHasSelected = true;
			}
		}
	}
	else
	{
		for (const FMonsterDexEntry& Entry : Entries)
		{
			if (FirstId == 0)
			{
				FirstId = Entry.MonsterID;
			}

			FText Name = FText::FromString(TEXT("Unknown"));
			bool bDiscovered = (Entry.KillCount > 0 || Entry.CaptureCount > 0);

			if (ActiveTableManager)
			{
				if (const FMonsterDexData* Def = ActiveTableManager->FindMonsterDex(Entry.MonsterID))
				{
					if (!Def->bHiddenUntilDiscovered || bDiscovered)
					{
						Name = Def->Name;
					}
					else
					{
						Name = FText::FromString(TEXT("???"));
					}
				}
			}

			FMonsterDexEntryData Data;
			Data.MonsterID = Entry.MonsterID;
			Data.Name = Name;
			Data.KillCount = Entry.KillCount;
			Data.CaptureCount = Entry.CaptureCount;
			Data.bDiscovered = bDiscovered;

			if (UMonsterDexEntryWidget* W = CreateWidget<UMonsterDexEntryWidget>(this, EntryWidgetClass))
			{
				W->Setup(Data);
				W->OnClicked.AddDynamic(this, &UMonsterDexWidget::HandleEntryClicked);
				MonsterListBox->AddChild(W);
			}

			if (SelectedMonsterID != 0 && SelectedMonsterID == Entry.MonsterID)
			{
				bHasSelected = true;
			}
		}
	}

	if (!bHasSelected)
	{
		SelectedMonsterID = FirstId;
	}

	if (SelectedMonsterID != 0)
	{
		UpdateDetailPanel(SelectedMonsterID);
	}
}

static FString BuildRewardSummary(const FRewardDef& Reward)
{
	FString Out;
	if (Reward.Exp > 0)
	{
		Out += FString::Printf(TEXT("Exp +%d "), Reward.Exp);
	}
	for (const FRewardItem& Item : Reward.Items)
	{
		Out += FString::Printf(TEXT("Item %d x%d "), Item.ItemID, Item.Count);
	}
	return Out;
}

void UMonsterDexWidget::UpdateDetailPanel(int32 MonsterID)
{
	if (!DexComponent.IsValid()) return;

	const FMonsterDexEntry* Entry = nullptr;
	for (const FMonsterDexEntry& E : DexComponent->GetDexEntries())
	{
		if (E.MonsterID == MonsterID)
		{
			Entry = &E;
			break;
		}
	}
	if (!Entry) return;

	BindTableManagerReady();
	USonheimTableManagerSubsystem* ActiveTableManager = TableManager.Get();
	if (!ActiveTableManager || !ActiveTableManager->IsReady())
	{
		return;
	}
	const FMonsterDexData* Def = ActiveTableManager ? ActiveTableManager->FindMonsterDex(MonsterID) : nullptr;

	if (TxtDetailTitle)
	{
		TxtDetailTitle->SetText(Def ? Def->Name : FText::FromString(TEXT("Unknown")));
	}
	if (TxtDetailDescription)
	{
		TxtDetailDescription->SetText(Def ? Def->Description : FText::GetEmpty());
	}
	if (TxtDetailCounts)
	{
		TxtDetailCounts->SetText(FText::FromString(
			FString::Printf(TEXT("Kills: %d  Captures: %d"), Entry->KillCount, Entry->CaptureCount)));
	}

	if (TxtDetailRewards && Def)
	{
		FString Lines;
		for (int32 i = 0; i < Def->RewardTiers.Num(); ++i)
		{
			const FMonsterDexRewardTier& Tier = Def->RewardTiers[i];
			const FRewardDef* RewardDef = nullptr;
			if (Tier.RewardTableID > 0 && ActiveTableManager)
			{
				RewardDef = ActiveTableManager->FindReward(Tier.RewardTableID);
			}
			if (!RewardDef)
			{
				RewardDef = &Tier.Reward;
			}
			const FString Summary = RewardDef ? BuildRewardSummary(*RewardDef) : FString(TEXT("Reward"));
			Lines += FString::Printf(TEXT("K%d C%d : %s\n"),
				Tier.RequiredKillCount, Tier.RequiredCaptureCount, *Summary);
		}
		TxtDetailRewards->SetText(FText::FromString(Lines));
	}

	RebuildRewardList(MonsterID);
}

void UMonsterDexWidget::RebuildRewardList(int32 MonsterID)
{
	if (!RewardWidgetClass)
	{
		RewardWidgetClass = LoadClass<UMonsterDexRewardWidget>(
			nullptr,
			TEXT("/Game/_BluePrint/Widget/MonsterDex/WBP_MonsterDexReward.WBP_MonsterDexReward_C"));
		if (!RewardWidgetClass)
		{
			UE_LOG(SONHEIM, Warning, TEXT("MonsterDexWidget: failed to resolve default reward widget class."));
		}
	}

	if (!RewardListBox || !RewardWidgetClass || !DexComponent.IsValid())
	{
		return;
	}

	RewardListBox->ClearChildren();

	BindTableManagerReady();
	USonheimTableManagerSubsystem* ActiveTableManager = TableManager.Get();
	if (!ActiveTableManager || !ActiveTableManager->IsReady())
	{
		return;
	}
	const FMonsterDexData* Def = ActiveTableManager ? ActiveTableManager->FindMonsterDex(MonsterID) : nullptr;
	if (!Def) return;

	const FMonsterDexEntry* Entry = nullptr;
	for (const FMonsterDexEntry& E : DexComponent->GetDexEntries())
	{
		if (E.MonsterID == MonsterID)
		{
			Entry = &E;
			break;
		}
	}
	if (!Entry) return;

	for (int32 i = 0; i < Def->RewardTiers.Num(); ++i)
	{
		const FMonsterDexRewardTier& Tier = Def->RewardTiers[i];
		const bool bClaimed = Entry->IsRewardClaimed(i);
		const bool bCanClaim = DexComponent->CanClaimReward(MonsterID, i);

		const FRewardDef* RewardDef = nullptr;
		if (Tier.RewardTableID > 0 && ActiveTableManager)
		{
			RewardDef = ActiveTableManager->FindReward(Tier.RewardTableID);
		}
		if (!RewardDef)
		{
			RewardDef = &Tier.Reward;
		}

		FMonsterDexRewardTierData Data;
		Data.MonsterID = MonsterID;
		Data.TierIndex = i;
		Data.RequiredKillCount = Tier.RequiredKillCount;
		Data.RequiredCaptureCount = Tier.RequiredCaptureCount;
		Data.RewardText = FText::FromString(RewardDef ? BuildRewardSummary(*RewardDef) : FString(TEXT("Reward")));
		Data.bClaimed = bClaimed;
		Data.bCanClaim = bCanClaim;

		if (UMonsterDexRewardWidget* W = CreateWidget<UMonsterDexRewardWidget>(this, RewardWidgetClass))
		{
			W->Setup(DexComponent.Get(), Data);
			RewardListBox->AddChild(W);
		}
	}
}

void UMonsterDexWidget::SelectMonsterInternal(int32 MonsterID)
{
	if (SelectedMonsterID == MonsterID) return;
	SelectedMonsterID = MonsterID;
	UpdateDetailPanel(MonsterID);
	OnMonsterSelected(MonsterID);
}

void UMonsterDexWidget::HandleEntryClicked(int32 MonsterID)
{
	SelectMonsterInternal(MonsterID);
}

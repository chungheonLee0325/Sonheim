#include "DungeonStagePresenter.h"
#include "DungeonUIRouterSubsystem.h"
#include "DungeonViewData.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Sonheim/GameManager/SonheimGameState.h"
#include "Sonheim/GameManager/Dungeon/DungeonAssetSubsystem.h"
#include "Sonheim/GameObject/Dungeon/DungeonDefinitionDataAsset.h"
#include "Sonheim/GameObject/Dungeon/DungeonPresentationDataAsset.h"

void UDungeonStagePresenter::Start(APlayerController* Controller, UDungeonUIRouterSubsystem* Router)
{
	Stop(); Owner = Controller; UIRouter = Router;
	if (!Controller || !Controller->IsLocalController()) return;
	Assets = Controller->GetGameInstance()->GetSubsystem<UDungeonAssetSubsystem>();
	WorldHandle = Controller->GetWorld()->GameStateSetEvent.AddUObject(this, &UDungeonStagePresenter::BindGameState);
	BindGameState(Controller->GetWorld()->GetGameState());
}
void UDungeonStagePresenter::Stop()
{
	if (Owner.IsValid() && Owner->GetWorld()) Owner->GetWorld()->GameStateSetEvent.Remove(WorldHandle);
	if (GameState.IsValid()) GameState->OnDungeonStageStateChanged.Remove(StateHandle);
	if (Assets.IsValid()) Assets->Release(AssetRequest);
	AssetRequest.Invalidate(); RequestedDefinition = FPrimaryAssetId(); Definition = nullptr;
	GameState.Reset(); Owner.Reset();
}
void UDungeonStagePresenter::BindGameState(AGameStateBase* State)
{
	if (GameState.IsValid()) GameState->OnDungeonStageStateChanged.Remove(StateHandle);
	GameState = Cast<ASonheimGameState>(State);
	if (GameState.IsValid())
	{
		StateHandle = GameState->OnDungeonStageStateChanged.AddUObject(this, &UDungeonStagePresenter::OnSnapshot);
		OnSnapshot(GameState->GetDungeonStageState());
	}
}
void UDungeonStagePresenter::OnSnapshot(const FDungeonStageRuntimeState& Snapshot)
{
	Latest = Snapshot;
	if (RequestedDefinition != Snapshot.DefinitionAssetId && Assets.IsValid())
	{
		Assets->Release(AssetRequest); Definition = nullptr;
		RequestedDefinition = Snapshot.DefinitionAssetId;
		if (RequestedDefinition.IsValid())
		{
			const auto Expected = RequestedDefinition;
			AssetRequest = Assets->LoadDefinition(Expected, [Weak = TWeakObjectPtr<UDungeonStagePresenter>(this), Expected](UDungeonDefinitionDataAsset* Loaded, const FString&)
			{
				if (auto* Self = Weak.Get(); Self && Self->RequestedDefinition == Expected)
				{ Self->Definition = Loaded; Self->Present(); }
			}, false);
		}
	}
	Present();
}
void UDungeonStagePresenter::Present()
{
	auto* Router = UIRouter.Get();
	if (!Router) return;
	FDungeonStageViewData View;
	View.Status = Latest.RunStatus; View.Revision = Latest.Revision;
	// A run has no stage while it loads and no branch before the branch point; FText::FromName would show "None" for both.
	View.Title = Latest.StageId.IsNone() ? FText::GetEmpty() : FText::FromName(Latest.StageId);
	View.Objective = FText::FromString(Latest.RunStatus == EDungeonRunStatus::Loading ? TEXT("던전을 불러오는 중") : TEXT("추적 중인 적을 처치하세요."));
	View.CountText = FText::FromString(FString::Printf(TEXT("%d / %d"), Latest.CurrentCount, Latest.RequiredCount));
	View.Progress = Latest.RequiredCount > 0 ? float(Latest.CurrentCount) / Latest.RequiredCount : 0.f;
	View.BranchText = Latest.SelectedBranchId.IsNone() ? FText::GetEmpty() : FText::FromName(Latest.SelectedBranchId);
	if (const auto* Presentation = Definition ? Definition->Presentation.Get() : nullptr)
	{
		if (Latest.StageId.IsNone()) View.Title = Presentation->DungeonTitle;
		if (const auto* Stage = Presentation->Stages.FindByPredicate([this](const auto& Item) { return Item.StageId == Latest.StageId; }))
		{ View.Title = Stage->Title; View.Objective = Stage->Objective; }
		if (const FText* Branch = Presentation->BranchLabels.Find(Latest.SelectedBranchId)) View.BranchText = *Branch;
	}
	if (Latest.RunStatus == EDungeonRunStatus::Failed) { View.Title = FText::FromString(TEXT("던전 실패")); View.Objective = FText::FromString(TEXT("입구로 돌아가 다시 시작하세요.")); }
	Router->ApplyView(View);
}

#include "ActionGameGameState.h"

#include "Net/UnrealNetwork.h"
#include "Engine/World.h"

AActionGameGameState::AActionGameGameState()
{
	ElapsedSurvivalTime = 0.f;
	DifficultyStage = 0;
	StageDuration = 60.f;

	// GameState 本身默认就是 replicated 的体系成员，
	// 显式打开仅为可读性
	bReplicates = true;
}

// 服务器权威设置当前生存时间，并按规则重算阶段
void AActionGameGameState::SetElapsedSurvivalTime(float InElapsedTime)
{
	if (!HasAuthority())
	{
		return;
	}

	ElapsedSurvivalTime = FMath::Max(0.f, InElapsedTime);
	RefreshDifficultyStageFromTime();
}

// 服务器权威设置当前难度阶段（一般调试/特殊流程时用）
void AActionGameGameState::SetDifficultyStage(int32 InStage)
{
	if (!HasAuthority())
	{
		return;
	}

	DifficultyStage = FMath::Max(0, InStage);
}

// 根据当前生存时间，重算难度阶段
void AActionGameGameState::RefreshDifficultyStageFromTime()
{
	if (!HasAuthority())
	{
		return;
	}

	const float SafeStageDuration = FMath::Max(1.f, StageDuration);
	const int32 NewStage = FMath::Max(0, FMath::FloorToInt(ElapsedSurvivalTime / SafeStageDuration));

	if (DifficultyStage != NewStage)
	{
		DifficultyStage = NewStage;

		UE_LOG(LogTemp, Log,
			TEXT("[GameState] DifficultyStage updated: %d (ElapsedSurvivalTime=%.1f, StageDuration=%.1f)"),
			DifficultyStage,
			ElapsedSurvivalTime,
			SafeStageDuration);
	}
}

// 仅供客户端调用，生存时间更新时的回调
void AActionGameGameState::OnRep_ElapsedSurvivalTime()
{
	UE_LOG(LogTemp, Verbose,
		TEXT("[GameState] OnRep_ElapsedSurvivalTime: %.1f"),
		ElapsedSurvivalTime);
}

// 仅供客户端调用，难度阶段更新时的回调
void AActionGameGameState::OnRep_DifficultyStage()
{
	UE_LOG(LogTemp, Log,
		TEXT("[GameState] OnRep_DifficultyStage: %d"),
		DifficultyStage);
}

// 注册需要复制的属性
void AActionGameGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AActionGameGameState, ElapsedSurvivalTime);
	DOREPLIFETIME(AActionGameGameState, DifficultyStage);
}
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "HWGameModeBase.generated.h"

enum class EHWGameStateProgress : uint8;
class AHWPlayerController;

/**
 * 
 */
UCLASS()
class UNREALHW09_API AHWGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

	virtual void PreLogin(const FString& Options, const FString& Address,  const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual void OnPostLogin(AController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	void StartGame();
	void ChangePlayerTurn();
	void CountdownTurnTime();

	FString GenerateSecretNumber();

	bool IsGuessNumberString(const FString& InNumberString);

	FString JudgeResult(const FString& InSecretNumberString, const FString& InGuessNumberString);

	void PrintChatMessageString(AHWPlayerController* InChattingPlayerController, const FString& InChatMessageString);

	void IncreaseGuessCount(AHWPlayerController* InChattingPlayerController);

	void ResetGame();

	void JudgeGame(AHWPlayerController* InChattingPlayerController, int InStrikeCount);

	bool CheckForDrawCondition();

	void EndRoundAndRestart(const FString& RoundEndMessage);

	void UpdateGameInfo(EHWGameStateProgress NewState, const FString& InResultMessage = TEXT(""));

	void PrepareNewRound();
	
protected:

	FString SecretNumberString;

	TArray<TObjectPtr<AHWPlayerController>> AllPlayerControllers;

	UPROPERTY(EditDefaultsOnly, Category = "GameRule")
	float TurnTimeLimit = 10.0f; 

	UPROPERTY(EditDefaultsOnly, Category = "GameRule")
	int32 NumPlayersToStart = 2;

	UPROPERTY(EditDefaultsOnly, Category = "GameRule")
	int32 MaxAllowedPlayers = 2;
	
	int32 CurrentPlayerTurnIndex;
	
	FTimerHandle TurnTimerHandle;

	FTimerHandle RestartRoundTimerHandle;
};
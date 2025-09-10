// Fill out your copyright notice in the Description page of Project Settings.


#include "HWGameModeBase.h"

#include "EngineUtils.h"
#include "HWGameStateBase.h"
#include "Player/HWPlayerController.h"
#include "Player/HWPlayerState.h"

void AHWGameModeBase::BeginPlay()
{
	Super::BeginPlay();
	SecretNumberString = GenerateSecretNumber();

	UpdateGameInfo(EHWGameStateProgress::WaitingToStart);
}

void AHWGameModeBase::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);

	if (!ErrorMessage.IsEmpty())
	{
		return;
	}

	int32 CurrentPlayerCount = 0;
	for (const auto& PlayerController : AllPlayerControllers)
	{
		if (IsValid(PlayerController) && IsValid(PlayerController->GetPawn()))
		{
			CurrentPlayerCount++;
		}
	}

	if (CurrentPlayerCount >= MaxAllowedPlayers)
	{
		ErrorMessage = TEXT("Server is full. Maximum players reached.");
		UE_LOG(LogTemp, Warning, TEXT("Connection rejected: Server full (%d/%d)"), 
			   CurrentPlayerCount, MaxAllowedPlayers);
		return;
	}

	AHWGameStateBase* HWGameState = GetGameState<AHWGameStateBase>();
	if (HWGameState && HWGameState->GetCurrentGameInfo().CurrentGameStateProgress == EHWGameStateProgress::InProgress)
	{
		ErrorMessage = TEXT("Cannot join. Game is already in progress.");
		UE_LOG(LogTemp, Warning, TEXT("Connection rejected: Game in progress"));
	}
}

void AHWGameModeBase::OnPostLogin(AController* NewPlayer)
{
	Super::OnPostLogin(NewPlayer);
	
	AHWPlayerController* HWPlayerController = Cast<AHWPlayerController>(NewPlayer);
	if (IsValid(HWPlayerController) == true)
	{
		AllPlayerControllers.Add(HWPlayerController);

		AHWPlayerState* HWPS = HWPlayerController->GetPlayerState<AHWPlayerState>();
		if (IsValid(HWPS) == true)
		{
			HWPS->PlayerNameString = TEXT("Player") + FString::FromInt(AllPlayerControllers.Num());
		}

		AHWGameStateBase* HWGameStateBase =  GetGameState<AHWGameStateBase>();
		if (IsValid(HWGameStateBase) == true)
		{
			HWGameStateBase->MulticastRPCBroadcastLoginMessage(HWPS->PlayerNameString);
		}

		if (AllPlayerControllers.Num() >= NumPlayersToStart)
		{
			StartGame();
		}
	}
}

void AHWGameModeBase::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
	AHWPlayerController* HWPlayerController = Cast<AHWPlayerController>(Exiting);
	if (HWPlayerController)
	{
		bool bWasCurrentTurn = false;
		if (AllPlayerControllers.IsValidIndex(CurrentPlayerTurnIndex) && AllPlayerControllers[CurrentPlayerTurnIndex] == HWPlayerController)
		{
			bWasCurrentTurn = true;
		}

		AllPlayerControllers.Remove(HWPlayerController);

		if (bWasCurrentTurn && AllPlayerControllers.Num() > 0)
		{
			CurrentPlayerTurnIndex--; 
			ChangePlayerTurn();
		}
	}
}

void AHWGameModeBase::StartGame()
{
	CurrentPlayerTurnIndex = -1;
	UpdateGameInfo(EHWGameStateProgress::InProgress);
	ChangePlayerTurn();
}

void AHWGameModeBase::ChangePlayerTurn()
{
	GetWorld()->GetTimerManager().ClearTimer(TurnTimerHandle);
	
	CurrentPlayerTurnIndex = (CurrentPlayerTurnIndex + 1) % AllPlayerControllers.Num();

	AHWPlayerController* CurrentPC = AllPlayerControllers[CurrentPlayerTurnIndex];
	if (CurrentPC)
	{
		AHWPlayerState* CurrentPS = CurrentPC->GetPlayerState<AHWPlayerState>();
		AHWGameStateBase* HWGameState = GetGameState<AHWGameStateBase>();

		if (CurrentPS && HWGameState)
		{
			HWGameState->CurrentTurnPlayerState = CurrentPS;
			HWGameState->TurnRemainingTime = TurnTimeLimit;

			GetWorld()->GetTimerManager().SetTimer(TurnTimerHandle, this, &AHWGameModeBase::CountdownTurnTime, 1.0f, true);
		}
	}
}

void AHWGameModeBase::CountdownTurnTime()
{
	AHWGameStateBase* HWGameState = GetGameState<AHWGameStateBase>();
	if (HWGameState && HWGameState->TurnRemainingTime > 0)
	{
		HWGameState->TurnRemainingTime -= 1.0f;
		if (HWGameState->TurnRemainingTime <= 0)
		{
			AHWPlayerController* CurrentPC = AllPlayerControllers[CurrentPlayerTurnIndex];
			AHWPlayerState* CurrentPS = IsValid(CurrentPC) ? CurrentPC->GetPlayerState<AHWPlayerState>() : nullptr;

			if (CurrentPS)
			{
				IncreaseGuessCount(CurrentPC);

				FString TimeoutMessage = CurrentPS->GetPlayerInfoString() + TEXT(" ran out of time!");
				for (const auto& PlayerController : AllPlayerControllers)
				{
					if (IsValid(PlayerController))
					{
						PlayerController->ClientRPCPrintChatMessageString(TimeoutMessage);
					}
				}

				if (CheckForDrawCondition())
				{
					EndRoundAndRestart(TEXT("Draw..."));
				}
				else
				{
					ChangePlayerTurn();
				}
			}
		}
	}
}

FString AHWGameModeBase::GenerateSecretNumber()
{
	TArray<int32> Numbers;
	for (int32 i = 1; i <= 9; ++i)
	{
		Numbers.Add(i);
	}

	FMath::RandInit(FDateTime::Now().GetTicks());
	Numbers = Numbers.FilterByPredicate([](int32 Num) { return Num > 0; });
	
	FString Result;
	for (int32 i = 0; i < 3; ++i)
	{
		int32 Index = FMath::RandRange(0, Numbers.Num() - 1);
		Result.Append(FString::FromInt(Numbers[Index]));
		Numbers.RemoveAt(Index);
	}

	return Result;
}

bool AHWGameModeBase::IsGuessNumberString(const FString& InNumberString)
{
	bool bCanPlay = false;

	do {
		if (InNumberString.Len() != 3)
		{
			break;
		}

		bool bIsUnique = true;
		TSet<TCHAR> UniqueDigits;
		for (TCHAR C : InNumberString)
		{
			if (FChar::IsDigit(C) == false || C == '0')
			{
				bIsUnique = false;
				break;
			}
			
			UniqueDigits.Add(C);
		}

		if (bIsUnique == false)
		{
			break;
		}

		bCanPlay = true;
		
	} while (false);	

	return bCanPlay;
}

FString AHWGameModeBase::JudgeResult(const FString& InSecretNumberString, const FString& InGuessNumberString)
{
	int32 StrikeCount = 0, BallCount = 0;

	for (int32 i = 0; i < 3; ++i)
	{
		if (InSecretNumberString[i] == InGuessNumberString[i])
		{
			StrikeCount++;
		}
		else 
		{
			FString PlayerGuessChar = FString::Printf(TEXT("%c"), InGuessNumberString[i]);
			if (InSecretNumberString.Contains(PlayerGuessChar))
			{
				BallCount++;				
			}
		}
	}

	if (StrikeCount == 0 && BallCount == 0)
	{
		return TEXT("OUT");
	}

	return FString::Printf(TEXT("%dS%dB"), StrikeCount, BallCount);
}

void AHWGameModeBase::PrintChatMessageString(AHWPlayerController* InChattingPlayerController, const FString& InChatMessageString)
{
	FString ChatMessageString = InChatMessageString;
	int Index = InChatMessageString.Len() - 3;
	FString GuessNumberString = InChatMessageString.RightChop(Index);
	if (IsGuessNumberString(GuessNumberString) == true)
	{
		AHWPlayerState* HWPS = InChattingPlayerController->GetPlayerState<AHWPlayerState>();
		AHWGameStateBase* HWGameState = GetGameState<AHWGameStateBase>();
		if (!IsValid(HWPS) || !IsValid(HWGameState))
		{
			return;
		}

		// 현재 RPC를 요청한 플레이어의 턴이 아닌 경우 return 
		if (HWPS != HWGameState->CurrentTurnPlayerState)
		{
			InChattingPlayerController->ClientRPCPrintChatMessageString(TEXT("Your turn is not now."));
			return;
		}
		
		if (HWPS->GetCurrentGuessCount() >= HWPS->GetMaxGuessCount())
		{
			FString NotificationString = TEXT("All guess attempts have been used.");
			InChattingPlayerController->ClientRPCPrintChatMessageString(NotificationString);
			return; 
		}
		
		FString JudgeResultString = JudgeResult(SecretNumberString, GuessNumberString);
		IncreaseGuessCount(InChattingPlayerController);
		
		FString PlayerInfoString = HWPS->GetPlayerInfoString(); 
		FString CombinedMessageString = PlayerInfoString + TEXT(": ") + GuessNumberString + TEXT(" -> ") + JudgeResultString;

		for (const auto& Controller : AllPlayerControllers)
		{
			if (IsValid(Controller))
			{
				Controller->ClientRPCPrintChatMessageString(CombinedMessageString);
			}
		}
		int32 StrikeCount = FCString::Atoi(*JudgeResultString.Left(1));
		JudgeGame(InChattingPlayerController, StrikeCount);
	}
	else
	{
		for (const auto& Controller : AllPlayerControllers)
		{
			if (IsValid(Controller))
			{
				Controller->ClientRPCPrintChatMessageString(InChatMessageString);
			}
		}
	}
}

void AHWGameModeBase::IncreaseGuessCount(AHWPlayerController* InChattingPlayerController)
{
	AHWPlayerState* HWPS = InChattingPlayerController->GetPlayerState<AHWPlayerState>();
	if (IsValid(HWPS) == true)
	{
		HWPS->SetCurrentGuessCount(HWPS->GetCurrentGuessCount() + 1);
	}
}

void AHWGameModeBase::ResetGame()
{
	SecretNumberString = GenerateSecretNumber();

	for (const auto& HWPlayerController : AllPlayerControllers)
	{
		AHWPlayerState* HWPS = HWPlayerController->GetPlayerState<AHWPlayerState>();
		if (IsValid(HWPS) == true)
		{
			HWPS->SetCurrentGuessCount(0);
		}
	}
}

void AHWGameModeBase::JudgeGame(AHWPlayerController* InChattingPlayerController, int InStrikeCount)
{
	if (3 == InStrikeCount)
	{
		AHWPlayerState* HWPS = InChattingPlayerController->GetPlayerState<AHWPlayerState>();
		if (IsValid(HWPS))
		{
			FString WinMessage = HWPS->PlayerNameString + TEXT(" has won the game.");
			EndRoundAndRestart(WinMessage);
		}
	}
	else
	{
		if (CheckForDrawCondition())
		{
			EndRoundAndRestart(TEXT("Draw..."));
		}
		else
		{
			ChangePlayerTurn();
		}
	}
}

bool AHWGameModeBase::CheckForDrawCondition()
{
	for (const auto& HWPlayerController : AllPlayerControllers)
	{
		AHWPlayerState* HWPS = HWPlayerController->GetPlayerState<AHWPlayerState>();
		if (IsValid(HWPS) && HWPS->GetCurrentGuessCount() < HWPS->GetMaxGuessCount())
		{
			return false; 
		}
	}

	return true;
}

void AHWGameModeBase::EndRoundAndRestart(const FString& RoundEndMessage)
{
	GetWorld()->GetTimerManager().ClearTimer(TurnTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(RestartRoundTimerHandle, this, &AHWGameModeBase::PrepareNewRound, 5.0f, false);

	UpdateGameInfo(EHWGameStateProgress::RoundOver, RoundEndMessage);
}

void AHWGameModeBase::UpdateGameInfo(EHWGameStateProgress NewState, const FString& InResultMessage)
{
	AHWGameStateBase* HWGameState = GetGameState<AHWGameStateBase>();
	if (HWGameState)
	{
		FHWGameInfo TempGameInfo = HWGameState->CurrentGameInfo;
		TempGameInfo.CurrentGameStateProgress = NewState;
		TempGameInfo.ResultMessage = InResultMessage;
		HWGameState->CurrentGameInfo = TempGameInfo;
	}
}

void AHWGameModeBase::PrepareNewRound()
{
	ResetGame();
	StartGame();
}


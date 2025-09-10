// Fill out your copyright notice in the Description page of Project Settings.


#include "HWPlayerState.h"
#include "Net/UnrealNetwork.h"

AHWPlayerState::AHWPlayerState()
	: 	PlayerNameString(TEXT("None"))
	, 	CurrentGuessCount(0)
	, 	MaxGuessCount(3)
{
	bReplicates = true;
}

void AHWPlayerState::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, PlayerNameString);
	DOREPLIFETIME(ThisClass, CurrentGuessCount);
	DOREPLIFETIME(ThisClass, MaxGuessCount);
}

FString AHWPlayerState::GetPlayerInfoString()
{
	FString PlayerInfoString = PlayerNameString + TEXT("(") + FString::FromInt(CurrentGuessCount) + TEXT("/") + FString::FromInt(MaxGuessCount) + TEXT(")");
	return PlayerInfoString;
}

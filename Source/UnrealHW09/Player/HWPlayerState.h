// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "HWPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class UNREALHW09_API AHWPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AHWPlayerState();
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	FString GetPlayerInfoString();
public:
	void SetCurrentGuessCount(const int32 InCurrentGuessCount) { CurrentGuessCount = InCurrentGuessCount; }
	int32 GetCurrentGuessCount() const { return CurrentGuessCount; }
	
	int32 GetMaxGuessCount() const { return MaxGuessCount; }
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FString GetPlayerNameString() const { return PlayerNameString; }
	
	UPROPERTY(Replicated)
	FString PlayerNameString;

private:
	UPROPERTY(Replicated)
	int32 CurrentGuessCount;

	UPROPERTY(Replicated)
	int32 MaxGuessCount;
};

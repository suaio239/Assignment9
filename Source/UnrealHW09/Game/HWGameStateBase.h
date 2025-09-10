// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "HWGameStateBase.generated.h"

UENUM(BlueprintType)
enum class EHWGameStateProgress : uint8
{
	WaitingToStart,
	InProgress,
	RoundOver
};

USTRUCT(BlueprintType)
struct FHWGameInfo
{
	GENERATED_BODY()

	FHWGameInfo() : CurrentGameStateProgress(EHWGameStateProgress::WaitingToStart), ResultMessage(TEXT(""))
	{
		
	}
	
	UPROPERTY(BlueprintReadOnly)
	EHWGameStateProgress CurrentGameStateProgress;

	UPROPERTY(BlueprintReadOnly)
	FString ResultMessage;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameInfoChanged, const FHWGameInfo&, NewGameInfo);

class AHWPlayerState;
/**
 * 
 */
UCLASS()
class UNREALHW09_API AHWGameStateBase : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastRPCBroadcastLoginMessage(const FString& InNameString = FString(TEXT("XXXXXXX")));

	UFUNCTION(BlueprintCallable, BlueprintPure)
	AHWPlayerState* GetCurrentTurnPlayerState() const { return CurrentTurnPlayerState; }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	float GetTurnRemainingTime() const { return TurnRemainingTime; }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	const FHWGameInfo& GetCurrentGameInfo() const { return CurrentGameInfo; }
	
	UPROPERTY(Replicated)
	TObjectPtr<AHWPlayerState> CurrentTurnPlayerState; 

	UPROPERTY(Replicated)
	float TurnRemainingTime;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentGameInfo)
	FHWGameInfo CurrentGameInfo;

	UPROPERTY(BlueprintAssignable)
	FOnGameInfoChanged OnGameInfoChanged;

protected:
	UFUNCTION()
	void OnRep_CurrentGameInfo();
};

// Fill out your copyright notice in the Description page of Project Settings.


#include "HWChatInput.h"

#include "Components/EditableTextBox.h"
#include "Player/HWPlayerController.h"
#include "Player/HWPlayerState.h"

void UHWChatInput::NativeConstruct()
{
	Super::NativeConstruct();

	if (EditableTextBox_ChatInput->OnTextCommitted.IsAlreadyBound(this, &ThisClass::OnChatInputTextCommitted) == false)
	{
		EditableTextBox_ChatInput->OnTextCommitted.AddDynamic(this, &ThisClass::OnChatInputTextCommitted);		
	}	
}

void UHWChatInput::NativeDestruct()
{
	Super::NativeDestruct();

	if (EditableTextBox_ChatInput->OnTextCommitted.IsAlreadyBound(this, &ThisClass::OnChatInputTextCommitted) == true)
	{
		EditableTextBox_ChatInput->OnTextCommitted.RemoveDynamic(this, &ThisClass::OnChatInputTextCommitted);
	}
}

void UHWChatInput::OnChatInputTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod == ETextCommit::OnEnter)
	{
		APlayerController* OwningPlayerController = GetOwningPlayer();
		if (IsValid(OwningPlayerController) == true)
		{
			AHWPlayerController* OwningHWPlayerController = Cast<AHWPlayerController>(OwningPlayerController);
			if (IsValid(OwningHWPlayerController) == true)
			{
				AHWPlayerState* HWPS = OwningHWPlayerController->GetPlayerState<AHWPlayerState>();
				
				// RPC 패킷 전달 부하 최적화 로직 추가
				// if(IsValid(HWPS) && HWPS->GetCurrentGuessCount() >= HWPS->GetMaxGuessCount())
				// {
				// 	// 횟수를 다 썼으면 로컬에서만 시도 횟수 초과 메시지 출력하도록 함
				// 	FString NotificationString = TEXT("All guess attempts have been used.");
				// 	OwningHWPlayerController->PrintChatMessageString(NotificationString);
				// 	EditableTextBox_ChatInput->SetText(FText()); 
				// 	return;
				// }
				
				OwningHWPlayerController->SetChatMessageString(Text.ToString());
				EditableTextBox_ChatInput->SetText(FText());
			}
		}
	}
}
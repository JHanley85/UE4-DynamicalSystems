// Fill out your copyright notice in the Description page of Project Settings.
#include "DynSysGameSession.h"
#include "Runtime/Engine/Classes/GameFramework/GameModeBase.h"
#include "Runtime/Engine/Classes/Kismet/GameplayStatics.h"
#include "Runtime/Engine/Classes/Engine/World.h"



FString ADynSysGameSession::ApproveLogin(const FString& Options)
{
	UWorld* const World = GetWorld();
	check(World);

	AGameModeBase* const GameMode = World->GetAuthGameMode();
	check(GameMode);

//	int32 SpectatorOnly = 0;
//	SpectatorOnly = UGameplayStatics::GetIntOption(Options, TEXT("SpectatorOnly"), SpectatorOnly);
//
//	/*if (AtCapacity(SpectatorOnly == 1))
//	{
//		return TEXT("Server full.");
//	}
//*/
//	int32 SplitscreenCount = 0;
//	SplitscreenCount = UGameplayStatics::GetIntOption(Options, TEXT("SplitscreenCount"), SplitscreenCount);
//
//	if (SplitscreenCount > MaxSplitscreensPerConnection)
//	{
//		UE_LOG(LogGameSession, Warning, TEXT("ApproveLogin: A maximum of %i splitscreen players are allowed"), MaxSplitscreensPerConnection);
//		return TEXT("Maximum splitscreen players");
//	}

	return TEXT("");
}

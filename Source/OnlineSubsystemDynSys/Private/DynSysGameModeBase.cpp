// Fill out your copyright notice in the Description page of Project Settings.
#include "DynSysGameModeBase.h"

#include "Runtime/Engine/Classes/GameFramework/GameModeBase.h"
#include "Runtime/Engine/Classes/Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Runtime/Engine/Classes/GameFramework/GameSession.h"
#include "Runtime/Engine/Classes/Kismet/GameplayStatics.h"


ADynSysGameModeBase::ADynSysGameModeBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.DoNotCreateDefaultSubobject(TEXT("Sprite")))
{
}
void ADynSysGameModeBase::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{

	ErrorMessage = GameSession->ApproveLogin(Options);
	
}
APlayerController* ADynSysGameModeBase::SpawnPlayerController(ENetRole InRemoteRole, FVector const& SpawnLocation, FRotator const& SpawnRotation)
{
	if (InRemoteRole == ROLE_SimulatedProxy) {
		return SpawnPlayerControllerCommon(InRemoteRole, SpawnLocation, SpawnRotation, PlayerControllerClass);
	}
	else {
		return SpawnPlayerControllerCommon(InRemoteRole, SpawnLocation, SpawnRotation, RemotePlayerControllerClass);
	}
}


void ADynSysGameModeBase::PostLogin(APlayerController* NewPlayer)
{
	// Runs shared initialization that can happen during seamless travel as well

	GenericPlayerInitialization(NewPlayer);

	// Perform initialization that only happens on initially joining a server

	UWorld* World = GetWorld();

//	NewPlayer->ClientCapBandwidth(NewPlayer->Player->CurrentNetSpeed);

	if (MustSpectate(NewPlayer))
	{
		NewPlayer->ClientGotoState(NAME_Spectating);
	}
	else
	{
		// If NewPlayer is not only a spectator and has a valid ID, add him as a user to the replay.
		if (NewPlayer->PlayerState->UniqueId.IsValid())
		{
			GetGameInstance()->AddUserToReplay(NewPlayer->PlayerState->UniqueId.ToString());
		}
	}

	if (GameSession)
	{
		GameSession->PostLogin(NewPlayer);
	}

	// Notify Blueprints that a new player has logged in.  Calling it here, because this is the first time that the PlayerController can take RPCs
	K2_PostLogin(NewPlayer);
	FGameModeEvents::GameModePostLoginEvent.Broadcast(this, NewPlayer);

	// Now that initialization is done, try to spawn the player's pawn and start match
	HandleStartingNewPlayer(NewPlayer);
}

APlayerController* ADynSysGameModeBase::Login(UPlayer* NewPlayer, ENetRole InRemoteRole, const FString& Portal, const FString& Options, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	APlayerController* NewPlayerController = nullptr;
	
		// We need to spawn a new PlayerController to replace the old one
	NewPlayerController = SpawnPlayerController(InRemoteRole, FVector::ZeroVector, FRotator::ZeroRotator);
	

	// Handle spawn failure.
	if (NewPlayerController == nullptr)
	{
		UE_LOG(LogGameMode, Log, TEXT("Login: Couldn't spawn player controller of class %s"), PlayerControllerClass ? *PlayerControllerClass->GetName() : TEXT("NULL"));
		ErrorMessage = FString::Printf(TEXT("Failed to spawn player controller"));
		return nullptr;
	}

	// Customize incoming player based on URL options
	ErrorMessage = InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);
	if (!ErrorMessage.IsEmpty())
	{
		return nullptr;
	}

	return NewPlayerController;
}

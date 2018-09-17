// Fill out your copyright notice in the Description page of Project Settings.

#include "RustyWorldSettings.h"
#include "EngineUtils.h"
#include "Runtime/CoreUObject/Public/UObject/UObjectIterator.h"
#include "Runtime/Engine/Classes/Kismet/GameplayStatics.h"
#include "ObjectWriter.h"
#include "ObjectReader.h"
#include "RustyNetConnection.h"
#include "RustyIpNetDriver.h"
#include "OnlineSubsystem.h"
#include "SocketSubsystem.h"
#include "Runtime/Networking/Public/Networking.h"
#include "Base64.h"
#include "Runtime/PacketHandlers/PacketHandler/Public/PacketHandler.h"
#include <vector>

#include "Misc/NetworkVersion.h"
#include "GameFramework/GameNetworkManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/NetworkProfiler.h"
#include "EngineUtils.h"
#include "DynamicalSystemsSettings.h"



ARustyWorldSettings::ARustyWorldSettings(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
	FString server = UDynamicalSystemsSettings::GetDynamicalSettings()->GetEnvSetting().Server;
	FString  mumble = UDynamicalSystemsSettings::GetDynamicalSettings()->GetEnvSetting().MumbleServer;

	TArray<FString> Out;
	mumble.ParseIntoArray(Out, TEXT(":"), true);
	Server = FURLAddress(server);
	Mumble = FURLAddress(mumble);
	
}
void ARustyWorldSettings::BeginPlay() {
	Super::BeginPlay();

}
UNetConnection* ARustyWorldSettings::GetConnection() {
	UWorld* world = GetWorld();
	if (world == nullptr) return nullptr;
	NetDriver = ((URustyIpNetDriver*)world->GetNetDriver());
	if (NetDriver == nullptr) return nullptr;
	NetConnection = (URustyNetConnection*)NetDriver->GetServerConnection();
	return NetConnection;
}
void ARustyWorldSettings::CheckProperties() {
	if (URustyNetConnection::_playerId != 0) {
		for (auto pair : PropertyMap) {
			FBufferArchive buf = FBufferArchive();
			if (PropChanged(pair.Key, buf)) {
				TArray<uint8> Bytes;
				UProperty* Property = pair.Value;
				if (Bytes.Num() > 1) {
					SerializedProp.Add(pair.Key, buf);
					NetConnection->Build_PropertyRep(Bytes, UserId, pair.Key, Bytes);

					NetConnection->SendServerRequest(ENetServerRequest::FunctionRep, Bytes, pair.Key);
				}
			}
		}
	}
	if(NetDriver->IsNetResourceValid())
		if (NetDriver->ServerConnection!=nullptr) {
			NetDriver->ServerConnection->Tick();
			NetDriver->ServerConnection->FlushNet();
			NetDriver->TickFlush(0.1);
		}
		else {
			UE_LOG(LogTemp, Log, TEXT("Something Went Wrong"));
		}
}
void ARustyWorldSettings::OnActorChannelOpen(FInBunch & InBunch,UNetConnection * Connection) {
	Super::OnActorChannelOpen(InBunch, Connection);
}
void ARustyWorldSettings::Setup() {
	if (NetDriver != nullptr) return;
	UWorld* world = GetWorld();
	if (world == nullptr)return;
	FURL ServerUrl = FURL();
	ServerUrl.Host = Server.Host;
	ServerUrl.Port = Server.Port;// + LevelId;
	FString _Error;
	NetDriver = NewObject<URustyIpNetDriver>(this, "RustyIPNetDriver");
	world->SetNetDriver(NetDriver);
	NetDriver->World = world;
	NetDriver->InitConnect((FNetworkNotify*)this, ServerUrl, _Error);
	NetConnection =(URustyNetConnection*) NetDriver->ServerConnection;
	NetConnection->settings = this;
	///
	TSharedRef<FInternetAddr> i4addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	TArray<FString> ips;
	Server.Host.ParseIntoArray(ips, TEXT("."), true);
	FIPv4Address ip(FCString::Atoi(*ips[0]), FCString::Atoi(*ips[1]), FCString::Atoi(*ips[2]), FCString::Atoi(*ips[3]));
	i4addr->SetIp(ip.Value);

	i4addr->SetPort(Server.Port);
	///
	NetDriver->Socket->Connect(*i4addr);
	NotifyAcceptedConnection(NetDriver->ServerConnection);
	UE_LOG(LogTemp, Log, TEXT("3 Driver: {%s}"), *NetDriver->GetClass()->GetName());
	if (!_Error.IsEmpty()) {
		UE_LOG(LogTemp, Error, TEXT("InitConnect {%s}"), *_Error);
	}
	TSharedRef<FInternetAddr> addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	FURL url = FURL(*addr->ToString(true));
	UE_LOG(LogTemp, Log, TEXT("InitConnect: Status %s"), *NetConnection->GetStateString());
	((URustyNetConnection*)NetConnection)->State = EConnectionState::USOCK_Open;
	NetBytes Bytes = NetBytes();
	((URustyNetConnection*)NetConnection)->Build_Register(Bytes, URustyNetConnection::_playerId);

	UE_LOG(LogTemp, Log, TEXT("%s"), *NetConnection->GetDriver()->GetName());
	((URustyNetConnection*)NetConnection)->SendServerRequest(ENetServerRequest::Register, Bytes);


	world->GetTimerManager().SetTimer(PingTimer, NetConnection, &URustyNetConnection::SendPing,PingFreq, true);
	world->GetTimerManager().SetTimer(ReplicationProperties, this, &ARustyWorldSettings::CheckProperties, PropertyChangeFreq, true);
}



EAcceptConnection::Type ARustyWorldSettings::NotifyAcceptingConnection()
{
	check(NetDriver);
	if (NetDriver->ServerConnection)
	{
		// We are a client and we don't welcome incoming connections.
		UE_LOG(LogNet, Log, TEXT("NotifyAcceptingConnection: Client refused"));
		return EAcceptConnection::Reject;
	}
	//else if (NextURL != TEXT(""))
	//{
	//	// Server is switching levels.
	//	UE_LOG(LogNet, Log, TEXT("NotifyAcceptingConnection: Server %s refused"), *GetName());
	//	return EAcceptConnection::Ignore;
	//}
	else
	{
		// Server is up and running.
		UE_LOG(LogNet, Verbose, TEXT("NotifyAcceptingConnection: Server %s accept"), *GetName());
		return EAcceptConnection::Accept;
	}
}

void ARustyWorldSettings::NotifyAcceptedConnection(UNetConnection* Connection)
{
	check(NetDriver != NULL);
	//check(NetDriver->ServerConnection == NULL);
	UE_LOG(LogNet, Log, TEXT("NotifyAcceptedConnection: Name: %s, TimeStamp: %s, %s"), *GetName(), FPlatformTime::StrTimestamp(), *Connection->Describe());
//	NETWORK_PROFILER(GNetworkProfiler.TrackEvent(TEXT("OPEN"), *(GetName() + TEXT(" ") + Connection->LowLevelGetRemoteAddress()), Connection));
	((URustyNetConnection*)Connection)->State = EConnectionState::USOCK_Open;
	NetBytes Bytes = NetBytes();
	((URustyNetConnection*)Connection)->Build_Register(Bytes, URustyNetConnection::_playerId);
	
	UE_LOG(LogTemp, Log, TEXT("%s"),*Connection->GetDriver()->GetName());
	((URustyNetConnection*)Connection)->SendServerRequest(ENetServerRequest::Register, Bytes);
}

bool ARustyWorldSettings::NotifyAcceptingChannel(UChannel* Channel)
{
	check(Channel);
	check(Channel->Connection);
	check(Channel->Connection->Driver);
	UNetDriver* Driver = Channel->Connection->Driver;

	if (Driver->ServerConnection)
	{
		// We are a client and the server has just opened up a new channel.
		UE_LOG(LogTemp, Log, TEXT("NotifyAcceptingChannel %i/%i client %s"), Channel->ChIndex, Channel->ChType, *GetName());
		if (Channel->ChType == CHTYPE_Actor)
		{
			// Actor channel.
			UE_LOG(LogTemp, Log, TEXT("Client accepting actor channel"));
			return 1;
		}
		else if (Channel->ChType == CHTYPE_Voice)
		{
			// Accept server requests to open a voice channel, allowing for custom voip implementations
			// which utilize multiple server controlled voice channels.
			UE_LOG(LogTemp, Log, TEXT("Client accepting voice channel"));
			return 1;
		}
		else
		{
			// Unwanted channel type.
			UE_LOG(LogTemp, Log, TEXT("Client refusing unwanted channel of type %i"), (uint8)Channel->ChType);
			return 0;
		}
	}
	else
	{
		// We are the server.
		if (Channel->ChIndex == 0 && Channel->ChType == CHTYPE_Control)
		{
			// The client has opened initial channel.
			UE_LOG(LogTemp, Log, TEXT("NotifyAcceptingChannel Control %i server %s: Accepted"), Channel->ChIndex, *GetFullName());
			return 1;
		}
		else if (Channel->ChType == CHTYPE_File)
		{
			// The client is going to request a file.
			UE_LOG(LogTemp, Log, TEXT("NotifyAcceptingChannel File %i server %s: Accepted"), Channel->ChIndex, *GetFullName());
			return 1;
		}
		else
		{
			// Client can't open any other kinds of channels.
			UE_LOG(LogTemp, Log, TEXT("NotifyAcceptingChannel %i %i server %s: Refused"), (uint8)Channel->ChType, Channel->ChIndex, *GetFullName());
			return 0;
		}
	}
}


void ARustyWorldSettings::NotifyControlMessage(UNetConnection* Connection, uint8 MessageType, class FInBunch& Bunch)
{
	UE_LOG(LogTemp, Log, TEXT("Notify Control Message"));
	//	if (NetDriver->ServerConnection)
	//	{
	//		check(Connection == NetDriver->ServerConnection);
	//
	//		// We are the client, traveling to a new map with the same server
	//#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	//		UE_LOG(LogNet, Verbose, TEXT("Level client received: %s"), FNetControlMessageInfo::GetName(MessageType));
	//#endif
	//		switch (MessageType)
	//		{
	//		case NMT_Failure:
	//		{
	//			// our connection attempt failed for some reason, for example a synchronization mismatch (bad GUID, etc) or because the server rejected our join attempt (too many players, etc)
	//			// here we can further parse the string to determine the reason that the server closed our connection and present it to the user
	//			FString EntryURL = TEXT("?failed");
	//			FString ErrorMsg;
	//
	//			if (FNetControlMessage<NMT_Failure>::Receive(Bunch, ErrorMsg))
	//			{
	//				if (ErrorMsg.IsEmpty())
	//				{
	//					ErrorMsg = NSLOCTEXT("NetworkErrors", "GenericConnectionFailed", "Connection Failed.").ToString();
	//				}
	//
	//				GEngine->BroadcastNetworkFailure(this, NetDriver, ENetworkFailure::FailureReceived, ErrorMsg);
	//				if (Connection)
	//				{
	//					Connection->Close();
	//				}
	//			}
	//
	//			break;
	//		}
	//		case NMT_DebugText:
	//		{
	//			// debug text message
	//			FString Text;
	//
	//			if (FNetControlMessage<NMT_DebugText>::Receive(Bunch, Text))
	//			{
	//				UE_LOG(LogNet, Log, TEXT("%s received NMT_DebugText Text=[%s] Desc=%s DescRemote=%s"),
	//					*Connection->Driver->GetDescription(), *Text, *Connection->LowLevelDescribe(),
	//					*Connection->LowLevelGetRemoteAddress());
	//			}
	//
	//			break;
	//		}
	//		case NMT_NetGUIDAssign:
	//		{
	//			FNetworkGUID NetGUID;
	//			FString Path;
	//
	//			if (FNetControlMessage<NMT_NetGUIDAssign>::Receive(Bunch, NetGUID, Path))
	//			{
	//				UE_LOG(LogNet, Verbose, TEXT("NMT_NetGUIDAssign  NetGUID %s. Path: %s. "), *NetGUID.ToString(), *Path);
	//				Connection->PackageMap->ResolvePathAndAssignNetGUID(NetGUID, Path);
	//			}
	//
	//			break;
	//		}
	//		}
	//	}
	//	else
	//	{
	//		// We are the server.
	//#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	//		UE_LOG(LogNet, Verbose, TEXT("Level server received: %s"), FNetControlMessageInfo::GetName(MessageType));
	//#endif
	//		if (!Connection->IsClientMsgTypeValid(MessageType))
	//		{
	//			// If we get here, either code is mismatched on the client side, or someone could be spoofing the client address
	//			UE_LOG(LogNet, Error, TEXT("IsClientMsgTypeValid FAILED (%i): Remote Address = %s"), (int)MessageType, *Connection->LowLevelGetRemoteAddress());
	//			Bunch.SetError();
	//			return;
	//		}
	//
	//		switch (MessageType)
	//		{
	//		case NMT_Hello:
	//		{
	//			uint8 IsLittleEndian = 0;
	//			uint32 RemoteNetworkVersion = 0;
	//			uint32 LocalNetworkVersion = FNetworkVersion::GetLocalNetworkVersion();
	//			FString EncryptionToken;
	//
	//			if (FNetControlMessage<NMT_Hello>::Receive(Bunch, IsLittleEndian, RemoteNetworkVersion, EncryptionToken))
	//			{
	//				if (!FNetworkVersion::IsNetworkCompatible(LocalNetworkVersion, RemoteNetworkVersion))
	//				{
	//					UE_LOG(LogNet, Log, TEXT("NotifyControlMessage: Client connecting with invalid version. LocalNetworkVersion: %i, RemoteNetworkVersion: %i"), LocalNetworkVersion, RemoteNetworkVersion);
	//					FNetControlMessage<NMT_Upgrade>::Send(Connection, LocalNetworkVersion);
	//					Connection->FlushNet(true);
	//					Connection->Close();
	//
	//					PerfCountersIncrement(TEXT("ClosedConnectionsDueToIncompatibleVersion"));
	//				}
	//				else
	//				{
	//					if (EncryptionToken.IsEmpty())
	//					{
	//						SendChallengeControlMessage(Connection);
	//					}
	//					else
	//					{
	//						if (FNetDelegates::OnReceivedNetworkEncryptionToken.IsBound())
	//						{
	//							TWeakObjectPtr<UNetConnection> WeakConnection = Connection;
	//							FNetDelegates::OnReceivedNetworkEncryptionToken.Execute(EncryptionToken, FOnEncryptionKeyResponse::CreateUObject(this, &UWorld::SendChallengeControlMessage, WeakConnection));
	//						}
	//						else
	//						{
	//							FString FailureMsg(TEXT("Encryption failure"));
	//							UE_LOG(LogNet, Warning, TEXT("%s: No delegate available to handle encryption token, disconnecting."), *Connection->GetName());
	//							FNetControlMessage<NMT_Failure>::Send(Connection, FailureMsg);
	//							Connection->FlushNet(true);
	//						}
	//					}
	//				}
	//			}
	//
	//			break;
	//		}
	//
	//		case NMT_Netspeed:
	//		{
	//			int32 Rate;
	//
	//			if (FNetControlMessage<NMT_Netspeed>::Receive(Bunch, Rate))
	//			{
	//				Connection->CurrentNetSpeed = FMath::Clamp(Rate, 1800, NetDriver->MaxClientRate);
	//				UE_LOG(LogNet, Log, TEXT("Client netspeed is %i"), Connection->CurrentNetSpeed);
	//			}
	//
	//			break;
	//		}
	//		case NMT_Abort:
	//		{
	//			break;
	//		}
	//		case NMT_Skip:
	//		{
	//			break;
	//		}
	//		case NMT_Login:
	//		{
	//			// Admit or deny the player here.
	//			FUniqueNetIdRepl UniqueIdRepl;
	//			FString OnlinePlatformName;
	//
	//			// Expand the maximum string serialization size, to accommodate extremely large Fortnite join URL's.
	//			Bunch.ArMaxSerializeSize += (16 * 1024 * 1024);
	//
	//			bool bReceived = FNetControlMessage<NMT_Login>::Receive(Bunch, Connection->ClientResponse, Connection->RequestURL,
	//				UniqueIdRepl, OnlinePlatformName);
	//
	//			Bunch.ArMaxSerializeSize -= (16 * 1024 * 1024);
	//
	//			if (bReceived)
	//			{
	//				UE_LOG(LogNet, Log, TEXT("Login request: %s userId: %s"), *Connection->RequestURL,
	//					(UniqueIdRepl.IsValid() ? *UniqueIdRepl->ToString() : TEXT("Invalid")));
	//
	//
	//				// Compromise for passing splitscreen playercount through to gameplay login code,
	//				// without adding a lot of extra unnecessary complexity throughout the login code.
	//				// NOTE: This code differs from NMT_JoinSplit, by counting + 1 for SplitscreenCount
	//				//			(since this is the primary connection, not counted in Children)
	//				FURL InURL(NULL, *Connection->RequestURL, TRAVEL_Absolute);
	//
	//				if (!InURL.Valid)
	//				{
	//					UE_LOG(LogNet, Error, TEXT("NMT_Login: Invalid URL %s"), *Connection->RequestURL);
	//					Bunch.SetError();
	//					break;
	//				}
	//
	//				uint8 SplitscreenCount = FMath::Min(Connection->Children.Num() + 1, 255);
	//
	//				// Don't allow clients to specify this value
	//				InURL.RemoveOption(TEXT("SplitscreenCount"));
	//				InURL.AddOption(*FString::Printf(TEXT("SplitscreenCount=%i"), SplitscreenCount));
	//
	//				Connection->RequestURL = InURL.ToString();
	//
	//				// skip to the first option in the URL
	//				const TCHAR* Tmp = *Connection->RequestURL;
	//				for (; *Tmp && *Tmp != '?'; Tmp++);
	//
	//				// keep track of net id for player associated with remote connection
	//				Connection->PlayerId = UniqueIdRepl;
	//
	//				// keep track of the online platform the player associated with this connection is using.
	//				Connection->SetPlayerOnlinePlatformName(FName(*OnlinePlatformName));
	//
	//				// ask the game code if this player can join
	//				FString ErrorMsg;
	//				AGameModeBase* GameMode = GetAuthGameMode();
	//
	//				if (GameMode)
	//				{
	//					GameMode->PreLogin(Tmp, Connection->LowLevelGetRemoteAddress(), Connection->PlayerId, ErrorMsg);
	//				}
	//				if (!ErrorMsg.IsEmpty())
	//				{
	//					UE_LOG(LogNet, Log, TEXT("PreLogin failure: %s"), *ErrorMsg);
	//					NETWORK_PROFILER(GNetworkProfiler.TrackEvent(TEXT("PRELOGIN FAILURE"), *ErrorMsg, Connection));
	//					FNetControlMessage<NMT_Failure>::Send(Connection, ErrorMsg);
	//					Connection->FlushNet(true);
	//					//@todo sz - can't close the connection here since it will leave the failure message 
	//					// in the send buffer and just close the socket. 
	//					//Connection->Close();
	//				}
	//				else
	//				{
	//					WelcomePlayer(Connection);
	//				}
	//			}
	//			else
	//			{
	//				Connection->ClientResponse.Empty();
	//				Connection->RequestURL.Empty();
	//			}
	//
	//			break;
	//		}
	//		case NMT_Join:
	//		{
	//			if (Connection->PlayerController == NULL)
	//			{
	//				// Spawn the player-actor for this network player.
	//				FString ErrorMsg;
	//				UE_LOG(LogNet, Log, TEXT("Join request: %s"), *Connection->RequestURL);
	//
	//				FURL InURL(NULL, *Connection->RequestURL, TRAVEL_Absolute);
	//
	//				if (!InURL.Valid)
	//				{
	//					UE_LOG(LogNet, Error, TEXT("NMT_Login: Invalid URL %s"), *Connection->RequestURL);
	//					Bunch.SetError();
	//					break;
	//				}
	//
	//				Connection->PlayerController = SpawnPlayActor(Connection, ROLE_AutonomousProxy, InURL, Connection->PlayerId, ErrorMsg);
	//				if (Connection->PlayerController == NULL)
	//				{
	//					// Failed to connect.
	//					UE_LOG(LogNet, Log, TEXT("Join failure: %s"), *ErrorMsg);
	//					NETWORK_PROFILER(GNetworkProfiler.TrackEvent(TEXT("JOIN FAILURE"), *ErrorMsg, Connection));
	//					FNetControlMessage<NMT_Failure>::Send(Connection, ErrorMsg);
	//					Connection->FlushNet(true);
	//					//@todo sz - can't close the connection here since it will leave the failure message 
	//					// in the send buffer and just close the socket. 
	//					//Connection->Close();
	//				}
	//				else
	//				{
	//					// Successfully in game.
	//					UE_LOG(LogNet, Log, TEXT("Join succeeded: %s"), *Connection->PlayerController->PlayerState->GetPlayerName());
	//					NETWORK_PROFILER(GNetworkProfiler.TrackEvent(TEXT("JOIN"), *Connection->PlayerController->PlayerState->GetPlayerName(), Connection));
	//
	//					Connection->SetClientLoginState(EClientLoginState::ReceivedJoin);
	//
	//					// if we're in the middle of a transition or the client is in the wrong world, tell it to travel
	//					FString LevelName;
	//					FSeamlessTravelHandler &SeamlessTravelHandler = GEngine->SeamlessTravelHandlerForWorld(this);
	//
	//					if (SeamlessTravelHandler.IsInTransition())
	//					{
	//						// tell the client to go to the destination map
	//						LevelName = SeamlessTravelHandler.GetDestinationMapName();
	//					}
	//					else if (!Connection->PlayerController->HasClientLoadedCurrentWorld())
	//					{
	//						// tell the client to go to our current map
	//						FString NewLevelName = GetOutermost()->GetName();
	//						UE_LOG(LogNet, Log, TEXT("Client joined but was sent to another level. Asking client to travel to: '%s'"), *NewLevelName);
	//						LevelName = NewLevelName;
	//					}
	//					if (LevelName != TEXT(""))
	//					{
	//						Connection->PlayerController->ClientTravel(LevelName, TRAVEL_Relative, true);
	//					}
	//
	//					// @TODO FIXME - TEMP HACK? - clear queue on join
	//					Connection->QueuedBits = 0;
	//				}
	//			}
	//			break;
	//		}
	//		case NMT_JoinSplit:
	//		{
	//			// Handle server-side request for spawning a new controller using a child connection.
	//			FString SplitRequestURL;
	//			FUniqueNetIdRepl SplitRequestUniqueIdRepl;
	//
	//			if (FNetControlMessage<NMT_JoinSplit>::Receive(Bunch, SplitRequestURL, SplitRequestUniqueIdRepl))
	//			{
	//				UE_LOG(LogNet, Log, TEXT("Join splitscreen request: %s userId: %s parentUserId: %s"),
	//					*SplitRequestURL,
	//					SplitRequestUniqueIdRepl.IsValid() ? *SplitRequestUniqueIdRepl->ToString() : TEXT("Invalid"),
	//					Connection->PlayerId.IsValid() ? *Connection->PlayerId->ToString() : TEXT("Invalid"));
	//
	//				// Compromise for passing splitscreen playercount through to gameplay login code,
	//				// without adding a lot of extra unnecessary complexity throughout the login code.
	//				// NOTE: This code differs from NMT_Login, by counting + 2 for SplitscreenCount
	//				//			(once for pending child connection, once for primary non-child connection)
	//				FURL InURL(NULL, *SplitRequestURL, TRAVEL_Absolute);
	//
	//				if (!InURL.Valid)
	//				{
	//					UE_LOG(LogNet, Error, TEXT("NMT_JoinSplit: Invalid URL %s"), *SplitRequestURL);
	//					Bunch.SetError();
	//					break;
	//				}
	//
	//				uint8 SplitscreenCount = FMath::Min(Connection->Children.Num() + 2, 255);
	//
	//				// Don't allow clients to specify this value
	//				InURL.RemoveOption(TEXT("SplitscreenCount"));
	//				InURL.AddOption(*FString::Printf(TEXT("SplitscreenCount=%i"), SplitscreenCount));
	//
	//				SplitRequestURL = InURL.ToString();
	//
	//				// skip to the first option in the URL
	//				const TCHAR* Tmp = *SplitRequestURL;
	//				for (; *Tmp && *Tmp != '?'; Tmp++);
	//
	//				// go through the same full login process for the split player even though it's all in the same frame
	//				FString ErrorMsg;
	//				AGameModeBase* GameMode = GetAuthGameMode();
	//				if (GameMode)
	//				{
	//					GameMode->PreLogin(Tmp, Connection->LowLevelGetRemoteAddress(), SplitRequestUniqueIdRepl, ErrorMsg);
	//				}
	//				if (!ErrorMsg.IsEmpty())
	//				{
	//					// if any splitscreen viewport fails to join, all viewports on that client also fail
	//					UE_LOG(LogNet, Log, TEXT("PreLogin failure: %s"), *ErrorMsg);
	//					NETWORK_PROFILER(GNetworkProfiler.TrackEvent(TEXT("PRELOGIN FAILURE"), *ErrorMsg, Connection));
	//					FNetControlMessage<NMT_Failure>::Send(Connection, ErrorMsg);
	//					Connection->FlushNet(true);
	//					//@todo sz - can't close the connection here since it will leave the failure message 
	//					// in the send buffer and just close the socket. 
	//					//Connection->Close();
	//				}
	//				else
	//				{
	//					// create a child network connection using the existing connection for its parent
	//					check(Connection->GetUChildConnection() == NULL);
	//					check(CurrentLevel);
	//
	//					UChildConnection* ChildConn = NetDriver->CreateChild(Connection);
	//					ChildConn->PlayerId = SplitRequestUniqueIdRepl;
	//					ChildConn->SetPlayerOnlinePlatformName(Connection->GetPlayerOnlinePlatformName());
	//					ChildConn->RequestURL = SplitRequestURL;
	//					ChildConn->SetClientWorldPackageName(CurrentLevel->GetOutermost()->GetFName());
	//
	//					// create URL from string
	//					FURL JoinSplitURL(NULL, *SplitRequestURL, TRAVEL_Absolute);
	//
	//					UE_LOG(LogNet, Log, TEXT("JOINSPLIT: Join request: URL=%s"), *JoinSplitURL.ToString());
	//					APlayerController* PC = SpawnPlayActor(ChildConn, ROLE_AutonomousProxy, JoinSplitURL, ChildConn->PlayerId, ErrorMsg, uint8(Connection->Children.Num()));
	//					if (PC == NULL)
	//					{
	//						// Failed to connect.
	//						UE_LOG(LogNet, Log, TEXT("JOINSPLIT: Join failure: %s"), *ErrorMsg);
	//						NETWORK_PROFILER(GNetworkProfiler.TrackEvent(TEXT("JOINSPLIT FAILURE"), *ErrorMsg, Connection));
	//						// remove the child connection
	//						Connection->Children.Remove(ChildConn);
	//						// if any splitscreen viewport fails to join, all viewports on that client also fail
	//						FNetControlMessage<NMT_Failure>::Send(Connection, ErrorMsg);
	//						Connection->FlushNet(true);
	//						//@todo sz - can't close the connection here since it will leave the failure message 
	//						// in the send buffer and just close the socket. 
	//						//Connection->Close();
	//					}
	//					else
	//					{
	//						// Successfully spawned in game.
	//						UE_LOG(LogNet, Log, TEXT("JOINSPLIT: Succeeded: %s PlayerId: %s"),
	//							*ChildConn->PlayerController->PlayerState->GetPlayerName(),
	//							*ChildConn->PlayerController->PlayerState->UniqueId.ToDebugString());
	//					}
	//				}
	//			}
	//
	//			break;
	//		}
	//		case NMT_PCSwap:
	//		{
	//			UNetConnection* SwapConnection = Connection;
	//			int32 ChildIndex;
	//
	//			if (FNetControlMessage<NMT_PCSwap>::Receive(Bunch, ChildIndex))
	//			{
	//				if (ChildIndex >= 0)
	//				{
	//					SwapConnection = Connection->Children.IsValidIndex(ChildIndex) ? Connection->Children[ChildIndex] : NULL;
	//				}
	//				bool bSuccess = false;
	//				if (SwapConnection != NULL)
	//				{
	//					bSuccess = DestroySwappedPC(SwapConnection);
	//				}
	//
	//				if (!bSuccess)
	//				{
	//					UE_LOG(LogNet, Log, TEXT("Received invalid swap message with child index %i"), ChildIndex);
	//				}
	//			}
	//
	//			break;
	//		}
	//		case NMT_DebugText:
	//		{
	//			// debug text message
	//			FString Text;
	//
	//			if (FNetControlMessage<NMT_DebugText>::Receive(Bunch, Text))
	//			{
	//				UE_LOG(LogNet, Log, TEXT("%s received NMT_DebugText Text=[%s] Desc=%s DescRemote=%s"),
	//					*Connection->Driver->GetDescription(), *Text, *Connection->LowLevelDescribe(),
	//					*Connection->LowLevelGetRemoteAddress());
	//			}
	//
	//			break;
	//		}
	//		}
	//	}
}
//
//void ARustyLevelScriptActor::SendServerRequest(ENetServerRequest RequestType, TArray<uint8> Value,int32 PropertyId) {
//	if (NetDriver == nullptr) return;
//	NetBytes Message = NetBytes();
//	NetIdentifier id = 35;
//	URustyNetConnection::Build_ServerRequest(RequestType, Message,id);
//	
//	ARustyWorldSettings* settings = (ARustyWorldSettings*)GetWorldSettings();
//
//	TSharedRef<FInternetAddr> addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
//	TArray<FString> ips;
//	settings->Server.Host.ParseIntoArray(ips, TEXT("."), true);
//
//	FIPv4Address ip(FCString::Atoi(*ips[0]), FCString::Atoi(*ips[1]), FCString::Atoi(*ips[2]), FCString::Atoi(*ips[3]));
//
//	addr->SetIp(ip.Value);
//
//	addr->SetPort(settings->Server.Port);
//
//	bool connected = NetDriver->Socket->Connect(*addr);
//	int32 sent = 0;
//	std::vector<uint8> msg;
//	msg.resize(Message.Num());
//	for (int i = 0; i< Message.Num(); i++) {
//		msg[i] = Message[i];
//	}
//	bool bSent = NetDriver->Socket->Send(msg.data(), msg.size(), sent);
//
//	NetDriver->Socket->GetAddress(*addr);
//	FString Accepted;
//	UE_LOG(LogTemp, Log, TEXT("Sent: [%s] {%i} {%i} {%i} {%i}"), *addr.Get().ToString(true), bSent, Message.Num(), msg.size(), sent);
//	UE_LOG(LogTemp, Log, TEXT("InitConnect: {%s} Status %s"), *Accepted,*NetConnection->GetStateString());
//
//}

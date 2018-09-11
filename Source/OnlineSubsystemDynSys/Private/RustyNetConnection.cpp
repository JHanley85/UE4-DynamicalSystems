// Fill out your copyright notice in the Description page of Project Settings.

#include "RustyNetConnection.h"
#include "SocketSubsystem.h"
#include "OnlineSubsystemTypes.h"
#include "RustyIpNetDriver.h"
#include "IPAddress.h"
#include "Runtime/Networking/Public/Networking.h"
#include "Base64.h"
#include "Runtime/PacketHandlers/PacketHandler/Public/PacketHandler.h"
#include <vector>

URustyNetConnection::URustyNetConnection(const FObjectInitializer& ObjectInitializer):Super(ObjectInitializer){
	SetId(_playerId);
}
FOnRegisterComplete URustyNetConnection::OnRegisterComplete = FOnRegisterComplete();
void URustyNetConnection::Tick() {
	if (Socket) {
		TArray<uint8> ReceivedData;
		TSharedRef<FInternetAddr> targetAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
		uint32 Size;
		while (Socket->HasPendingData(Size))
		{
			ReceivedData.SetNumUninitialized(FMath::Min(Size, 65507u));

			int32 Read = 0;
			Socket->RecvFrom(ReceivedData.GetData(), ReceivedData.Num(), Read, *targetAddr);
			UE_LOG(LogTemp, Log, TEXT("From %s : bytes:%i"), *targetAddr.Get().ToString(true), Size);
			RecvMesage(ReceivedData);
		}
	}
}
FString URustyNetConnection::Describe()
{
	return FString::Printf(TEXT("[URustyNetConnection]"));
	return FString::Printf(TEXT("[URustyNetConnection] RemoteAddr: %s, Name: %s, Driver: %s, IsServer: %s, PC: %s, Owner: %s, UniqueId: %s"),
		*LowLevelGetRemoteAddress(true),
		*GetName(),
		Driver ? *Driver->GetDescription() : TEXT("NULL"),
		Driver && Driver->IsServer() ? TEXT("YES") : TEXT("NO"),
		PlayerController ? *PlayerController->GetName() : TEXT("NULL"),
		OwningActor ? *OwningActor->GetName() : TEXT("NULL"),
		*PlayerId.ToDebugString());
}

bool URustyNetConnection::SendServerRequest(ENetServerRequest RequestType, TArray<uint8> Value, int32 PropertyId) {
	if (Socket == nullptr) {
		Socket = ((URustyIpNetDriver*)Driver)->CreateSocket();
	}
	NetBytes Message = NetBytes();
	NetIdentifier id = _playerId;// FCString::IsNumeric(*PlayerId.GetUniqueNetId().Get()->ToString()) ? FCString::Atoi(*PlayerId.GetUniqueNetId().Get()->ToString()) : 0;
	URustyNetConnection::Build_ServerRequest(RequestType, Message, id);

	TSharedRef<FInternetAddr> addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	TArray<FString> ips;
	settings->Server.Host.ParseIntoArray(ips, TEXT("."), true);
	FIPv4Address ip(FCString::Atoi(*ips[0]), FCString::Atoi(*ips[1]), FCString::Atoi(*ips[2]), FCString::Atoi(*ips[3]));

	addr->SetIp(ip.Value);

	addr->SetPort(settings->Server.Port);

	bool connected = Socket->Connect(*addr);
	int32 sent = 0;
	std::vector<uint8> msg;
	msg.resize(Message.Num());
	for (int i = 0; i< Message.Num(); i++) {
		msg[i] = Message[i];
	}
	bool bSent = Socket->Send(msg.data(), msg.size(), sent);

	Socket->GetAddress(*addr);
	FString Accepted;
	Socket->Accept(Accepted);
	UE_LOG(LogTemp, Log, TEXT("Sent: [%s] {%i} {%i} {%i} {%i}"), *addr.Get().ToString(true), bSent, Message.Num(), msg.size(), sent);
	UE_LOG(LogTemp, Log, TEXT("InitConnect: {%s} Status %s"), *Accepted, *GetStateString());
	return true;
}

void URustyNetConnection::RecvMesage(const NetBytes In) {
	if (In.Num() < 2) return;
	uint8 route = In[0];
	switch (route) {
	case (uint8)ENetAddress::ServerReq:
		Recv_ServerRequest(In);
		break;
	default:
		break;
	}
}
int32 URustyNetConnection::_playerId = 1;
const TSharedPtr<const FUniqueNetId> URustyNetConnection::playerIdptr = 0;// MakeShareable<FUniqueNetId>(&player_uid);

void URustyNetConnection::SetId(int32 newId) {
	FUniqueNetIdString player_uid = FUniqueNetIdString(FString::Printf(TEXT("%i"),newId));
	//const TSharedPtr<const FUniqueNetId> playerIdptr= MakeShareable<FUniqueNetId>(&player_uid);
	//PlayerId.SetUniqueNetId(playerIdptr);
	_playerId = newId;
}
//
//UChannel* URustyNetConnection::CreateChannel(EChannelType ChType, bool bOpenedLocally, int32 ChIndex)
//{
//	check(Driver->IsKnownChannelType(ChType));
//	AssertValid();
//
//	// If no channel index was specified, find the first available.
//	if (ChIndex == INDEX_NONE)
//	{
//		int32 FirstChannel = 1;
//		// Control channel is hardcoded to live at location 0
//		if (ChType == CHTYPE_Control)
//		{
//			FirstChannel = 0;
//		}
//
//		// If this is a voice channel, use its predefined channel index
//		if (ChType == CHTYPE_Voice)
//		{
//			FirstChannel = VOICE_CHANNEL_INDEX;
//		}
//
//		// Search the channel array for an available location
//		for (ChIndex = FirstChannel; ChIndex<MAX_CHANNELS; ChIndex++)
//		{
//			if (!Channels[ChIndex])
//			{
//				break;
//			}
//		}
//		// Fail to create if the channel array is full
//		if (ChIndex == MAX_CHANNELS)
//		{
//			return NULL;
//		}
//	}
//
//	// Make sure channel is valid.
//	check(ChIndex<MAX_CHANNELS);
//	check(Channels[ChIndex] == NULL);
//
//	// Create channel.
//	UChannel* Channel = NewObject<UChannel>(GetTransientPackage(), Driver->ChannelClasses[ChType]);
//	Channel->Init(this, ChIndex, bOpenedLocally);
//	Channels[ChIndex] = Channel;
//	OpenChannels.Add(Channel);
//	// Always tick the control & voice channels
//	if (Channel->ChType == CHTYPE_Control || Channel->ChType == CHTYPE_Voice)
//	{
//		StartTickingChannel(Channel);
//	}
//	UE_LOG(LogNetTraffic, Log, TEXT("Created channel %i of type %i"), ChIndex, (int32)ChType);
//
//	return Channel;
//}

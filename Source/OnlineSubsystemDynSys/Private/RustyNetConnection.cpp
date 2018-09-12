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
#include "RustyWorldSettings.h"

URustyNetConnection::URustyNetConnection(const FObjectInitializer& ObjectInitializer):Super(ObjectInitializer){
	SetId(_playerId);
}
FOnRegisterComplete URustyNetConnection::OnRegisterComplete = FOnRegisterComplete();
void URustyNetConnection::Tick() {
	Super::Tick();
	if( ((URustyIpNetDriver*)Driver)->Socket)  {
		auto Socket = ((URustyIpNetDriver*)Driver)->Socket;
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
		TArray<uint8> Bytes= TArray<uint8>();
		if(Build_Ping(Bytes, _playerId))
			SendServerRequest(ENetServerRequest::FunctionRep, Bytes);
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
	if (Driver == nullptr) {
		return false;
	}
	if (Socket == nullptr) {
		Socket = ((URustyIpNetDriver*)Driver)->Socket;
	}
	NetBytes Message = NetBytes();
	NetIdentifier id = _playerId;// FCString::IsNumeric(*PlayerId.GetUniqueNetId().Get()->ToString()) ? FCString::Atoi(*PlayerId.GetUniqueNetId().Get()->ToString()) : 0;
	URustyNetConnection::Build_ServerRequest(RequestType, Message, id);

	TSharedRef<FInternetAddr> addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	//TArray<FString> ips;
	//settings->Server.Host.ParseIntoArray(ips, TEXT("."), true);
	//FIPv4Address ip(FCString::Atoi(*ips[0]), FCString::Atoi(*ips[1]), FCString::Atoi(*ips[2]), FCString::Atoi(*ips[3]));

	//addr->SetIp(ip.Value);

	//addr->SetPort(settings->Server.Port);

	//bool connected = Socket->Connect(*addr);
	int32 sent = 0;

	//NetBytes compressed = NetBytes();
	//Compress(Message, compressed);
	NetBytes compressed = Message;
	std::vector<uint8> msg;
	msg.resize(compressed.Num());
	for (int i = 0; i< compressed.Num(); i++) {
		msg[i] = compressed[i];
	}
	bool bSent = Socket->Send(msg.data(), msg.size(), sent);

	Socket->GetAddress(*addr);
	Socket->GetPeerAddress(*addr);
	//FString Accepted;
	UE_LOG(LogTemp, Log, TEXT("Sent: %s=>%s {%i} {%i} {%i} {%i}"),*((URustyIpNetDriver*)Driver)->LocalAddr->ToString(true) ,*addr->ToString(true), bSent, Message.Num(), msg.size(), sent);
	//UE_LOG(LogTemp, Log, TEXT("InitConnect: {%s} Status %s"), *Accepted, *GetStateString());
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
bool URustyNetConnection::Decompress(NetBytes Compressed, NetBytes& Decompressed) {
	FArchiveLoadCompressedProxy Decompressor =
		FArchiveLoadCompressedProxy(Compressed, ECompressionFlags::COMPRESS_ZLIB);
	if (Decompressor.GetError())
	{
		UE_LOG(LogTemp, VeryVerbose, TEXT("FArchiveLoadCompressedProxy>> ERROR : File Was Not Compressed "));
		return false;
	}
	FBufferArchive DecompressedBinaryArray;
	Decompressor << DecompressedBinaryArray;
	if (DecompressedBinaryArray.Num() <= 0) return false;
	FMemoryReader FromBinary = FMemoryReader(DecompressedBinaryArray, true); //true, free data after done
	FromBinary.Seek(0);
	FromBinary << Decompressed;
	FromBinary.FlushCache();
	DecompressedBinaryArray.Empty();
	FromBinary.Close();
	return true;
}
bool URustyNetConnection::Compress(NetBytes Msg, NetBytes& Out) {
	FBufferArchive Ar = FBufferArchive();
	FArchiveSaveCompressedProxy Compressor =
		FArchiveSaveCompressedProxy(Msg, ECompressionFlags::COMPRESS_ZLIB);
	Compressor << Ar;
	Compressor.Flush();
	Out = Ar;
	return true;
}
void URustyNetConnection::TriggerPropertyReplication(NetIdentifier PropId, NetBytes In) {

	UE_LOG(LogTemp, Log, TEXT("Received PropId {%i}"), PropId);
	settings->TriggerPropertyReplication(PropId, In);
}


int32 URustyNetConnection::_playerId = 1;
const TSharedPtr<const FUniqueNetId> URustyNetConnection::playerIdptr = 0;// MakeShareable<FUniqueNetId>(&player_uid);

void URustyNetConnection::SetId(int32 newId) {
	FUniqueNetIdString player_uid = FUniqueNetIdString(FString::Printf(TEXT("%i"),newId));
	//const TSharedPtr<const FUniqueNetId> playerIdptr= MakeShareable<FUniqueNetId>(&player_uid);
	//PlayerId.SetUniqueNetId(playerIdptr);
	_playerId = newId;
}
void URustyNetConnection::Recv_ServerRequest(const NetBytes In) {
	NetIdentifier Sender = 0;
	NetIdentifier PropId = 0;
	int32 uid = 0;
	NetBytes Value = NetBytes();
	if (In.Num() > 2) {
		if (In[0] == (uint8)ENetAddress::ServerReq) {
			switch (In[1]) {
			case (uint8)ENetServerRequest::Ack:
				URustyNetConnection::Recv_Ping(In);
				break;
			case (uint8)ENetServerRequest::Register:
				URustyNetConnection::Recv_Register(In, uid);
				settings->UserId;
				_playerId = uid;
				SetId(uid);
				break;
			case (uint8)ENetServerRequest::Time:
				uint64 ServerTime;
				URustyNetConnection::Recv_Time(In, ServerTime);
				break;
			case (uint8)ENetServerRequest::FunctionRep:
				URustyNetConnection::Recv_FunctionRep(In, Sender, PropId, Value);
				//Do Something;
				break;
			case (uint8)ENetServerRequest::CallbackUpdate:
				URustyNetConnection::Recv_CallbackRep(In, Sender, PropId, Value);
				//Do Something;
				break;
			case (uint8)ENetServerRequest::PropertyRep:
				URustyNetConnection::Recv_PropertyRep(In, Sender, PropId, Value);
				TriggerPropertyReplication(PropId, Value);
				//Do Something;
				break;
			default:
				break;
			}
		}
	}
}

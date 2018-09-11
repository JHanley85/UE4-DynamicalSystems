// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/NetConnection.h"
#include "NetClient.h"
#include "RustyWorldSettings.h"
#include "IpConnection.h"
#include "RustyNetConnection.generated.h"

typedef TArray<uint8> Header;
typedef TArray<uint8> NetBytes;
typedef uint32 NetIdentifier;

struct RustyNetId : public FUniqueNetId {

};
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRegisterComplete, int32,RegisteredNetId);
DECLARE_DYNAMIC_DELEGATE(FOnConnectionTick);
/**
 * 
 */
UCLASS()
class ONLINESUBSYSTEMDYNSYS_API URustyNetConnection : public UIpConnection
{
	GENERATED_BODY()
public:
	URustyNetConnection(const FObjectInitializer& ObjectInitializer);
	virtual void Tick() override;
	//UChannel* CreateChannel(EChannelType Type, bool bOpenedLocally, int32 ChannelIndex = INDEX_NONE) override; 
	static bool Build_ServerRequest(const ENetServerRequest request ,NetBytes& Out,const NetIdentifier Sender, const NetIdentifier PropertyId=0,const NetBytes PropertyValue=NetBytes()) {
		switch (request) {
		case ENetServerRequest::Ack:
			return Build_Ping(Out, Sender);
			break;
		case ENetServerRequest::CallbackUpdate:
			return Build_CallbackRep(Out, Sender,PropertyId,PropertyValue);
			break;
		case ENetServerRequest::FunctionRep:
			return Build_FunctionRep(Out, Sender, PropertyId, PropertyValue);
			break;
		case ENetServerRequest::PropertyRep:
			return Build_PropertyRep(Out, Sender, PropertyId, PropertyValue);
			break;
		case ENetServerRequest::Time:
			return Build_Time(Out, Sender);
			break;
		case ENetServerRequest::Register:
			return Build_Register(Out, Sender);
			break;
		default:
			return false;
		}
	}
	void Recv_ServerRequest(const NetBytes In) {
		NetIdentifier Sender = 0;
		NetIdentifier PropId = 0;
		int32 uid = 0;
		NetBytes Value = NetBytes();
		if (In.Num() > 2) {
			if (In[0] == (uint8)ENetAddress::ServerReq) {
				switch (In[1]) {
				case (uint8) ENetServerRequest::Ack:
					URustyNetConnection::Recv_Ping(In);
					break;
				case (uint8) ENetServerRequest::Register:
					URustyNetConnection::Recv_Register(In, uid);
					settings->UserId;
					_playerId = uid;
					SetId(uid);
					break;
				case (uint8) ENetServerRequest::Time:
					uint64 ServerTime;
					URustyNetConnection::Recv_Time(In,ServerTime);
					break;
				case (uint8)ENetServerRequest::FunctionRep:
					URustyNetConnection::Recv_FunctionRep(In,Sender,PropId,Value);
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
	static void Recv_PropertyRep(const NetBytes In, NetIdentifier& Sender, NetIdentifier& PropertyId,NetBytes& Value) {
		const uint8 uidStartBits = 2;
		const uint8 pidStartBits = uidStartBits + 4;
		const uint8 ValueStart = pidStartBits + 4;
		TArray<uint8> UidBits = TArray<uint8>();
		TArray<uint8> PidBits = TArray<uint8>();

#if PLATFORM_LITTLE_ENDIAN
		UidBits[3] = In[uidStartBits];
		UidBits[2] = In[uidStartBits + 1];
		UidBits[1] = In[uidStartBits + 2];
		UidBits[0] = In[uidStartBits + 3];
#else
		UidBits[0] = In[uidStartBits];
		UidBits[1] = In[uidStartBits + 1];
		UidBits[2] = In[uidStartBits + 2];
		UidBits[3] = In[uidStartBits + 3];
#endif
#if PLATFORM_LITTLE_ENDIAN
		PidBits[3] = In[pidStartBits];
		PidBits[2] = In[pidStartBits + 1];
		PidBits[1] = In[pidStartBits + 2];
		PidBits[0] = In[pidStartBits + 3];
#else
		PidBits[0] = In[pidStartBits];
		PidBits[1] = In[pidStartBits + 1];
		PidBits[2] = In[pidStartBits + 2];
		PidBits[3] = In[pidStartBits + 3];
#endif
	}
	static bool Build_PropertyRep(Header& Out,const NetIdentifier Sender,const NetIdentifier PropertyId, NetBytes Value) {
		Out.SetNum(2);
		Out[0] = (uint8)ENetAddress::ServerReq;
		Out[1] = (uint8)ENetServerRequest::PropertyRep;
		Out.Shrink();
		Out.Append(ToBytesNetId(Sender));
		Out.Append(ToBytesNetId(PropertyId));
		return true;
	}
	static void Recv_FunctionRep(const NetBytes In, NetIdentifier& Sender, NetIdentifier& PropertyId, NetBytes& Value) {
		const uint8 uidStartBits = 2;
		const uint8 pidStartBits = uidStartBits + 4;
		const uint8 ValueStart = pidStartBits + 4;
		TArray<uint8> UidBits = TArray<uint8>();
		TArray<uint8> PidBits = TArray<uint8>();

#if PLATFORM_LITTLE_ENDIAN
		UidBits[3] = In[uidStartBits];
		UidBits[2] = In[uidStartBits + 1];
		UidBits[1] = In[uidStartBits + 2];
		UidBits[0] = In[uidStartBits + 3];
#else
		UidBits[0] = In[uidStartBits];
		UidBits[1] = In[uidStartBits + 1];
		UidBits[2] = In[uidStartBits + 2];
		UidBits[3] = In[uidStartBits + 3];
#endif
#if PLATFORM_LITTLE_ENDIAN
		PidBits[3] = In[pidStartBits];
		PidBits[2] = In[pidStartBits + 1];
		PidBits[1] = In[pidStartBits + 2];
		PidBits[0] = In[pidStartBits + 3];
#else
		PidBits[0] = In[pidStartBits];
		PidBits[1] = In[pidStartBits + 1];
		PidBits[2] = In[pidStartBits + 2];
		PidBits[3] = In[pidStartBits + 3];
#endif
	}
	static bool Build_FunctionRep(Header& Out,const NetIdentifier Sender, const NetIdentifier FunctionId, NetBytes Value) {
		Out.SetNum(2);
		Out[0] = (uint8)ENetAddress::ServerReq;
		Out[1] = (uint8)ENetServerRequest::FunctionRep;
		Out.Shrink();
		Out.Append(ToBytesNetId(Sender));
		Out.Append(ToBytesNetId(FunctionId));
		return true;
	}
	static void Recv_CallbackRep(const NetBytes In, NetIdentifier& Sender, NetIdentifier& PropertyId, NetBytes& Value) {
		const uint8 uidStartBits = 2;
		const uint8 pidStartBits = uidStartBits + 4;
		const uint8 ValueStart = pidStartBits + 4;
		TArray<uint8> UidBits = TArray<uint8>();
		TArray<uint8> PidBits = TArray<uint8>();

#if PLATFORM_LITTLE_ENDIAN
		UidBits[3] = In[uidStartBits];
		UidBits[2] = In[uidStartBits + 1];
		UidBits[1] = In[uidStartBits + 2];
		UidBits[0] = In[uidStartBits + 3];
#else
		UidBits[0] = In[uidStartBits];
		UidBits[1] = In[uidStartBits + 1];
		UidBits[2] = In[uidStartBits + 2];
		UidBits[3] = In[uidStartBits + 3];
#endif
#if PLATFORM_LITTLE_ENDIAN
		PidBits[3] = In[pidStartBits];
		PidBits[2] = In[pidStartBits + 1];
		PidBits[1] = In[pidStartBits + 2];
		PidBits[0] = In[pidStartBits + 3];
#else
		PidBits[0] = In[pidStartBits];
		PidBits[1] = In[pidStartBits + 1];
		PidBits[2] = In[pidStartBits + 2];
		PidBits[3] = In[pidStartBits + 3];
#endif
	}
	static bool Build_CallbackRep(Header& Out, const NetIdentifier Sender, const NetIdentifier CallbackId, NetBytes Value) {
		Out.SetNum(2);
		Out[0] = (uint8)ENetAddress::ServerReq;
		Out[1] = (uint8)ENetServerRequest::CallbackUpdate;
		Out.Shrink();
		Out.Append(ToBytesNetId(Sender));
		Out.Append(ToBytesNetId(CallbackId));
		return true;
	}
	static bool Build_Ping(Header& Out, NetIdentifier Sender) {
		Out = TArray<uint8>();
		UE_LOG(LogTemp, Log, TEXT("send 1 Ping {%u}"), Sender);
		Out.SetNum(2);
		Out[0] = (uint8) ENetAddress::ServerReq;
		Out[1] = (uint8) ENetServerRequest::Ack;
		Out.Shrink();
		Out.Append(ToBytesNetId(Sender));
		UE_LOG(LogTemp, Log, TEXT("send 4 Ping {%u}"), Sender);
		return Out.Num() == 6;
	}
	static void Recv_Ping(const NetBytes In) {
		uint8 StartBits = 2;
		TArray<uint8> GuidBits = TArray<uint8>();
		GuidBits.SetNum(4);
#if PLATFORM_LITTLE_ENDIAN
		GuidBits[3] = In[StartBits];
		GuidBits[2] = In[StartBits + 1];
		GuidBits[1] = In[StartBits + 2];
		GuidBits[0] = In[StartBits + 3];
#else
		GuidBits[0] = In[StartBits];
		GuidBits[1] = In[StartBits + 1];
		GuidBits[2] = In[StartBits + 2];
		GuidBits[3] = In[StartBits + 3];
#endif
		NetIdentifier uid = 0;
		uid = ((GuidBits[0] << 24) |
			(GuidBits[1] << 16) |
			(GuidBits[2] << 8) |
			GuidBits[3]);
		UE_LOG(LogTemp, Log, TEXT("Rec'd Ping {%u}"), uid);

	}
	static bool Build_Register(Header& Out, NetIdentifier Sender) {
		Out = TArray<uint8>();
		Out.SetNum(2);
		Out[0] = (uint8) ENetAddress::ServerReq;
		Out[1] = (uint8) ENetServerRequest::Register;
		Out.Shrink();
		Out.Append(ToBytesNetId(Sender));
		return Out.Num()==6;
	}
	static void Recv_Register(const NetBytes In,int32& OutId) {
		uint8 StartBits = 2;
		TArray<uint8> GuidBits = TArray<uint8>();
		GuidBits.SetNum(4);
#if PLATFORM_LITTLE_ENDIAN
		GuidBits[3] = In[StartBits];
		GuidBits[2] = In[StartBits + 1];
		GuidBits[1] = In[StartBits + 2];
		GuidBits[0] = In[StartBits + 3];
#else
		GuidBits[0] = In[StartBits];
		GuidBits[1] = In[StartBits + 1];
		GuidBits[2] = In[StartBits + 2];
		GuidBits[3] = In[StartBits + 3];
#endif
		NetIdentifier uid = 0;
		uid = ((GuidBits[0] << 24) |
			(GuidBits[1] << 16) |
			(GuidBits[2] << 8) |
			GuidBits[3]);
		UE_LOG(LogTemp, Log, TEXT("New UUID: %u"), uid);
		OnRegisterComplete.Broadcast(uid);
		OutId = uid;
	}
	static FOnRegisterComplete OnRegisterComplete;
	static bool Build_Time(Header& Out,NetIdentifier Sender) {
		Out = TArray<uint8>();
		Out.SetNum(2);
		Out[0] = (uint8) ENetAddress::ServerReq;
		Out[1] = (uint8) ENetServerRequest::Time;
		Out.Shrink();
		Out.Append(ToBytesNetId(Sender));
		return Out.Num() == 6;
	}
	static void Recv_Time(const NetBytes In,uint64& TimeOut) {
		uint8 StartBits = 2;
		TArray<uint8> GuidBits = TArray<uint8>();
		GuidBits.SetNum(4);
#if PLATFORM_LITTLE_ENDIAN
		GuidBits[3] = In[StartBits];
		GuidBits[2] = In[StartBits + 1];
		GuidBits[1] = In[StartBits + 2];
		GuidBits[0] = In[StartBits + 3];
#else
		GuidBits[0] = In[StartBits];
		GuidBits[1] = In[StartBits + 1];
		GuidBits[2] = In[StartBits + 2];
		GuidBits[3] = In[StartBits + 3];
#endif
	}
	ARustyWorldSettings* settings;

	FString GetStateString() {
		if (State) {
			switch (State) {
			case EConnectionState::USOCK_Closed:
				return "Closed";
			case EConnectionState::USOCK_Invalid:
				return "Invalid";
			case EConnectionState::USOCK_Open:
				return "Open";
			case EConnectionState::USOCK_Pending:
				return "Pending";
			default:
				return "";
			}
		}
		return "";
	}
	static TArray<uint8> ToBytesNetId(NetIdentifier guid) {
		TArray<uint8> msg= TArray<uint8>();
		msg.SetNumUninitialized(4);
		msg[0] = guid;
		msg[1] = guid >> 8;
		msg[2] = guid >> 16;
		msg[3] = guid >> 24;
		msg.Shrink();
		return msg;
	}
	virtual FString Describe() override;
	FSocket* Socket;
	UFUNCTION(BlueprintCallable, Category = OSS, meta = (AutoCreateRefTerm = "Value"))
		bool SendServerRequest(ENetServerRequest RequestType, TArray<uint8> Value, int32 PropertyId = 0);
	void RecvMesage(const NetBytes In);

	static int32 _playerId;
	void SetId(int32 newId);
	static const TSharedPtr<const FUniqueNetId> playerIdptr;

	void TriggerPropertyReplication(NetIdentifier PropId, NetBytes In) {
		UE_LOG(LogTemp,Log,TEXT("Received PropId {%i}"), PropId);
	}
};

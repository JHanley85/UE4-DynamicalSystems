#include "NetClient.h"
#include "NetAvatar.h"
#include "NetVoice.h"
#include "NetRigidBody.h"
#include "RustyDynamics.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"
#include "Base64.h"
#include "DynamicalSystemsSettings.h"
#include "DynamicalSystemsPrivatePCH.h"

DEFINE_LOG_CATEGORY(RustyNet);
#define APIVersion 1

ANetClient::ANetClient()
{
	PrimaryActorTick.bCanEverTick = true;
	InitializeWithSettings();

	if (Local == "0.0.0.0") {
		bool bCanBindAll;
		TSharedPtr<class FInternetAddr> localIp = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLocalHostAddr(*GLog, bCanBindAll);
		Local = localIp->ToString(true);
	}
	if (Instance == nullptr)
		Instance = this;
}

void ANetClient::RegisterRigidBody(UNetRigidBody* RigidBody)
{
	UE_LOG(RustyNet, VeryVerbose, TEXT("ANetClient::RegisterRigidBody %s"), *RigidBody->GetOwner()->GetName());
	NetRigidBodies.Add(RigidBody);
}

void ANetClient::RegisterAvatar(UNetAvatar* _Avatar)
{
    UE_LOG(RustyNet, VeryVerbose, TEXT("ANetClient::RegisterAvatar %i %s"), _Avatar->NetID, *_Avatar->GetOwner()->GetName());
    NetAvatars.Add(_Avatar);
}

void ANetClient::RegisterVoice(UNetVoice* Voice)
{
    UE_LOG(RustyNet, VeryVerbose, TEXT("ANetClient::RegisterVoice %s"), *Voice->GetOwner()->GetName());
    NetVoices.Add(Voice);
}

void ANetClient::Say(uint8* Bytes, uint32 Count)
{
}

void ANetClient::RebuildConsensus()
{
    int Count = NetClients.Num();
    FRandomStream Rnd(Count);
    
    MappedClients.Empty();
    NetClients.GetKeys(MappedClients);
    MappedClients.Sort();

	NetIndex = MappedClients.IndexOfByKey(Uuid);
    
    NetRigidBodies.Sort([](const UNetRigidBody& LHS, const UNetRigidBody& RHS) {
        return LHS.NetID > RHS.NetID; });
    for (auto It = NetRigidBodies.CreateConstIterator(); It; ++It) {
        (*It)->NetOwner = Rnd.RandRange(0, Count);
    }
}

void ANetClient::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	Uuid = rb_uuid();
}

void ANetClient::BeginPlay()
{
    Super::BeginPlay();
	InitializeWithSettings();

    bool bCanBindAll;
	if (Local == "0.0.0.0") {
		TSharedPtr<class FInternetAddr> localIp = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLocalHostAddr(*GLog, bCanBindAll);
		Local = localIp->ToString(true);
	}
    UE_LOG(RustyNet, VeryVerbose, TEXT("GetLocalHostAddr %s"), *Local);
    LastPingTime = UGameplayStatics::GetRealTimeSeconds(GetWorld());
    LastAvatarTime = LastPingTime;
	LastRigidbodyTime = LastPingTime;
    if (Client == NULL) {
        Client = rd_netclient_open(TCHAR_TO_ANSI(*Local), TCHAR_TO_ANSI(*Server), TCHAR_TO_ANSI(*MumbleServer), TCHAR_TO_ANSI(*AudioDevice));
        NetClients.Add(Uuid, -1);
        UE_LOG(RustyNet, VeryVerbose, TEXT("NetClient BeginPlay %i"), Uuid);
    }
}

void ANetClient::BeginDestroy()
{
    Super::BeginDestroy();
    if (Client != NULL) {
		UE_LOG(RustyNet, VeryVerbose, TEXT("NetClient BeginDestroy %i"), Uuid);
        rd_netclient_drop(Client);
        Client = NULL;
    }
}
void ANetClient::EndPlay(EEndPlayReason::Type EndPlayReason){
	Super::EndPlay(EndPlayReason);
	if (Client != NULL) {
		UE_LOG(RustyNet, VeryVerbose, TEXT("NetClient BeginDestroy %i"), Uuid);
		rd_netclient_drop(Client);
		Client = NULL;
	}
}
void ANetClient::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
    
    float CurrentTime = UGameplayStatics::GetRealTimeSeconds(GetWorld());
    float CurrentAvatarTime = CurrentTime;
	float CurrentRigidbodyTime = CurrentTime;
    
	for (int Idx=0; Idx<NetRigidBodies.Num();) {
		if (!IsValid(NetRigidBodies[Idx])) {
			NetRigidBodies.RemoveAt(Idx);
		}
		else {
			++Idx;
		}
	}

    for (int Idx=0; Idx<NetAvatars.Num();) {
        if (!IsValid(NetAvatars[Idx])) {
            NetAvatars.RemoveAt(Idx);
        }
        else {
            ++Idx;
        }
    }
    
    for (int Idx=0; Idx<NetVoices.Num();) {
        if (!IsValid(NetVoices[Idx])) {
            NetVoices.RemoveAt(Idx);
        }
        else {
            ++Idx;
        }
    }
    
	{
        TArray<int32> DeleteList;
        for (auto& Elem : NetClients) {
            if (Elem.Value > 0 && (CurrentTime - Elem.Value) > 20) {
				UE_LOG(RustyNet, VeryVerbose, TEXT("NetClient DELETED: %i"), Elem.Key);
                DeleteList.Add(Elem.Key);
            }
        }
        for (auto& Key : DeleteList) {
            NetClients.Remove(Key);
			RebuildConsensus();
        }
    }
    
    if (CurrentTime > (LastPingTime + PingTimeout)) { // Ping
		uint8 Msg[5];
        Msg[0] = 0;
		//TODO: byte order
		uint8* bytes = (uint8*)(&Uuid);
		Msg[1] = bytes[0];
		Msg[2] = bytes[1];
		Msg[3] = bytes[2];
		Msg[4] = bytes[3];
        rd_netclient_msg_push(Client, Msg, 5);
        LastPingTime = CurrentTime;
    }
    
    if (CurrentAvatarTime > LastAvatarTime + 0.1) { // Avatar
		if (IsValid(Avatar) && !Avatar->IsNetProxy) {
			AvatarPack Pack;
			memset(&Pack, 0, sizeof(AvatarPack));
			FQuat Rotation;
			Pack.id = Avatar->NetID;
			Pack.root_px = Avatar->Location.X; Pack.root_py = Avatar->Location.Y; Pack.root_pz = Avatar->Location.Z;
			Rotation = Avatar->Rotation.Quaternion();
			Pack.root_rx = Rotation.X; Pack.root_ry = Rotation.Y; Pack.root_rz = Rotation.Z; Pack.root_rw = Rotation.W;
			Pack.head_px = Avatar->LocationHMD.X; Pack.head_py = Avatar->LocationHMD.Y; Pack.head_pz = Avatar->LocationHMD.Z;
			Rotation = Avatar->RotationHMD.Quaternion();
			Pack.head_rx = Rotation.X; Pack.head_ry = Rotation.Y; Pack.head_rz = Rotation.Z; Pack.head_rw = Rotation.W;
			Pack.handL_px = Avatar->LocationHandL.X; Pack.handL_py = Avatar->LocationHandL.Y; Pack.handL_pz = Avatar->LocationHandL.Z;
			Rotation = Avatar->RotationHandL.Quaternion();
			Pack.handL_rx = Rotation.X; Pack.handL_ry = Rotation.Y; Pack.handL_rz = Rotation.Z; Pack.handL_rw = Rotation.W;
			Pack.handR_px = Avatar->LocationHandR.X; Pack.handR_py = Avatar->LocationHandR.Y; Pack.handR_pz = Avatar->LocationHandR.Z;
			Rotation = Avatar->RotationHandR.Quaternion();
			Pack.handR_rx = Rotation.X; Pack.handR_ry = Rotation.Y; Pack.handR_rz = Rotation.Z; Pack.handR_rw = Rotation.W;
			Pack.height = Avatar->Height;
			Pack.floor = Avatar->Floor;
			//rd_netclient_push_avatar(Client, &Pack);
		}
        LastAvatarTime = CurrentAvatarTime;
    }

	if (CurrentRigidbodyTime > LastRigidbodyTime + 0.1) { // Rigidbody
		for (int Idx=0; Idx<NetRigidBodies.Num(); ++Idx) {
		    UNetRigidBody* Body = NetRigidBodies[Idx];
		    if (IsValid(Body) && Body->NetOwner == NetIndex) {
		        AActor* Actor = Body->GetOwner();
		        if (IsValid(Actor)) {
		            FVector Location = Actor->GetActorLocation();
					FQuat Rotation = Actor->GetActorRotation().Quaternion();
		            UStaticMeshComponent* StaticMesh = Actor->FindComponentByClass<UStaticMeshComponent>();
		            if (StaticMesh) {
						FVector LinearVelocity = StaticMesh->GetBodyInstance()->GetUnrealWorldVelocity();
						//FVector AngularVelocity = StaticMesh->GetBodyInstance()->GetUnrealWorldAngularVelocity();
						FVector AngularVelocity = FMath::RadiansToDegrees(StaticMesh->GetBodyInstance()->GetUnrealWorldAngularVelocityInRadians());
		                RigidbodyPack Pack = {Body->NetID,
		                    Location.X, Location.Y, Location.Z, 1,
		                    LinearVelocity.X, LinearVelocity.Y, LinearVelocity.Z, 0,
							Rotation.X, Rotation.Y, Rotation.Z, Rotation.W,
							AngularVelocity.X, AngularVelocity.Y, AngularVelocity.Z, 0
		                };
						rd_netclient_push_rigidbody(Client, &Pack);
		            }
		        }
		    }
		}
		LastRigidbodyTime = CurrentRigidbodyTime;
	}
    
	int Loop = 0;
	for (; Loop < 1000; Loop += 1) {
		RustVec* RustMsg = rd_netclient_msg_pop(Client);
		uint8* Msg = (uint8*)RustMsg->vec_ptr;
		if (RustMsg->vec_len > 0) {

			UE_LOG(RustyNet, VeryVerbose, TEXT("Msg IN %i"), Msg);
			;
			if (Msg[0] == ENetAddress::Ping) { // Ping
				uint32 RemoteUuid = *((uint32*)(Msg + 1));
				// float* KeyValue = NetClients.Find(RemoteUuid);
				// if (KeyValue != NULL) {
				// UE_LOG(RustyNet, VeryVerbose, TEXT("PING: %i"), RemoteUuid);
				// }
				NetClients.Add(RemoteUuid, CurrentTime);
				RebuildConsensus();
			}
			else if (Msg[0] == ENetAddress::World) { // World
				int32* MsgValue = (int64*)(Msg + 1);
				UE_LOG(RustyNet, VeryVerbose, TEXT("Msg IN ServerTime MsgValue: %f"), Msg[1], Msg[2], *MsgValue);

			}
			else if (Msg[0] == ENetAddress::Avatar ) { // Avatar
				AvatarPack* Pack = rd_netclient_dec_avatar(&Msg[1], RustMsg->vec_len - 1);
				uint32 NetID = Pack->id;
				UNetAvatar** NetAvatar = NetAvatars.FindByPredicate([NetID](const UNetAvatar* Item) {
					return IsValid(Item) && Item->NetID == NetID;
				});
				if (NetAvatar != NULL && *NetAvatar != NULL) {
					(*NetAvatar)->LastUpdateTime = CurrentTime;
					(*NetAvatar)->Location = FVector(Pack->root_px, Pack->root_py, Pack->root_pz);
					(*NetAvatar)->Rotation = FRotator(FQuat(Pack->root_rx, Pack->root_ry, Pack->root_rz, Pack->root_rw));
					(*NetAvatar)->LocationHMD = FVector(Pack->head_px, Pack->head_py, Pack->head_pz);
					(*NetAvatar)->RotationHMD = FRotator(FQuat(Pack->head_rx, Pack->head_ry, Pack->head_rz, Pack->head_rw));
					(*NetAvatar)->LocationHandL = FVector(Pack->handL_px, Pack->handL_py, Pack->handL_pz);
					(*NetAvatar)->RotationHandL = FRotator(FQuat(Pack->handL_rx, Pack->handL_ry, Pack->handL_rz, Pack->handL_rw));
					(*NetAvatar)->LocationHandR = FVector(Pack->handR_px, Pack->handR_py, Pack->handR_pz);
					(*NetAvatar)->RotationHandR = FRotator(FQuat(Pack->handR_rx, Pack->handR_ry, Pack->handR_rz, Pack->handR_rw));
					(*NetAvatar)->Height = Pack->height;
					(*NetAvatar)->Floor = Pack->floor;
				}
				else {
					MissingAvatar = (int)NetID;
					UE_LOG(RustyNet, VeryVerbose, TEXT("NetClient MissingAvatar: %i"), NetID);
				}
				rd_netclient_drop_avatar(Pack);
			}
			else if (Msg[0] == ENetAddress::RigidBody ) { // Rigidbody
				RigidbodyPack* Pack = rd_netclient_dec_rigidbody(&Msg[1], RustMsg->vec_len - 1);
				uint32 NetID = Pack->id;
				UNetRigidBody** NetRigidBody = NetRigidBodies.FindByPredicate([NetID](const UNetRigidBody* Item) {
					return IsValid(Item) && Item->NetID == NetID;
				});
				if (NetRigidBody != NULL && *NetRigidBody != NULL) {
					(*NetRigidBody)->TargetLocation = FVector(Pack->px, Pack->py, Pack->pz);
					(*NetRigidBody)->TargetLinearVelocity = FVector(Pack->lx, Pack->ly, Pack->lz);
					(*NetRigidBody)->TargetRotation = FRotator(FQuat(Pack->rx, Pack->ry, Pack->rz, Pack->rw));
					(*NetRigidBody)->TargetAngularVelocity = FVector(Pack->ax, Pack->ay, Pack->az);
				}
				rd_netclient_drop_rigidbody(Pack);
			}
			else if (Msg[0] == ENetAddress::Float) { // System Float
				uint8 MsgSystem = Msg[1];
				uint8 MsgId = Msg[2];
				float* MsgValue = (float*)(Msg + 3);
				UE_LOG(RustyNet, VeryVerbose, TEXT("Msg IN MsgSystem: %u MsgId: %u MsgValue: %f"), Msg[1], Msg[2], *MsgValue);
				OnSystemFloatMsg.Broadcast(MsgSystem, MsgId, *MsgValue);
			}
			else if (Msg[0] == ENetAddress::Int) { // System Int
				uint8 MsgSystem = Msg[1];
				uint8 MsgId = Msg[2];
				int32* MsgValue = (int32*)(Msg + 3);
				UE_LOG(RustyNet, VeryVerbose, TEXT("Msg IN MsgSystem: %u MsgId: %u MsgValue: %i"), Msg[1], Msg[2], *MsgValue);
				OnSystemIntMsg.Broadcast(MsgSystem, MsgId, *MsgValue);
			}
			else if (Msg[0] == ENetAddress::String ) { // System String
				uint8 MsgSystem = Msg[1];
				uint8 MsgId = Msg[2];
				FString MsgValueBase64 = BytesToString(Msg + 3, RustMsg->vec_len - 3);
				FString MsgValue;
				FBase64::Decode(MsgValueBase64, MsgValue);
				UE_LOG(RustyNet, VeryVerbose, TEXT("Msg IN MsgSystem: %u MsgId: %u MsgValue: %s"), Msg[1], Msg[2], *MsgValue);
				OnSystemStringMsg.Broadcast(MsgSystem, MsgId, *MsgValue);
			}
			else if (Msg[0] == ENetAddress::ByteArray) {

				//ArchivePack* ARPack = rd_netclient_dec_archive(&Msg[5], RustMsg->vec_len - 5);
				//auto data = ARPack->data;
				///reverse
				uint8 bits[4] = { Msg[1],Msg[2],Msg[3],Msg[4] };
				
				int32 guid = bitsToInt(bits);
				int64 dataSize = (RustMsg->vec_len - 5);
				TArray<uint8> compressedData = TArray<uint8>();
				compressedData.SetNumUninitialized(dataSize,false);

				for (int i = 0; i<dataSize; i++) {
					compressedData[i] = Msg[i + 5];
				}
				if (OnByteArray.IsBound()) {
					OnByteArray.Execute(compressedData, guid);
				}
				/*ArchivePack* ARPack = rd_netclient_dec_archive(&Msg[1], RustMsg->vec_len - 1);
				std::vector<uint8_t> data = ARPack->data;
				TArray<uint8> ArchiveData = TArray<uint8>();
				ArchiveData.AddZeroed(data.size());
				for (int i = 0; i < data.size();i++) {
					ArchiveData[i] = data[i];
				}*/
				//rd_netclient_drop_archive(ARPack);
				/*if (OnByteArray.IsBound()) {
					OnByteArray.Execute(ArchiveData, ARPack->guid);
				}*/
				UE_LOG(RustyNet, VeryVerbose, TEXT("Msg IN ENetAddress::ByteArray bytes %i for %i"), compressedData.Num(),guid);

			}
			else {

				UE_LOG(RustyNet, VeryVerbose, TEXT("Msg IN %i: %i - bytes %i"), Msg, Msg[0],sizeof(Msg));
			}

		}
		rd_netclient_msg_drop(RustMsg);
		if (RustMsg->vec_len == 0) {
			//UE_LOG(RustyNet, VeryVerbose, TEXT("NetClient Loop: %i"), Loop);
			break;
		}
	}
}
int32 ANetClient::bitsToInt( const uint8* bits, bool little_endian)
{
	int32 result = 0;
	if (little_endian)
		for (int n = sizeof(result); n >= 0; n--)
			result = (result << 8) + bits[n];
	else
		for (unsigned n = 0; n < sizeof(result); n++)
			result = (result << 8) + bits[n];
	return result;
}
void ANetClient::SendSystemFloat(int32 System, int32 Id, float Value)
{
	uint8 Msg[7];
	Msg[0] = 10;
	Msg[1] = (uint8)System;
	Msg[2] = (uint8)Id;

	//TODO: byte order
	uint8* fbytes = (uint8*)(&Value);
	Msg[3] = fbytes[0];
	Msg[4] = fbytes[1];
	Msg[5] = fbytes[2];
	Msg[6] = fbytes[3];

	UE_LOG(RustyNet, VeryVerbose, TEXT("Msg OUT System Float: %u MsgId: %u MsgValue: %f"), Msg[1], Msg[2], Value);
	rd_netclient_msg_push(Client, Msg, 7);
}

void ANetClient::SendSystemInt(int32 System, int32 Id, int32 Value)
{
	uint8 Msg[7];
	Msg[0] = 11;
	Msg[1] = (uint8)System;
	Msg[2] = (uint8)Id;

	//TODO: byte order
	uint8* ibytes = (uint8*)(&Value);
	Msg[3] = ibytes[0];
	Msg[4] = ibytes[1];
	Msg[5] = ibytes[2];
	Msg[6] = ibytes[3];

	UE_LOG(RustyNet, VeryVerbose, TEXT("Msg OUT System Int: %u MsgId: %u MsgValue: %i"), Msg[1], Msg[2], Value);
	rd_netclient_msg_push(Client, Msg, 7);
}
void ANetClient::SendSystemString(int32 System, int32 Id, FString Value)
{
	uint8 Msg[2000];
	memset(Msg, 0, 2000);

	Msg[0] = 12;
	Msg[1] = (uint8)System;
	Msg[2] = (uint8)Id;

	int16 Len = StringToBytes(FBase64::Encode(Value), Msg + 3, 1024) + 1;

	UE_LOG(RustyNet, VeryVerbose, TEXT("Msg OUT System String: %u MsgId: %u MsgValue: %s"), Msg[1], Msg[2], *Value);
	rd_netclient_msg_push(Client, Msg, Len + 3);
}
#pragma region BPGetters
void ANetClient::InitializeWithSettings() {
	UE_LOG(LogTemp, VeryVerbose, TEXT("Initializing Netclient Env: %s"), *UDynamicalSystemsSettings::GetDynamicalSettings()->CurrentEnvironment.ToString());
	Server = GetServer();
	AudioDevice = GetAudioDevice();
	MumbleServer = GetMumbleServer();
	PingTimeout = UDynamicalSystemsSettings::GetDynamicalSettings()->PingTimeout;
}

FString ANetClient::GetServer() {
	FString temp = UDynamicalSystemsSettings::GetDynamicalSettings()->GetEnvSetting().Server;
	UE_LOG(LogTemp, VeryVerbose, TEXT("Setting Netclient Server to %s"), *temp);
	return temp;
}
FString ANetClient::GetMumbleServer() {
	FString temp = UDynamicalSystemsSettings::GetDynamicalSettings()->GetEnvSetting().MumbleServer;
	UE_LOG(LogTemp, VeryVerbose, TEXT("Setting Netclient Mumble Server to %s"), *temp);
	return temp;
}
FString ANetClient::GetAudioDevice() {
	FString temp = UDynamicalSystemsSettings::GetDynamicalSettings()->GetEnvSetting().AudioDevice;
	UE_LOG(LogTemp, VeryVerbose, TEXT("Setting Netclient Audio Device to %s"), *temp);
	return temp;
}
#pragma endregion BPGetters




#pragma region SendRec

FOnArchive ANetClient::OnByteArray;
bool ANetClient::SendByteArray(TArray<uint8> In, int32 guid) {

	std::vector<uint8> msg;

	msg.resize(In.Num() + 5);
	msg[0] = ENetAddress::ByteArray;
	msg[1] = guid;
	msg[2] = guid >> 8;
	msg[3] = guid >> 16;
	msg[4] = guid >> 24;

	for (int i = 0; i< In.Num(); i++) {
		msg[i + 5] = In[i];
	}

	UE_LOG(RustyNet, VeryVerbose, TEXT("Msg OUT System Archive size: %i for: %i"), In.Num() + 5, guid);
	rd_netclient_msg_push(Client, msg.data(), In.Num() + 5);
	return true;
	//rd_netclient_push_archive(Client, &pack);
}
bool ANetClient::AR_SendSystemFloatGlobal(int32 System, int32 Id, float Value) {
	TArray<uint8> Msg = TArray<uint8>();
	Msg[0] = ENetAddress::Float;
	Msg[1] = (uint8)System;
	Msg[2] = (uint8)Id;
	AppendSingleFloat(Value,Msg);
	//rd_netclient_msg_push(Client, Msg.GetData(), Msg.Num());
	return false;
}
bool CompressMessage(TArray<uint8> Uncompressed, TArray<uint8>& Compressed) {
	FBufferArchive Ar = FBufferArchive();
	Ar << Uncompressed;
	FArchiveSaveCompressedProxy Compressor =
		FArchiveSaveCompressedProxy(Compressed, ECompressionFlags::COMPRESS_ZLIB);
	//Send entire binary array/archive to compressor
	Compressor << Ar;
	//send archive serialized data to binary array
	Compressor.Flush();
	UE_LOG(LogTemp, VeryVerbose, TEXT(" Compressed Size ~ %i"), Compressed.Num());
	UE_LOG(LogTemp, VeryVerbose, TEXT(" Uncompressed Size ~ %i"), Ar.Num());
	return true;
}
bool DecompressMessage(TArray<uint8> Compressed, TArray<uint8>& Uncompressed) {
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
	FromBinary << Uncompressed;
	FromBinary.FlushCache();
	DecompressedBinaryArray.Empty();
	FromBinary.Close();
	return true;
}
bool  ANetClient::AppendSingleFloat(float Value,TArray<uint8>& AppendTo) {
	uint8* fbytes = (uint8*)(&Value);
	TArray<uint8> bytes;
	bytes.SetNumUninitialized(4);
	bytes[0] = fbytes[0];
	bytes[1] = fbytes[1];
	bytes[2] = fbytes[2];
	bytes[3] = fbytes[3];
	AppendTo.Append(bytes);
}
bool  ANetClient::AR_SendSystemIntGlobal(int32 System, int32 Id, int32 Value){
	TArray<uint8> Msg = TArray<uint8>();
	Msg[0] = ENetAddress::Int;
	Msg[1] = (uint8)System;
	Msg[2] = (uint8)Id;
	AppendSingleInt(Value, Msg);
	//rd_netclient_msg_push(Client, Msg.GetData(), Msg.Num());
	return false;
}

bool ANetClient::AppendSingleInt(int8 Value, TArray<uint8>& AppendTo) {
	uint8* ibytes = (uint8*)(&Value);
	TArray<uint8> bytes;
	bytes[0] = ibytes[0];
	bytes[1] = ibytes[1];
	bytes[2] = ibytes[2];
	bytes[3] = ibytes[3];
	AppendTo.Append(bytes);
}
bool  ANetClient::AR_SendSystemStringGlobal(int32 System, int32 Id, FString Value) {
	uint8 Msg[2000];
	memset(Msg, 0, 2000);
	Msg[0] = ENetAddress::String;
	Msg[1] = (uint8)System;
	Msg[2] = (uint8)Id;
	int16 Len = StringToBytes(FBase64::Encode(Value), Msg + 3, 1024) + 1;
	//rd_netclient_msg_push(Client, Msg, Len+3);
	return false;
}

bool  ANetClient::AppendSingleString(FString Value, TArray<uint8>& AppendTo) {
	return TArray<uint8>();
}
#pragma endregion SendRec

RouteMap  ANetClient::delegateMap = Routemap();
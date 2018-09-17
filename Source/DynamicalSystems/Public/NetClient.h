#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "NetClient.generated.h"



class UNetRigidBody;
class UNetAvatar;
class UNetVoice;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSystemFloatMsgDecl, int32, System, int32, Id, float, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSystemIntMsgDecl, int32, System, int32, Id, int32, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSystemStringMsgDecl, int32, System, int32, Id, FString, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FVoiceActivityMsgDecl, int32, NetId, float, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUint8MsgDecl, uint8, System, uint8, Id);

DECLARE_DYNAMIC_DELEGATE_OneParam(FDeserializeArchiveDelegate, TArray<uint8>,bytes);

DECLARE_DELEGATE_TwoParams(FOnArchive, TArray<uint8>, int32 );
DECLARE_LOG_CATEGORY_EXTERN(RustyNet, Log, All);

struct FCallbackValue {
public: 
	FCallbackValue(uint8 _classid, uint8 _objectid, uint16 _functionid): classid(_classid),objectid(_objectid),functionid(_functionid){}
	uint8 classid;
	uint8 objectid;
	uint16 functionid;
};

struct FCallbackMap {
public:

	FCallbackValue RegisterCallback(const FDeserializeArchiveDelegate del) {
		FDeserializeArchiveDelegate cb = FDeserializeArchiveDelegate(del);
		FName func = cb.GetFunctionName();
		UObject* obj = cb.GetUObject();
		FString oN = UKismetSystemLibrary::GetObjectName(obj);
		FString oDn = UKismetSystemLibrary::GetDisplayName(obj);
		oN.RemoveFromStart(oDn, ESearchCase::CaseSensitive);
		oN.RemoveFromStart("_", ESearchCase::CaseSensitive);
		uint8 objid  = FCString::Atoi(*oN);
		
		FName objclass = obj->GetClass()->GetFName();
		uint8* classidptr =(uint8*) ClassIdList.FindKey(objclass);
		uint8 classid = *classidptr;
		if (classidptr == nullptr) {
			classid =(uint8) GetNextClassId();
			ClassIdList.Add(classid, objclass);
		}
		UFunction* f=obj->FindFunctionChecked(func);
		uint16 funcId=f->RPCId;
		TArray<uint8> _funcId(Split16(funcId));
		TArray<uint8> uuidbytes = TArray<uint8>();
		uuidbytes={classid, objid};
		uuidbytes[2] = _funcId[0];
		uuidbytes[3] = _funcId[1];
		return FCallbackValue(0, 0, 0);
	}
	TArray<uint8> Split16(uint16 In) {

	}
	uint16 ObjClassTo16(uint8 objid,uint8 classid,uint16& Out) {
		Out = 0;
		Out |= (classid << 8);
		Out |= (objid << 0);
	}
	uint8 GetNextClassId() {
		TArray<uint8> keys;
		uint8 highest=0;
		ClassIdList.GetKeys(keys);
		Max(keys, &highest);
		return keys[highest] + 1;
	}
	void int16ToObjClass(uint8& objid, uint8& classid, const uint16 In) {

		uint16 evenIdBits = 0xFF00;
		uint16 evenValueBits = 0x00FF;
		unsigned char id1 = ((In & evenIdBits) >> 8);
		unsigned char intValue1 = In & evenValueBits;
		classid=id1;
		objid = intValue1;
	}
	TMap<uint8, FName> ClassIdList;
	TMap<uint16, uint8> ObjectIdList;
	TMap<uint16, FName> FunctionIdList;
	
	//internal;
	TMap<uint32, UObject*> ObjectList;
	TMap<uint32, FDeserializeArchiveDelegate> CallbackList;

	static FORCEINLINE uint8 Max(const TArray<uint8>& Values, uint8* MaxIndex = NULL)
	{
		if (Values.Num() == 0)
		{
			if (MaxIndex)
			{
				*MaxIndex = INDEX_NONE;
			}
			return uint8();
		}

		uint8 CurMax = Values[0];
		int32 CurMaxIndex = 0;
		for (int32 v = 1; v < Values.Num(); ++v)
		{
			const uint8 Value = Values[v];
			if (CurMax < Value)
			{
				CurMax = Value;
				CurMaxIndex = v;
			}
		}

		if (MaxIndex)
		{
			*MaxIndex = CurMaxIndex;
		}
		return CurMax;
	}
};

typedef TMap<uint32, FDeserializeArchiveDelegate> RouteMap;


UENUM(BlueprintType)
enum class ENetAddress : uint8 {
	Ping = 0,
	ServerReq = 1,
	Avatar = 2,  //Hardcoded
	RigidBody = 3, //Hardcoded
	ByteArray = 4, //Hardcoded
	NetGuid = 5, //Hardcoded
	Float = 10,
	Int = 11,
	String = 12,
};
UENUM(BlueprintType)
enum class ENetServerRequest : uint8 {
	Time = 0,
	Register = 1,
	Ping = 2,
	CallbackUpdate=3,
	RPC=4,
	PropertyRep=5,
	FunctionRep=6,
	RepAuthority =7,
	RepState=8,
	NewClientConnected=9,
	Closing=10
};
UCLASS( ClassGroup=(DynamicalSystems), meta=(BlueprintSpawnableComponent) )
class DYNAMICALSYSTEMS_API ANetClient : public AActor
{
	GENERATED_BODY()
    
    void* Client = NULL;
    
    float LastPingTime;

	float LastAvatarTime;
    float LastRigidbodyTime;
    
    void RebuildConsensus();

public:

	ANetClient();

protected:
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason)  override;
    virtual void BeginDestroy() override;

public:
    virtual void Tick(float DeltaTime) override;
    
    void RegisterRigidBody(UNetRigidBody* RigidBody);
	void RegisterAvatar(UNetAvatar* Avatar);
    void RegisterVoice(UNetVoice* Voice);
    void Say(uint8* Bytes, uint32 Count);

#pragma region Singleton
	static ANetClient* Instance;
	UFUNCTION(BlueprintCallable,Category = "OSS|Network")
	static bool GetNetClient(ANetClient*& Client) {
		Client = Instance;
		return Instance != nullptr;
	}
#pragma endregion Singleton
#pragma region Send_Rec
	///Im Nanoseconds
	uint64 RequestTime;
	///Im Nanoseconds
	uint64 ServerTime;
	///Im Nanoseconds
	uint64 SystemTime;
	///Im Nanoseconds
	uint64 PingTime;
	float ServerTimeFrequency =2.0f;
	void RequestServerTime();
	
	FTimerHandle ServerTimeHandle;
	void OnServerTime(uint64 serverTime);


	bool Registered;
	FTimerHandle RegistrationHandle;
	void RegisterPlayer();

	void AR_CallbackMapUpdate(TMap<int32,TCHAR> classMap);
	FDeserializeArchiveDelegate OnCallbackMapUpdate;

	//static TMap<int32, FDeserializeArchiveDelegate>
	FTimerHandle PingHandle;
	
	
	void SendPing();

	UFUNCTION(BlueprintCallable, Category="NetClient")
	void SendSystemFloat(int32 System, int32 Id, float Value);

	UFUNCTION(BlueprintCallable, Category="NetClient")
	void SendSystemInt(int32 System, int32 Id, int32 Value);

	UFUNCTION(BlueprintCallable, Category="NetClient")
	void SendSystemString(int32 System, int32 Id, FString Value);

	UFUNCTION(BlueprintCallable, Category = "NetClient")
		bool SendByteArray(TArray<uint8> In,int32 guid);
	static FOnArchive OnByteArray;
	UFUNCTION(BlueprintCallable, Category = "OSS|Send")
		bool AR_SendSystemFloatGlobal(int32 System, int32 Id, float Value);
	static bool AppendSingleFloat(float Value, TArray<uint8>& AppendTo);
	UFUNCTION(BlueprintCallable, Category = "OSS|Send")
		bool AR_SendSystemIntGlobal(int32 System, int32 Id, int32 Value);
	static bool AppendSingleInt(uint8 Value, TArray<uint8>& AppendTo);
	UFUNCTION(BlueprintCallable, Category = "OSS|Send")
		bool AR_SendSystemStringGlobal(int32 System, int32 Id, FString Value);
	static bool AppendSingleString(FString Value, TArray<uint8>& AppendTo);
		
	static RouteMap delegateMap;
	static bool SendCurrentRouteMap() {
		return false;
	};


	UPROPERTY(BlueprintAssignable)
		FSystemFloatMsgDecl OnSystemFloatMsg;

	UPROPERTY(BlueprintAssignable)
		FSystemIntMsgDecl OnSystemIntMsg;

	UPROPERTY(BlueprintAssignable)
		FSystemStringMsgDecl OnSystemStringMsg;

#pragma endregion Send_Rec


	UPROPERTY(BlueprintAssignable)
	FVoiceActivityMsgDecl OnVoiceActivityMsg;

	UPROPERTY(BlueprintAssignable)
	FUint8MsgDecl OnUint8Msg;


	void InitializeWithSettings();
	UFUNCTION(BlueprintGetter)
	FString GetServer();
	UFUNCTION(BlueprintGetter)
	FString GetMumbleServer();
	UFUNCTION(BlueprintGetter)
	FString GetAudioDevice();

		FURL LocalUrl;
		FURL ServerUrl;
		FURL MumbleUrl;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NetClient")
    FString Local="0.0.0.0";

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NetClient", BlueprintGetter = GetServer)
	FString Server = "127.0.0.1:8080";

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NetClient", BlueprintGetter = GetMumbleServer)
	FString MumbleServer = "127.0.0.1:8080";

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NetClient", BlueprintGetter = GetAudioDevice)
	FString AudioDevice = "";

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NetClient")
	int32 Uuid;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NetClient")
	int NetIndex = -1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NetClient")
	TArray<UNetRigidBody*> NetRigidBodies;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NetClient")
	TArray<UNetAvatar*> NetAvatars;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NetClient")
    TArray<UNetVoice*> NetVoices;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NetClient")
    TMap<int32, float> NetClients;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NetClient")
    TArray<int32> MappedClients;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NetClient")
	UNetAvatar* Avatar;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NetClient")
    bool ConsensusReached = false;
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NetClient")
    FColor ChosenColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NetClient")
	int MissingAvatar = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NetClient|Debug")
	bool MirrorSyncY = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NetClient|Debug")
	float PingTimeout = 1.0f;

	UFUNCTION(BlueprintCallable, Category = "NetClient|Experimental")
	void Subscribe(UObject* object) {
			Subscribers.Add(object->GetUniqueID(), object);
	}
	int32 bitsToInt(const uint8* bits, bool little_endian=true);
	protected:
		TMap<uint32, UObject*> Subscribers;
};
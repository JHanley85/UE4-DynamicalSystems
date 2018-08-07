#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NetClient.generated.h"

class UNetRigidBody;
class UNetAvatar;
class UNetVoice;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSystemFloatMsgDecl, int32, System, int32, Id, float, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSystemIntMsgDecl, int32, System, int32, Id, int32, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSystemStringMsgDecl, int32, System, int32, Id, FString, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FVoiceActivityMsgDecl, int32, NetId, float, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUint8MsgDecl, uint8, System, uint8, Id);

DECLARE_LOG_CATEGORY_EXTERN(RustyNet, Log, All);

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

	UFUNCTION(BlueprintCallable, Category="NetClient")
	void SendSystemFloat(int32 System, int32 Id, float Value);

	UFUNCTION(BlueprintCallable, Category="NetClient")
	void SendSystemInt(int32 System, int32 Id, int32 Value);

	UFUNCTION(BlueprintCallable, Category="NetClient")
	void SendSystemString(int32 System, int32 Id, FString Value);

	UPROPERTY(BlueprintAssignable)
	FSystemFloatMsgDecl OnSystemFloatMsg;

	UPROPERTY(BlueprintAssignable)
	FSystemIntMsgDecl OnSystemIntMsg;

	UPROPERTY(BlueprintAssignable)
	FSystemStringMsgDecl OnSystemStringMsg;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NetClient")
    FString Local;

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
	protected:
		TMap<uint32, UObject*> Subscribers;
};
///*
USTRUCT(BlueprintType)
struct FNetGuid {
	GENERATED_USTRUCT_BODY()
	/** Default constructor. */
	FNetGuid()
		: A(0)
		, B(0)
		, C(0)
		, D(0)
	{ }

	/**
	* Creates and initializes a new GUID from the specified components.
	*
	* @param InA The first component.
	* @param InB The second component.
	* @param InC The third component.
	* @param InD The fourth component.
	*/
	FNetGuid(uint8 InA, uint8 InB, uint8 InC, uint8 InD)
		: A(InA), B(InB), C(InC), D(InD)
	{ }

public:

	/** Holds the first component. */
	uint8 A; //object

	/** Holds the second component. */
	uint8 B; //level

	/** Holds the third component. */
	uint8 C; //instance

	/** Holds the fourth component. */
	uint8 D; //system


			 /**
			 * Invalidates the GUID.
			 *
			 * @see IsValid
			 */
	void Invalidate()
	{
		A = B = C = D = 0;
	}

	/**
	* Checks whether this GUID is valid or not.
	*
	* A GUID that has all its components set to zero is considered invalid.
	*
	* @return true if valid, false otherwise.
	* @see Invalidate
	*/
	bool IsValid() const
	{
		return ((A | B | C | D) != 0);
	}

};
//*/
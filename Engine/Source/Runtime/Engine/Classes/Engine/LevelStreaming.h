// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "UObject/ScriptMacros.h"
#include "Engine/LatentActionManager.h"
#include "LatentActions.h"

class ALevelScriptActor;
class ALevelStreamingVolume;
class ULevel;
class ULevelStreaming;

// Stream Level Action
class FStreamLevelAction : public FPendingLatentAction
{
public:
	bool			bLoading;
	bool			bMakeVisibleAfterLoad;
	bool			bShouldBlock;
	ULevelStreaming* Level;
	FName			LevelName;

	FLatentActionInfo LatentInfo;

	FStreamLevelAction(bool bIsLoading, const FName& InLevelName, bool bIsMakeVisibleAfterLoad, bool bShouldBlock, const FLatentActionInfo& InLatentInfo, UWorld* World);

	/**
	 * Given a level name, returns level name that will work with Play on Editor or Play on Console
	 *
	 * @param	InLevelName		Raw level name (no UEDPIE or UED<console> prefix)
	 */
	static FString MakeSafeLevelName( const FName& InLevelName, UWorld* InWorld );

	/**
	 * Helper function to potentially find a level streaming object by name and cache the result
	 *
	 * @param	LevelName							Name of level to search streaming object for in case Level is NULL
	 * @return	level streaming object or NULL if none was found
	 */
	static ULevelStreaming* FindAndCacheLevelStreamingObject( const FName LevelName, UWorld* InWorld );

	/**
	 * Handles "Activated" for single ULevelStreaming object.
	 *
	 * @param	LevelStreamingObject	LevelStreaming object to handle "Activated" for.
	 */
	void ActivateLevel( ULevelStreaming* LevelStreamingObject );
	/**
	 * Handles "UpdateOp" for single ULevelStreaming object.
	 *
	 * @param	LevelStreamingObject	LevelStreaming object to handle "UpdateOp" for.
	 *
	 * @return true if operation has completed, false if still in progress
	 */
	bool UpdateLevel( ULevelStreaming* LevelStreamingObject );

	virtual void UpdateOperation(FLatentResponse& Response) override;

#if WITH_EDITOR
	// Returns a human readable description of the latent operation's current state
	virtual FString GetDescription() const override;
#endif
};

#include "LevelStreaming.generated.h"

// Delegate signatures
DECLARE_DYNAMIC_MULTICAST_DELEGATE( FLevelStreamingLoadedStatus );
DECLARE_DYNAMIC_MULTICAST_DELEGATE( FLevelStreamingVisibilityStatus );

/**
 * Abstract base class of container object encapsulating data required for streaming and providing 
 * interface for when a level should be streamed in and out of memory.
 *
 */
UCLASS(abstract, editinlinenew, BlueprintType, Within=World)
class ENGINE_API ULevelStreaming : public UObject
{
	GENERATED_UCLASS_BODY()

	enum class ECurrentState : uint8
	{
		Removed,
		Unloaded,
		FailedToLoad,
		Loading,
		LoadedNotVisible,
		MakingVisible,
		LoadedVisible,
		MakingInvisible
	};

private:
	enum class ETargetState : uint8
	{
		Unloaded,
		UnloadedAndRemoved,
		LoadedNotVisible,
		LoadedVisible,
	};

public:

#if WITH_EDITORONLY_DATA
	/** Deprecated name of the package containing the level to load. Use GetWorldAsset() or GetWorldAssetPackageFName() instead.		*/
	UPROPERTY()
	FName PackageName_DEPRECATED;
#endif

private:
	/** The reference to the world containing the level to load																	*/
	UPROPERTY(Category=LevelStreaming, VisibleAnywhere, BlueprintReadOnly, meta=(DisplayName = "Level", AllowPrivateAccess="true"))
	TSoftObjectPtr<UWorld> WorldAsset;

public:

	/** If this isn't Name_None, then we load from this package on disk to the new package named PackageName					*/
	UPROPERTY()
	FName PackageNameToLoad;

	/** LOD versions of this level																								*/
	UPROPERTY()
	TArray<FName> LODPackageNames;

	/** LOD package names on disk																								*/
	TArray<FName> LODPackageNamesToLoad;

	/** Transform applied to actors after loading.                                                                              */
	UPROPERTY(EditAnywhere, Category=LevelStreaming, BlueprintReadWrite)
	FTransform LevelTransform;

private:

	/** Requested LOD. Non LOD sub-levels have Index = -1  */
	UPROPERTY(transient, Category = LevelStreaming, BlueprintSetter = SetLevelLODIndex)
	int32 LevelLODIndex;

	/** The relative priority of considering the streaming level. Changing the priority will not interrupt the currently considered level, but will affect the next time a level is being selected for evaluation. */
	UPROPERTY(EditAnywhere, Category=LevelStreaming, BlueprintSetter = SetPriority)
	int32 StreamingPriority;

	/** What the current streamed state of the streaming level is */
	ECurrentState CurrentState;

	/** What streamed state the streaming level is transitioning towards */
	ETargetState TargetState;

	/** Whether this level streaming object's level should be unloaded and the object be removed from the level list.			*/
	uint8 bIsRequestingUnloadAndRemoval : 1;

	/* Whether CachedWorldAssetPackageFName is valid */
	mutable uint8 bHasCachedWorldAssetPackageFName:1;

#if WITH_EDITORONLY_DATA
	/** Whether this level should be visible in the Editor																		*/
	UPROPERTY()
	uint8 bShouldBeVisibleInEditor:1;
#endif

	/** Whether the level should be visible if it is loaded																		*/
	UPROPERTY(Category=LevelStreaming, BlueprintSetter=SetShouldBeVisible)
	uint8 bShouldBeVisible:1;

protected:
	/** Whether the level should be loaded																						*/
	UPROPERTY(Category=LevelStreaming, BlueprintSetter=SetShouldBeLoaded, BlueprintGetter=ShouldBeLoaded)
	uint8 bShouldBeLoaded:1;

public:

	/** Whether this level is locked; that is, its actors are read-only. */
	UPROPERTY()
	uint8 bLocked:1;

	/**
	 * Whether this level only contains static actors that aren't affected by gameplay or replication.
	 * If true, the engine can make certain optimizations and will add this level to the StaticLevels collection.
	 */
	UPROPERTY(EditDefaultsOnly, Category=LevelStreaming)
	uint8 bIsStatic:1;

	/** Whether we want to force a blocking load																				*/
	UPROPERTY(Category=LevelStreaming, BlueprintReadWrite)
	uint8 bShouldBlockOnLoad:1;

	/** Whether we want to force a blocking unload																				*/
	UPROPERTY(Category=LevelStreaming, BlueprintReadWrite)
	uint8 bShouldBlockOnUnload:1;

	/** 
	 *  Whether this level streaming object should be ignored by world composition distance streaming, 
	 *  so streaming state can be controlled by other systems (ex: in blueprints)
	 */
	UPROPERTY(transient, Category=LevelStreaming, BlueprintReadWrite)
	uint8 bDisableDistanceStreaming:1;

	/** If true, will be drawn on the 'level streaming status' map (STAT LEVELMAP console command) */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = LevelStreaming)
	uint8 bDrawOnLevelStatusMap : 1;

#if WITH_EDITORONLY_DATA
	/** Deprecated level color used for visualization. */
	UPROPERTY()
	FColor DrawColor_DEPRECATED;
#endif

	/** The level color used for visualization. (Show -> Advanced -> Level Coloration) */
	UPROPERTY(EditAnywhere, Category = LevelStreaming)
	FLinearColor LevelColor;

	/** The level streaming volumes bound to this level. */
	UPROPERTY(EditAnywhere, Category=LevelStreaming, meta=(DisplayName = "Streaming Volumes", NoElementDuplicate))
	TArray<ALevelStreamingVolume*> EditorStreamingVolumes;

	/** Cooldown time in seconds between volume-based unload requests.  Used in preventing spurious unload requests. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category=LevelStreaming, meta=(ClampMin = "0", UIMin = "0", UIMax = "10"))
	float MinTimeBetweenVolumeUnloadRequests;

	/** Time of last volume unload request.  Used in preventing spurious unload requests. */
	float LastVolumeUnloadRequestTime;

#if WITH_EDITORONLY_DATA
	/** List of keywords to filter on in the level browser */
	UPROPERTY()
	TArray<FString> Keywords;
#endif

	//~ Begin UObject Interface
	virtual void PostLoad() override;
	virtual void Serialize( FArchive& Ar ) override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	
	/** Remove duplicates in EditorStreamingVolumes list*/
	void RemoveStreamingVolumeDuplicates();
#endif
	//~ End UObject Interface

	/** Returns the current loaded/visible state of the streaming level. */
	ECurrentState GetCurrentState() const { return CurrentState; }

private:

	/** Determine what the streaming levels target state should be. Returns whether the streaming level should be in the consider list. */
	bool DetermineTargetState();

	/** Update the load process of the streaming level. Out parameters instruct calling code how to proceed. */
	void UpdateStreamingState(bool& bOutUpdateAgain, bool& bOutRedetermineTarget);

	/** Update internal variables when the level is added from the streaming levels array */
	void OnLevelAdded();

	/** Update internal variables when the level is removed from the streaming levels array */
	void OnLevelRemoved();

	/** Internal function for checking if the desired level is the currently loaded level */
	bool IsDesiredLevelLoaded() const;

public:

	/** Returns the value of bShouldBeVisible. Use ShouldBeVisible to query whether a streaming level should be visible based on its own criteria. */
	bool GetShouldBeVisibleFlag() const { return bShouldBeVisible; }

	/** Sets the should be visible flag and marks the streaming level as requiring consideration. */
	UFUNCTION(BlueprintSetter)
	void SetShouldBeVisible(bool bInShouldBeVisible);

	/** 
	 * Virtual that can be overriden to change whether a streaming level should be loaded.
	 * Doesn't do anything at the base level as should be loaded defaults to true 
	 */
	UFUNCTION(BlueprintSetter)
	virtual void SetShouldBeLoaded(bool bInShouldBeLoaded);

	/** Returns the world composition level LOD index. */
	int32 GetLevelLODIndex() const { return LevelLODIndex; }

	/** Sets the world composition level LOD index and marks the streaming level as requiring consideration. */
	UFUNCTION(BlueprintSetter)
	void SetLevelLODIndex(int32 LODIndex);

	/** Sets the relative priority of considering the streaming level. Changing the priority will not interrupt the currently considered level, but will affect the next time a level is being selected for evaluation. */
 	int32 GetPriority() const { return StreamingPriority; }

	/** Sets the relative priority of considering the streaming level. Changing the priority will not interrupt the currently considered level, but will affect the next time a level is being selected for evaluation. */
	UFUNCTION(BlueprintSetter)
	void SetPriority(int32 NewPriority);

	/** Returns whether the streaming level is in the loading state. */
	bool HasLoadRequestPending() const { return GetCurrentState() == ECurrentState::Loading; }

	/** Returns whether the streaming level has loaded a level. */
	bool HasLoadedLevel() const
	{
		return (LoadedLevel || PendingUnloadLevel);
	}

	/** Returns if the streaming level has requested to be unloaded and removed. */
	bool GetIsRequestingUnloadAndRemoval() const { return bIsRequestingUnloadAndRemoval; }

	/** Sets if the streaming level should be unloaded and removed. */
	void SetIsRequestingUnloadAndRemoval(bool bInIsRequestingUnloadAndRemoval);

#if WITH_EDITORONLY_DATA
	/** Returns if the streaming level should be visible in the editor. */
	bool GetShouldBeVisibleInEditor() const { return bShouldBeVisibleInEditor; }
#endif
#if WITH_EDITOR
	/** Sets if the streaming level should be visible in the editor. */
	void SetShouldBeVisibleInEditor(bool bInShouldBeVisibleInEditor);
#endif

	/** Returns a constant reference to the world asset this streaming level object references  */
	const TSoftObjectPtr<UWorld>& GetWorldAsset() const { return WorldAsset; }

	/** Setter for WorldAsset. Use this instead of setting WorldAsset directly to update the cached package name. */
	void SetWorldAsset(const TSoftObjectPtr<UWorld>& NewWorldAsset);

	/** Gets the package name for the world asset referred to by this level streaming */
	FString GetWorldAssetPackageName() const;

	/** Gets the package name for the world asset referred to by this level streaming as an FName */
	UFUNCTION(BlueprintCallable, Category = "Game")
	FName GetWorldAssetPackageFName() const;

	/** Sets the world asset based on the package name assuming it contains a world of the same name. */
	void SetWorldAssetByPackageName(FName InPackageName);

	/** Rename package name to PIE appropriate name */
	void RenameForPIE(int PIEInstanceID);

	/**
	 * Return whether this level should be present in memory which in turn tells the 
	 * streaming code to stream it in. Please note that a change in value from false 
	 * to true only tells the streaming code that it needs to START streaming it in 
	 * so the code needs to return true an appropriate amount of time before it is 
	 * needed.
	 *
	 * @return true if level should be loaded/ streamed in, false otherwise
	 */
	UFUNCTION(BlueprintGetter)
	virtual bool ShouldBeLoaded() const { return true; }

	/**
	 * Return whether this level should be visible/ associated with the world if it is
	 * loaded.
	 * 
	 * @return true if the level should be visible, false otherwise
	 */
	virtual bool ShouldBeVisible() const;

	virtual bool ShouldBeAlwaysLoaded() const { return false; }

	/** Get a bounding box around the streaming volumes associated with this LevelStreaming object */
	FBox GetStreamingVolumeBounds();

	/** Gets a pointer to the LoadedLevel value */
	UFUNCTION(BlueprintCallable, Category="Game")
	ULevel* GetLoadedLevel() const { return LoadedLevel; }
	
	/** Sets the LoadedLevel value to NULL */
	void ClearLoadedLevel() { SetLoadedLevel(nullptr); }
	
#if WITH_EDITOR
	/** Override Pre/PostEditUndo functions to handle editor transform */
	virtual void PostEditUndo() override;
#endif
	
	/** Matcher for searching streaming levels by PackageName */
	struct FPackageNameMatcher
	{
		FPackageNameMatcher( const FName& InPackageName )
			: PackageName( InPackageName )
		{
		}

		bool operator()(const ULevelStreaming* Candidate) const
		{
			return Candidate->GetWorldAssetPackageFName() == PackageName;
		}

		FName PackageName;
	};

	virtual UWorld* GetWorld() const override final;

	/** Returns whether streaming level is visible */
	UFUNCTION(BlueprintCallable, Category="Game")
	bool IsLevelVisible() const;

	/** Returns whether streaming level is loaded */
	UFUNCTION(BlueprintCallable, Category="Game")
	bool IsLevelLoaded() const { return (LoadedLevel != nullptr); }

	/** Returns whether level has streaming state change pending */
	UFUNCTION(BlueprintCallable, Category="Game")
	bool IsStreamingStatePending() const;

	/** Creates a new instance of this streaming level with a provided unique instance name */
	UFUNCTION(BlueprintCallable, Category="Game")
	ULevelStreaming* CreateInstance(const FString& UniqueInstanceName);

	/** Returns the Level Script Actor of the level if the level is loaded and valid */
	UFUNCTION(BlueprintPure, meta = (BlueprintInternalUseOnly = "true"))
	ALevelScriptActor* GetLevelScriptActor();

#if WITH_EDITOR
	/** Get the folder path for this level for use in the world browser. Only available in editor builds */
	const FName& GetFolderPath() const;

	/** Sets the folder path for this level in the world browser. Only available in editor builds */
	void SetFolderPath(const FName& InFolderPath);
#endif	// WITH_EDITOR

	//~==============================================================================================
	// Delegates
	
	/** Called when level is streamed in  */
	UPROPERTY(BlueprintAssignable)
	FLevelStreamingLoadedStatus			OnLevelLoaded;
	
	/** Called when level is streamed out  */
	UPROPERTY(BlueprintAssignable)
	FLevelStreamingLoadedStatus			OnLevelUnloaded;
	
	/** Called when level is added to the world  */
	UPROPERTY(BlueprintAssignable)
	FLevelStreamingVisibilityStatus		OnLevelShown;
	
	/** Called when level is removed from the world  */
	UPROPERTY(BlueprintAssignable)
	FLevelStreamingVisibilityStatus		OnLevelHidden;

	/** 
	 * Traverses all streaming level objects in the persistent world and in all inner worlds and calls appropriate delegate for streaming objects that refer specified level 
	 *
	 * @param PersistentWorld	World to traverse
	 * @param LevelPackageName	Level which loaded status was changed
	 * @param bLoaded			Whether level was loaded or unloaded
	 */
	static void BroadcastLevelLoadedStatus(UWorld* PersistentWorld, FName LevelPackageName, bool bLoaded);
	
	/** 
	 * Traverses all streaming level objects in the persistent world and in all inner worlds and calls appropriate delegate for streaming objects that refer specified level 
	 *
	 * @param PersistentWorld	World to traverse
	 * @param LevelPackageName	Level which visibility status was changed
	 * @param bVisible			Whether level become visible or not
	 */
	static void BroadcastLevelVisibleStatus(UWorld* PersistentWorld, FName LevelPackageName, bool bVisible);

	enum EReqLevelBlock
	{
		/** Block load AlwaysLoaded levels. Otherwise Async load. */
		BlockAlwaysLoadedLevelsOnly,
		/** Block all loads */
		AlwaysBlock,
		/** Never block loads */
		NeverBlock,
	};

#if WITH_EDITOR
	// After a sub level is reloaded in the editor the cache state needs to be refreshed
	void RemoveLevelFromCollectionForReload();
	void AddLevelToCollectionAfterReload();
#endif

private:
	/** @return Name of the LOD level package used for loading.																		*/
	FName GetLODPackageName() const;

	/** @return Name of the LOD package on disk to load to the new package named PackageName, Name_None otherwise					*/
	FName GetLODPackageNameToLoad() const;

	/** 
	 * Try to find loaded level in memory, issue a loading request otherwise
	 *
	 * @param	PersistentWorld			Persistent world
	 * @param	bAllowLevelLoadRequests	Whether to allow level load requests
	 * @param	BlockPolicy				Whether loading operation should block
	 * @return							true if the load request was issued or a package was already loaded
	 */
	bool RequestLevel(UWorld* PersistentWorld, bool bAllowLevelLoadRequests, EReqLevelBlock BlockPolicy);
	
	/** Sets the value of LoadedLevel */
	void SetLoadedLevel(ULevel* Level);
	
	/** Hide and queue for unloading previously used level */
	void DiscardPendingUnloadLevel(UWorld* PersistentWorld);

	/** 
	 * Handler for level async loading completion 
	 *
	 * @param LevelPackage	Loaded level package
	 */
	void AsyncLevelLoadComplete(const FName& PackageName, UPackage* LevelPackage, EAsyncLoadingResult::Type Result);

	/** Pointer to Level object if currently loaded/ streamed in.																*/
	UPROPERTY(transient)
	class ULevel* LoadedLevel;

	/** Pointer to a Level object that was previously active and was replaced with a new LoadedLevel (for LOD switching) */
	UPROPERTY(transient)
	class ULevel* PendingUnloadLevel;

#if WITH_EDITORONLY_DATA
	/** The folder path for this level within the world browser. This is only available in editor builds. 
		A NONE path indicates that it exists at the root. It is '/' separated. */
	UPROPERTY()
	FName FolderPath;
#endif	// WITH_EDITORONLY_DATA

	/** The cached package name of the world asset that is loaded by the levelstreaming */
	mutable FName CachedWorldAssetPackageFName;

	FName CachedLoadedLevelPackageName;

	friend struct FStreamingLevelPrivateAccessor;
};

struct FStreamingLevelPrivateAccessor
{
private:

	/** Specifies which level should be the loaded level for the streaming level. */
	static void SetLoadedLevel(ULevelStreaming* StreamingLevel, ULevel* Level) { StreamingLevel->SetLoadedLevel(Level); }
	/** Issue a loading request for the streaming level. */
	static bool RequestLevel(ULevelStreaming* StreamingLevel, UWorld* PersistentWorld, bool bAllowLevelLoadRequests, ULevelStreaming::EReqLevelBlock BlockPolicy) { return StreamingLevel->RequestLevel(PersistentWorld, bAllowLevelLoadRequests, BlockPolicy); }
	/** Update internal variables when the level is added from the streaming levels array */
	static void OnLevelAdded(ULevelStreaming* StreamingLevel) { StreamingLevel->OnLevelAdded(); }
	/** Update internal variables when the level is removed from the streaming levels array */
	static void OnLevelRemoved(ULevelStreaming* StreamingLevel) { StreamingLevel->OnLevelRemoved(); }
	/** Determine what the streaming levels target state should be. Returns whether the streaming level should be in the consider list. */
	static bool DetermineTargetState(ULevelStreaming* StreamingLevel) { return StreamingLevel->DetermineTargetState(); }
	/** Update the load process of the streaming level. Out parameters instruct calling code how to proceed. */
	static void UpdateStreamingState(ULevelStreaming* StreamingLevel, bool& bOutUpdateAgain, bool& bOutRedetermineTarget) { StreamingLevel->UpdateStreamingState(bOutUpdateAgain, bOutRedetermineTarget); }

	/** Friend classes to manipulate the streaming level more extensively */
	friend class UEngine;
	friend class UWorld;
};
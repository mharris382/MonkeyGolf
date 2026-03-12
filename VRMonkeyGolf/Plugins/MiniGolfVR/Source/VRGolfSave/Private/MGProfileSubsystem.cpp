#include "MGProfileSubsystem.h"
#include "Kismet/GameplayStatics.h"

void UMGProfileSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    // Nothing to auto-load — UI drives the load explicitly
}

// ---------------------------------------------------------------------------
// Profile
// ---------------------------------------------------------------------------

void UMGProfileSubsystem::LoadOrCreateProfile(const FString& SlotName, const FString& FallbackName)
{
    if (UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
    {
        USaveGame* Raw = UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex);
        UMGProfileSaveGame* Loaded = Cast<UMGProfileSaveGame>(Raw);
        if (Loaded)
        {
            Loaded->SlotName = SlotName;
            ActiveProfile = Loaded;
            OnProfileLoaded.Broadcast(ActiveProfile);
            return;
        }
    }

    // Slot empty or cast failed — create fresh
    CreateNewProfile(FallbackName);
}

void UMGProfileSubsystem::CreateNewProfile(const FString& ProfileName)
{
    UMGProfileSaveGame* NewProfile = Cast<UMGProfileSaveGame>(
        UGameplayStatics::CreateSaveGameObject(UMGProfileSaveGame::StaticClass()));

    NewProfile->ProfileID   = FGuid::NewGuid();
    NewProfile->ProfileName = ProfileName.IsEmpty()
        ? FString::Printf(TEXT("Player_%s"), *NewProfile->ProfileID.ToString().Left(4))
        : ProfileName;
    NewProfile->SlotName    = MakeProfileSlotName(NewProfile->ProfileID);

    ActiveProfile = NewProfile;
    SaveActiveProfile();
}

void UMGProfileSubsystem::SaveActiveProfile()
{
    if (!ActiveProfile) return;

    ActiveProfile->bIsPersisted = true;
    UGameplayStatics::SaveGameToSlot(ActiveProfile, ActiveProfile->SlotName, UserIndex);
    OnProfileSaved.Broadcast(ActiveProfile);
}

TArray<FString> UMGProfileSubsystem::GetAllProfileSlotNames() const
{
    // TODO: Replace with a slot index save (UMGProfileIndexSave) once multi-profile
    // UI is needed. For now caller passes known slot names.
    return TArray<FString>();
}

FString UMGProfileSubsystem::MakeProfileSlotName(const FGuid& ID)
{
    return FString::Printf(TEXT("Profile_%s"), *ID.ToString());
}

// ---------------------------------------------------------------------------
// Room Save
// ---------------------------------------------------------------------------

UMGRoomSaveGame* UMGProfileSubsystem::LoadRoomSave(const FString& RoomName, const FString& CourseName)
{
    const FString SlotName = UMGRoomSaveGame::MakeSlotName(RoomName, CourseName);
    if (!UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
    {
        return nullptr;
    }

    USaveGame* Raw = UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex);
    return Cast<UMGRoomSaveGame>(Raw);
}

UMGRoomSaveGame* UMGProfileSubsystem::CreateRoomSave(const FString& RoomName,
                                                      const FString& CourseName,
                                                      int32 TotalHoles)
{
    UMGRoomSaveGame* NewSave = Cast<UMGRoomSaveGame>(
        UGameplayStatics::CreateSaveGameObject(UMGRoomSaveGame::StaticClass()));

    NewSave->RoomName   = RoomName;
    NewSave->CourseName = CourseName;
    NewSave->TotalHoles = TotalHoles;
    NewSave->CurrentHole = 0;
    NewSave->bIsComplete = false;
    NewSave->LastSaved   = FDateTime::UtcNow();

    return NewSave;
}

void UMGProfileSubsystem::SaveRoomProgress(UMGRoomSaveGame* RoomSave)
{
    if (!RoomSave) return;

    RoomSave->LastSaved = FDateTime::UtcNow();
    const FString SlotName = UMGRoomSaveGame::MakeSlotName(RoomSave->RoomName, RoomSave->CourseName);
    UGameplayStatics::SaveGameToSlot(RoomSave, SlotName, UserIndex);
}

void UMGProfileSubsystem::CompleteRoomSave(UMGRoomSaveGame* RoomSave)
{
    if (!RoomSave) return;

    RoomSave->bIsComplete = true;
    SaveRoomProgress(RoomSave);
}

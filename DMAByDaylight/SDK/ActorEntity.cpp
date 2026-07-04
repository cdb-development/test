#include "Pch.h"
#include "ActorEntity.h"
#include "Camera.h"
#include "Globals.h"

ActorEntity::ActorEntity(uint64_t address, VMMDLL_SCATTER_HANDLE handle)
{
    Class = address;
    if (!address) return;

    TargetProcess.AddScatterReadRequest(handle, Class + PlayerState, reinterpret_cast<void*>(&PlayerState), sizeof(uint64_t));
    TargetProcess.AddScatterReadRequest(handle, Class + RootComponent, reinterpret_cast<void*>(&RootComponent), sizeof(uint64_t));
}

void ActorEntity::SetUp1(VMMDLL_SCATTER_HANDLE handle)
{
    if (!Class || !RootComponent) return;

    // Fetch the player role if this is a player
    if (PlayerState)
    {
        TargetProcess.AddScatterReadRequest(handle, PlayerState + GameRole, reinterpret_cast<void*>(&PlayerRole), sizeof(EPlayerRole));
    }
}

void ActorEntity::SetUp2()
{
    if (!Class || !RootComponent) return;

    if (PlayerState)
    {
        if (PlayerRole != EPlayerRole::EPlayerRole__VE_Camper && PlayerRole != EPlayerRole::EPlayerRole__VE_Slasher)
            return;

        Name = (PlayerRole == EPlayerRole::EPlayerRole__VE_Camper) ? LIT(L"Survivor") : LIT(L"Killer");

        // Now that RootComponent is validated, read the position
        UEPosition = TargetProcess.Read<UEVector>(RootComponent + RelativeLocation);
        Position = Vector3((float)UEPosition.X, (float)UEPosition.Y, (float)UEPosition.Z);
    }
}

EPlayerRole ActorEntity::GetPlayerRole() { return PlayerRole; }
uint64_t ActorEntity::GetClass() { return Class; }
std::wstring ActorEntity::GetName() { return Name; }

Vector3 ActorEntity::GetPosition()
{
    return Position;
}

UEVector ActorEntity::GetUEPosition()
{
    return UEPosition;
}

void ActorEntity::SetPosition(Vector3 pos)
{
    Position = pos;
}

void ActorEntity::UpdatePosition(VMMDLL_SCATTER_HANDLE handle)
{
    if (!Class || !RootComponent || !PlayerState) return;

    // Only update if it's a valid player
    if (PlayerRole == EPlayerRole::EPlayerRole__VE_Camper || PlayerRole == EPlayerRole::EPlayerRole__VE_Slasher)
    {
        TargetProcess.AddScatterReadRequest(handle, RootComponent + RelativeLocation, reinterpret_cast<void*>(&UEPosition), sizeof(UEVector));
    }
}
#include "Pch.h"
#include "ActorEntity.h"
#include "Camera.h"
#include "Globals.h"
#include "Drawing.h"

ActorEntity::ActorEntity(uint64_t address, VMMDLL_SCATTER_HANDLE handle)
{
    Class = address;
    if (!address) return;

    // Core components
    TargetProcess.AddScatterReadRequest(handle, Class + PlayerState, reinterpret_cast<void*>(&PlayerState), sizeof(uint64_t));
    TargetProcess.AddScatterReadRequest(handle, Class + RootComponent, reinterpret_cast<void*>(&RootComponent), sizeof(uint64_t));

    // Mesh for skeleton (verify this offset with Dumper-7 - common around 0x410-0x430)
    TargetProcess.AddScatterReadRequest(handle, Class + MeshComponent, reinterpret_cast<void*>(&MeshComponent), sizeof(uint64_t));
}

void ActorEntity::SetUp1(VMMDLL_SCATTER_HANDLE handle)
{
    if (!Class || !PlayerState) return;

    TargetProcess.AddScatterReadRequest(handle, PlayerState + GameRole, reinterpret_cast<void*>(&PlayerRole), sizeof(EPlayerRole));
}

void ActorEntity::SetUp2()
{
    if (!Class || !RootComponent || !PlayerState) return;

    if (PlayerRole != EPlayerRole::EPlayerRole__VE_Camper && PlayerRole != EPlayerRole::EPlayerRole__VE_Slasher)
        return;

    Name = (PlayerRole == EPlayerRole::EPlayerRole__VE_Camper) ? LIT(L"Survivor") : LIT(L"Killer");

    UEPosition = TargetProcess.Read<UEVector>(RootComponent + RelativeLocation);
    Position = Vector3((float)UEPosition.X, (float)UEPosition.Y, (float)UEPosition.Z);
}

void ActorEntity::UpdatePosition(VMMDLL_SCATTER_HANDLE handle)
{
    if (!RootComponent || (PlayerRole != EPlayerRole::EPlayerRole__VE_Camper && PlayerRole != EPlayerRole::EPlayerRole__VE_Slasher))
        return;

    TargetProcess.AddScatterReadRequest(handle, RootComponent + RelativeLocation, reinterpret_cast<void*>(&UEPosition), sizeof(UEVector));
}

void ActorEntity::UpdateBones(VMMDLL_SCATTER_HANDLE handle)
{
    if (!MeshComponent) return;

    uint64_t TransformsArray = TargetProcess.Read<uint64_t>(MeshComponent + BoneArray);
    if (!TransformsArray) return;

    // Common survivor bone chain (head -> spine -> pelvis -> legs + arms)
    std::vector<int> commonBones = {
        0,   // Root / Head area
        66, 67, 68,  // Neck / Head
        15, 16, 17,  // Torso
        1,           // Pelvis
        8, 9, 10,    // Left leg
        3, 4, 5,     // Right leg
        19, 20, 21,  // Left arm
        43, 44, 45   // Right arm
    };

    BonePositions.clear();

    for (int idx : commonBones)
    {
        uint64_t boneAddr = TransformsArray + (uint64_t)idx * 0x30;
        FTransform transform = TargetProcess.Read<FTransform>(boneAddr);

        Vector3 rootPos = GetPosition();
        Vector3 bonePos(
            transform.Translation.X + rootPos.x,
            transform.Translation.Y + rootPos.y,
            transform.Translation.Z + rootPos.z
        );

        BonePositions.push_back(bonePos);
    }
}

// Make sure this is AFTER UpdateBones and before the getters
void ActorEntity::DrawSkeleton()
{
    if (BonePositions.size() < 3) return;

    MyColour color = (PlayerRole == EPlayerRole::EPlayerRole__VE_Slasher)
        ? MyColour(255, 0, 0, 255)
        : MyColour(0, 255, 0, 255);

    for (size_t i = 0; i < BonePositions.size(); ++i)
    {
        Vector2 screen = Camera::WorldToScreen(EngineInstance->GetCameraCache().POV, BonePositions[i]);

        if (screen != Vector2::Zero())
        {
            FilledLine(screen.x - 3, screen.y, screen.x + 3, screen.y, 1.0f, color);
            FilledLine(screen.x, screen.y - 3, screen.x, screen.y + 3, 1.0f, color);
        }
    }
}

Vector3 ActorEntity::GetBoneWorldPosition(int boneIndex)
{
    if (boneIndex < 0 || boneIndex >= BonePositions.size()) return Vector3(0, 0, 0);
    return BonePositions[boneIndex];
}

// Getters
EPlayerRole ActorEntity::GetPlayerRole() { return PlayerRole; }
uint64_t ActorEntity::GetClass() { return Class; }
std::wstring ActorEntity::GetName() { return Name; }
Vector3 ActorEntity::GetPosition() { return Position; }
UEVector ActorEntity::GetUEPosition() { return UEPosition; }
void ActorEntity::SetPosition(Vector3 pos) { Position = pos; }
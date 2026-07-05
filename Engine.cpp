#include "Pch.h"
#include "Engine.h"
#include "ActorEntity.h"
#include "Globals.h"
#include "../Graphics/Drawing.h"     // Make sure this path is correct

Engine::Engine()
{
    uint64_t base = TargetProcess.GetBaseAddress(ProcessName);
    if (base == 0) {
        printf("Engine ctor: Base address invalid\n");
        return;
    }

    GWorld = TargetProcess.Read<uint64_t>(base + GWorld);
    if (GWorld == 0) {
        printf("Engine ctor: Failed to read GWorld\n");
        return;
    }

    PersistentLevel = TargetProcess.Read<uint64_t>(GWorld + PersistentLevel);
    OwningGameInstance = TargetProcess.Read<uint64_t>(GWorld + OwningGameInstance);

    // LocalPlayer chain
    uint64_t localPlayersPtr = TargetProcess.Read<uint64_t>(OwningGameInstance + LocalPlayers);
    LocalPlayers = TargetProcess.Read<uint64_t>(localPlayersPtr);

    PlayerController = TargetProcess.Read<uint64_t>(LocalPlayers + PlayerController);
    CameraManager = TargetProcess.Read<uint64_t>(PlayerController + CameraManager);

    // GameState & PlayerArray
    GameState = TargetProcess.Read<uint64_t>(GWorld + GameStateOffset);
    if (GameState != 0) {
        PlayerArray = TargetProcess.Read<uint64_t>(GameState + PlayerArrayOffset);
    }

    printf("Engine initialized - GWorld: 0x%llX | GameState: 0x%llX\n", GWorld, GameState);
}

void Engine::Cache()
{
    if (PersistentLevel == 0) {
        printf("Cache skipped - PersistentLevel is 0\n");
        return;
    }

    struct TArray { uint64_t DataPointer; int Count; int Max; };
    TArray actorsArray = TargetProcess.Read<TArray>(PersistentLevel + 0xC0);

    if (actorsArray.DataPointer == 0 || actorsArray.Count <= 0 || actorsArray.Count > 100000) {
        printf("Invalid actor array (count: %d)\n", actorsArray.Count);
        return;
    }

    std::vector<uint64_t> entityList(actorsArray.Count);
    TargetProcess.Read(actorsArray.DataPointer, entityList.data(), actorsArray.Count * sizeof(uint64_t));

    std::vector<std::shared_ptr<ActorEntity>> newActors;
    auto handle = TargetProcess.CreateScatterHandle();

    for (uint64_t address : entityList)
    {
        if (address) {
            newActors.push_back(std::make_shared<ActorEntity>(address, handle));
        }
    }
    TargetProcess.ExecuteReadScatter(handle);
    TargetProcess.CloseScatterHandle(handle);

    handle = TargetProcess.CreateScatterHandle();
    for (auto& entity : newActors) {
        entity->SetUp1(handle);
    }
    TargetProcess.ExecuteReadScatter(handle);
    TargetProcess.CloseScatterHandle(handle);

    std::vector<std::shared_ptr<ActorEntity>> playerlist;
    for (auto& entity : newActors)
    {
        entity->SetUp2();
        if (entity->GetName() == L"Survivor" || entity->GetName() == L"Killer")
        {
            playerlist.push_back(entity);
        }
    }

    Actors = playerlist;
    printf("Total Players Cached: %zu\n", Actors.size());
}

void Engine::UpdatePlayers()
{
    if (Actors.empty()) return;

    auto handle = TargetProcess.CreateScatterHandle();

    for (auto& entity : Actors)
    {
        entity->UpdatePosition(handle);
        // Bones disabled for now
        // entity->UpdateBones(handle);
    }

    TargetProcess.ExecuteReadScatter(handle);
    TargetProcess.CloseScatterHandle(handle);

    for (auto& entity : Actors)
    {
        UEVector pos = entity->GetUEPosition();
        entity->SetPosition(Vector3((float)pos.X, (float)pos.Y, (float)pos.Z));
    }
}

void Engine::DrawLobbyInfo()
{
    if (GameState == 0) return;

    uint64_t playerArrayAddr = TargetProcess.Read<uint64_t>(GameState + PlayerArrayOffset);
    if (playerArrayAddr == 0) return;

    int playerCount = TargetProcess.Read<int>(playerArrayAddr + 0x8); // TArray count

    if (playerCount > 1) {
        std::string text = "Lobby Players: " + std::to_string(playerCount);
        std::wstring wtext(text.begin(), text.end());

        // Using a safe alignment that should exist
        DrawText(20, 20, wtext, "Verdana", 18, MyColour(255, 255, 0, 255), FontAlignment::Left);
    }
}

void Engine::RefreshViewMatrix(VMMDLL_SCATTER_HANDLE handle)
{
    if (CameraManager == 0) return;
    TargetProcess.AddScatterReadRequest(handle, CameraManager + CameraCachePrivateOffset, reinterpret_cast<void*>(&CameraEntry), sizeof(CameraCacheEntry));
}

CameraCacheEntry Engine::GetCameraCache() { return CameraEntry; }
std::vector<std::shared_ptr<ActorEntity>> Engine::GetActors() { return Actors; }
uint32_t Engine::GetActorSize() { return (uint32_t)Actors.size(); }
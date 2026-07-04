#include "Pch.h"
#include "Engine.h"
#include "ActorEntity.h"
#include "Globals.h"

Engine::Engine()
{
    uint64_t base = TargetProcess.GetBaseAddress(ProcessName);
    GWorld = TargetProcess.Read<uint64_t>(base + GWorld);
    PersistentLevel = TargetProcess.Read<uint64_t>(GWorld + PersistentLevel);
    OwningGameInstance = TargetProcess.Read<uint64_t>(GWorld + OwningGameInstance);

    uint64_t localPlayersPtr = TargetProcess.Read<uint64_t>(OwningGameInstance + LocalPlayers);
    LocalPlayers = TargetProcess.Read<uint64_t>(localPlayersPtr);
    PlayerController = TargetProcess.Read<uint64_t>(LocalPlayers + PlayerController);
    CameraManager = TargetProcess.Read<uint64_t>(PlayerController + CameraManager);
}

void Engine::Cache()
{
    struct TArray { uint64_t DataPointer; int Count; int Max; };
    TArray actorsArray = TargetProcess.Read<TArray>(PersistentLevel + 0xC0);

    if (actorsArray.DataPointer == 0 || actorsArray.Count <= 0 || actorsArray.Count > 100000)
        return;

    std::vector<uint64_t> entityList(actorsArray.Count);
    TargetProcess.Read(actorsArray.DataPointer, entityList.data(), actorsArray.Count * sizeof(uint64_t));

    std::list<std::shared_ptr<ActorEntity>> actors;
    auto handle = TargetProcess.CreateScatterHandle();

    for (uint64_t address : entityList)
    {
        if (address) actors.push_back(std::make_shared<ActorEntity>(address, handle));
    }
    TargetProcess.ExecuteReadScatter(handle);
    TargetProcess.CloseScatterHandle(handle);

    handle = TargetProcess.CreateScatterHandle();
    for (auto& entity : actors) entity->SetUp1(handle);
    TargetProcess.ExecuteReadScatter(handle);
    TargetProcess.CloseScatterHandle(handle);

    std::vector<std::shared_ptr<ActorEntity>> playerlist;
    for (auto& entity : actors)
    {
        entity->SetUp2();
        if (entity->GetName() == L"Survivor" || entity->GetName() == L"Killer")
        {
            playerlist.push_back(entity);
            printf("Found Actor: %ls at X:%.f Y:%.f\n", entity->GetName().c_str(), entity->GetPosition().x, entity->GetPosition().y);
        }
    }
    Actors = playerlist;
    printf("Total Players Cached: %zu\n", Actors.size());
}

void Engine::UpdatePlayers()
{
    auto handle = TargetProcess.CreateScatterHandle();
    for (auto& entity : Actors) entity->UpdatePosition(handle);
    TargetProcess.ExecuteReadScatter(handle);
    TargetProcess.CloseScatterHandle(handle);

    // Update local variables from the scatter results
    for (auto& entity : Actors)
    {
        UEVector pos = entity->GetUEPosition(); // Ensure you have a getter for this
        entity->SetPosition(Vector3((float)pos.X, (float)pos.Y, (float)pos.Z));
    }
}

void Engine::RefreshViewMatrix(VMMDLL_SCATTER_HANDLE handle)
{
    TargetProcess.AddScatterReadRequest(handle, CameraManager + CameraCachePrivateOffset, reinterpret_cast<void*>(&CameraEntry), sizeof(CameraCacheEntry));
}

CameraCacheEntry Engine::GetCameraCache() { return CameraEntry; }
std::vector<std::shared_ptr<ActorEntity>> Engine::GetActors() { return Actors; }
uint32_t Engine::GetActorSize() { return (uint32_t)Actors.size(); }
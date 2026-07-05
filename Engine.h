#pragma once
#include "ActorEntity.h"
#include "EngineStructs.h"

class Engine
{
private:
	uint64_t OwningActor;
	uint64_t MaxPacket;

	uint64_t OwningGameInstance = 0x0250;
	uint64_t PersistentLevel = 0x0050;
	uint64_t GameStateOffset = 0x01D8;
	uint64_t PlayerArrayOffset = 0x0308;

	uint64_t GWorld = 0xd3f7958;
	uint64_t LocalPlayers = 0x0058;
	uint64_t PlayerController = 0x0050;
	uint64_t AcknowledgedPawn = 0x03A0;
	uint64_t CameraManager = 0x03B0;
	uint64_t CameraCachePrivateOffset = 0x15A0;

	CameraCacheEntry CameraEntry;
	MinimalViewInfo CameraViewInfo;

	std::vector<std::shared_ptr<ActorEntity>> Actors;

public:
	Engine();
	void Cache();
	void UpdatePlayers();
	std::vector<std::shared_ptr<ActorEntity>> GetActors();
	CameraCacheEntry GetCameraCache();
	void RefreshViewMatrix(VMMDLL_SCATTER_HANDLE handle);
	uint32_t GetActorSize();

	void DrawLobbyInfo();   // ← Add this

	uint64_t GetGameState() const { return GameState; }
	uint64_t GetPlayerArray() const { return PlayerArray; }

private:
	uint64_t GameState = 0;
	uint64_t PlayerArray = 0;
};
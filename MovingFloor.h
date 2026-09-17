#pragma once
#include "Engine/GameObject.h"
#include <string>
#include <vector>

class MovingFloor : public GameObject
{
public:
	MovingFloor(GameObject* parent);

	void Initialize() override;
	void Update() override;
	void Draw() override;
	void Release() override;

private:
	struct FloorParam
	{
		std::string modelName;
		XMFLOAT3 startPosition;
		XMFLOAT3 endPosition;
		float moveSpeed;
		int waitFrame;
		bool loop;
	};

	struct FloorState
	{
		FloorParam param;
		XMFLOAT3 currentPosition;
		int modelHandle;
		float direction;
		int waitTimer;
		bool reachedEnd;
	};

	std::vector<FloorState> floors_;

	void LoadFromCsv(const std::string& fileName);
	void UpdateFloor(FloorState& floor);
};

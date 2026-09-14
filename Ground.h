#pragma once
#include "Engine/GameObject.h"
#include <vector>

class Ground :
    public GameObject
{
public:
	//コンストラクタ
	//引数：parent  親オブジェクト（SceneManager）
	Ground(GameObject* parent);
	//初期化
	void Initialize() override;
	const std::vector<std::vector<int>> &GetMapData() const { return mapData_; }
	//更新
	void Update() override;
	//描画
	void Draw() override;
	//開放
	void Release() override;
private:
	int hModel_;
	int hModelt_;
	std::vector<std::vector<int>> mapData_;
	int mapWidth_;
	int mapHeight_;
};


#pragma once
#include "Engine/GameObject.h"
#include <vector>
#include <tuple>

class Ground :
    public GameObject
{
public:
	//コンストラクタ
	//引数：parent  親オブジェクト（SceneManager）
	Ground(GameObject* parent);
	//初期化
	void Initialize() override;
	std::vector<std::vector<int>> GetMapData() { return mapData_; }
	//更新
	void Update() override;
	//描画
	void Draw() override;
	//開放
	void Release() override;
	std::tuple<int,int,int> GetEsaCount() { return std::make_tuple(esaCount_, normalEsaCount_, powerEsaCount_); }
	void DecEsaCount(int type) {
		esaCount_--;
		if (type == 0) {
			normalEsaCount_--;
		}
		else if (type == 1) {
			powerEsaCount_--;
		}
	}
private:
	int hModel_;
	int hModelt_;
	int hEsaModel_;
	int hPEsaModel_;
	std::vector<std::vector<int>> mapData_;
	std::vector<std::vector<int>> objMap_;
	int mapWidth_;
	int mapHeight_;
	int esaCount_;
	int normalEsaCount_;
	int powerEsaCount_;
};


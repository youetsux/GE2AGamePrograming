#pragma once
#include "Engine/GameObject.h"



class Player :
    public GameObject
{
	int hWalkModel_;//歩きアニメーションのモデルハンドル
	int hIdleModel_;//待機アニメーションのモデルハンドル
public:
	//コンストラクタ
	//引数：parent  親オブジェクト（SceneManager）
	Player(GameObject* parent);

	//初期化
	void Initialize() override;

	//更新
	void Update() override;

	//描画
	void Draw() override;

	//開放
	void Release() override;
};


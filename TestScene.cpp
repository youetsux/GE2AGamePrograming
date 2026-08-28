#define  _CRT_SECURE_NO_WARNINGS
#include "TestScene.h"
#include "Player.h"
#include "Ground.h"
#include "Engine/Camera.h"
#include "Engine/Text.h"

namespace {
	Ground* pGround;
}

//コンストラクタ
TestScene::TestScene(GameObject * parent)
	: GameObject(parent, "TestScene"), myScore(0)
{
}

//初期化
void TestScene::Initialize()
{	
	//pWp = Instantiate<Weapon>(this);
	Player* pPlayer = Instantiate <Player>(this);
	pGround = Instantiate<Ground>(this);
	pPlayer->SetGround(pGround);

	Camera::SetPosition({ 0,10,-20 });
	Camera::SetTarget({ 0,0,0 });

	pText_ = new Text;
	pText_->Initialize();//テキストの初期化
}

//更新
void TestScene::Update()
{
}

//やること！
// 餌を数えて、残り餌数を表示
// スコアを表示 
// やり方は任せる！
// sprintfでCの文字列を直で作ってもいいよ

//描画
void TestScene::Draw()
{
	std::string scrText;
	char buffer[256];
	sprintf(buffer, "%010d", myScore);
	scrText = "SCORE:" + std::string(buffer);
	pText_->Draw(500, 50, scrText.c_str());
	int esaCount, normalEsaCount, powerEsaCount;
	std::tuple<int, int, int> esa = pGround->GetEsaCount();
	esaCount = std::get<0>(esa);
	normalEsaCount = std::get<1>(esa);
	powerEsaCount = std::get<2>(esa);
	std::string EsaString = "NORMAL ESA:" + std::to_string(normalEsaCount) + " POWER ESA:" + std::to_string(powerEsaCount);
	pText_->Draw(500, 100, EsaString.c_str());
}

//開放
void TestScene::Release()
{
	pText_->Release();//テキストの開放
}

#include "TestScene.h"
#include "Player.h"
#include "Ground.h"
#include "Engine/Camera.h"

//コンストラクタ
TestScene::TestScene(GameObject * parent)
	: GameObject(parent, "TestScene")
{
}

//初期化
void TestScene::Initialize()
{	
	//pWp = Instantiate<Weapon>(this);
	Instantiate <Player>(this);
	Instantiate<Ground>(this);
	Camera::SetPosition({ 0,10,-20 });
	Camera::SetTarget({ 0,0,0 });

}

//更新
void TestScene::Update()
{
}

//描画
void TestScene::Draw()
{
}

//開放
void TestScene::Release()
{
}

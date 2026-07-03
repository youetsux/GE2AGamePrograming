#include "Player.h"
#include "Engine/Model.h"
#include "Engine/Debug.h"
#include "TestScene.h"
#include "Engine/Input.h"

namespace
{
	enum PLAYER_STATE
	{
		PLAYER_IDLE,
		PLAYER_WALK,
		PLAYER_TURN, //回転中
		PLAYER_STATE_MAX //状態の数
	};
	PLAYER_STATE pstate = PLAYER_STATE::PLAYER_IDLE; //プレイヤーの状態を管理する変数

	enum PLAYER_DIRECTION
	{
		PLAYER_UP,
		PLAYER_DOWN,
		PLAYER_LEFT,
		PLAYER_RIGHT,
		PLAYER_DIRECTION_MAX //方向の数
	};

	PLAYER_DIRECTION pdirection = PLAYER_DOWN; //プレイヤーの向きを管理する変数
	float P_ANGLE[4] = { 180.0f, 0.0f, 90.0f, 270.0f }; //プレイヤーの向きに応じた角度を格納する配列
	XMVECTOR P_MOVE[4] = { XMVectorSet(0, 0, 1, 0),
		                   XMVectorSet(0, 0, -1, 0),
		                   XMVectorSet(-1, 0, 0, 0), 
		                   XMVectorSet(1, 0, 0, 0) }; //プレイヤーの向きに応じた移動ベクトルを格納する配列
	float TURN_FRAME = 30.0f; //回転にかかるフレーム数
}


Player::Player(GameObject* parent)
	:GameObject(parent), hWalkModel_(-1), hIdleModel_(-1){
	//swordDirには、初期方向として、ローカルモデルの剣の根っこから
	//先端までのベクトルとして（0,1,0)を代入しておく
	//初期位置は原点
}

void Player::Initialize()
{
	hWalkModel_ = Model::Load("Walking.fbx");
	Model::SetAnimFrame(hWalkModel_, 0, 59, 1.0);
	
	hIdleModel_ = Model::Load("Idle.fbx");
	Model::SetAnimFrame(hIdleModel_, 0, 117, 1.0);


}

void Player::Update()
{
	//transform_.rotate_.y +=1;
	//static float angle = 0.0;
	//angle = angle + 0.3f;
	//XMMATRIX scale = XMMatrixScaling(1.0f, 1.0f, 1.0f);
	//XMMATRIX rotateX = XMMatrixRotationX(XMConvertToRadians(angle));
	//XMMATRIX rotate = XMMatrixRotationY(XMConvertToRadians(angle));
	//XMMATRIX translate = XMMatrixTranslation(1.0f, 0.0f, 0.0f);
	//SetWorldMatrix(scale *  rotate * translate);
	
	XMVECTOR pos = XMLoadFloat3(&transform_.position_);
	XMVECTOR move = XMVectorSet(0, 0, 0, 0);
	const float SPEED = 0.05f;
	float angle = 0.0f;
	pstate = PLAYER_STATE::PLAYER_IDLE;
	PLAYER_DIRECTION oldDir = pdirection; //pdirection　<=　今の向き 

	if (Input::IsKey(DIK_LEFT))
	{
		pdirection = PLAYER_DIRECTION::PLAYER_LEFT;
		pstate = PLAYER_STATE::PLAYER_WALK;
	}
	if(Input::IsKey(DIK_RIGHT))
	{
		pdirection = PLAYER_DIRECTION::PLAYER_RIGHT;
		pstate = PLAYER_STATE::PLAYER_WALK;
	}
	if (Input::IsKey(DIK_UP))
	{
		pdirection = PLAYER_DIRECTION::PLAYER_UP;
		pstate = PLAYER_STATE::PLAYER_WALK;
	}
	if (Input::IsKey(DIK_DOWN))
	{
		pdirection = PLAYER_DIRECTION::PLAYER_DOWN;
		pstate = PLAYER_STATE::PLAYER_WALK;
	}
	if (oldDir != pdirection) {
		//回転しなきゃだよ。
		pstate = PLAYER_STATE::PLAYER_TURN;
	}
	//  ↑ 状態切り替えの処理
	//　↓ 状態ごとの処理

	if (pstate != PLAYER_STATE::PLAYER_IDLE)
	{
		move = P_MOVE[pdirection];
		angle = P_ANGLE[pdirection];
		transform_.rotate_.y = angle;
	}
	else if (pstate == PLAYER_STATE::PLAYER_TURN)
	{
		//回転中の処理
		oldDir　→　今の角度 
		pdirection -> 目標角度
		//３０フレームで回転するようにする
	}
	pos = pos + SPEED * move;
	XMStoreFloat3(&transform_.position_, pos);
	//pos = XMVectorAdd(pos, SPEED*move);
}

void Player::Draw()
{
	//transform_.scale_ = { 0.01,0.01,0.01 };
	//transform_.position_ = { 0, 0.0, 0 };

	if (pstate == PLAYER_STATE::PLAYER_IDLE)
	{
		Model::SetTransform(hIdleModel_, transform_);
		Model::Draw(hIdleModel_);
	}
	else if(pstate == PLAYER_STATE::PLAYER_WALK)
	{
		Model::SetTransform(hWalkModel_, transform_);
		Model::Draw(hWalkModel_);
	}

}


void Player::Release()
{
}

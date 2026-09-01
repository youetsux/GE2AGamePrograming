#include "Player.h"
#include "Engine/Model.h"
#include "Engine/Debug.h"
#include "TestScene.h"
#include "Engine/Input.h"
#include "Ground.h"

namespace
{
	//定数
	const float MAX_SPEED = 0.2f;						//最大移動速度
	const float BASE_SPEED = 0.1f;						//アニメ速度1.0の基準速度
	const float ACCELERATION = 0.005f;					//加速度
	const float FRICTION = 0.008f;						//摩擦（減速度）
	const float BRAKE = 0.02f;							//逆入力ブレーキ
	const float TURN_FRAME = 10.0f;						//回転にかかるフレーム数
	const float BLOCK_SIZE = 2.0f;						//1マスのワールドサイズ
	const XMFLOAT3 START_POS = { 15.0f, 0.75f, 0.5f };	//初期位置

	//enum
	enum PLAYER_STATE
	{
		PLAYER_IDLE,
		PLAYER_WALK,
		PLAYER_TURN,		//回転中
		PLAYER_STATE_MAX	//状態の数
	};

	enum PLAYER_DIRECTION
	{
		PLAYER_UP,
		PLAYER_DOWN,
		PLAYER_LEFT,
		PLAYER_RIGHT,
		PLAYER_DIRECTION_MAX //方向の数
	};

	//向きに応じたテーブル
	float P_ANGLE[4] = { 180.0f, 0.0f, 90.0f, 270.0f };	//プレイヤーの向きに応じた角度
	XMVECTOR P_MOVE[4] = { XMVectorSet(0, 0, 1, 0),
						   XMVectorSet(0, 0, -1, 0),
						   XMVectorSet(-1, 0, 0, 0),
						   XMVectorSet(1, 0, 0, 0) };		//プレイヤーの向きに応じた移動ベクトル

	//状態変数
	PLAYER_STATE pstate = PLAYER_STATE::PLAYER_IDLE;	//プレイヤーの状態
	PLAYER_DIRECTION pdirection = PLAYER_DOWN;			//プレイヤーの向き
	float turnStartAngle = 0.0f;						//回転開始時の角度
	float turnEndAngle = 0.0f;							//回転終了時の角度
	PLAYER_DIRECTION turnEndDirection = PLAYER_DOWN;	//回転終了時の向き
	float currentSpeed = 0.0f;							//現在の速度
	std::vector<std::vector<int>> gmap;					//マップデータ

	//関数
	float AdjustAngle(float angle) {
		if(angle >= 180)
		{
			angle -= 360.0f;
		}
		else if(angle < -180.0f)
		{
			angle += 360.0f;
		}
		return angle;
	}
}


Player::Player(GameObject* parent)
	:GameObject(parent, "Player"), hWalkModel_(-1), hIdleModel_(-1) {
}

void Player::Initialize()
{
	hWalkModel_ = Model::Load("Walking.fbx");
	Model::SetAnimFrame(hWalkModel_, 0, 59, 1.0);
	transform_.position_ = START_POS;
	hIdleModel_ = Model::Load("Idle.fbx");
	Model::SetAnimFrame(hIdleModel_, 0, 117, 1.0);
	SphereCollider* collision = new SphereCollider(XMFLOAT3(0, 0.25, 0), 0.5f);
	AddCollider(collision);

}

void Player::Update()
{	
	XMVECTOR pos = XMLoadFloat3(&transform_.position_);
	XMVECTOR move = XMVectorSet(0, 0, 0, 0);
	float angle = 0.0f;
	static float turnFrame = 0.0f; //回転中のフレーム数を管理する変数

	if (pstate != PLAYER_STATE::PLAYER_TURN) {
		pstate = PLAYER_STATE::PLAYER_IDLE;
	}//回転中でなければ、状態を待機にする

	PLAYER_DIRECTION oldDir = pdirection; //pdirection　<=　今の向き
	bool isBraking = false; //逆入力ブレーキフラグ

	//速度0の時だけ方向転換を受け付ける
	if (pstate != PLAYER_STATE::PLAYER_TURN && currentSpeed == 0.0f)
	{
		if (Input::IsKey(DIK_LEFT))
		{
			pdirection = PLAYER_DIRECTION::PLAYER_LEFT;
			pstate = PLAYER_STATE::PLAYER_WALK;
		}
		if (Input::IsKey(DIK_RIGHT))
		{
			pdirection = PLAYER_DIRECTION::PLAYER_RIGHT;
			pstate = PLAYER_STATE::PLAYER_WALK;
		}
	}
	else if (pstate != PLAYER_STATE::PLAYER_TURN)
	{
		if (Input::IsKey(DIK_LEFT))
		{
			if (pdirection == PLAYER_DIRECTION::PLAYER_LEFT)
				pstate = PLAYER_STATE::PLAYER_WALK;
			else if (pdirection == PLAYER_DIRECTION::PLAYER_RIGHT)
				isBraking = true;
		}
		if (Input::IsKey(DIK_RIGHT))
		{
			if (pdirection == PLAYER_DIRECTION::PLAYER_RIGHT)
				pstate = PLAYER_STATE::PLAYER_WALK;
			else if (pdirection == PLAYER_DIRECTION::PLAYER_LEFT)
				isBraking = true;
		}
	}

	if (oldDir != pdirection) {
		//速度0の時だけ回転開始
		pstate = PLAYER_STATE::PLAYER_TURN;
		turnFrame = 0.0f;
		turnStartAngle = P_ANGLE[oldDir];
		float diff = AdjustAngle(P_ANGLE[pdirection] - P_ANGLE[oldDir]);
		turnEndDirection = pdirection;
		turnEndAngle = turnStartAngle + diff;
	}
	//  ↑ 状態切り替えの処理
	//　↓ 状態ごとの処理

	if (pstate == PLAYER_STATE::PLAYER_TURN)
	{
		turnFrame += 1.0f;
		float t = turnFrame / TURN_FRAME; //0.0～1.0
		if (t > 1.0f)
		{
			t = 1.0f;//1.0を超えないようにする(保険）
		}
		angle = turnStartAngle + (turnEndAngle - turnStartAngle) * t;
		transform_.rotate_.y = angle;
		// 30フレーム経過したら、回転終了
		if (turnFrame >= TURN_FRAME)
		{
			pdirection = turnEndDirection;
			transform_.rotate_.y = P_ANGLE[pdirection];
			pstate = PLAYER_STATE::PLAYER_WALK;
		}
		return;//早期リターンで、回転中は移動しないようにする
	}
	else if (pstate == PLAYER_STATE::PLAYER_WALK)
	{
		//加速
		currentSpeed += ACCELERATION;
		if (currentSpeed > MAX_SPEED) currentSpeed = MAX_SPEED;
		move = P_MOVE[pdirection];
		angle = P_ANGLE[pdirection];
		transform_.rotate_.y = angle;
	}
	else //PLAYER_IDLE
	{
		//慣性で減速（逆入力時はブレーキ）
		if (currentSpeed > 0.0f)
		{
			currentSpeed -= isBraking ? BRAKE : FRICTION;
			if (currentSpeed < 0.0f) currentSpeed = 0.0f;
			move = P_MOVE[pdirection];
		}
	}

	//速度に応じてアニメーションスピードを更新（BASE_SPEEDの時にanimSpeed=1.0）
	float animSpeed = currentSpeed / BASE_SPEED;
	Model::SetAnimSpeed(hWalkModel_, animSpeed);

	pos = pos + currentSpeed * move;
	XMStoreFloat3(&transform_.position_, pos);
	XMFLOAT3 wpos = transform_.position_;
	//壁オブジェクトに食い込んでたら戻す！
	gmap = ground_->GetMapData();//マップを取得
	//マップの座標に変換する、めり込んでたら戻す。
	int mapWidth = (int)gmap[0].size();
	int mapHeight = (int)gmap.size();
	int mapX = (int)((wpos.x + BLOCK_SIZE / 2.0f) / BLOCK_SIZE);
	int mapZ = (int)((START_POS.z + (BLOCK_SIZE * mapHeight / 2.0f) - wpos.z) / BLOCK_SIZE);
	if (mapX >= 0 && mapX < mapWidth && mapZ >= 0 && mapZ < mapHeight)
	{
		if (gmap[mapZ][mapX] == 1 && (pdirection == PLAYER_LEFT || pdirection == PLAYER_RIGHT))
		{
			pos = pos - currentSpeed * move;
			XMStoreFloat3(&transform_.position_, pos);
			currentSpeed = 0.0f; //壁に当たったら速度リセット
		}
	}
}

void Player::Draw()
{
	if (pstate == PLAYER_STATE::PLAYER_IDLE)
	{
		Model::SetTransform(hIdleModel_, transform_);
		Model::Draw(hIdleModel_);
	}
	else if(pstate == PLAYER_STATE::PLAYER_WALK || pstate == PLAYER_STATE::PLAYER_TURN)
	{
		Model::SetTransform(hWalkModel_, transform_);
		Model::Draw(hWalkModel_);
	}

}


void Player::Release()
{
}

void Player::OnCollision(GameObject* pTarget)
{
}

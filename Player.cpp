#include "Player.h"
#include "Engine/Model.h"
#include "Engine/Debug.h"
#include "TestScene.h"
#include "Engine/Input.h"
#include "Ground.h"
#include <cmath>

namespace
{
	// ------------------------------------------------------------
	// プレイヤーの移動に関する定数
	// ------------------------------------------------------------
	const float MAX_SPEED = 0.2f;			// 最大移動速度
	const float BASE_SPEED = 0.1f;			// アニメーション速度1.0になる基準速度
	const float ACCELERATION = 0.005f;		// 移動入力中の加速度
	const float FRICTION = 0.008f;			// 入力を離したときの減速度
	const float BRAKE = 0.02f;				// 進行方向と逆入力したときの減速度
	const float TURN_FRAME = 10.0f;			// 方向転換にかけるフレーム数
	const float BLOCK_INTERVAL_X = 2.0f;			// マップ1マス分のワールドサイズ

	// プレイヤーの初期位置
	// このY座標を、地面に立っているときの高さとして使用する
	const XMFLOAT3 START_POS = { 15.0f, 0.75f, 0.5f };	// プレイヤーの初期座標

	// ------------------------------------------------------------
	// ジャンプに関する定数
	// ------------------------------------------------------------
	const float JUMP_POWER = 0.2f;			// ジャンプ開始時の上向き速度
	const float GRAVITY = 0.01f;			// 1フレームごとに減少する垂直速度
	const float AIR_CONTROL = 0.5f;			// 空中での加速・減速の強さ（地上比）

	// ブロックの配置間隔・水平寸法・ゲーム上の歩行面。
	const float BLOCK_INTERVAL_Y = 1.0f;		// ブロック1マス分の縦方向の間隔
	const float BLOCK_HALF_WIDTH = 0.99375f;	// ブロックの当たり判定の半分の幅
	const float BLOCK_SURFACE_HEIGHT = 0.75f;	// ブロック上面の高さ

	// プレイヤーの判定寸法。原点を足元として扱う。
	const float PLAYER_FOOT_OFFSET = 0.0f;		// プレイヤーの足元位置の補正値
	// 身長はマップの縦2マス。判定寸法はワールド座標で定義する。
	const float PLAYER_HEIGHT = BLOCK_INTERVAL_Y * 2.0f;	// プレイヤーの当たり判定の高さ
	const float PLAYER_MODEL_HEIGHT = 3.76537f;		// プレイヤーモデルの元の高さ（スケール計算用）
	const float PLAYER_MODEL_SCALE = PLAYER_HEIGHT / PLAYER_MODEL_HEIGHT;
	// 横幅は従来の判定幅を描画モデルと同じ割合で縮小する。
	const float PLAYER_HALF_WIDTH = 0.4f * PLAYER_MODEL_SCALE;	// プレイヤーの当たり判定の半分の幅
	const float CONTACT_EPSILON = 0.0001f;				// 当たり判定の誤差吸収用の微小値
	const float WALL_WALK_ANIM_SPEED = 1.0f; // 壁押し中の歩行再生速度

	// 矩形の当たり判定を表す構造体
	struct CollisionRect
	{
		float left, right, bottom, top;
	};

	// プレイヤーの座標から当たり判定用の矩形を生成する
	CollisionRect MakePlayerRect(const XMFLOAT3& position)
	{
		const float foot = position.y - PLAYER_FOOT_OFFSET;
		return { position.x - PLAYER_HALF_WIDTH,
			position.x + PLAYER_HALF_WIDTH, foot, foot + PLAYER_HEIGHT };
	}

	// ブロックの行・列・マップの高さから当たり判定用の矩形を生成する
	CollisionRect MakeBlockRect(int row, int col, int mapHeight)
	{
		const float x = col * BLOCK_INTERVAL_X;
		const float y = (mapHeight - 1 - row) * BLOCK_INTERVAL_Y;
		return { x - BLOCK_HALF_WIDTH, x + BLOCK_HALF_WIDTH,
			y, y + BLOCK_SURFACE_HEIGHT };
	}

	// 2つの矩形がX軸方向に重なっているか判定する
	bool OverlapX(const CollisionRect& a, const CollisionRect& b)
	{
		return a.right > b.left + CONTACT_EPSILON &&
			a.left < b.right - CONTACT_EPSILON;
	}

	// 2つの矩形がY軸方向に重なっているか判定する
	bool OverlapY(const CollisionRect& a, const CollisionRect& b)
	{
		return a.top > b.bottom + CONTACT_EPSILON &&
			a.bottom < b.top - CONTACT_EPSILON;
	}

	// ------------------------------------------------------------
	// プレイヤーの向きに対応する角度
	//
	// PLAYER_DIRECTION の並びと同じ順番
	// UP / DOWN / LEFT / RIGHT
	// ------------------------------------------------------------
	const float P_ANGLE[4] =
	{
		180.0f,
		0.0f,
		90.0f,
		270.0f
	};

	// ------------------------------------------------------------
	// プレイヤーの向きに対応する移動ベクトル
	// ------------------------------------------------------------
	const XMVECTOR P_MOVE[4] =
	{
		XMVectorSet(0, 0,  1, 0),
		XMVectorSet(0, 0, -1, 0),
		XMVectorSet(-1, 0,  0, 0),
		XMVectorSet(1, 0,  0, 0)
	};

	// ------------------------------------------------------------
	// 角度差を -180度 ～ 180度 の範囲に補正する
	//
	// 例：
	// 270度回転する代わりに -90度回転することで、
	// 常に近い方向へ回転する
	// ------------------------------------------------------------
	float AdjustAngle(float angle)
	{
		if (angle >= 180.0f)
		{
			angle -= 360.0f;
		}
		else if (angle < -180.0f)
		{
			angle += 360.0f;
		}

		return angle;
	}
}


// ------------------------------------------------------------
// コンストラクタ
// ------------------------------------------------------------
Player::Player(GameObject* parent)
	: GameObject(parent, "Player"),
	hWalkModel_(-1),
	hIdleModel_(-1),
	ground_(nullptr),
	pstate_(PLAYER_IDLE),
	pdirection_(PLAYER_DOWN),
	turnStartAngle_(0.0f),
	turnEndAngle_(0.0f),
	turnEndDirection_(PLAYER_DOWN),
	currentSpeed_(0.0f),
	turnFrame_(0.0f),
	jumpVelocity_(0.0f),
	isGrounded_(true)
{}


// ------------------------------------------------------------
// 初期化
// ------------------------------------------------------------
void Player::Initialize()
{
	// 歩行用モデル
	hWalkModel_ = Model::Load("Walking.fbx");
	Model::SetAnimFrame(hWalkModel_, 0, 59, 1.0);

	// 初期位置
	transform_.position_ = START_POS;

	// 待機用モデル
	hIdleModel_ = Model::Load("Idle.fbx");
	Model::SetAnimFrame(hIdleModel_, 0, 117, 1.0);

	// プレイヤー用の球コライダー
	SphereCollider* collision =
		new SphereCollider(XMFLOAT3(0, 0.25f, 0), 0.5f);

	AddCollider(collision);
}


// ------------------------------------------------------------
// 毎フレームの更新
//
// 1. 入力処理
// 2. 方向転換
// 3. 水平方向の移動
// 4. 壁との衝突処理
// 5. ジャンプ・着地・重力処理
// ------------------------------------------------------------
void Player::Update()
{
	// 方向転換中でなければ、いったん待機状態に戻す
	// HandleInput() で移動入力があれば WALK に変化する
	if (pstate_ != PLAYER_TURN)
	{
		pstate_ = PLAYER_IDLE;
	}

	bool isBraking = HandleInput();

	// 方向転換中は通常の移動処理を行わない
	if (UpdateTurn())
	{
		UpdateJump();
		return;
	}

	XMVECTOR pos = XMLoadFloat3(&transform_.position_);
	XMVECTOR move = XMVectorSet(0, 0, 0, 0);

	// --------------------------------------------------------
	// 移動入力中
	// --------------------------------------------------------
	if (pstate_ == PLAYER_WALK)
	{
		// 空中では加速を弱くする
		float accel = isGrounded_
			? ACCELERATION
			: ACCELERATION * AIR_CONTROL;

		currentSpeed_ += accel;

		if (currentSpeed_ > MAX_SPEED)
		{
			currentSpeed_ = MAX_SPEED;
		}

		move = P_MOVE[pdirection_];
		transform_.rotate_.y = P_ANGLE[pdirection_];
	}

	// --------------------------------------------------------
	// 移動入力がない場合
	// --------------------------------------------------------
	else
	{
		if (currentSpeed_ > 0.0f)
		{
			float decel;

			if (isGrounded_)
			{
				// 逆方向入力なら強く減速する
				decel = isBraking ? BRAKE : FRICTION;
			}
			else
			{
				// 空中では減速を弱くする
				decel = FRICTION * AIR_CONTROL;
			}

			currentSpeed_ -= decel;

			if (currentSpeed_ < 0.0f)
			{
				currentSpeed_ = 0.0f;
			}

			// 入力を離しても、減速中は今までの方向へ進む
			move = P_MOVE[pdirection_];
		}
	}

	// --------------------------------------------------------
	// 水平方向の移動
	// --------------------------------------------------------
	pos = pos + currentSpeed_ * move;
	XMStoreFloat3(&transform_.position_, pos);

	// 衝突で速度がゼロになったかを、解決前後で確認する。
	const float speedBeforeCollision = currentSpeed_;
	ResolveWallCollision(pos, move);
	const bool blockedByWall =
		speedBeforeCollision > 0.0f && currentSpeed_ == 0.0f;

	// 実際の移動速度と、壁に向かって歩くアニメの速度を分離する。
	// 入力を離した場合や逆方向へのブレーキ中は固定再生しない。
	const bool pushingWall = blockedByWall && pstate_ == PLAYER_WALK;
	const float walkAnimSpeed = pushingWall
		? WALL_WALK_ANIM_SPEED
		: currentSpeed_ / BASE_SPEED;
	Model::SetAnimSpeed(hWalkModel_, walkAnimSpeed);

	// ジャンプ・重力・ブロックへの着地
	UpdateJump();
}


// ------------------------------------------------------------
// 入力処理
//
// 戻り値：
// true  = 進行方向と逆方向を入力している
// false = 通常
// ------------------------------------------------------------
bool Player::HandleInput()
{
	bool isBraking = false;

	// 入力前の向きを保存しておく
	PLAYER_DIRECTION oldDir = pdirection_;

	if (pstate_ != PLAYER_TURN)
	{
		// ----------------------------------------------------
		// 完全停止中
		// ----------------------------------------------------
		if (currentSpeed_ == 0.0f && isGrounded_)
		{
			if (Input::IsKey(DIK_LEFT))
			{
				pdirection_ = PLAYER_LEFT;
				pstate_ = PLAYER_WALK;
			}

			if (Input::IsKey(DIK_RIGHT))
			{
				pdirection_ = PLAYER_RIGHT;
				pstate_ = PLAYER_WALK;
			}
		}

		// ----------------------------------------------------
		// 移動中または空中
		// ----------------------------------------------------
		else
		{
			if (Input::IsKey(DIK_LEFT))
			{
				if (pdirection_ == PLAYER_LEFT)
				{
					pstate_ = PLAYER_WALK;
				}
				else if (pdirection_ == PLAYER_RIGHT)
				{
					// 地上で逆方向入力したときだけブレーキ
					isBraking = isGrounded_;
				}
			}

			if (Input::IsKey(DIK_RIGHT))
			{
				if (pdirection_ == PLAYER_RIGHT)
				{
					pstate_ = PLAYER_WALK;
				}
				else if (pdirection_ == PLAYER_LEFT)
				{
					// 地上で逆方向入力したときだけブレーキ
					isBraking = isGrounded_;
				}
			}
		}
	}

	// --------------------------------------------------------
	// ジャンプ開始
	// --------------------------------------------------------
	if (Input::IsKeyDown(DIK_SPACE) && isGrounded_)
	{
		jumpVelocity_ = JUMP_POWER;
		isGrounded_ = false;
	}

	// --------------------------------------------------------
	// 向きが変わったら方向転換を開始
	// --------------------------------------------------------
	if (oldDir != pdirection_)
	{
		pstate_ = PLAYER_TURN;

		turnFrame_ = 0.0f;
		turnStartAngle_ = P_ANGLE[oldDir];

		// 最短方向へ回転するため角度差を補正する
		float diff =
			AdjustAngle(P_ANGLE[pdirection_] - P_ANGLE[oldDir]);

		turnEndDirection_ = pdirection_;
		turnEndAngle_ = turnStartAngle_ + diff;
	}

	return isBraking;
}


// ------------------------------------------------------------
// 方向転換処理
//
// TURN_FRAME フレームかけて回転する
// ------------------------------------------------------------
bool Player::UpdateTurn()
{
	if (pstate_ != PLAYER_TURN)
	{
		return false;
	}

	turnFrame_ += 1.0f;

	// 0.0 ～ 1.0 の補間率
	float t = min(turnFrame_ / TURN_FRAME, 1.0f);

	transform_.rotate_.y =
		turnStartAngle_
		+ (turnEndAngle_ - turnStartAngle_) * t;

	// 方向転換終了
	if (turnFrame_ >= TURN_FRAME)
	{
		pdirection_ = turnEndDirection_;

		// 補間誤差が残らないよう最終角度を設定する
		transform_.rotate_.y = P_ANGLE[pdirection_];

		pstate_ = PLAYER_WALK;
	}

	return true;
}


// ------------------------------------------------------------
// ジャンプ・重力・ブロックへの着地処理
//
// 落下中は、
// 「前フレームの足位置」と「現在の足位置」の間で
// ブロック上面を通過したかを調べる。
//
// そのため、1フレームの落下量が大きくても
// ブロックを飛び越えにくい。
// ------------------------------------------------------------
// Y方向：接地維持・上面への着地・下面への頭突き
void Player::UpdateJump()
{
	if (ground_ == nullptr)
		return;

	const auto& gmap = ground_->GetMapData();
	const int mapHeight = static_cast<int>(gmap.size());
	const CollisionRect before = MakePlayerRect(transform_.position_);

	if (isGrounded_)
	{
		// 既存仕様の常設床。穴を作る場合はこの床もマップで管理する。
		bool supported = transform_.position_.y <= START_POS.y + CONTACT_EPSILON;
		float supportY = START_POS.y;
		for (int row = 0; row < mapHeight; ++row)
		{
			for (int col = 0; col < static_cast<int>(gmap[row].size()); ++col)
			{
				if (gmap[row][col] != 1) continue;
				const CollisionRect block = MakeBlockRect(row, col, mapHeight);
				if (OverlapX(before, block) &&
					std::fabs(before.bottom - block.top) <= CONTACT_EPSILON)
				{
					supported = true;
					supportY = block.top + PLAYER_FOOT_OFFSET;
				}
			}
		}
		if (supported)
		{
			transform_.position_.y = supportY;
			jumpVelocity_ = 0.0f;
			return;
		}
		isGrounded_ = false;
		jumpVelocity_ = 0.0f;
	}

	const float dy = jumpVelocity_;
	transform_.position_.y += dy;
	jumpVelocity_ -= GRAVITY;
	const CollisionRect after = MakePlayerRect(transform_.position_);
	float resolvedY = transform_.position_.y;
	bool hit = false;

	// 移動前後で面を跨いだかを調べ、最初に接触する面で止める。
	for (int row = 0; row < mapHeight; ++row)
	{
		for (int col = 0; col < static_cast<int>(gmap[row].size()); ++col)
		{
			if (gmap[row][col] != 1) continue;
			const CollisionRect block = MakeBlockRect(row, col, mapHeight);
			if (!OverlapX(after, block)) continue;

			if (dy <= 0.0f && before.bottom >= block.top - CONTACT_EPSILON &&
				after.bottom <= block.top)
			{
				const float y = block.top + PLAYER_FOOT_OFFSET;
				if (!hit || y > resolvedY) resolvedY = y;
				hit = true;
			}
			else if (dy > 0.0f && before.top <= block.bottom + CONTACT_EPSILON &&
				after.top >= block.bottom)
			{
				const float y = block.bottom - PLAYER_HEIGHT + PLAYER_FOOT_OFFSET;
				if (!hit || y < resolvedY) resolvedY = y;
				hit = true;
			}
		}
	}

	// 常設床も着地候補に含める。
	if (dy <= 0.0f && resolvedY <= START_POS.y)
	{
		resolvedY = START_POS.y;
		hit = true;
	}
	transform_.position_.y = resolvedY;
	if (hit)
	{
		jumpVelocity_ = 0.0f;
		// 頭突きでは接地させない。次の更新から重力で落下する。
		isGrounded_ = dy <= 0.0f;
	}
}


// ------------------------------------------------------------
// 壁との衝突処理
//
// ブロックより低い位置にいるときだけ、
// ブロックを横方向の壁として扱う。
//
// ブロック上面より高ければ、その上を移動できる。
// ------------------------------------------------------------
// X方向：移動前後で左右の面を跨ぐかを判定
void Player::ResolveWallCollision(XMVECTOR& pos, const XMVECTOR& move)
{
	if (ground_ == nullptr)
		return;

	// Update() で適用した水平移動から、移動前の矩形を復元する。
	XMFLOAT3 oldPosition;
	XMStoreFloat3(&oldPosition, pos - currentSpeed_ * move);
	const float dx = transform_.position_.x - oldPosition.x;
	if (dx == 0.0f) return;

	const CollisionRect before = MakePlayerRect(oldPosition);
	const CollisionRect after = MakePlayerRect(transform_.position_);
	const auto& gmap = ground_->GetMapData();
	const int mapHeight = static_cast<int>(gmap.size());
	float resolvedX = transform_.position_.x;
	bool hit = false;

	for (int row = 0; row < mapHeight; ++row)
	{
		for (int col = 0; col < static_cast<int>(gmap[row].size()); ++col)
		{
			if (gmap[row][col] != 1) continue;
			const CollisionRect block = MakeBlockRect(row, col, mapHeight);
			if (!OverlapY(before, block)) continue;

			if (dx > 0.0f && before.right <= block.left + CONTACT_EPSILON &&
				after.right >= block.left)
			{
				const float x = block.left - PLAYER_HALF_WIDTH;
				if (!hit || x < resolvedX) resolvedX = x;
				hit = true;
			}
			else if (dx < 0.0f && before.left >= block.right - CONTACT_EPSILON &&
				after.left <= block.right)
			{
				const float x = block.right + PLAYER_HALF_WIDTH;
				if (!hit || x > resolvedX) resolvedX = x;
				hit = true;
			}
		}
	}
	if (hit)
	{
		transform_.position_.x = resolvedX;
		pos = XMLoadFloat3(&transform_.position_);
		currentSpeed_ = 0.0f;
	}
}


// ------------------------------------------------------------
// 描画
// ------------------------------------------------------------
void Player::Draw()
{
	// 描画用のコピーだけを縮小する。位置と矩形判定には倍率を重ねない。
	// Idle.fbx も Walking.fbx と同じ元サイズを前提とする。
	Transform drawTransform = transform_;
	drawTransform.scale_.x *= PLAYER_MODEL_SCALE;
	drawTransform.scale_.y *= PLAYER_MODEL_SCALE;
	drawTransform.scale_.z *= PLAYER_MODEL_SCALE;

	if (pstate_ == PLAYER_IDLE)
	{
		Model::SetTransform(hIdleModel_, drawTransform);
		Model::Draw(hIdleModel_);
	}
	else if (pstate_ == PLAYER_WALK || pstate_ == PLAYER_TURN)
	{
		Model::SetTransform(hWalkModel_, drawTransform);
		Model::Draw(hWalkModel_);
	}
}


// ------------------------------------------------------------
// 終了処理
// ------------------------------------------------------------
void Player::Release()
{}


// ------------------------------------------------------------
// 他オブジェクトとの衝突通知
// ------------------------------------------------------------
void Player::OnCollision(GameObject* pTarget)
{}
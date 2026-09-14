#include "Player.h"
#include "Engine/Model.h"
#include "Engine/Debug.h"
#include "TestScene.h"
#include "Engine/Input.h"
#include "Ground.h"

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
	const float BLOCK_SIZE = 2.0f;			// マップ1マス分のワールドサイズ

	// プレイヤーの初期位置
	const XMFLOAT3 START_POS = { 15.0f, 0.75f, 0.5f };

	// ------------------------------------------------------------
	// ジャンプに関する定数
	// ------------------------------------------------------------
	const float JUMP_POWER = 0.2f;			// ジャンプ開始時の上向き速度
	const float GRAVITY = 0.01f;			// 1フレームごとに減少する垂直速度
	const float AIR_CONTROL = 0.5f;			// 空中での加速・減速の強さ（地上比）

	// ------------------------------------------------------------
	// プレイヤーの向きに対応する角度
	//
	// PLAYER_DIRECTION の並びと同じ順番にしている
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
	//
	// P_ANGLE と同様に PLAYER_DIRECTION の並びに対応している
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
	//   270度回転する代わりに -90度回転させることで、
	//   常に近い方向へ回転できるようにする
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
//
// プレイヤーが持つ状態変数を初期化する
// ------------------------------------------------------------
Player::Player(GameObject* parent)
	: GameObject(parent, "Player"),
	hWalkModel_(-1),
	hIdleModel_(-1),
	pstate_(PLAYER_IDLE),
	pdirection_(PLAYER_DOWN),
	turnStartAngle_(0.0f),
	turnEndAngle_(0.0f),
	turnEndDirection_(PLAYER_DOWN),
	currentSpeed_(0.0f),
	turnFrame_(0.0f),
	jumpVelocity_(0.0f),
	isGrounded_(true),
	ground_(nullptr)
{}


// ------------------------------------------------------------
// 初期化
//
// モデル、アニメーション、初期位置、コライダーを設定する
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
// 5. ジャンプ・重力処理
//
// の順番で処理する
// ------------------------------------------------------------
void Player::Update()
{
	// 方向転換中でなければ、いったん待機状態に戻す
	// この後 HandleInput() で移動入力があれば WALK に変わる
	if (pstate_ != PLAYER_TURN)
	{
		pstate_ = PLAYER_IDLE;
	}

	// 入力を調べる
	// 進行方向と逆方向が入力されていれば true が返る
	bool isBraking = HandleInput();

	// 方向転換中は、そのフレームでは通常の移動処理を行わない
	if (UpdateTurn())
	{
		return;
	}

	// 現在位置を DirectXMath のベクトルとして取得
	XMVECTOR pos = XMLoadFloat3(&transform_.position_);

	// このフレームで進む方向
	// 入力がなければゼロベクトル
	XMVECTOR move = XMVectorSet(0, 0, 0, 0);

	// --------------------------------------------------------
	// 移動入力中
	// --------------------------------------------------------
	if (pstate_ == PLAYER_WALK)
	{
		// 空中では地上より加速を弱くする
		float accel = isGrounded_
			? ACCELERATION
			: ACCELERATION * AIR_CONTROL;

		currentSpeed_ += accel;

		// 最大速度を超えないようにする
		if (currentSpeed_ > MAX_SPEED)
		{
			currentSpeed_ = MAX_SPEED;
		}

		// 現在向いている方向へ移動する
		move = P_MOVE[pdirection_];

		// モデルの向きも移動方向に合わせる
		transform_.rotate_.y = P_ANGLE[pdirection_];
	}

	// --------------------------------------------------------
	// 移動入力がない場合
	// --------------------------------------------------------
	else
	{
		// 慣性で動いている間は徐々に減速する
		if (currentSpeed_ > 0.0f)
		{
			float decel;

			if (isGrounded_)
			{
				// 地上では、
				// 逆方向入力中なら強いブレーキ、
				// 入力なしなら通常の摩擦で減速する
				decel = isBraking ? BRAKE : FRICTION;
			}
			else
			{
				// 空中では減速を弱くする
				decel = FRICTION * AIR_CONTROL;
			}

			currentSpeed_ -= decel;

			// 速度がマイナスにならないようにする
			if (currentSpeed_ < 0.0f)
			{
				currentSpeed_ = 0.0f;
			}

			// 入力を離しても、減速中はこれまでの方向へ進み続ける
			move = P_MOVE[pdirection_];
		}
	}

	// 移動速度に応じて歩行アニメーション速度も変える
	//
	// currentSpeed_ == BASE_SPEED のとき
	// アニメーション速度は 1.0 になる
	Model::SetAnimSpeed(
		hWalkModel_,
		currentSpeed_ / BASE_SPEED
	);

	// --------------------------------------------------------
	// 水平方向の移動
	// --------------------------------------------------------
	pos = pos + currentSpeed_ * move;
	XMStoreFloat3(&transform_.position_, pos);

	// 移動後の位置が壁に入っていないか調べる
	// 壁に入っていた場合は水平方向の移動を取り消す
	ResolveWallCollision(pos, move);

	// 水平方向の処理が終わってから、
	// ジャンプ・重力によるY方向の移動を行う
	UpdateJump();
}


// ------------------------------------------------------------
// 入力処理
//
// 左右移動、逆方向入力によるブレーキ、ジャンプを処理する
//
// 戻り値：
//   true  = 進行方向と逆方向の入力中
//   false = 通常状態
// ------------------------------------------------------------
bool Player::HandleInput()
{
	bool isBraking = false;

	// 入力前の向きを保存しておく
	// 入力後に向きが変わったかを判定するために使用する
	PLAYER_DIRECTION oldDir = pdirection_;

	// 方向転換中は新しい左右入力を受け付けない
	if (pstate_ != PLAYER_TURN)
	{
		// ----------------------------------------------------
		// 完全に停止していて、地上にいる場合
		//
		// 左右入力された方向へすぐに向きを変更する
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
		// すでに移動中、または空中にいる場合
		// ----------------------------------------------------
		else
		{
			if (Input::IsKey(DIK_LEFT))
			{
				// 左へ進んでいる状態で左入力
				// →そのまま移動を続ける
				if (pdirection_ == PLAYER_LEFT)
				{
					pstate_ = PLAYER_WALK;
				}

				// 右へ進んでいる状態で左入力
				// →地上ならブレーキをかける
				else if (pdirection_ == PLAYER_RIGHT)
				{
					isBraking = isGrounded_;
				}
			}

			if (Input::IsKey(DIK_RIGHT))
			{
				// 右へ進んでいる状態で右入力
				// →そのまま移動を続ける
				if (pdirection_ == PLAYER_RIGHT)
				{
					pstate_ = PLAYER_WALK;
				}

				// 左へ進んでいる状態で右入力
				// →地上ならブレーキをかける
				else if (pdirection_ == PLAYER_LEFT)
				{
					isBraking = isGrounded_;
				}
			}
		}
	}

	// --------------------------------------------------------
	// ジャンプ開始
	//
	// Spaceを押した瞬間、かつ地上にいる場合だけジャンプする
	// --------------------------------------------------------
	if (Input::IsKeyDown(DIK_SPACE) && isGrounded_)
	{
		jumpVelocity_ = JUMP_POWER;
		isGrounded_ = false;
	}

	// --------------------------------------------------------
	// 入力によって向きが変わった場合は方向転換を開始する
	// --------------------------------------------------------
	if (oldDir != pdirection_)
	{
		pstate_ = PLAYER_TURN;

		// 方向転換の経過フレームをリセット
		turnFrame_ = 0.0f;

		// 回転開始時の角度
		turnStartAngle_ = P_ANGLE[oldDir];

		// 回転量を -180～180度に補正し、
		// 最短方向へ回転させる
		float diff =
			AdjustAngle(P_ANGLE[pdirection_] - P_ANGLE[oldDir]);

		// 回転完了後の向き
		turnEndDirection_ = pdirection_;

		// 補間に使用する終了角度
		turnEndAngle_ = turnStartAngle_ + diff;
	}

	return isBraking;
}


// ------------------------------------------------------------
// 方向転換処理
//
// TURN_FRAME フレームかけて開始角度から終了角度へ回転する
//
// 戻り値：
//   true  = 現在方向転換中
//   false = 方向転換していない
// ------------------------------------------------------------
bool Player::UpdateTurn()
{
	if (pstate_ != PLAYER_TURN)
	{
		return false;
	}

	// 方向転換開始からの経過フレーム
	turnFrame_ += 1.0f;

	// 0.0 ～ 1.0 の補間率を求める
	float t = min(turnFrame_ / TURN_FRAME, 1.0f);

	// 開始角度から終了角度まで線形補間する
	transform_.rotate_.y =
		turnStartAngle_
		+ (turnEndAngle_ - turnStartAngle_) * t;

	// 指定フレーム数に到達したら方向転換終了
	if (turnFrame_ >= TURN_FRAME)
	{
		pdirection_ = turnEndDirection_;

		// 補間誤差が残らないように最終角度を設定する
		transform_.rotate_.y = P_ANGLE[pdirection_];

		// 回転後は歩行状態へ戻す
		pstate_ = PLAYER_WALK;
	}

	return true;
}


// ------------------------------------------------------------
// ジャンプ・重力処理
//
// jumpVelocity_ をY座標に加算し、
// 毎フレーム GRAVITY 分だけ下向きに加速させる
// ------------------------------------------------------------
void Player::UpdateJump()
{
	// 地上にいる場合はY座標を地面の高さに固定する
	if (isGrounded_)
	{
		transform_.position_.y = START_POS.y;
		return;
	}

	// 現在の垂直速度だけ上下方向へ移動する
	transform_.position_.y += jumpVelocity_;

	// 重力によって垂直速度を毎フレーム減らす
	//
	// 上昇中：
	//   正の速度が徐々に0へ近づく
	//
	// 落下中：
	//   速度が負になり下方向へ移動する
	jumpVelocity_ -= GRAVITY;

	// 地面の高さまで落ちたら着地
	if (transform_.position_.y <= START_POS.y)
	{
		transform_.position_.y = START_POS.y;

		jumpVelocity_ = 0.0f;
		isGrounded_ = true;
	}
}


// ------------------------------------------------------------
// 壁との衝突処理
//
// 現在位置からマップ上のマスを求め、
// 壁のマスに入っていた場合は直前の水平移動を取り消す
// ------------------------------------------------------------
void Player::ResolveWallCollision(
	XMVECTOR& pos,
	const XMVECTOR& move)
{
	// Ground が持つマップデータを参照する
	// コピーせず、そのまま利用する
	const auto& gmap = ground_->GetMapData();

	int mapWidth = static_cast<int>(gmap[0].size());
	int mapHeight = static_cast<int>(gmap.size());

	XMFLOAT3 wpos = transform_.position_;

	// --------------------------------------------------------
	// ワールド座標Xからマップ上のX座標へ変換する
	//
	// BLOCK_SIZE / 2 を加えることで、
	// マスの中心位置を基準に判定している
	// --------------------------------------------------------
	int mapX =
		static_cast<int>(
			(wpos.x + BLOCK_SIZE / 2.0f) / BLOCK_SIZE
			);

	// 外壁はすべての行に存在するため、
	// 判定用として固定行を参照する
	int mapZ = 1;

	// マップ範囲外を参照しないようにチェックする
	if (mapX >= 0 &&
		mapX < mapWidth &&
		mapZ >= 0 &&
		mapZ < mapHeight)
	{
		// 現在のマスが壁で、
		// プレイヤーが左右方向を向いている場合
		if (gmap[mapZ][mapX] == 1 &&
			(pdirection_ == PLAYER_LEFT ||
				pdirection_ == PLAYER_RIGHT))
		{
			// すでに壁の中へ移動した後なので、
			// このフレームで行った移動量を引いて元に戻す
			pos = pos - currentSpeed_ * move;

			XMStoreFloat3(
				&transform_.position_,
				pos
			);

			// 壁にぶつかったので水平速度を0にする
			currentSpeed_ = 0.0f;
		}
	}
}


// ------------------------------------------------------------
// 描画
//
// 状態に応じて待機モデルと歩行モデルを切り替える
// ------------------------------------------------------------
void Player::Draw()
{
	// 待機中
	if (pstate_ == PLAYER_IDLE)
	{
		Model::SetTransform(
			hIdleModel_,
			transform_
		);

		Model::Draw(hIdleModel_);
	}

	// 歩行中・方向転換中
	else if (pstate_ == PLAYER_WALK ||
		pstate_ == PLAYER_TURN)
	{
		Model::SetTransform(
			hWalkModel_,
			transform_
		);

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
//
// 現在は処理なし
// ------------------------------------------------------------
void Player::OnCollision(GameObject* pTarget)
{}
#pragma once
#include "KamataEngine.h"
#include "Math.h"

class Player;

/// @brief 壊せる箱クラス
class Box {
public:
	/// @brief 初期化
	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position);

	/// @brief 更新
	void Update();

	/// @brief 描画
	void Draw();

	/// 破壊処理（プレイヤーの移動方向を受け取り、その方向に吹っ飛ばす）
	void OnCollision(const KamataEngine::Vector3& blowDirection);

	/// 落下開始（上の支えが消えたとき呼ぶ）
	void StartFalling(float targetY);


	// --- ゲッター ---
	bool IsAlive() const { return alive_; }
	bool IsFalling() const { return falling_; }
	bool IsBlownAway() const { return isBlownAway_; }
	float GetCurrentY() const { return worldTransform_.translation_.y; }
	float GetFallTargetY() const { return fallTargetY_; }
	KamataEngine::Vector3 GetWorldPosition();
	AABB GetAABB();

	// --- 統計用（EnemyのclearCountと同じ仕組み） ---
	inline static int breakCount = 0;

private:
	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Vector3 position_;
	KamataEngine::Vector3 size_;
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;  

	bool alive_ = true; // 壊れていないか

	// --- 吹っ飛び処理 ---
	bool isBlownAway_ = false;                  // 吹っ飛び中フラグ
	KamataEngine::Vector3 blowVelocity_ = {};   // 吹っ飛び速度（水平＋垂直）
	float blowAngularVelocity_ = 0.0f;          // 吹っ飛び回転角速度（tumbling効果用）

	// --- 落下処理 ---
	bool falling_ = false;      // 落下中フラグ
	float fallVelocity_ = 0.0f; // 落下速度（フレームごとに加速）
	float fallTargetY_ = 0.0f;  // 落下先のY座標


	static inline const float kGravity = 9.8f;              // 重力加速度 (units/秒²)
	static inline const float kBlowSpeed = 10.0f;           // 吹っ飛び水平初速 (units/秒)
	static inline const float kBlowUpSpeed = 3.0f;          // 吹っ飛び上向き初速 (units/秒)
	static inline const float kBlowAngularVelocity = 10.0f; // 吹っ飛び回転角速度 (rad/秒)
	static inline const float kDisappearY = -10.0f;         // この高さを下回ったら消滅


};
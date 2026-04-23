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

	/// @brief 破壊処理
	void OnCollision();

	/// @brief 落下開始（上の支えが消えたとき呼ぶ）
	void StartFalling(float targetY);

	// --- ゲッター ---
	bool IsAlive() const { return alive_; }
	bool IsFalling() const { return falling_; }
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

	// --- 落下処理 ---
	bool falling_ = false;       // 落下中フラグ
	float fallVelocity_ = 0.0f;  // 落下速度（フレームごとに加速）
	float fallTargetY_ = 0.0f;   // 落下先のY座標

	static inline const float kGravity = 60.0f; // 重力加速度 (units/秒²) ─ 達磨落とし的な即落ち感
};
#include "Box.h"
#include "Math.h"

using namespace KamataEngine;

void Box::Initialize(Model* model, KamataEngine::Camera* camera, const Vector3& position) {
	assert(model);
	model_ = model;
	camera_ = camera;
	position_ = position;

	worldTransform_.Initialize();
	worldTransform_.translation_ = position;

	alive_ = true;
}

void Box::Update() {
	if (!alive_) {
		return;
	}

	const float dt = 1.0f / 60.0f;

	// 吹っ飛び中：水平速度 ＋ 重力 ＋ 回転
	if (isBlownAway_) {
		blowVelocity_.y -= kGravity * dt;
		worldTransform_.translation_.x += blowVelocity_.x * dt;
		worldTransform_.translation_.y += blowVelocity_.y * dt;
		worldTransform_.translation_.z += blowVelocity_.z * dt;
		worldTransform_.rotation_.z += blowAngularVelocity_ * dt;

		// 画面外（十分低い位置）まで落ちたら消滅
		if (worldTransform_.translation_.y < kDisappearY) {
			alive_ = false;
		}

	// 落下中なら重力を適用
	} else if (falling_) {
		fallVelocity_ += kGravity * dt;
		worldTransform_.translation_.y -= fallVelocity_;
		if (worldTransform_.translation_.y <= fallTargetY_) {
			worldTransform_.translation_.y = fallTargetY_;
			falling_ = false;
			fallVelocity_ = 0.0f;
		}
	}



	// 行列の更新 (Enemy.cpp と同じ計算式)
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);

	// 定数バッファに転送
	worldTransform_.TransferMatrix();
}

void Box::Draw() {
	if (alive_) {
		model_->Draw(worldTransform_, *camera_);
	}
}

void Box::OnCollision(const Vector3& blowDirection) {
	if (!alive_ || isBlownAway_) return;

	isBlownAway_ = true;
	breakCount++; // 壊した数を加算

	// 吹っ飛び初速：プレイヤーの移動方向（水平）＋ わずかな上方向
	blowVelocity_.x = blowDirection.x * kBlowSpeed;
	blowVelocity_.y = kBlowUpSpeed;
	blowVelocity_.z = blowDirection.z * kBlowSpeed;

	// 吹っ飛び回転角速度を設定
	blowAngularVelocity_ = kBlowAngularVelocity;
}

void Box::StartFalling(float targetY) {
	falling_ = true;
	fallVelocity_ = 0.0f;
	fallTargetY_ = targetY;
}




Vector3 Box::GetWorldPosition() {
	Vector3 worldPos;
	worldPos.x = worldTransform_.matWorld_.m[3][0];
	worldPos.y = worldTransform_.matWorld_.m[3][1];
	worldPos.z = worldTransform_.matWorld_.m[3][2];
	return worldPos;
}

AABB Box::GetAABB() {
	if (!alive_ || isBlownAway_) {
		return {
		    {0, 0, 0},
            {0, 0, 0}
        };
	}

	Vector3 worldPos = GetWorldPosition();
	const float kSize = 1.0f; // 箱のサイズ

	AABB aabb;
	aabb.min = {
	    worldPos.x - kSize / 2.0f,
	    worldPos.y - kSize / 2.0f,
	    worldPos.z - kSize / 2.0f,
	};
	aabb.max = {
	    worldPos.x + kSize / 2.0f,
	    worldPos.y + kSize / 2.0f,
	    worldPos.z + kSize / 2.0f,
	};
	return aabb;
}
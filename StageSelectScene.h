#pragma once
#include "KamataEngine.h"
#include "Fade.h"

using namespace KamataEngine;

/// @brief ステージ選択シーン
class StageSelectScene {
public:
	enum class Phase {
		kFadeIn,  // フェードイン
		kMain,    // メイン（選択中）
		kFadeOut, // フェードアウト
	};

	~StageSelectScene();

	void Initialize();

	void Update();

	void Draw();

	bool IsFinished() const { return finished_; }

	/// @brief 選択されたステージ番号を取得
	int GetSelectedStage() const { return selectedStage_; }

private:
	static inline const int kMaxStage = 2;
	static inline const float kStageSpacing = 8.0f;
	static inline const float kTimeBob = 2.0f;

	Camera camera_;

	Model* blockModel_ = nullptr;
	Model* playerModel_ = nullptr;

	// ステージごとのブロック表示用
	WorldTransform worldTransformStages_[2];
	// カーソル（プレイヤーモデル）
	WorldTransform worldTransformCursor_;

	int selectedStage_ = 1;

	float counter_ = 0.0f;
	bool finished_ = false;

	Fade* fade_ = nullptr;
	Phase phase_ = Phase::kFadeIn;
};

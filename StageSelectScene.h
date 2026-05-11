#pragma once
#include "KamataEngine.h"
#include "Fade.h"
#include <array>
#include <cstdint>

using namespace KamataEngine;

/**
 * @brief ステージセレクト画面
 *
 * ←/→ キーでステージを選択し、SPACE で確定する。
 * 確定後は IsFinished() が true になり、GetSelectedStage() で
 * 選択されたステージ番号（0始まり）を取得できる。
 */
class StageSelectScene {
public:
	static const uint32_t kNumStages = 3; // 選択可能なステージ数

	~StageSelectScene();

	/// @brief 初期化
	void Initialize();

	/// @brief 更新
	void Update();

	/// @brief 描画
	void Draw();

	/// @brief シーン終了フラグを取得
	bool IsFinished() const { return finished_; }

	/// @brief 選択されたステージ番号を取得（0始まり）
	uint32_t GetSelectedStage() const { return selectedStage_; }

private:
	enum class Phase {
		kFadeIn,  // フェードイン
		kMain,    // 選択操作中
		kFadeOut, // フェードアウト（確定後）
	};
	Phase phase_ = Phase::kFadeIn;

	// --- 定数 ---
	static inline const float kPanelW    = 220.0f; // パネル横幅
	static inline const float kPanelH    = 220.0f; // パネル縦幅
	static inline const float kGap       = 80.0f;  // パネル間の隙間
	static inline const float kCursorPad = 12.0f;  // カーソルのはみ出し量

	// --- ステージパネル（各ステージを表す色付き矩形）---
	std::array<Sprite*, kNumStages> stagePanels_{};

	// --- カーソル（選択中ステージの背景に描画する枠）---
	Sprite* cursorSprite_ = nullptr;

	// --- フェード ---
	Fade* fade_ = nullptr;

	uint32_t selectedStage_ = 0; // 現在の選択インデックス（0始まり）
	bool finished_ = false;

	/// @brief 各スプライトの色を選択状態に合わせて更新
	void UpdatePanelColors();
};

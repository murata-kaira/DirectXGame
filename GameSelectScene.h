#pragma once
#include "KamataEngine.h"
#include "Fade.h"

using namespace KamataEngine;

class GameSelectScene {
public:
	enum class Phase {
		kFadeIn,
		kMain,
		kFadeOut,
	};

	~GameSelectScene();

	void Initialize();
	void Update();
	void Draw();

	bool IsFinished() const { return finished_; }

private:
	void UpdateSelectionVisual();

	Camera camera_;
	WorldTransform worldTransformTitle_;
	WorldTransform worldTransformSelection_[2];

	Model* modelTitle_ = nullptr;
	Model* modelPlayer_ = nullptr;

	Fade* fade_ = nullptr;
	Phase phase_ = Phase::kFadeIn;

	bool finished_ = false;
	int selectedIndex_ = 0;
	float counter_ = 0.0f;
};

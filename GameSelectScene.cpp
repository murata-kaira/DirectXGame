#include <numbers>
#include "GameSelectScene.h"
#include "Math.h"

namespace {
constexpr float kSelectionAnimationCycle = 2.0f;
}

GameSelectScene::~GameSelectScene() {
	delete modelTitle_;
	delete modelPlayer_;
	delete fade_;
}

void GameSelectScene::Initialize() {
	modelTitle_ = Model::CreateFromOBJ("titleFont", true);
	modelPlayer_ = Model::CreateFromOBJ("player");

	camera_.Initialize();

	worldTransformTitle_.Initialize();
	worldTransformTitle_.scale_ = {1.6f, 1.6f, 1.6f};
	worldTransformTitle_.translation_ = {0.0f, 10.0f, 0.0f};

	for (int i = 0; i < kSelectionOptionCount; ++i) {
		worldTransformSelection_[i].Initialize();
		worldTransformSelection_[i].scale_ = {8.0f, 8.0f, 8.0f};
		worldTransformSelection_[i].rotation_.y = 0.95f * std::numbers::pi_v<float>;
		worldTransformSelection_[i].translation_.x = (i == 0) ? -4.0f : 4.0f;
		worldTransformSelection_[i].translation_.y = -10.0f;
	}

	fade_ = new Fade();
	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, 1.0f);
}

void GameSelectScene::UpdateSelectionVisual() {
	const float selectedScale = 10.0f;
	const float unselectedScale = 8.0f;
	for (int i = 0; i < kSelectionOptionCount; ++i) {
		const float scale = (i == selectedIndex_) ? selectedScale : unselectedScale;
		worldTransformSelection_[i].scale_ = {scale, scale, scale};
	}
}

void GameSelectScene::Update() {
	Input* input = Input::GetInstance();

	switch (phase_) {
	case Phase::kFadeIn:
		fade_->Update();
		if (fade_->IsFinished()) {
			phase_ = Phase::kMain;
		}
		break;
	case Phase::kMain:
		if (input->PushKey(DIK_LEFT) || input->PushKey(DIK_UP)) {
			selectedIndex_ = 0;
		}
		if (input->PushKey(DIK_RIGHT) || input->PushKey(DIK_DOWN)) {
			selectedIndex_ = 1;
		}
		if (input->PushKey(DIK_SPACE)) {
			fade_->Start(Fade::Status::FadeOut, 1.0f);
			phase_ = Phase::kFadeOut;
		}
		break;
	case Phase::kFadeOut:
		fade_->Update();
		if (fade_->IsFinished()) {
			finished_ = true;
		}
		break;
	}

	counter_ += 1.0f / 60.0f;
	counter_ = std::fmod(counter_, kSelectionAnimationCycle);

	const float angle = counter_ / kSelectionAnimationCycle * 2.0f * std::numbers::pi_v<float>;
	worldTransformTitle_.translation_.y = std::sin(angle) + 10.0f;

	UpdateSelectionVisual();
	camera_.TransferMatrix();
	WorldTransformUpdate(worldTransformTitle_);
	WorldTransformUpdate(worldTransformSelection_[0]);
	WorldTransformUpdate(worldTransformSelection_[1]);
}

void GameSelectScene::Draw() {
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();
	ID3D12GraphicsCommandList* commandList = dxCommon->GetCommandList();

	Model::PreDraw(commandList);
	modelTitle_->Draw(worldTransformTitle_, camera_);
	modelPlayer_->Draw(worldTransformSelection_[0], camera_);
	modelPlayer_->Draw(worldTransformSelection_[1], camera_);
	Model::PostDraw();

	fade_->Draw();
}

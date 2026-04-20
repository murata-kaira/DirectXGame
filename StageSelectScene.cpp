#include <numbers>
#include "StageSelectScene.h"
#include "Math.h"

StageSelectScene::~StageSelectScene() {
	delete blockModel_;
	delete playerModel_;
	delete fade_;
}

void StageSelectScene::Initialize() {
	blockModel_ = Model::CreateFromOBJ("block");
	playerModel_ = Model::CreateFromOBJ("player");

	// カメラ初期化
	camera_.Initialize();

	// ステージブロックの初期化（横に並べる）
	for (int i = 0; i < kMaxStage; i++) {
		worldTransformStages_[i].Initialize();
		worldTransformStages_[i].translation_ = {
		    -kStageSpacing / 2.0f + i * kStageSpacing,
		    2.0f,
		    20.0f};
		worldTransformStages_[i].scale_ = {3.0f, 3.0f, 3.0f};
	}

	// カーソル（プレイヤーモデル）の初期化
	worldTransformCursor_.Initialize();
	worldTransformCursor_.scale_ = {5.0f, 5.0f, 5.0f};
	worldTransformCursor_.rotation_.y = 0.95f * std::numbers::pi_v<float>;

	fade_ = new Fade();
	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, 1.0f);

	selectedStage_ = 1;
	finished_ = false;
	counter_ = 0.0f;
	phase_ = Phase::kFadeIn;
}

void StageSelectScene::Update() {
	switch (phase_) {
	case Phase::kFadeIn:
		fade_->Update();
		if (fade_->IsFinished()) {
			phase_ = Phase::kMain;
		}
		break;
	case Phase::kMain:
		// 左右キーでステージ選択
		if (Input::GetInstance()->TriggerKey(DIK_RIGHT)) {
			if (selectedStage_ < kMaxStage) {
				selectedStage_++;
			}
		}
		if (Input::GetInstance()->TriggerKey(DIK_LEFT)) {
			if (selectedStage_ > 1) {
				selectedStage_--;
			}
		}
		// スペースキーで決定
		if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
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

	// カーソルのアニメーション（上下に揺れる）
	counter_ += 1.0f / 60.0f;
	counter_ = std::fmod(counter_, kTimeBob);
	float angle = counter_ / kTimeBob * 2.0f * std::numbers::pi_v<float>;

	// カーソルを選択中のステージの下に配置
	float selectedX = -kStageSpacing / 2.0f + (selectedStage_ - 1) * kStageSpacing;
	worldTransformCursor_.translation_ = {selectedX, std::sin(angle) - 6.0f, 20.0f};

	// 選択中のステージを大きく表示
	for (int i = 0; i < kMaxStage; i++) {
		if (i + 1 == selectedStage_) {
			worldTransformStages_[i].scale_ = {4.0f, 4.0f, 4.0f};
		} else {
			worldTransformStages_[i].scale_ = {2.5f, 2.5f, 2.5f};
		}
	}

	camera_.TransferMatrix();
	for (int i = 0; i < kMaxStage; i++) {
		WorldTransformUpdate(worldTransformStages_[i]);
	}
	WorldTransformUpdate(worldTransformCursor_);
}

void StageSelectScene::Draw() {
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();
	ID3D12GraphicsCommandList* commandList = dxCommon->GetCommandList();

	Model::PreDraw(commandList);

	for (int i = 0; i < kMaxStage; i++) {
		blockModel_->Draw(worldTransformStages_[i], camera_);
	}
	playerModel_->Draw(worldTransformCursor_, camera_);

	Model::PostDraw();

	fade_->Draw();
}

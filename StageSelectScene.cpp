#include "StageSelectScene.h"

StageSelectScene::~StageSelectScene() {
	for (auto* panel : stagePanels_) {
		delete panel;
	}
	delete cursorSprite_;
	delete fade_;
}

void StageSelectScene::Initialize() {
	// パネルの開始座標を計算（画面中央に 3 枚を並べる）
	const float totalW = kNumStages * kPanelW + (kNumStages - 1) * kGap;
	const float startX = (WinApp::kWindowWidth  - totalW) / 2.0f;
	const float panelY = (WinApp::kWindowHeight - kPanelH) / 2.0f;

	// 各ステージパネルを生成（テクスチャ 0 = white1x1 をカラー塗り）
	for (uint32_t i = 0; i < kNumStages; ++i) {
		float x = startX + static_cast<float>(i) * (kPanelW + kGap);
		stagePanels_[i] = Sprite::Create(0, Vector2{x, panelY});
		stagePanels_[i]->SetSize(Vector2{kPanelW, kPanelH});
	}

	// カーソル（選択枠）：最初は index 0 の位置に配置
	float cursorX = startX - kCursorPad;
	float cursorY = panelY - kCursorPad;
	cursorSprite_ = Sprite::Create(0, Vector2{cursorX, cursorY});
	cursorSprite_->SetSize(Vector2{kPanelW + kCursorPad * 2.0f, kPanelH + kCursorPad * 2.0f});
	cursorSprite_->SetColor(Vector4{1.0f, 1.0f, 0.0f, 1.0f}); // 黄色の枠

	// 初期カラー設定
	selectedStage_ = 0;
	UpdatePanelColors();

	// フェードイン開始
	fade_ = new Fade();
	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, 1.0f);
}

void StageSelectScene::Update() {
	Input* input = Input::GetInstance();

	switch (phase_) {
	case Phase::kFadeIn:
		fade_->Update();
		if (fade_->IsFinished()) {
			phase_ = Phase::kMain;
		}
		break;

	case Phase::kMain: {
		// 左矢印 or A：前のステージへ
		if (input->TriggerKey(DIK_LEFT) || input->TriggerKey(DIK_A)) {
			if (selectedStage_ > 0) {
				--selectedStage_;
				UpdatePanelColors();
			}
		}
		// 右矢印 or D：次のステージへ
		if (input->TriggerKey(DIK_RIGHT) || input->TriggerKey(DIK_D)) {
			if (selectedStage_ < kNumStages - 1) {
				++selectedStage_;
				UpdatePanelColors();
			}
		}

		// SPACE or ENTER で確定
		if (input->TriggerKey(DIK_SPACE) || input->TriggerKey(DIK_RETURN)) {
			fade_->Start(Fade::Status::FadeOut, 1.0f);
			phase_ = Phase::kFadeOut;
		}
		break;
	}

	case Phase::kFadeOut:
		fade_->Update();
		if (fade_->IsFinished()) {
			finished_ = true;
		}
		break;
	}

	// カーソル位置を選択インデックスに追従させる
	const float totalW = kNumStages * kPanelW + (kNumStages - 1) * kGap;
	const float startX = (WinApp::kWindowWidth  - totalW) / 2.0f;
	const float panelY = (WinApp::kWindowHeight - kPanelH) / 2.0f;
	float cursorX = startX + static_cast<float>(selectedStage_) * (kPanelW + kGap) - kCursorPad;
	float cursorY = panelY - kCursorPad;
	cursorSprite_->SetPosition(Vector2{cursorX, cursorY});
}

void StageSelectScene::Draw() {
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();
	ID3D12GraphicsCommandList* commandList = dxCommon->GetCommandList();

	Sprite::PreDraw(commandList);

	// カーソル（枠）を先に描画してパネルの下に見せる
	cursorSprite_->Draw();

	// ステージパネル
	for (auto* panel : stagePanels_) {
		panel->Draw();
	}

	Sprite::PostDraw();

	// フェードを最前面に描画
	fade_->Draw();
}

void StageSelectScene::UpdatePanelColors() {
	// ステージごとの基本色（選択外）
	static const Vector4 kBaseColors[StageSelectScene::kNumStages] = {
	    {0.2f, 0.4f, 0.8f, 1.0f}, // ステージ 1：青
	    {0.2f, 0.7f, 0.3f, 1.0f}, // ステージ 2：緑
	    {0.8f, 0.2f, 0.2f, 1.0f}, // ステージ 3：赤
	};
	// 選択中は明るくする係数
	static const float kSelectedBrightness = 1.0f;
	static const float kUnselectedDim      = 0.5f;

	for (uint32_t i = 0; i < kNumStages; ++i) {
		float k = (i == selectedStage_) ? kSelectedBrightness : kUnselectedDim;
		Vector4 c = kBaseColors[i];
		stagePanels_[i]->SetColor(Vector4{c.x * k, c.y * k, c.z * k, 1.0f});
	}
}

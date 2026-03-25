#include "GameScene.h"
#include "Math.h"

using namespace KamataEngine;

/**
 * @brief デストラクタ
 */
GameScene::~GameScene() { 
	delete model_;
	delete player_;
	delete debugCamera_;
	delete skydome_;
	delete mapChipField_;
	delete cameraController_;
	delete fade_;

	for (BlockObject& block : worldTransformBlocks_) {
		delete block.transform;
	}
	worldTransformBlocks_.clear();
	
	delete deathParticles_;
	
	delete playerModel_;
	delete blockModel_;
	delete skydomeModel_;
	delete deathParticleModel_;
}

/**
 * @brief 初期化
 */
void GameScene::Initialize() {
	// --- 1. システム・カメラの初期化 ---
	camera_.Initialize();
	// カメラを斜め上からの俯瞰視点に設定
	camera_.translation_ = { 0.0f, 15.0f, -10.0f }; // 高く、手前に
	camera_.rotation_ = { 0.8f, 0.0f, 0.0f };      // 下を向く
	
	debugCamera_ = new DebugCamera(WinApp::kWindowWidth, WinApp::kWindowHeight);
	
	fade_ = new Fade();
	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, 1.0f);

	// --- 2. モデルデータのロード ---
	model_ = Model::Create();
	skydomeModel_ = Model::CreateFromOBJ("skydome", true);
	blockModel_ = Model::CreateFromOBJ("block");
	playerModel_ = Model::CreateFromOBJ("player");
	deathParticleModel_ = Model::CreateFromOBJ("deathParticle");

	// --- 3. マップの生成 ---
	mapChipField_ = new MapChipField();
	mapChipField_->LoadMapChipCsv("Resources/blocks.csv");
	GenerateBlocks();

	// --- 4. プレイヤーの生成と初期化 ---
	player_ = new Player();
	player_->SetMapChipField(mapChipField_);
	// マップ上の初期位置（9×11の通行可能エリア中央付近：全4方向に移動できる位置）
	Vector3 playerPosition = mapChipField_->GetMapChipPositionByIndex(5, 4);
	player_->Initialize(playerModel_, &camera_, playerPosition);

	// --- 5. 背景（スカイドーム）の初期化 ---
	skydome_ = new Skydome();
	skydome_->Initialize(skydomeModel_, &camera_);

	// --- 6. カメラコントローラーの初期化 ---
	cameraController_ = new CameraController(); 
	cameraController_->Initialize(&camera_);    
	cameraController_->SetTarget(player_);
	cameraController_->Reset();
	// カメラの移動可能範囲（XZ平面に合わせて調整）
	CameraController::Rect cameraArea = {0.0f, 100.0f, 0.0f, 20.0f};
	cameraController_->SetMovableArea(cameraArea);

	phase_ = Phase::kPlay;
}

/**
 * @brief フェーズの変更判定
 */
void GameScene::ChangePhase() {
	switch (phase_) {
	case Phase::kPlay:
		if (player_->IsDead()) {
			phase_ = Phase::kDeath;
			const Vector3& deathParticlesPosition = player_->GetWorldPosition();
			deathParticles_ = new DeathParticles;
			deathParticles_->Initialize(deathParticleModel_, &camera_, deathParticlesPosition);
		}
		break;
	case Phase::kDeath:
		if (deathParticles_ && deathParticles_->IsFinished()) {
			phase_ = Phase::kFadeOut;
			fade_->Start(Fade::Status::FadeOut, 1.0f);
		}
		break;
	case Phase::kFadeOut:
		if (fade_->IsFinished()) {
			finished_ = true;
		}
		break;
	}
}

/**
 * @brief マップデータに基づいてブロックを生成
 */
void GameScene::GenerateBlocks() {
	uint32_t numBlockVertical = mapChipField_->GetNumBlockVirtical();
	uint32_t numBlockHorizontal = mapChipField_->GetNumBlockHorizontal();

	for (uint32_t i = 0; i < numBlockVertical; ++i) {
		for (uint32_t j = 0; j < numBlockHorizontal; ++j) {
			MapChipType type = mapChipField_->GetMapChipTypeByIndex(j, i);
			if (type == MapChipType::kBlank) continue;

			Vector3 basePos = mapChipField_->GetMapChipPositionByIndex(j, i);
			MapChipField::IndexSet idx = {static_cast<int32_t>(j), static_cast<int32_t>(i)};

			if (type == MapChipType::kBlock) {
				// 足場ブロック：Y=0
				WorldTransform* wt = new WorldTransform();
				wt->Initialize();
				wt->translation_ = basePos;
				worldTransformBlocks_.push_back({idx, wt});
			} else if (type == MapChipType::kBlockAbove) {
				// 足場の上に乗るブロック：Y=1
				WorldTransform* wt = new WorldTransform();
				wt->Initialize();
				wt->translation_ = basePos;
				wt->translation_.y = 1.0f;
				worldTransformBlocks_.push_back({idx, wt});
			} else if (type == MapChipType::kBlockStack) {
				// 重なりブロック：Y=0・Y=1・Y=2 の3段をまとめて生成
				for (int layer = 0; layer <= 2; ++layer) {
					WorldTransform* wt = new WorldTransform();
					wt->Initialize();
					wt->translation_ = basePos;
					wt->translation_.y = static_cast<float>(layer);
					worldTransformBlocks_.push_back({idx, wt});
				}
			}
		}
	}
}

/**
 * @brief 更新
 */
void GameScene::Update() { 
	ChangePhase();

	skydome_->Update();
	cameraController_->Update();
	
	switch (phase_) {
	case Phase::kFadeIn:
		fade_->Update();
		if (fade_->IsFinished()) {
			phase_ = Phase::kPlay;
		}
		player_->Update();
		break;

	case Phase::kPlay:
		player_->Update();
		// プレイヤーがブロックを破壊した場合、対応するブロックオブジェクトを除去する
		if (player_->HasDestroyedBlock()) {
			RemoveBlocksAtIndex(player_->GetDestroyedBlockIndex());
			player_->ClearDestroyedBlock();
		}
		CheckAllCollisions();
		break;

	case Phase::kDeath:
		if (deathParticles_) {
			deathParticles_->Update();
		}
		break;

	case Phase::kFadeOut:
		fade_->Update();
		break;
	}

	if (isDebugCameraActive_) {
		debugCamera_->Update();
		camera_.matView = debugCamera_->GetCamera().matView;
		camera_.matProjection = debugCamera_->GetCamera().matProjection;
		camera_.TransferMatrix();
	} else {
		camera_.UpdateMatrix();
	}

	for (BlockObject& block : worldTransformBlocks_) {
		if (block.transform) WorldTransformUpdate(*block.transform);
	}
}

/**
 * @brief 描画
 */
void GameScene::Draw() { 
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	Model::PreDraw(dxCommon->GetCommandList());

	skydome_->Draw(); 

	if (!player_->IsDead()) {
		player_->Draw(); 
	}

	for (BlockObject& block : worldTransformBlocks_) {
		if (block.transform) blockModel_->Draw(*block.transform, camera_);
	}

	if (player_->IsDead() && deathParticles_) {
		deathParticles_->Draw();
	}

	Model::PostDraw();

	Sprite::PreDraw(dxCommon->GetCommandList());
	Sprite::PostDraw();
}

/**
 * @brief 当たり判定のチェック
 */
void GameScene::CheckAllCollisions() {
	// エネミー削除に伴い、現在はプレイヤーとマップの判定のみ（Playerクラス内で処理済み）
	// 将来的にアイテム等の判定が必要になればここに追加する
}

/**
 * @brief 指定グリッドインデックスのブロックオブジェクトをすべて削除
 */
void GameScene::RemoveBlocksAtIndex(const MapChipField::IndexSet& index) {
	auto it = worldTransformBlocks_.begin();
	while (it != worldTransformBlocks_.end()) {
		if (it->index.xIndex == index.xIndex && it->index.yIndex == index.yIndex) {
			delete it->transform;
			it = worldTransformBlocks_.erase(it);
		} else {
			++it;
		}
	}
}

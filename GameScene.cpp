#include "GameScene.h"
#include "Math.h"
#include <cstdio>

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

	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			delete worldTransformBlock;
		}
	}
	worldTransformBlocks_.clear();
	
	for (Box* box : boxes_) {
		delete box;
	}
	boxes_.clear();

	delete deathParticles_;
	
	delete playerModel_;
	delete blockModel_;
	delete skydomeModel_;
	delete deathParticleModel_;
}

/**
 * @brief 初期化
 */
void GameScene::Initialize(int stageNumber) {
	stageNumber_ = stageNumber;
	// Box破壊カウントをリセット
	Box::breakCount = 0;
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

	// --- 3. マップの生成（ステージ番号に応じたCSVを読み込み） ---
	mapChipField_ = new MapChipField();
	std::string mapFilePath;
	if (stageNumber_ == 1) {
		mapFilePath = "Resources/blocks.csv";
	} else {
		char buf[64];
		snprintf(buf, sizeof(buf), "Resources/stage%02d.csv", stageNumber_);
		mapFilePath = buf;
	}
	mapChipField_->LoadMapChipCsv(mapFilePath);
	GenerateBlocks();

	// --- 4. プレイヤーの生成と初期化 ---
	player_ = new Player();
	player_->SetMapChipField(mapChipField_);
	// マップ上の初期位置（インデックス 0, 0）
	Vector3 playerPosition = mapChipField_->GetMapChipPositionByIndex(0, 0);
	player_->Initialize(playerModel_, &camera_, playerPosition);

	// ステージごとのbox配置
	std::vector<KamataEngine::Vector2> boxPositions;
	if (stageNumber_ == 1) {
		boxPositions = {
		    {3,  0},
		    {4,  0},
		    {5,  0},

			{1,1},
		    {2,2},
			{3,3},
		    {4,4},

			{1,0},
		    {0,1},
		};
	} else if (stageNumber_ == 2) {
		boxPositions = {
		    {2, 1},
		    {4, 1},
		    {6, 1},
		    {8, 1},

		    {1, 3},
		    {3, 3},
		    {5, 3},
		    {7, 3},
		    {9, 3},

		    {2, 5},
		    {4, 5},
		    {6, 5},
		    {8, 5},
		    {10, 5},

		    {3, 7},
		    {5, 7},
		    {7, 7},
		};
	}

	for (const auto& tilePos : boxPositions) {
		Box* newBox = new Box();
		Vector3 boxPosition = mapChipField_->GetMapChipPositionByIndex(static_cast<uint32_t>(tilePos.x), static_cast<uint32_t>(tilePos.y));
		boxPosition.y = 1.0f; //とりあえずこれで座標を一個上にしている
		Vector3 boxSize = {1.0f, 1.0f, 1.0f};
		newBox->Initialize(blockModel_, &camera_, boxPosition);

		boxes_.push_back(newBox);
	}


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
		// 死亡判定
		if (player_->IsDead()) {
			phase_ = Phase::kDeath;
			const Vector3& deathParticlesPosition = player_->GetWorldPosition();
			deathParticles_ = new DeathParticles;
			deathParticles_->Initialize(deathParticleModel_, &camera_, deathParticlesPosition);
		}
		// クリア判定（すべてのBoxが壊されたらクリア）
		else {
			bool allDestroyed = true;
			for (Box* box : boxes_) {
				if (box->IsAlive()) {
					allDestroyed = false;
					break;
				}
			}
			if (allDestroyed) {
				phase_ = Phase::kClear;
				clearTimer_ = 0.0f;
			}
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
	case Phase::kClear:
		// クリア演出の待機時間
		clearTimer_ += 1.0f / 60.0f;
		if (clearTimer_ >= kClearWaitTime) {
			phase_ = Phase::kClearFadeOut;
			fade_->Start(Fade::Status::FadeOut, 1.0f);
		}
		break;
	case Phase::kClearFadeOut:
		if (fade_->IsFinished()) {
			isCleared_ = true;
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

	worldTransformBlocks_.resize(numBlockVertical);
	for (uint32_t i = 0; i < numBlockVertical; ++i) {
		worldTransformBlocks_[i].resize(numBlockHorizontal, nullptr);
	}

	for (uint32_t i = 0; i < numBlockVertical; ++i) {
		for (uint32_t j = 0; j < numBlockHorizontal; ++j) {
			if (mapChipField_->GetMapChipTypeByIndex(j, i) == MapChipType::kBlock) {
				WorldTransform* worldTransform = new WorldTransform();
				worldTransform->Initialize();
				worldTransform->translation_ = mapChipField_->GetMapChipPositionByIndex(j, i);
				worldTransformBlocks_[i][j] = worldTransform;
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
		CheckAllCollisions();

		for (Box* box : boxes_) {
			box->Update();
		}
		break;

	case Phase::kDeath:
		if (deathParticles_) {
			deathParticles_->Update();
		}
		break;

	case Phase::kFadeOut:
		fade_->Update();
		break;

	case Phase::kClear:
		// クリア演出中もプレイヤーは表示し続ける
		player_->Update();
		break;

	case Phase::kClearFadeOut:
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

	for (auto& line : worldTransformBlocks_) {
		for (auto& block : line) {
			if (block) WorldTransformUpdate(*block);
		}
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

	for (auto& line : worldTransformBlocks_) {
		for (auto& block : line) {
			if (block) blockModel_->Draw(*block, camera_);
		}
	}

	if (player_->IsDead() && deathParticles_) {
		deathParticles_->Draw();
	}

	for (Box* box : boxes_) {
		box->Draw();
	}

	Model::PostDraw();

	Sprite::PreDraw(dxCommon->GetCommandList());
	fade_->Draw();
	Sprite::PostDraw();
}

/**
 * @brief 当たり判定のチェック
 */
void GameScene::CheckAllCollisions() {
	// エネミー削除に伴い、現在はプレイヤーとマップの判定のみ（Playerクラス内で処理済み）
	// 将来的にアイテム等の判定が必要になればここに追加する

	AABB playerAABB = player_->GetAABB();

	for (Box* box : boxes_) {
		// すでに壊れている場合はスキップ
		if (!box->IsAlive()) {
			continue;
		}

		AABB boxAABB = box->GetAABB();

		if (IsCollision(playerAABB, boxAABB)) {
				box->OnCollision();
			
		}
	}
}

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

	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			delete worldTransformBlock;
		}     
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
	// マップ上の初期位置（インデックス 0, 0）
	Vector3 playerPosition = mapChipField_->GetMapChipPositionByIndex(0, 0);
	player_->Initialize(playerModel_, &camera_, playerPosition);

struct TilePlacement {
		uint32_t xIndex;
		uint32_t yIndex;
		uint32_t level; // 1始まり
	};

	// とりあえずの box 配置（xIndex:横インデックス, yIndex:縦インデックス, level:段数）
	std::vector<TilePlacement> tilePlacements = {
	    {3, 0, 1}, // 1段目
	    {4, 0, 1},
	    {5, 0, 1},
	    {1, 1, 1},
	    {2, 2, 1},
	    {3, 3, 1},
	    {4, 4, 1},
	    {0, 1, 1},
	    // 6段積み
	    {1, 1, 1},
	    {1, 1, 2},
	    {1, 1, 3},
	    {1, 1, 4},
	    {1, 1, 5},
	    {1, 1, 6},
	};

	for (const auto& tilePos : tilePlacements) {
		Box* newBox = new Box();
		Vector3 boxPosition = mapChipField_->GetMapChipPositionByIndex(tilePos.xIndex, tilePos.yIndex);
		boxPosition.y = kBoxBaseY + (static_cast<float>(tilePos.level) - 1.0f) * kBoxHeight;
		newBox->Initialize(blockModel_, &camera_, boxPosition);

	boxes_.push_back({newBox, tilePos.xIndex, tilePos.yIndex, tilePos.level});
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
		UpdateBoxFalls();

		for (auto& entry : boxes_) {
			entry.box->Update();
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

void GameScene::UpdateBoxFalls() {
	// 支持関係が連鎖して変わるため、落下開始が発生しなくなるまで評価する
	bool startedFall = false;
	uint32_t guard = 0;
	const uint32_t maxIterations = static_cast<uint32_t>(boxes_.size()) * 8 + 1;
	do {
		startedFall = false;
		for (auto& entry : boxes_) {
			if (!entry.box->IsAlive() || entry.box->IsFalling() || entry.level <= 1) {
				continue;
			}

			const uint32_t supportLevel = entry.level - 1;
			bool hasSupport = false;
			for (const auto& supportCandidate : boxes_) {
				if (!supportCandidate.box->IsAlive() || supportCandidate.box->IsFalling()) {
					continue;
				}
				if (supportCandidate.xIndex == entry.xIndex && supportCandidate.yIndex == entry.yIndex &&
				    supportCandidate.level == supportLevel) {
					hasSupport = true;
					break;
				}
			}

			if (!hasSupport) {
				const float targetY = kBoxBaseY + (static_cast<float>(supportLevel) - 1.0f) * kBoxHeight;
				entry.box->StartFalling(targetY);
				entry.level = supportLevel;
				startedFall = true;
			}
		}
		++guard;
	} while (startedFall && guard < maxIterations);
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
	for (auto& entry : boxes_) {
		entry.box->Draw();
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

	AABB playerAABB = player_->GetAABB();

	for (auto& entry : boxes_) {
		// すでに壊れている、または落下中の場合はスキップ
		if (!entry.box->IsAlive() || entry.box->IsFalling()) {
			continue;
		}

			AABB boxAABB = entry.box->GetAABB();


		if (IsCollision(playerAABB, boxAABB)) {
			    entry.box->OnCollision();
			  
		// 1フレームで1個だけ壊す
			    break;
		}
	}
}

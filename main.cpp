#include "KamataEngine.h"
#include <Windows.h>
#include "GameScene.h"
#include "TitleScene.h" 

using namespace KamataEngine;

// --- グローバル変数 ---
TitleScene* titleScene = nullptr; // タイトルシーンのインスタンス
GameScene* gameScene = nullptr;   // ゲームシーンのインスタンス

// シーンの種類を定義
enum class Scene {
	kUnknown = 0, // 未定義
	kTitle,       // タイトル
	kGame,        // ゲーム本編
};

Scene scene = Scene::kUnknown; // 現在のシーン

static const uint32_t kNumStages = 3; // ステージ総数
static uint32_t currentStage = 0;     // 現在のステージ番号（0始まり）

/**
 * @brief シーンの切り替え判定と実行
 * 各シーンの終了フラグをチェックし、次のシーンへ遷移させる。
 */
void ChangeScene() {
	switch (scene) {
	case Scene::kTitle:
		// タイトルシーンが終了していたら、ゲームシーンへ
		if (titleScene->IsFinished()) {
			scene = Scene::kGame;
			
			// タイトルのメモリを解放
			delete titleScene;
			titleScene = nullptr;

			// ゲームシーンの作成と初期化（ステージ番号を渡す）
			gameScene = new GameScene;
			gameScene->Initialize(currentStage);
		}
		break;

	case Scene::kGame:
		// ゲームシーンが終了していたら、次のステージへ or タイトルへ
		if (gameScene->IsFinished()) {
			if (gameScene->IsCleared()) {
				// ステージクリア：次のステージへ進む
				++currentStage;
				if (currentStage < kNumStages) {
					// 次のステージを読み込む（シーンはそのまま kGame）
					delete gameScene;
					gameScene = new GameScene;
					gameScene->Initialize(currentStage);
				} else {
					// 全ステージクリア：タイトルへ戻る
					currentStage = 0;
					scene = Scene::kTitle;
					delete gameScene;
					gameScene = nullptr;
					titleScene = new TitleScene;
					titleScene->Initialize();
				}
			} else {
				// 死亡：ステージをリセットしてタイトルへ戻る
				currentStage = 0;
				scene = Scene::kTitle;
				delete gameScene;
				gameScene = nullptr;
				titleScene = new TitleScene;
				titleScene->Initialize();
			}
		}
		break;
	}
}

/**
 * @brief 現在のシーンの更新処理
 */
void UpdateScene() {
	switch (scene) {
	case Scene::kTitle:
		if (titleScene) titleScene->Update();
		break;
	case Scene::kGame:
		if (gameScene) gameScene->Update();
		break;
	}
}

/**
 * @brief 現在のシーンの描画処理
 */
void DrawScene() {
	switch (scene) {
	case Scene::kTitle:
		if (titleScene) titleScene->Draw();
		break;
	case Scene::kGame:
		if (gameScene) gameScene->Draw();
		break;
	}
}

// Windowsアプリでのエントリーポイント(プログラムがここから始まる)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	
	// --- エンジンの初期化 ---
	KamataEngine::Initialize(L"DirectX Game Application");
	
	// DirectXの描画管理クラスを取得
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();
	
	// 最初のシーンをタイトルに設定
	currentStage = 0;
	scene = Scene::kTitle;
	titleScene = new TitleScene;
	titleScene->Initialize();

	// --- メインループ ---
	while (true) {
		// エンジンの更新処理（ウィンドウの終了などを監視）
		if (KamataEngine::Update()) {
			break;
		}

		// 1. シーンの切り替えチェック
		ChangeScene();

		// 2. シーン内のデータ更新
		UpdateScene();

		// --- 描画開始 ---
		dxCommon->PreDraw();

		// 3. シーンの描画
		DrawScene();

		// デバッグ用の軸表示などを描画
		AxisIndicator::GetInstance()->Draw();
		PrimitiveDrawer::GetInstance()->Reset();

		// --- 描画終了 ---
		dxCommon->PostDraw();
	}

	// --- 終了処理 ---
	// メモリの解放を忘れずに行う
	delete titleScene;
	delete gameScene;
	
	// エンジンの終了処理
	KamataEngine::Finalize();

	return 0;
}

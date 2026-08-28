#include "Ground.h"
#include "Engine/Model.h"
#include "Engine/CsvReader.h"
#include "Food.h"


namespace
{
	//int mapData_[10][10] = {};
	using std::vector;
	//int model_t = -1;
	//vector< vector<int>> mapData =
	//{
	//{1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, // 0行目 (外壁)
	//{1, 0, 0, 0, 1, 0, 0, 0, 0, 1}, // 1行目 (左上がスタート)
	//{1, 0, 1, 0, 1, 0, 1, 1, 0, 1}, // 2行目
	//{1, 0, 1, 0, 0, 0, 1, 0, 0, 1}, // 3行目
	//{1, 1, 1, 1, 1, 0, 1, 0, 1, 1}, // 4行目
	//{1, 0, 0, 0, 1, 0, 0, 0, 1, 1}, // 5行目
	//{1, 0, 1, 0, 1, 1, 1, 0, 0, 1}, // 6行目
	//{1, 0, 1, 0, 0, 0, 1, 1, 0, 1}, // 7行目
	//{1, 1, 1, 1, 1, 0, 0, 0, 0, 1}, // 8行目 (右下がゴール)
	//{1, 1, 1, 1, 1, 1, 1, 1, 1, 1}  // 9行目 (外壁)
	////		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, // 0行目 (外壁)
	////{1, 0, 0, 0, 0, 0, 0, 0, 0, 1}, // 1行目 (左上がスタート)
	////{1, 0, 0, 0, 0, 0, 0, 0, 0, 1}, // 2行目
	////{1, 0, 0, 0, 0, 0, 0, 0, 0, 1}, // 3行目
	////{1, 0, 0, 0, 0, 0, 0, 0, 0, 1}, // 4行目
	////{1, 0, 0, 0, 0, 0, 0, 0, 0, 1}, // 5行目
	////{1, 0, 0, 0, 0, 0, 0, 0, 0, 1}, // 6行目
	////{1, 0, 0, 0, 0, 0, 0, 0, 0, 1}, // 7行目
	////{1, 0, 0, 0, 0, 0, 0, 0, 0, 1}, // 8行目 (右下がゴール)
	////{1, 1, 1, 1, 1, 1, 1, 1, 1, 1}  // 9行目 (外壁)
	//};
}

Ground::Ground(GameObject* parent)
	:GameObject(parent, "Ground"), hModel_(-1), mapWidth_(-1), mapHeight_(-1)
{
	CsvReader csvData;
	csvData.Load("map.csv"); //CSVファイルを読み込む
	mapWidth_ = csvData.GetWidth(); //列数を取得
	mapHeight_ = csvData.GetHeight()/2; //行数を取得
	// mapData_を初期化 mapHeight_個のvector<int>の配列を作る
	mapData_ = vector<vector<int>>(mapHeight_, vector<int>(mapWidth_, 0));
	objMap_ = vector<vector<int>>(mapHeight_, vector<int>(mapWidth_, 0));
	for (int x = 0; x < mapWidth_; x++)
	{
		for(int y= 0; y < mapHeight_; y++)
		{
			mapData_[y][x] = csvData.GetValue(x, y); //CSVの値をmapData_に格納
		}
	}
	for (int x = 0; x < mapWidth_; x++)
	{
		for (int y = 0; y < mapHeight_; y++)
		{
			objMap_[y][x] = csvData.GetValue(x, y+mapHeight_); //CSVの値をobjMap_に格納
			if(objMap_[y][x] > 0){
				Food* food = (Food*)Instantiate<Food>(this);
				esaCount_++;//餌の数をカウント
				food->SetPosition({ -9.0f + x * 2.0f,  9.0f - y * 2.0f, -1.0f });
				if (objMap_[y][x] == 1)
				{
					food->SetFoodType(FoodType::FOODTYPE_NORMAL);
					normalEsaCount_++;//通常餌の数をカウント
				}
				else if (objMap_[y][x] == 2)
				{
					food->SetFoodType(FoodType::FOODTYPE_POWER);
					powerEsaCount_++;//パワー餌の数をカウント
				}
			}
		}
	}


}

void Ground::Initialize()
{
	hModel_ = Model::Load("jimen3.fbx");
	hModelt_ = Model::Load("BrickG.fbx");
	//hEsaModel_ = Model::Load("esa.fbx");
	//hPEsaModel_ = Model::Load("Poweresa.fbx");
}

void Ground::Update()
{
}

void Ground::Draw()
{
	const int GROUND_WIDTH = 20.0f;
	for(int i = 0;i < 3; i++) {
		transform_.position_ = { 9.0f + GROUND_WIDTH*i, 10.0f, 1.0f };
		transform_.rotate_ = { -90.0f, 0.0f, 0.0f };
		Model::SetTransform(hModel_, transform_);
		Model::Draw(hModel_);
	}

	Model::SetTransform(hModel_, transform_);
	for (int j = 0;j < mapHeight_;j++) {
		for (int i = 0;i < mapWidth_;i++) {
			if (mapData_[j][i] == 1) {
				Transform tr;
				tr.position_ = {  i * 2.0f,  j * 1.0f, 0.0f };
				Model::SetTransform(hModelt_, tr);
				Model::Draw(hModelt_);
			}
			//if (objMap_[j][i] == 1) {
			//	Transform tr2;
			//	tr2.position_ = { -9.0f + i * 2.0f, 0.0f, 9.0f - j * 2.0f };
			//	tr2.scale_ = { 0.3f, 0.3f, 0.3f };
			//	Model::SetTransform(hEsaModel_, tr2);
			//	Model::Draw(hEsaModel_);
			//}
			//else if (objMap_[j][i] == 2)
			//{
			//	static Transform tr2;
			//	tr2.position_ = { -9.0f + i * 2.0f, 0.0f, 9.0f - j * 2.0f };
			//	tr2.scale_ = { 0.3f, 0.3f, 0.3f };
			//	tr2.rotate_.y += 1.0f;
			//	Model::SetTransform(hPEsaModel_, tr2);
			//	Model::Draw(hPEsaModel_);
			//}
		}
	}
}

void Ground::Release()
{
}

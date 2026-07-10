#include "Ground.h"
#include "Engine/Model.h"


namespace
{
	using std::vector;
	int model_t = -1;
	vector< vector<int>> mapData =
	{
	{1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, // 0行目 (外壁)
	{1, 0, 0, 0, 1, 0, 0, 0, 0, 1}, // 1行目 (左上がスタート)
	{1, 0, 1, 0, 1, 0, 1, 1, 0, 1}, // 2行目
	{1, 0, 1, 0, 0, 0, 1, 0, 0, 1}, // 3行目
	{1, 1, 1, 1, 1, 0, 1, 0, 1, 1}, // 4行目
	{1, 0, 0, 0, 1, 0, 0, 0, 1, 1}, // 5行目
	{1, 0, 1, 0, 1, 1, 1, 0, 0, 1}, // 6行目
	{1, 0, 1, 0, 0, 0, 1, 1, 0, 1}, // 7行目
	{1, 1, 1, 1, 1, 0, 0, 0, 0, 1}, // 8行目 (右下がゴール)
	{1, 1, 1, 1, 1, 1, 1, 1, 1, 1}  // 9行目 (外壁)
	//		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, // 0行目 (外壁)
	//{1, 0, 0, 0, 0, 0, 0, 0, 0, 1}, // 1行目 (左上がスタート)
	//{1, 0, 0, 0, 0, 0, 0, 0, 0, 1}, // 2行目
	//{1, 0, 0, 0, 0, 0, 0, 0, 0, 1}, // 3行目
	//{1, 0, 0, 0, 0, 0, 0, 0, 0, 1}, // 4行目
	//{1, 0, 0, 0, 0, 0, 0, 0, 0, 1}, // 5行目
	//{1, 0, 0, 0, 0, 0, 0, 0, 0, 1}, // 6行目
	//{1, 0, 0, 0, 0, 0, 0, 0, 0, 1}, // 7行目
	//{1, 0, 0, 0, 0, 0, 0, 0, 0, 1}, // 8行目 (右下がゴール)
	//{1, 1, 1, 1, 1, 1, 1, 1, 1, 1}  // 9行目 (外壁)
	};
}

Ground::Ground(GameObject* parent)
	:GameObject(parent, "Ground"), hModel_(-1)
{
	mapData_ = mapData; //ファイルグローバルのmapDataをコピーして、メンバ変数mapData_に格納
}

void Ground::Initialize()
{
	hModel_ = Model::Load("jimen3.fbx");
	model_t = Model::Load("BrickV.fbx");

}

void Ground::Update()
{
}

void Ground::Draw()
{
	Model::SetTransform(hModel_, transform_);
	Model::Draw(hModel_);
	for (int j = 0;j < 10;j++) {
		for (int i = 0;i < 10;i++) {
			if (mapData_[j][i] == 1) {
				Transform tr;
				tr.position_ = { -9.0f + i * 2.0f, 0.0f, 9.0f - j * 2.0f };
				Model::SetTransform(model_t, tr);
				Model::Draw(model_t);
			}
		}
	}
}

void Ground::Release()
{
}

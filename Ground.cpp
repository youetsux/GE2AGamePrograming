#include "Ground.h"
#include "Engine/Model.h"

Ground::Ground(GameObject* parent)
	:GameObject(parent, "Ground"), hModel_(-1)
{
}

void Ground::Initialize()
{
	hModel_ = Model::Load("jimen.fbx");
}

void Ground::Update()
{
}

void Ground::Draw()
{
	Model::SetTransform(hModel_, transform_);
	Model::Draw(hModel_);
}

void Ground::Release()
{
}

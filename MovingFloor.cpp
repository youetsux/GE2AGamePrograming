#include "MovingFloor.h"
#include "Engine/Model.h"
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace
{
	const char* STAGE_OBJECT_CSV = "stage_objects.csv";
	const char* DEFAULT_MODEL = "BrickG.fbx";
	const float MIN_SPEED = 0.0001f;
	const float ARRIVE_EPSILON = 0.0001f;

	enum CsvColumn
	{
		COL_TYPE = 0,
		COL_MODEL,
		COL_START_X,
		COL_START_Y,
		COL_START_Z,
		COL_END_X,
		COL_END_Y,
		COL_END_Z,
		COL_SPEED,
		COL_WAIT,
		COL_LOOP,
		COL_MAX
	};

	std::string NormalizeCsvToken(const std::string& raw)
	{
		std::string token(raw.c_str());
		token.erase(std::remove(token.begin(), token.end(), '\r'), token.end());
		token.erase(std::remove(token.begin(), token.end(), '\n'), token.end());
		token.erase(std::remove(token.begin(), token.end(), '\0'), token.end());

		if (token.size() >= 3 &&
			static_cast<unsigned char>(token[0]) == 0xEF &&
			static_cast<unsigned char>(token[1]) == 0xBB &&
			static_cast<unsigned char>(token[2]) == 0xBF)
		{
			token = token.substr(3);
		}

		auto notSpace = [](unsigned char c) { return !std::isspace(c); };
		token.erase(token.begin(), std::find_if(token.begin(), token.end(), notSpace));
		token.erase(std::find_if(token.rbegin(), token.rend(), notSpace).base(), token.end());

		if (token.size() >= 2 && token.front() == '"' && token.back() == '"')
		{
			token = token.substr(1, token.size() - 2);
		}

		return token;
	}

	bool TryOpenCsv(const std::string& fileName, std::ifstream& ifs)
	{
		const std::string candidates[] =
		{
			fileName,
			"./" + fileName,
			"../" + fileName,
			"../../" + fileName,
			"../../../" + fileName
		};

		for (const auto& path : candidates)
		{
			ifs.open(path);
			if (ifs.is_open())
			{
				return true;
			}
			ifs.clear();
		}

		return false;
	}

	std::vector<std::string> SplitCsvLine(const std::string& line)
	{
		std::vector<std::string> tokens;
		std::stringstream ss(line);
		std::string token;

		while (std::getline(ss, token, ','))
		{
			tokens.push_back(NormalizeCsvToken(token));
		}

		return tokens;
	}

	float ReadFloat(const std::vector<std::string>& tokens, int col, float fallback)
	{
		if (col < 0 || col >= static_cast<int>(tokens.size()))
		{
			return fallback;
		}

		const std::string token = tokens[col];
		if (token.empty())
		{
			return fallback;
		}
		return static_cast<float>(std::atof(token.c_str()));
	}

	int ReadInt(const std::vector<std::string>& tokens, int col, int fallback)
	{
		if (col < 0 || col >= static_cast<int>(tokens.size()))
		{
			return fallback;
		}

		const std::string token = tokens[col];
		if (token.empty())
		{
			return fallback;
		}
		return std::atoi(token.c_str());
	}
}

MovingFloor::MovingFloor(GameObject* parent)
	: GameObject(parent, "MovingFloor")
{
	LoadFromCsv(STAGE_OBJECT_CSV);
}

void MovingFloor::Initialize()
{
	for (auto& floor : floors_)
	{
		floor.modelHandle = Model::Load(floor.param.modelName);
	}
}

void MovingFloor::Update()
{
	for (auto& floor : floors_)
	{
		UpdateFloor(floor);
	}
}

void MovingFloor::Draw()
{
	for (const auto& floor : floors_)
	{
		if (floor.modelHandle < 0)
		{
			continue;
		}

		Transform tr;
		tr.position_ = floor.currentPosition;
		Model::SetTransform(floor.modelHandle, tr);
		Model::Draw(floor.modelHandle);
	}
}

void MovingFloor::Release()
{
}

void MovingFloor::LoadFromCsv(const std::string& fileName)
{
	floors_.clear();

	std::ifstream ifs(fileName);
	if (!ifs.is_open())
	{
		ifs.clear();
		if (!TryOpenCsv(fileName, ifs))
		{
			return;
		}
	}

	if (!ifs.is_open())
	{
		return;
	}

	std::string line;
	while (std::getline(ifs, line))
	{
		const std::vector<std::string> tokens = SplitCsvLine(line);
		if (tokens.empty())
		{
			continue;
		}

		const std::string type = NormalizeCsvToken(tokens[COL_TYPE]);
		if (type.empty() || type == "type" || type == "TYPE" || type[0] == '#')
		{
			continue;
		}
		if (type != "moving_floor")
		{
			continue;
		}

		FloorParam param;
		param.modelName = (COL_MODEL < static_cast<int>(tokens.size())) ? NormalizeCsvToken(tokens[COL_MODEL]) : "";
		if (param.modelName.empty())
		{
			param.modelName = DEFAULT_MODEL;
		}
		param.startPosition = XMFLOAT3(
			ReadFloat(tokens, COL_START_X, 0.0f),
			ReadFloat(tokens, COL_START_Y, 0.75f),
			ReadFloat(tokens, COL_START_Z, 0.0f));
		param.endPosition = XMFLOAT3(
			ReadFloat(tokens, COL_END_X, param.startPosition.x),
			ReadFloat(tokens, COL_END_Y, param.startPosition.y),
			ReadFloat(tokens, COL_END_Z, param.startPosition.z));
		param.moveSpeed = ReadFloat(tokens, COL_SPEED, 0.05f);
		if (param.moveSpeed < MIN_SPEED)
		{
			param.moveSpeed = MIN_SPEED;
		}
		param.waitFrame = ReadInt(tokens, COL_WAIT, 0);
		if (param.waitFrame < 0)
		{
			param.waitFrame = 0;
		}
		param.loop = ReadInt(tokens, COL_LOOP, 1) != 0;

		FloorState state;
		state.param = param;
		state.currentPosition = param.startPosition;
		state.modelHandle = -1;
		state.direction = 1.0f;
		state.waitTimer = 0;
		state.reachedEnd = false;
		floors_.push_back(state);
	}
}

void MovingFloor::UpdateFloor(FloorState& floor)
{
	if (!floor.param.loop && floor.reachedEnd)
	{
		return;
	}

	if (floor.waitTimer > 0)
	{
		--floor.waitTimer;
		return;
	}

	const XMFLOAT3 target = (floor.direction > 0.0f) ? floor.param.endPosition : floor.param.startPosition;
	const float dx = target.x - floor.currentPosition.x;
	const float dy = target.y - floor.currentPosition.y;
	const float dz = target.z - floor.currentPosition.z;
	const float distance = std::sqrt(dx * dx + dy * dy + dz * dz);

	if (distance <= floor.param.moveSpeed + ARRIVE_EPSILON)
	{
		floor.currentPosition = target;
		floor.waitTimer = floor.param.waitFrame;

		if (floor.direction > 0.0f)
		{
			floor.reachedEnd = true;
		}

		if (floor.param.loop)
		{
			floor.direction *= -1.0f;
		}
		return;
	}

	const float invDist = 1.0f / distance;
	floor.currentPosition.x += dx * invDist * floor.param.moveSpeed;
	floor.currentPosition.y += dy * invDist * floor.param.moveSpeed;
	floor.currentPosition.z += dz * invDist * floor.param.moveSpeed;
}

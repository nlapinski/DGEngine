#include "LevelMap.h"
#include "PathFinder.h"
#include "Utils/EasingFunctions.h"

uint32_t LevelMap::maxLights{ MaxNumberOfLightsToUse };

LevelMap::LevelMap(const std::string_view tilFileName, const std::string_view flagsFileName,
	int32_t width_, int32_t height_, int32_t defaultTile)
	: tileSet(tilFileName), flagsVariant(flagsFileName)
{
	resize(width_, height_);
	clear(defaultTile);
}

LevelMap::LevelMap(std::weak_ptr<LevelFlags> flags_, int32_t width_, int32_t height_, int32_t defaultTile) : flagsVariant(flags_)
{
	resize(width_, height_);
	clear(defaultTile);
}

LevelMap::LevelMap(int32_t width_, int32_t height_, int32_t defaultTile)
{
	resize(width_, height_);
	clear(defaultTile);
}

void LevelMap::resize(int32_t width_, int32_t height_)
{
	mapSizei.x = std::clamp(width_, 0, (int32_t)std::numeric_limits<uint16_t>::max());
	mapSizei.y = std::clamp(height_, 0, (int32_t)std::numeric_limits<uint16_t>::max());
	mapSizef.x = (float)mapSizei.x;
	mapSizef.y = (float)mapSizei.y;

	cells.resize(mapSizei.x * mapSizei.y, {});
}

void LevelMap::clear(int32_t defaultTile)
{
	if (defaultTile < 0)
	{
		cells.assign(cells.size(), {});
	}
	else
	{
		if ((size_t)defaultTile < tileSet.size())
		{
			cells.assign(cells.size(), {});

			auto flags = getFlags();
			const auto& defaultTileBlock = tileSet[defaultTile];
			for (int32_t j = 0; j < mapSizei.y; j++)
			{
				for (int32_t i = 0; i < mapSizei.x; i++)
				{
					auto tileIdx = defaultTileBlock.getTileIndex(i, j);
					(*this)[i][j].setTileIndex(0, tileIdx);
					(*this)[i][j].setTileIndex(LevelCell::FlagsLayer, flags->getFlags(tileIdx));
				}
			}
		}
		else
		{
			cells.assign(cells.size(), { defaultTile });
		}
	}
};

void LevelMap::setDefaultTileSize(int32_t tileWidth_, int32_t tileHeight_, uint32_t subTiles_) noexcept
{
	defaultTileWidth = tileWidth_;
	defaultTileHeight = tileHeight_;
	defaultBlockWidth = std::max(1, tileWidth_ / 2);
	defaultBlockHeight = std::max(1, tileHeight_ / 2);
	defaultSubTiles = subTiles_;
}

void LevelMap::loadLightMap(const std::string_view fileName)
{
	lightMap.load(fileName, 0, 0xFFFF);
	lightsNeedUpdate = true;
}

uint8_t LevelMap::getLight(size_t index) const
{
	return std::max(getDefaultLight(), lightMap.get(index));
}

void LevelMap::updateMapLights()
{
	LightStruct ls;
	ls.lightSource = defaultLight;
	for (ls.mapPos.y = 0; ls.mapPos.y < mapSizei.y; ls.mapPos.y++)
	{
		for (ls.mapPos.x = 0; ls.mapPos.x < mapSizei.x; ls.mapPos.x++)
		{
			auto& cell = (*this)[ls.mapPos];
			ls.lightSource.light = lightMap.get(cell.getTileIndex(0));
			if (ls.lightSource.light > 0 &&
				ls.lightSource.radius > 0)
			{
				ls.drawPos = toDrawCoord(ls.mapPos);
				ls.drawPos.x += defaultBlockWidth;
				ls.drawPos.y += defaultBlockHeight;
				mapLights.push_back(ls);
			}
		}
	}
}

void LevelMap::updateLights(const std::vector<std::shared_ptr<LevelObject>>& levelObjects, const sf::Vector2f& drawCenter)
{
	if (defaultLight.light == 255 ||
		maxLights == 0)
	{
		return;
	}
	if (lightsNeedUpdate == true)
	{
		lightsNeedUpdate = false;
		updateMapLights();
	}

	LightStruct ls;
	allLights.clear();
	for (const auto& levelObject : levelObjects)
	{
		ls.lightSource = levelObject->getLightSource();
		if (ls.lightSource.light > 0 &&
			ls.lightSource.radius > 0)
		{
			ls.mapPos = levelObject->MapPosition();
			ls.drawPos = levelObject->getBasePosition();
			allLights.push_back(ls);
		}
	}

	allLights.insert(allLights.end(), mapLights.begin(), mapLights.end());

	auto mapCenter = toMapCoord(drawCenter);

	std::sort(allLights.begin(), allLights.end(),
		[&mapCenter](const LightStruct& lhs, const LightStruct& rhs)
		{
			auto diffX = lhs.mapPos.x - mapCenter.x;
			auto diffY = lhs.mapPos.y - mapCenter.y;
			auto diffA = std::sqrt(diffX * diffX + diffY * diffY);
			auto costA = diffA * 1000.f + (float)(lhs.lightSource.light * lhs.lightSource.radius);

			diffX = rhs.mapPos.x - mapCenter.x;
			diffY = rhs.mapPos.y - mapCenter.y;
			auto diffB = std::sqrt(diffX * diffX + diffY * diffY);
			auto costB = diffB * 1000.f + (float)(rhs.lightSource.light * rhs.lightSource.radius);

			return costA < costB;
		});

	if (allLights.size() > maxLights)
	{
		allLights.resize(maxLights);
	}
}

void LevelMap::MaxLights(uint32_t maxLights_) noexcept
{
	maxLights = std::min(maxLights_, MaxNumberOfLightsToUse);
}

const LevelFlags* LevelMap::getFlags()
{
	if (std::holds_alternative<std::weak_ptr<LevelFlags>>(flagsVariant) == true)
	{
		auto flagsPtr = std::get<std::weak_ptr<LevelFlags>>(flagsVariant).lock().get();
		if (flagsPtr != nullptr)
		{
			return flagsPtr;
		}
		flagsVariant = FlagsVector();
	}
	return &std::get<FlagsVector>(flagsVariant);
}

void LevelMap::getSubIndex(int32_t x, int32_t y, uint32_t& subIndex)
{
	subIndex = 0;
}

void LevelMap::setTileSetAreaUseFlags(int32_t x, int32_t y, const Vector2D<int32_t>& vec)
{
	lightsNeedUpdate = true;
	auto flags = getFlags();
	auto dWidth = vec.Width() * 2;
	auto dHeight = vec.Height() * 2;
	for (size_t j = 0; j < dHeight; j++)
	{
		for (size_t i = 0; i < dWidth; i++)
		{
			size_t xDunIndex = i;
			size_t xTilIndex = 0;
			if ((xDunIndex % 2) != 0)
			{
				xDunIndex--;
				xTilIndex = 1;
			}
			xDunIndex /= 2;

			size_t yDunIndex = j;
			size_t yTilIndex = 0;
			if ((yDunIndex % 2) != 0)
			{
				yDunIndex--;
				yTilIndex = 1;
			}
			yDunIndex /= 2;

			int32_t dunIndex = vec[xDunIndex][yDunIndex];
			if (dunIndex < 0 || (size_t)dunIndex >= tileSet.size())
			{
				continue;
			}

			int16_t tileIndex = 0;
			if (xTilIndex)
			{
				if (yTilIndex)
					tileIndex = tileSet[dunIndex].get<3>(); // bottom
				else
					tileIndex = tileSet[dunIndex].get<1>(); // left
			}
			else
			{
				if (yTilIndex)
					tileIndex = tileSet[dunIndex].get<2>(); // right
				else
					tileIndex = tileSet[dunIndex].get<0>(); // top
			}

			auto cellX = x + (int32_t)i;
			auto cellY = y + (int32_t)j;

			if (cellX < 0 || cellX >= mapSizei.x ||
				cellY < 0 || cellY >= mapSizei.y)
			{
				continue;
			}

			auto& cell = cells[cellX + (cellY * mapSizei.x)];

			cell.setTileIndex(0, tileIndex);
			cell.setTileIndex(LevelCell::FlagsLayer, flags->getFlags(tileIndex));
		}
	}
}

void LevelMap::setSimpleAreaUseFlags(size_t layer, int32_t x, int32_t y, const Vector2D<int32_t>& vec)
{
	if (layer == 0)
	{
		lightsNeedUpdate = true;
	}
	auto flags = getFlags();
	for (size_t j = 0; j < vec.Height(); j++)
	{
		for (size_t i = 0; i < vec.Width(); i++)
		{
			auto cellX = x + (int32_t)i;
			auto cellY = y + (int32_t)j;

			if (cellX < 0 || cellX >= mapSizei.x ||
				cellY < 0 || cellY >= mapSizei.y)
			{
				continue;
			}

			auto& cell = cells[(size_t)(cellX + (cellY * mapSizei.x))];

			auto tileIndex = vec[i][j];
			cell.setTileIndex(layer, tileIndex);
			cell.setTileIndex(
				LevelCell::FlagsLayer,
				(tileIndex >= 0 ? flags->getFlags(tileIndex) : 0)
			);
		}
	}
}

void LevelMap::setSimpleArea(size_t layer, int32_t x, int32_t y, const Vector2D<int32_t>& vec, bool normalizeFlagsLayer)
{
	if (layer > LevelCell::NumberOfLayers)
	{
		return;
	}
	if (layer == 0)
	{
		lightsNeedUpdate = true;
	}
	for (size_t j = 0; j < vec.Height(); j++)
	{
		for (size_t i = 0; i < vec.Width(); i++)
		{
			auto cellX = x + (int32_t)i;
			auto cellY = y + (int32_t)j;

			if (cellX < 0 || cellX >= mapSizei.x ||
				cellY < 0 || cellY >= mapSizei.y)
			{
				continue;
			}

			auto& cell = cells[(size_t)(cellX + (cellY * mapSizei.x))];

			auto tileIndex = vec[i][j];
			if (layer == LevelCell::FlagsLayer && normalizeFlagsLayer == true)
			{
				tileIndex = (tileIndex != 0 ? 1 : 0);
			}
			cell.setTileIndex(layer, tileIndex);
		}
	}
}

#ifdef DGENGINE_DIABLO_FORMAT_SUPPORT
void LevelMap::setD2Area(int32_t x, int32_t y, DS1::Decoder& dun)
{
	resize(dun.width * defaultSubTiles, dun.height * defaultSubTiles);

	size_t currLayer = 0;
	auto flags = getFlags();

	auto addLevelLayer = [&](std::unordered_map<int, DS1::Cell> dunCells, int increment,
		int offset, const std::set<int>& orientations) {

			if (currLayer >= LevelCell::NumberOfLayers)
			{
				return;
			}

			int index = offset;
			for (int y = 0; y < dun.height; y++)
			{
				for (int x = 0; x < dun.width; x++)
				{
					int32_t id = -1;
					auto it = dunCells.find(index);
					if (it != dunCells.end())
					{
						auto id2 = it->second.id;
						if (orientations.count(DT1::Tile::getOrientation(id2)) != 0)
						{
							id = id2;
							auto celIdx = (x * defaultSubTiles) + ((y * defaultSubTiles) * mapSizei.x);
							auto& cell = cells[celIdx];
							cell.setTileIndex(currLayer, id);
						}
					}
					if (currLayer == 0 || id >= 0)
					{
						for (uint32_t subY = 0; subY < defaultSubTiles; subY++)
						{
							for (uint32_t subX = 0; subX < defaultSubTiles; subX++)
							{
								auto subIndex = subX + ((defaultSubTiles - subY - 1) * defaultSubTiles);
								int32_t tileFlags = id >= 0 ? flags->getFlags(id, subIndex) : 0;

								auto celIdx = (x * defaultSubTiles + subX) + ((y * defaultSubTiles + subY) * mapSizei.x);
								auto& cell = cells[celIdx];
								cell.setTileIndex(LevelCell::FlagsLayer, tileFlags);
							}
						}
					}
					index += increment;
				}
			}
			currLayer++;
	};

	// lower walls
	for (int i = 0; i < dun.numWalls; i++)
	{
		addLevelLayer(dun.walls, dun.numWalls, i, {
				DT1::Orientation::LOWER_LEFT_WALL,
				DT1::Orientation::LOWER_RIGHT_WALL,
				DT1::Orientation::LOWER_NORTH_CORNER_WALL,
				DT1::Orientation::LOWER_SOUTH_CORNER_WALL }
		);
	}

	// floors
	for (int i = 0; i < dun.numFloors; i++)
	{
		addLevelLayer(dun.floors, dun.numFloors, i, { DT1::Orientation::FLOOR });
	}

	// shadows
	// TODO: Draws shadows everywhere, must be more to it...
	//for (int i = 0; i < dun.numShadows; i++)
	//    addLevelLayer(dun.shadows, dun.numShadows, i, { DT1::Orientation::SHADOW });

	// TODO: walkable

	// walls
	for (int i = 0; i < dun.numWalls; i++)
	{
		addLevelLayer(dun.walls, dun.numWalls, i, {
				DT1::Orientation::LEFT_WALL,
				DT1::Orientation::LEFT_NORTH_CORNER_WALL,
				DT1::Orientation::LEFT_END_WALL,
				DT1::Orientation::LEFT_WALL_DOOR,
				DT1::Orientation::RIGHT_WALL,
				DT1::Orientation::RIGHT_NORTH_CORNER_WALL,
				DT1::Orientation::RIGHT_END_WALL,
				DT1::Orientation::RIGHT_WALL_DOOR,
				DT1::Orientation::SOUTH_CORNER_WALL,
				DT1::Orientation::PILLAR,
				DT1::Orientation::TREE }
		);
	}

	// roofs
	for (int i = 0; i < dun.numWalls; i++)
	{
		addLevelLayer(dun.walls, dun.numWalls, i, { DT1::Orientation::ROOF });
	}
}
#endif

bool LevelMap::isLayerUsed(size_t layer) const noexcept
{
	for (const auto& elem : cells)
	{
		if (elem.getTileIndex(layer) >= 0)
		{
			return true;
		}
	}
	return false;
}

bool LevelMap::isMapCoordValid(int32_t x, int32_t y) const noexcept
{
	return x >= 0 && x < mapSizei.x &&
		y >= 0 && y < mapSizei.y;
}

bool LevelMap::isMapCoordValid(const PairInt32& mapCoord) const noexcept
{
	return isMapCoordValid(mapCoord.x, mapCoord.y);
}

bool LevelMap::isMapCoordValid(float x, float y) const noexcept
{
	return x >= 0.f && x < mapSizef.x &&
		y >= 0.f && y < mapSizef.y;
}

bool LevelMap::isMapCoordValid(const PairFloat& mapCoord) const noexcept
{
	return isMapCoordValid(mapCoord.x, mapCoord.y);
}

PairFloat LevelMap::toMapCoord(const sf::Vector2f& drawCoord) const noexcept
{
	return toMapCoord(drawCoord, defaultBlockWidth, defaultBlockHeight);
}

PairFloat LevelMap::toMapCoord(const sf::Vector2f& drawCoord, int32_t blockWidth, int32_t blockHeight) noexcept
{
	return PairFloat(
		std::round((drawCoord.x / blockWidth + drawCoord.y / blockHeight) / 2),
		std::round((drawCoord.y / blockHeight - (drawCoord.x / blockWidth)) / 2)
	);
}

TileBlock LevelMap::getTileBlock(int16_t index) const
{
	if (tileSet.empty() == true)
	{
		return { index };
	}
	else if (index >= 0 && (size_t)index < tileSet.size())
	{
		return tileSet[index];
	}
	return {};
}

bool LevelMap::addLevelObject(std::unique_ptr<LevelObject> obj)
{
	return obj->MapPosition(*this, obj->MapPosition());
}

bool LevelMap::removeLevelObject(const LevelObject* obj)
{
	return obj->remove(*this);
}

std::vector<PairFloat> LevelMap::getPath(const PairFloat& a, const PairFloat& b) const
{
	std::vector<PairFloat> path;

	if (a == b)
	{
		path.push_back(a);
		return path;
	}

	MapSearchNode start(*this, (int16_t)a.x, (int16_t)a.y, PlayerDirection::All);
	MapSearchNode end(*this, (int16_t)b.x, (int16_t)b.y, PlayerDirection::All);
	MapSearchNode endOrig(end);

	if (end.IsValid() == false)
	{
		return path;
	}
	if (end.IsPassable() == false)
	{
		if (((*this)[b]).hasObjects() == true)
		{
			if (start.GoalDistanceEstimateC(end) == 1.f ||
				getNearestPassableEndNode(*this, start, end) == false)
			{
				path.push_back(b);
				return path;
			}
			if ((*this)[end.x][end.y].PassableIgnoreObject() == false)
			{
				return path;
			}
		}
		else
		{
			return path;
		}
	}

	PathFinder pathFinder(this);
	pathFinder.SetStartAndGoalStates(start, end);

	unsigned int SearchState;
	do
	{
		SearchState = pathFinder.SearchStep();
		if (pathFinder.GetStepCount() == PathFinder::MaxNodes)
		{
			pathFinder.CancelSearch();
		}
	} while (SearchState == PathFinder::SEARCH_STATE_SEARCHING);

	if (SearchState == PathFinder::SEARCH_STATE_SUCCEEDED)
	{
		if (endOrig.IsPassable() == false)
		{
			path.push_back(PairFloat((float)endOrig.x, (float)endOrig.y));
		}
		auto node = pathFinder.GetSolutionEnd();
		while (true)
		{
			if (node == nullptr)
			{
				break;
			}
			path.push_back(PairFloat((float)node->x, (float)node->y));
			node = pathFinder.GetSolutionPrev();
		};
		pathFinder.FreeSolutionNodes();
	}
	pathFinder.EnsureMemoryFreed();

	return path;
}

std::string LevelMap::toCSV(bool zeroBasedIndex) const
{
	std::string str;
	int32_t inc = (zeroBasedIndex == true ? 0 : 1);

	for (int j = 0; j < mapSizei.y; j++)
	{
		for (int i = 0; i < mapSizei.x; i++)
		{
			str += Utils::toString((*this)[i][j].getTileIndex(0) + inc) + ",";
		}
		str += "\n";
	}
	str.pop_back();
	str.pop_back();
	return str;
}

#pragma once

#include <cstdint>
#include "LevelFlags.h"
#include "LevelObject.h"
#include "Item.h"
#include <vector>

class LevelCell
{
public:
	// number of layers including sol layer
	static constexpr size_t NumberOfLayers = 12;
	static constexpr size_t FlagsLayer = NumberOfLayers - 1;

private:
	std::array<int32_t, NumberOfLayers> tileIndexes{ -1, -1, -1, -1,- 1, -1, -1, -1, -1, -1, -1, 0 };
	std::vector<LevelObject*> objects;

public:
	LevelCell() {}
	LevelCell(int32_t defaultTileLayer1)
		: tileIndexes{ defaultTileLayer1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0 } {}

	using iterator = std::vector<LevelObject*>::iterator;
	using const_iterator = std::vector<LevelObject*>::const_iterator;
	using reverse_iterator = std::vector<LevelObject*>::reverse_iterator;
	using const_reverse_iterator = std::vector<LevelObject*>::const_reverse_iterator;

	iterator begin() noexcept { return objects.begin(); }
	iterator end() noexcept { return objects.end(); }
	const_iterator begin() const noexcept { return objects.begin(); }
	const_iterator end() const noexcept { return objects.end(); }
	const_iterator cbegin() const noexcept { return objects.cbegin(); }
	const_iterator cend() const noexcept { return objects.cend(); }
	reverse_iterator rbegin() noexcept { return objects.rbegin(); }
	reverse_iterator rend() noexcept { return objects.rend(); }
	const_reverse_iterator rbegin() const noexcept { return objects.rbegin(); }
	const_reverse_iterator rend() const noexcept { return objects.rend(); }
	const_reverse_iterator crbegin() const noexcept { return objects.crbegin(); }
	const_reverse_iterator crend() const noexcept { return objects.crend(); }

	int32_t getTileIndex(size_t layer) const noexcept { return tileIndexes[layer]; }

	void setTileIndex(size_t layer, int32_t tileIndex_) noexcept { tileIndexes[layer] = tileIndex_; }

	bool PassableIgnoreObject() const noexcept { return LevelFlags::Passable(tileIndexes[FlagsLayer]); }
	bool PassableIgnoreObject(const LevelObject* ignoreObj) const;
	bool Passable() const;

	LevelObject* back() const;
	LevelObject* front() const;

	bool hasObjects() const noexcept { return objects.empty() == false; }

	template <class T>
	T* getObject() const noexcept
	{
		for (const auto object : objects)
		{
			const auto castObj = dynamic_cast<T*>(object);
			if (castObj != nullptr)
			{
				return castObj;
			}
		}
		return nullptr;
	}

	void addFront(LevelObject* obj);
	void addBack(LevelObject* obj);
	bool removeObject(const LevelObject* obj);

	template <class T>
	T* removeObject()
	{
		for (auto it = objects.begin(); it != objects.end(); ++it)
		{
			auto oldObj = dynamic_cast<T*>(*it);
			if (oldObj != nullptr)
			{
				objects.erase(it);
				return oldObj;
			}
		}
		return nullptr;
	}
};

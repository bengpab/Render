#pragma once

#include <vector>

template<typename ID, typename DataType>
struct IDArray
{
	explicit IDArray()
	{
		Data.push_back({});
		RefCounts.push_back(0);
	}

	ID Create()
	{
		ID id = MakeID();
		Data[(uint32_t)id] = {};
		return id;
	}

	ID Create(const DataType& data)
	{
		ID id = MakeID();
		Data[(uint32_t)id] = data;
		return id;
	}

	ID Create(DataType&& data)
	{
		ID id = MakeID();
		Data[(uint32_t)id] = std::move(data);
		return id;
	}

	ID Create(DataType** outData)
	{
		ID id = MakeID();
		*outData = &Data[(uint32_t)id];
		return id;
	}

	void Update(ID id, DataType& data)
	{
		assert((size_t)id < RefCounts.size());
		Data[(uint32_t)id] = data;
	}

	void AddRef(ID id)
	{
		assert((size_t)id < RefCounts.size());
		RefCounts[(uint32_t)id]++;
	}

	uint32_t RefCount(ID id) const
	{
		assert((size_t)id < RefCounts.size());
		return RefCounts[(size_t)id];
	}

	bool Reffed(ID id) const
	{
		return RefCount(id) > 0u;
	}

	bool Valid(ID id) const noexcept
	{
		return id != ID::INVALID && (size_t)id < RefCounts.size() && RefCounts[(uint32_t)id] > 0;
	}

	DataType* Release(ID id)
	{
		if (!Valid(id))
			return nullptr;

		if (--RefCounts[(uint32_t)id] == 0)
		{
			FreeIDs.push_back(id);
			return &Data[(uint32_t)id];
		}

		return nullptr;
	}

	DataType* GetUnchecked(ID id) noexcept
	{
		return (uint32_t)id <= Data.size() ? &Data[(uint32_t)id] : nullptr;
	}

	DataType* Get(ID id)
	{
		return (Valid(id) && Reffed(id)) ? &Data[(uint32_t)id] : nullptr;
	}

	inline std::vector<DataType>& GetArray() noexcept { return Data; }
	inline size_t Size() const noexcept { return Data.size(); }
	inline size_t UsedSize() const noexcept { return Data.size() - FreeIDs.size(); }
	auto begin() noexcept { return Data.begin(); }
	auto end() noexcept { return Data.end(); }

	DataType& operator[](ID id) { return Data[(size_t)id]; }
	const DataType& operator[](ID id) const { return Data[(size_t)id]; }

	template<typename Func>
	void ForEachNullIfValid(Func&& func)
	{
		for (size_t i = 0; i < Data.size(); i++)
		{
			func(Valid((ID)i) ? &Data[i] : nullptr);
		}
	}

	template<typename Func>
	void ForEachValidConst(Func&& func) const
	{
		for (size_t i = 0; i < Data.size(); i++)
		{
			if (Valid((ID)i))
			{
				if (!func((ID)i, Data[i]))
				{
					break;
				}
			}
				
		}
	}

private:
	std::vector<ID>			FreeIDs;
	std::vector<DataType>	Data;
	std::vector<uint32_t>	RefCounts;

	ID MakeID()
	{
		if (!FreeIDs.empty())
		{
			ID id = FreeIDs.back();
			FreeIDs.pop_back();
			RefCounts[(uint32_t)id]++;
			return id;
		}
		else
		{
			ID id = (ID)Data.size();
			Data.push_back({});
			RefCounts.push_back(1);

			return id;
		}
	}
};

#pragma once

#include <vector>

namespace rl
{

template<typename Type, typename Handle>
struct SparseArray
{
	std::vector<Type> Array;

	Type& Alloc(Handle handle)
	{
		MakeRoom(handle);

		return Array[static_cast<size_t>(handle)];
	}

	void AllocCopy(Handle handle, const Type& type)
	{
		MakeRoom(handle);

		Array[static_cast<size_t>(handle)] = type;
	}

	void AllocEmplace(Handle handle, const Type&& type)
	{
		MakeRoom(handle);

		Array[static_cast<size_t>(handle)] = std::move(type);
	}

	void Free(Handle handle)
	{
		if (static_cast<size_t>(handle) < Array.size())
		{
			Array[static_cast<size_t>(handle)] = Type{};
		}
	}

	bool Valid(Handle handle) const
	{
		return static_cast<size_t>(handle) < Array.size();
	}

	size_t Size() const
	{
		return Array.size();
	}

	auto begin() { return Array.begin(); }
	auto end() { return Array.end(); }

	Type& operator[](Handle handle) { return Array[(size_t)handle]; }
	const Type& operator[](Handle handle) const { return Array[(size_t)handle]; }

private:

	void MakeRoom(Handle handle)
	{
		const size_t size = static_cast<size_t>(handle);
		if (size >= Array.size())
		{
			Array.resize(size + 1, Type{});
		}
	}
};

}
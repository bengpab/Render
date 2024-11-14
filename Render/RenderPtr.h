#pragma once

#include <utility>

namespace tpr
{

template<typename RenderType_t>
struct RenderPtr
{
	RenderPtr()
		: Handle(RenderType_t::INVALID)
	{}

	// copy, increase ref
	RenderPtr(const RenderPtr& other)
		: Handle(other.Handle)
	{
		RenderRef(Handle);
	}

	// move, do nothing
	RenderPtr(RenderPtr&& other)
		: Handle(std::move(other.Handle))
	{
		other.Handle = RenderType_t::INVALID;
	}

	~RenderPtr()
	{
		RenderRelease(Handle);
	}

	// We always assume the incoming type is reffed already 
	RenderPtr(RenderType_t handle)
		: Handle(handle)
	{}

	RenderPtr& operator=(const RenderPtr& other)
	{
		if (&this->Handle != &other)
		{
			RenderRelease(Handle);
			Handle = other.Handle;
			RenderRef(Handle);
		}
		return *this;
	}

	RenderPtr& operator=(RenderPtr&& other)
	{
		if (&this->Handle != &other)
		{
			RenderRelease(Handle);
			Handle = other.Handle;
			other.Handle = RenderType_t::INVALID;
		}
		return *this;
	}

	operator RenderType_t() const
	{
		return Handle;
	}

	RenderType_t Get() const 
	{ 
		return Handle; 
	}

	operator bool() const
	{
		return Handle != RenderType_t::INVALID;
	}

	const RenderType_t* operator&() const
	{
		return &Handle;
	}

private:
	RenderType_t Handle = RenderType_t::INVALID;
};

// Prevent automatic type conversion allowing legal calls to RenderRelease for managed objects
template<typename T>
struct assert_false : std::false_type
{ };

template<typename RenderType_t>
void RenderRelease(RenderPtr<RenderType_t> p)
{
	static_assert(assert_false<RenderPtr<RenderType_t>>::value, "Never call RenderRelease on managed RenderPtr objects");
}

}
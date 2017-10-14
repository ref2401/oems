#pragma once

#include <type_traits>
#include <oems/common.h>
#include <d3d11.h>
#include <d3dcommon.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <windows.h>


namespace oems {

// com_ptr is a smart pointer that owns and manages a COM object through a pointer 
// and disposes of that object when the com_ptr goes out of scope.
template<typename T>
struct com_ptr final {

	static_assert(std::is_base_of<IUnknown, T>::value, "T must be derived from IUnknown.");


	com_ptr() noexcept = default;

	explicit com_ptr(T* ptr) noexcept
		: ptr(ptr)
	{}

	com_ptr(nullptr_t) noexcept {}

	com_ptr(com_ptr&& com_ptr) noexcept
		: ptr(com_ptr.ptr)
	{
		com_ptr.ptr = nullptr;
	}

	com_ptr& operator=(com_ptr&& com_ptr) noexcept
	{
		if (this == &com_ptr) return *this;

		dispose();
		ptr = com_ptr.ptr;
		com_ptr.ptr = nullptr;
		return *this;
	}

	~com_ptr() noexcept
	{
		dispose();
	}


	com_ptr& operator=(T* ptr) noexcept
	{
		dispose();
		this->ptr = ptr;
		return *this;
	}

	com_ptr& operator=(nullptr_t) noexcept
	{
		dispose();
		return *this;
	}

	T& operator*() const noexcept
	{
		return *ptr;
	}

	T* operator->() const noexcept
	{
		return ptr;
	}

	operator bool() const noexcept
	{
		return (ptr != nullptr);
	}

	operator T*() const noexcept
	{
		return ptr;
	}


	// Releases the managed COM object if such is present.
	void dispose() noexcept
	{
		T* temp = ptr;
		if (temp == nullptr) return;

		ptr = nullptr;
		temp->Release();
	}

	// Releases the ownership of the managed COM object and returns a pointer to it.
	// Does not call ptr->Release(). ptr == nullptr after that. 
	T* release_ownership() noexcept
	{
		T* tmp = ptr;
		ptr = nullptr;
		return tmp;
	}

	// Pointer to the managed COM object.
	T* ptr = nullptr;
};

class dx11_rhi final {
public:

	dx11_rhi();

	dx11_rhi(dx11_rhi&&) = delete;
	dx11_rhi& operator=(dx11_rhi&&) = delete;

	~dx11_rhi() noexcept;


	ID3D11Device* p_device() noexcept
	{
		return p_device_;
	}

	ID3D11DeviceContext* p_ctx() noexcept
	{
		return p_ctx_;
	}

	ID3D11Debug* p_debug() noexcept
	{
		return p_debug_;
	}

	IDXGISwapChain* p_swap_chain() noexcept
	{
		return p_swap_chain_;
	}

private:

	HWND							p_hwnd_;
	com_ptr<ID3D11Device>			p_device_;
	com_ptr<ID3D11DeviceContext>	p_ctx_;
	com_ptr<ID3D11Debug>			p_debug_;
	com_ptr<IDXGISwapChain>			p_swap_chain_;
};

struct hlsl_compute final {

	hlsl_compute() noexcept = default;

	hlsl_compute(ID3D11Device* p_device, const char* p_source_filename);

	hlsl_compute(hlsl_compute&& s) noexcept = default;
	hlsl_compute& operator=(hlsl_compute&& s) noexcept = default;


	com_ptr<ID3D11ComputeShader>	p_shader;
	com_ptr<ID3DBlob>				p_bytecode;
};


template<typename T>
inline bool operator==(const com_ptr<T>& l, const com_ptr<T>& r) noexcept
{
	return l.ptr == r.ptr;
}

template<typename T>
inline bool operator==(const com_ptr<T>& com_ptr, nullptr_t) noexcept
{
	return com_ptr.ptr == nullptr;
}

template<typename T>
inline bool operator==(nullptr_t, const com_ptr<T>& com_ptr) noexcept
{
	return com_ptr.ptr == nullptr;
}

template<typename T>
inline bool operator!=(const com_ptr<T>& l, const com_ptr<T>& r) noexcept
{
	return l.ptr != r.ptr;
}

template<typename T>
inline bool operator!=(const com_ptr<T>& com_ptr, nullptr_t) noexcept
{
	return com_ptr.ptr != nullptr;
}

template<typename T>
inline bool operator!=(nullptr_t, const com_ptr<T>& com_ptr) noexcept
{
	return com_ptr.ptr != nullptr;
}

// Creates a standard buffer resource
com_ptr<ID3D11Buffer> make_buffer(ID3D11Device* p_device, const D3D11_SUBRESOURCE_DATA* p_data,
	UINT byte_count, D3D11_USAGE usage, UINT bing_flags, UINT cpu_access_flags = 0);

// ditto
inline com_ptr<ID3D11Buffer> make_buffer(ID3D11Device* p_device, UINT byte_count,
	D3D11_USAGE usage, UINT bind_flags, UINT cpu_access_flags = 0)
{
	return make_buffer(p_device, nullptr, byte_count, usage, bind_flags, cpu_access_flags);
}

//  Creates a structured buffer resource
com_ptr<ID3D11Buffer> make_structured_buffer(ID3D11Device* p_device,
	UINT item_count, UINT item_byte_count,
	D3D11_USAGE usage, UINT bing_flags, UINT cpu_access_flags = 0);

} // namespace oems

#include "oems/oems_sorter.h"

#include <cassert>


namespace oems {

// ---- oems_sorter ----

oems_sorter::oems_sorter(ID3D11Device* p_device, ID3D11DeviceContext* p_ctx, ID3D11Debug* p_debug)
	: p_device_(p_device), p_ctx_(p_ctx), p_debug_(p_debug)
{
	assert(p_device);
	assert(p_ctx);
	assert(p_debug); // p_debug == nullptr in Release mode.

	shader_ = hlsl_compute(p_device, "../../data/oems.compute.hlsl");
}

void oems_sorter::get_list_from_gpu(float* p_list, size_t byte_count)
{
	assert(p_list);
	assert(byte_count > 0);

	// copy from regular buffer to staging buffer ---
	p_ctx_->CopyResource(p_buffer_, p_buffer_staging_);

	// fill p_list using staging buffer ---
	D3D11_MAPPED_SUBRESOURCE map;
	HRESULT hr = p_ctx_->Map(p_buffer_staging_, 0, D3D11_MAP_READ, 0, &map);
	assert(hr == S_OK);
	std::memcpy(p_list, map.pData, byte_count);
	p_ctx_->Unmap(p_buffer_staging_, 0);
}

void oems_sorter::put_list_to_gpu(const float* p_list, size_t byte_count)
{
	assert(p_list);
	assert(byte_count > 0);

	p_buffer_uav_.dispose();

	const UINT bc = UINT(byte_count);
	assert(size_t(bc) == byte_count);

	// resize buffers ---
	p_buffer_staging_ = make_buffer(p_device_, bc, D3D11_USAGE_STAGING, 0,
		D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE);
	p_buffer_ = make_buffer(p_device_, bc, D3D11_USAGE_DEFAULT, D3D11_BIND_UNORDERED_ACCESS);

	// fill staging buffer ---
	D3D11_MAPPED_SUBRESOURCE map;
	HRESULT hr = p_ctx_->Map(p_buffer_staging_, 0, D3D11_MAP_WRITE, 0, &map);
	assert(hr == S_OK);
	std::memcpy(map.pData, p_list, byte_count);
	p_ctx_->Unmap(p_buffer_staging_, 0);

	// copy from staging buffer to regular buffer ---
	p_ctx_->CopyResource(p_buffer_, p_buffer_staging_);
}

void oems_sorter::sort(std::vector<float>& list)
{
	assert(list.size() > 8);

	const size_t byte_count = sizeof(float) * list.size();

	put_list_to_gpu(list.data(), byte_count);
	


	get_list_from_gpu(list.data(), byte_count);
}

} // namespace oems

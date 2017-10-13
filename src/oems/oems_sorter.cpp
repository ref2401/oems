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

	oems_base_4_shader_ = hlsl_compute(p_device, "../../data/oems_base_4.compute.hlsl");
	p_oems_4_constant_buffer_ = make_buffer(p_device, sizeof(UINT) * 4, 
		D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER);
}

void oems_sorter::get_list_from_gpu(float* p_list, size_t byte_count)
{
	assert(p_list);
	assert(byte_count > 0);

	// copy from regular buffer to staging buffer ---
	p_ctx_->CopyResource(p_buffer_staging_, p_buffer_);

	// fill p_list using staging buffer ---
	D3D11_MAPPED_SUBRESOURCE map;
	HRESULT hr = p_ctx_->Map(p_buffer_staging_, 0, D3D11_MAP_READ, 0, &map);
	assert(hr == S_OK);
	std::memcpy(p_list, map.pData, byte_count);
	p_ctx_->Unmap(p_buffer_staging_, 0);
}

void oems_sorter::perform_oems_base_4_sort(UINT item_count) 
{
	assert(item_count > 0);

	// update constant buffer ---
	UINT data[4] = { item_count, 0, 0, 0 };
	p_ctx_->UpdateSubresource(p_oems_4_constant_buffer_, 0, nullptr, data, 0, 0);

	// setup compute pipeline ---
	p_ctx_->CSSetShader(oems_base_4_shader_.p_shader, nullptr, 0);
	p_ctx_->CSSetConstantBuffers(0, 1, &p_oems_4_constant_buffer_.ptr);

	// dispatch work ---
#ifdef OEMS_DEBUG
	HRESULT hr = p_debug_->ValidateContextForDispatch(p_ctx_);
	assert(hr == S_OK);
#endif
	p_ctx_->Dispatch(1, 1, 1);
}

void oems_sorter::perform_oems_main_sort()
{

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
	D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
	uav_desc.Format =				DXGI_FORMAT_R32_FLOAT;
	uav_desc.ViewDimension =		D3D11_UAV_DIMENSION_BUFFER;
	uav_desc.Buffer.FirstElement =	0;
	uav_desc.Buffer.NumElements =	bc / sizeof(float);
	HRESULT hr = p_device_->CreateUnorderedAccessView(p_buffer_.ptr, &uav_desc, &p_buffer_uav_.ptr);
	assert(hr == S_OK);

	// fill staging buffer ---
	D3D11_MAPPED_SUBRESOURCE map;
	hr = p_ctx_->Map(p_buffer_staging_, 0, D3D11_MAP_WRITE, 0, &map);
	assert(hr == S_OK);
	std::memcpy(map.pData, p_list, byte_count);
	p_ctx_->Unmap(p_buffer_staging_, 0);

	// copy from staging buffer to regular buffer ---
	p_ctx_->CopyResource(p_buffer_, p_buffer_staging_);
}

void oems_sorter::sort(std::vector<float>& list)
{
	assert(list.size() > 32);

	const size_t byte_count = sizeof(float) * list.size();

	put_list_to_gpu(list.data(), byte_count);

	constexpr UINT uav_count = 1;
	ID3D11UnorderedAccessView* uav_list[uav_count] = { p_buffer_uav_ };
	p_ctx_->CSSetUnorderedAccessViews(0, uav_count, uav_list, nullptr);

	perform_oems_base_4_sort(UINT(list.size()));
	perform_oems_main_sort();

	// clear uav ---
	uav_list[0] = { nullptr };
	p_ctx_->CSSetUnorderedAccessViews(0, uav_count, uav_list, nullptr);

	get_list_from_gpu(list.data(), byte_count);
}

} // namespace oems

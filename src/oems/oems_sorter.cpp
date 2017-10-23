#include "oems/oems_sorter.h"

#include <cassert>


namespace {

using namespace oems;

void copy_data_from_gpu(ID3D11Device* p_device, ID3D11DeviceContext* p_ctx,
	ID3D11Buffer* p_buffer_src, float* p_dest, size_t byte_count)
{
	assert(p_device);
	assert(p_ctx);
	assert(p_buffer_src);
	assert(p_dest);
	assert(byte_count > 0);

	const UINT bc = UINT(byte_count);
	assert(size_t(bc) == byte_count);

	// create stagint buffer and fill it with p_buffer_src's contents ---
	com_ptr<ID3D11Buffer >p_buffer_staging = make_buffer(p_device, bc, 
		D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_READ);

	p_ctx->CopyResource(p_buffer_staging, p_buffer_src);

	// fill p_dest using the staging buffer ---
	D3D11_MAPPED_SUBRESOURCE map;
	HRESULT hr = p_ctx->Map(p_buffer_staging, 0, D3D11_MAP_READ, 0, &map);
	THROW_IF_DX_DEVICE_ERROR(hr, p_device);
	std::memcpy(p_dest, map.pData, byte_count);
	p_ctx->Unmap(p_buffer_staging, 0);
}

com_ptr<ID3D11Buffer> copy_data_to_gpu(ID3D11Device* p_device, const float* p_list, size_t byte_count)
{
	assert(p_device);
	assert(p_list);
	assert(byte_count > 0);

	const UINT bc = UINT(byte_count);
	assert(size_t(bc) == byte_count);

	const D3D11_SUBRESOURCE_DATA data = { p_list, 0, 0 };
	return make_buffer(p_device, &data, bc, D3D11_USAGE_DEFAULT, D3D11_BIND_UNORDERED_ACCESS);
}

} // namespace


namespace oems {

// ---- oems_sorter ----

oems_sorter::oems_sorter(ID3D11Device* p_device, ID3D11DeviceContext* p_ctx, ID3D11Debug* p_debug)
	: p_device_(p_device), p_ctx_(p_ctx), p_debug_(p_debug)
{
	assert(p_device);
	assert(p_ctx);
	assert(p_debug); // p_debug == nullptr in Release mode.

	oems_main_shader_ = hlsl_compute(p_device, "../../data/oems_main.compute.hlsl");
}

void oems_sorter::sort(std::vector<float>& list)
{
	assert(list.size() > 4);

	const size_t byte_count = sizeof(float) * list.size();
	const UINT item_count = UINT(list.size());

	// copy list to the gpu & set uav ---
	com_ptr<ID3D11Buffer> p_buffer = copy_data_to_gpu(p_device_, list.data(), byte_count);
	com_ptr<ID3D11UnorderedAccessView> p_buffer_uav;
	D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
	uav_desc.Format					= DXGI_FORMAT_R32_FLOAT;
	uav_desc.ViewDimension			= D3D11_UAV_DIMENSION_BUFFER;
	uav_desc.Buffer.FirstElement	= 0;
	uav_desc.Buffer.NumElements		= item_count;
	HRESULT hr = p_device_->CreateUnorderedAccessView(p_buffer.ptr, &uav_desc, &p_buffer_uav.ptr);
	THROW_IF_DX_ERROR(hr);

	// (sort) setup compute pipeline & dispatch work ---
	p_ctx_->CSSetShader(oems_main_shader_.p_shader, nullptr, 0);
	p_ctx_->CSSetUnorderedAccessViews(0, 1, &p_buffer_uav.ptr, nullptr);

#ifdef OEMS_DEBUG
	hr = p_debug_->ValidateContextForDispatch(p_ctx_);
	THROW_IF_DX_ERROR(hr);
#endif

	p_ctx_->Dispatch(1, 1, 1);

	ID3D11UnorderedAccessView* uav_list[1] = { nullptr };
	p_ctx_->CSSetUnorderedAccessViews(0, 1, uav_list, nullptr);

	// copy list from the gpu ---
	copy_data_from_gpu(p_device_, p_ctx_, p_buffer, list.data(), byte_count);
}

} // namespace oems

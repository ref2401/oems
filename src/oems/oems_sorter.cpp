#include "oems/oems_sorter.h"

#include <cassert>
#include <iostream>
#include <comdef.h>

namespace {

using namespace oems;

struct sorting_network_column {
	UINT block_count;
	UINT comparisons_per_block;
	UINT origin;
	UINT origin_step;
	UINT right_offset;
};


// copies c into p_desc and advances the pointer by 1 structure futher.
inline void append_column(sorting_network_column*& p_dest, const sorting_network_column& c)
{
	static constexpr size_t bc = sizeof(sorting_network_column);

	std::memcpy(p_dest, &c, bc);
	++p_dest;
}

inline UINT compute_sorting_network_column_count(UINT item_count)
{
	assert(item_count >= 8);

	// Observation:
	// item_count		column_count	formula
	// 8				5				item_count - 3
	// 16				14				item_count - 2
	// 32				31				item_count - 1
	// 64				64				item_count
	// 128				129				item_count + 1
	// 256				258				item_count + 2
	// 512				512				item_count + 3
	// 1024				1028			item_count + 4
	// ...
	// column_count = item_count + log2(item_count / 64);


	const double term = std::log2(item_count / 64.0);
	const double res_d = double(item_count) + term;
	const UINT res = UINT(res_d);

	assert(res_d == double(res));

	return res;
}

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
	if (hr != S_OK) {
		_com_error err(hr);
		LPCTSTR errMsg = err.ErrorMessage();
		std::cout << errMsg << std::endl;

		HRESULT rrrr = p_device->GetDeviceRemovedReason();
		_com_error reason(rrrr);
		LPCTSTR reason_msg = reason.ErrorMessage();
		std::cout << reason_msg << std::endl;
	}
	assert(hr == S_OK);
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
	return make_buffer(p_device, bc, D3D11_USAGE_DEFAULT, D3D11_BIND_UNORDERED_ACCESS);
}

void generate_sorting_network_columns(UINT item_count, sorting_network_column* p_out_column)
{
	UINT power = 3;
	UINT tip = 8; // == 2 ^ power

	while (tip <= item_count) {

		const UINT block_count = item_count / tip;

		// first pass ---
		{
			sorting_network_column column;
			column.block_count				= block_count;
			column.comparisons_per_block	= tip >> 2;
			column.origin					= 0;
			column.origin_step				= tip;
			column.right_offset				= tip >> 1;

			append_column(p_out_column, column); // even
			column.origin += 1;
			append_column(p_out_column, column); // odd
		}

		// second pass ---
		for (UINT b = 0; b < block_count; ++b) {
			const UINT column_count = power - 2;
			UINT row_count = 0;
			UINT row_offset = tip >> 1;

			for (UINT col = 0; col < column_count; ++col) {
				row_count = 2 * row_count + 1;
				row_offset >>= 1;

				sorting_network_column column;
				column.block_count				= row_count;
				column.comparisons_per_block	= row_offset >> 1;
				column.origin					= b * tip + row_offset;
				column.origin_step				= 2 * row_offset;
				column.right_offset				= row_offset;

				append_column(p_out_column, column); // even
				column.origin += 1;
				append_column(p_out_column, column); // odd
			}
		}

		// third pass ---
		{
			sorting_network_column column;
			column.block_count				= block_count;
			column.comparisons_per_block	= (tip >> 1) - 1;
			column.origin					= 1;
			column.origin_step				= tip;
			column.right_offset				= 1;

			//---oems3_split_batch(batch, c_split_factor, false);
			append_column(p_out_column, column);
		}

		tip <<= 1;
		++power;
	}
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

	oems_base_4_shader_ = hlsl_compute(p_device, "../../data/oems_base_4.compute.hlsl");
	oems_main_shader_ = hlsl_compute(p_device, "../../data/oems_main.compute.hlsl");
}

void oems_sorter::perform_oems_base_4_sort(UINT item_count) 
{
	assert(item_count > 0);

	// setup compute pipeline & dispatch work ---
	p_ctx_->CSSetShader(oems_base_4_shader_.p_shader, nullptr, 0);

#ifdef OEMS_DEBUG
	HRESULT hr = p_debug_->ValidateContextForDispatch(p_ctx_);
	assert(hr == S_OK);
#endif
	p_ctx_->Dispatch(1, 1, 1);
}

void oems_sorter::perform_oems_main_sort(UINT item_count)
{
	assert(item_count >= 8);

	// sorting network columns buffer ---
	const UINT column_count = compute_sorting_network_column_count(item_count);
	
	com_ptr<ID3D11Buffer> p_buffer_columns = make_structured_buffer(p_device_, column_count,
		sizeof(sorting_network_column), D3D11_USAGE_DYNAMIC, D3D11_BIND_SHADER_RESOURCE,
		D3D11_CPU_ACCESS_WRITE);

	com_ptr<ID3D11ShaderResourceView> p_buffer_columns_srv;
	HRESULT hr = p_device_->CreateShaderResourceView(p_buffer_columns, nullptr, &p_buffer_columns_srv.ptr);
	assert(hr == S_OK);

	// generate sorting network and fill the buffer with it ---
	D3D11_MAPPED_SUBRESOURCE map;
	hr = p_ctx_->Map(p_buffer_columns, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
	assert(hr == S_OK);

	sorting_network_column* p_column = reinterpret_cast<sorting_network_column*>(map.pData);
	generate_sorting_network_columns(item_count, p_column);

	p_ctx_->Unmap(p_buffer_columns, 0);

	// setup compute pipeline & dispatch work ---
	p_ctx_->CSSetShader(oems_main_shader_.p_shader, nullptr, 0);
	p_ctx_->CSSetShaderResources(0, 1, &p_buffer_columns_srv.ptr);

#ifdef OEMS_DEBUG
	hr = p_debug_->ValidateContextForDispatch(p_ctx_);
	assert(hr == S_OK);
#endif

	p_ctx_->Dispatch(1, 1, 1);
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
	uav_desc.Format =				DXGI_FORMAT_R32_FLOAT;
	uav_desc.ViewDimension =		D3D11_UAV_DIMENSION_BUFFER;
	uav_desc.Buffer.FirstElement =	0;
	uav_desc.Buffer.NumElements =	UINT(list.size());
	HRESULT hr = p_device_->CreateUnorderedAccessView(p_buffer.ptr, &uav_desc, &p_buffer_uav.ptr);
	assert(hr == S_OK);
	
	p_ctx_->CSSetUnorderedAccessViews(0, 1, &p_buffer_uav.ptr, nullptr);

	// sort ---
	perform_oems_base_4_sort(item_count);
	perform_oems_main_sort(item_count);

	ID3D11UnorderedAccessView* uav_list[1] = { nullptr };
	p_ctx_->CSSetUnorderedAccessViews(0, 1, uav_list, nullptr);

	// copy list from the gpu
	copy_data_from_gpu(p_device_, p_ctx_, p_buffer, list.data(), byte_count);
}

} // namespace oems

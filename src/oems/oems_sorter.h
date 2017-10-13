#pragma once

#include <vector>
#include "oems/dx11.h"


namespace oems {

class oems_sorter final {
public:

	oems_sorter(ID3D11Device* p_device, ID3D11DeviceContext* p_ctx, ID3D11Debug* p_debug);

	oems_sorter(oems_sorter&&) = delete;
	oems_sorter& operator=(oems_sorter&&) = delete;

	void sort(std::vector<float>& list);

private:

	static constexpr UINT c_compute_group_x_size = 32;


	void get_list_from_gpu(float* p_list, size_t byte_count);

	void perform_oems_base_4_sort(UINT item_count);

	void perform_oems_main_sort();

	void put_list_to_gpu(const float* p_list, size_t byte_count);

	ID3D11Device*			p_device_;
	ID3D11DeviceContext*	p_ctx_;
	ID3D11Debug*			p_debug_;

	hlsl_compute						oems_base_4_shader_;
	hlsl_compute						oems_main_shader_;
	com_ptr<ID3D11Buffer>				p_oems_4_constant_buffer_;
	com_ptr<ID3D11Buffer>				p_buffer_;
	com_ptr<ID3D11UnorderedAccessView>	p_buffer_uav_;
	com_ptr<ID3D11Buffer>				p_buffer_staging_;
};

} // namespace oems


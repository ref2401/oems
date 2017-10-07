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

	ID3D11Device*			p_device_;
	ID3D11DeviceContext*	p_ctx_;
	ID3D11Debug*			p_debug_;
	hlsl_compute			shader_;
	com_ptr<ID3D11Buffer>	p_buffer_;
};

} // namespace oems


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

void oems_sorter::sort(std::vector<float>& list)
{
	assert(list.size() > 8);
}

} // namespace oems

#include "common.hlsl"

static const uint c_thread_count = 32;

struct network_column {
	uint block_count;
	uint comparisons_per_block;
	uint origin;
	uint origin_step;
	uint right_offset;
};


RWBuffer<float>						g_buffer			: register(u0);
StructuredBuffer<network_column>	g_network_columns	: register(t0);


inline void compare_and_swap(uint l, uint r)
{
	float vl = g_buffer[l];
	float vr = g_buffer[r];
	if (vl <= vr) return;

	g_buffer[l] = vr;
	g_buffer[r] = vl;
}

void perform_comparisons(uint offset, uint comparison_count, network_column column)
{
	for (uint i = 0; i < comparison_count; ++i) {
		const uint b = (offset + i) / column.comparisons_per_block;
		const uint c = (offset + i) % column.comparisons_per_block;

		const uint first = column.origin + b * column.origin_step;
		const uint l = first + 2 * c;
		const uint r = l + column.right_offset;
		compare_and_swap(l, r);
	}
}

[numthreads(c_thread_count, 1, 1)]
void cs_main(uint3 group_id : SV_GroupID, uint3 group_thread_id : SV_GroupThreadID)
{
	const network_column column	= g_network_columns[group_id.x];
	const uint iteration_count = column.block_count * column.comparisons_per_block;
	const uint iterations_per_thread = max(1, iteration_count / c_thread_count);
	const uint required_thread_count = ceil(float(iteration_count) / iterations_per_thread);


	const uint curr_thread_id = group_thread_id.x;
	if (curr_thread_id >= required_thread_count) return;

	const uint offset = curr_thread_id * iterations_per_thread;
	perform_comparisons(offset, iterations_per_thread, column);

	if ((curr_thread_id + 1 == c_thread_count) && (required_thread_count > c_thread_count)) {
		const uint extra_iteration_count = iteration_count - iterations_per_thread * c_thread_count;

		const uint offset = c_thread_count * iterations_per_thread;
		perform_comparisons(offset, extra_iteration_count, column);
	}
}

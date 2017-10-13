#include "common.hlsl"

static const uint c_thread_count = 32;


cbuffer constant_buffer : register(b0) {
	uint g_item_count : packoffset(c0);
};

RWBuffer<float> g_buffer : register(u0);


void sort_merge_block_4(uint origin)
{
	// read 4 sequential values ---
	const uint	idx0 = origin;
	const uint	idx1 = idx0 + 1;
	const uint	idx2 = idx1 + 1;
	const uint	idx3 = idx2 + 1;
	float v0 = g_buffer[idx0];
	float v1 = g_buffer[idx1];
	float v2 = g_buffer[idx2];
	float v3 = g_buffer[idx3];

	// compare and swap: (0, 1) (2, 3) (0, 2) (1, 3) (1, 2) ---
	compare_and_swap(v0, v1);
	compare_and_swap(v2, v3);
	compare_and_swap(v0, v2);
	compare_and_swap(v1, v3);
	compare_and_swap(v1, v2);

	// flush values back ---
	g_buffer[idx0] = v0;
	g_buffer[idx1] = v1;
	g_buffer[idx2] = v2;
	g_buffer[idx3] = v3;
}

[numthreads(c_thread_count, 1, 1)]
void cs_main(uint id : SV_GroupIndex)
{
	const uint chunk_size = g_item_count / c_thread_count;
	const uint origin = id * chunk_size;

	for (uint i = 0; i < chunk_size; i += 4) {
		sort_merge_block_4(origin + i);
	}
}

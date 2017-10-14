static const uint c_thread_count = 32;


RWBuffer<float> g_buffer : register(u0);


inline void compare_and_swap(inout float l, inout float r)
{
	if (l <= r) return;

	const float tmp = l;
	l = r;
	r = tmp;
}

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
void cs_main(uint id : SV_GroupThreadID)
{
	uint item_count;
	g_buffer.GetDimensions(item_count);

	const uint tuple_count = item_count >> 2; // 4 elements represent a single tuple
	if (id >= tuple_count) return;

	const uint tuples_per_thread = max(1, tuple_count / c_thread_count);
	const uint origin = 4 * id * tuples_per_thread;

	for (uint t = 0; t < tuples_per_thread; ++t) {
		const uint first = origin + 4 * t;
		sort_merge_block_4(first);
	}
}

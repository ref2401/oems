static const uint c_thread_count = 256;

struct sorting_network_column {
	// index of the first element in column
	uint first_element_index;
	// group desc ---
	uint group_stride;
	uint group_count;
	// row desc ---
	uint row_stride;
	uint row_count;
	uint comparisons_per_row;
	// 
	uint left_factor;
	uint right_offset;
};


RWBuffer<float> g_buffer : register(u0);


inline void compare_and_swap(uint l, uint r)
{
	const float x = g_buffer[l];
	const float y = g_buffer[r];
	
	if (x <= y) return;

	g_buffer[l] = y;
	g_buffer[r] = x;

	// the following approach has worse performance
	//const float a = step(x, y);
	//const float b = 1 - a;
	//
	//g_buffer[l] = a * x + b * y;
	//g_buffer[r] = b * x + a * y;
}

inline uint get_item_count()
{
	uint c;
	g_buffer.GetDimensions(c);
	return c;
}

void process_column(uint curr_thread_id, sorting_network_column column)
{
	const uint comparisons_per_group		= column.row_count * column.comparisons_per_row;
	const uint comparison_count				= column.group_count * comparisons_per_group;
	const uint comparisons_per_thread		= max(1, comparison_count / c_thread_count);
	const uint required_thread_count		= uint(ceil(float(comparison_count) / comparisons_per_thread));

	if (curr_thread_id >= required_thread_count) return;
	
	// sometimes there are a few remained comparisons
	// the last thread is solely responsible for those
	const uint remained_comparison_count = comparison_count - comparisons_per_thread * c_thread_count;
	const uint cmpt_first_index = curr_thread_id * comparisons_per_thread;
	const uint cmpt_count = comparisons_per_thread + ((curr_thread_id + 1) / c_thread_count) * remained_comparison_count;

	for (uint i = 0; i < cmpt_count; ++i) {
		const uint ci = cmpt_first_index + i;											// current comparison index
		const uint gi = ci / comparisons_per_group;										// current group index
		const uint ri = (ci % comparisons_per_group) / column.comparisons_per_row;		// current row index inside group #gi
		const uint start_index = column.first_element_index 
			+ gi * column.group_stride
			+ ri * column.row_stride;	// index of the first element of the row #ri inside group #gi

		const uint l = start_index + column.left_factor * (ci % column.comparisons_per_row);
		const uint r = l + column.right_offset;
		compare_and_swap(l, r);
	}
}

void process_first_column(uint curr_thread_id, uint item_count, uint v_2power)
{
	const sorting_network_column column = {
		/* first_element_index */	0,
		/* uint group_stride */		0,
		/* uint group_count */		1,
		/* row_stride */			v_2power,
		/* row_count */				item_count / v_2power,
		/* comparisons_per_row */	v_2power >> 1,
		/* left_factor */			1,
		/* right_offset */			v_2power >> 1
	};

	process_column(curr_thread_id, column);
	DeviceMemoryBarrierWithGroupSync();
}

void process_intermediate_columns(uint curr_thread_id, uint item_count, 
	uint v_2power, uint column_count)
{
	sorting_network_column column = {
		/* first_element_index */	v_2power >> 2,
		/* uint group_stride */		v_2power,
		/* uint group_count */		item_count / v_2power,
		/* row_stride */			v_2power >> 1,
		/* row_count */				1,
		/* comparisons_per_row */	v_2power >> 2,
		/* left_factor */			1,
		/* right_offset */			v_2power >> 2
	};

	for (uint ci = 0; ci < column_count; ++ci) {

		process_column(curr_thread_id, column);
		DeviceMemoryBarrierWithGroupSync();

		column.first_element_index >>= 1;
		column.row_stride >>= 1;
		column.row_count = 2 * column.row_count + 1;
		column.comparisons_per_row >>= 1;
		column.right_offset >>= 1;
	}
}

void process_last_column(uint curr_thread_id, uint item_count, uint v_2power)
{
	const sorting_network_column column = {
		/* first_element_index */	1,
		/* uint group_stride */		0,
		/* uint group_count */		1,
		/* row_stride */			v_2power,
		/* row_count */				item_count / v_2power,
		/* comparisons_per_row */	(v_2power >> 1) - 1,
		/* left_factor */			2,
		/* right_offset */			1
	};

	process_column(curr_thread_id, column);
	DeviceMemoryBarrierWithGroupSync();
}

// Partitions g_buffer as sequence of 4 elements tuples and sorts each tuple separatly.
void sort_4(uint curr_thread_id, uint item_count)
{
	const uint tuple_count = item_count >> 2; // 4 elements represent a single tuple
	if (curr_thread_id >= tuple_count) return;

	const uint tuples_per_thread = max(1, tuple_count / c_thread_count);
	const uint origin = 4 * curr_thread_id * tuples_per_thread;

	for (uint t = 0; t < tuples_per_thread; ++t) {
		const uint idx0 = origin + 4 * t;
		const uint idx1 = idx0 + 1;
		const uint idx2 = idx1 + 1;
		const uint idx3 = idx2 + 1;

		// compare and swap: (0, 1) (2, 3) (0, 2) (1, 3) (1, 2) ---
		compare_and_swap(idx0, idx1);
		compare_and_swap(idx2, idx3);
		compare_and_swap(idx0, idx2);
		compare_and_swap(idx1, idx3);
		compare_and_swap(idx1, idx2);
	}
}

[numthreads(c_thread_count, 1, 1)]
void cs_main(uint3 gt_id : SV_GroupThreadID)
{
	const uint curr_thread_id = gt_id.x;
	const uint item_count = get_item_count();

	sort_4(curr_thread_id, item_count);
	DeviceMemoryBarrierWithGroupSync();

	uint power = 3;
	uint v_2power = 8; // == 2 ^ power

	while (v_2power <= item_count) {

		process_first_column(curr_thread_id, item_count, v_2power);
		process_intermediate_columns(curr_thread_id, item_count, v_2power, power - 2);
		process_last_column(curr_thread_id, item_count, v_2power);

		++power;
		v_2power <<= 1;
	}
}

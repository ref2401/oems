
inline void compare_and_swap(inout float l, inout float r)
{
	if (l <= r) return;

	const float tmp = l;
	l = r;
	r = tmp;
}

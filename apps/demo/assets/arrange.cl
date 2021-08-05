__kernel void arrange(__global float* a) {
	int idx = get_global_id(2);
	idx *= get_global_size(1);
	idx += get_global_id(1);
	idx *= get_global_size(0);
	idx += get_global_id(0);
	a[idx] = idx;
}

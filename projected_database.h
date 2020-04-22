__device__ int  device_last_itemset_cnt = 0;

__global__ void get_projected_Database(int* __restrict__  p, int* __restrict__  start, int* __restrict__ end, int total_row, int* __restrict__ items, int count, int* __restrict__  projected_databse, int* __restrict__  isItemset) {
	int idx = threadIdx.x + blockIdx.x * blockDim.x;
	if (idx < total_row) {
		int s = start[idx];
		int flag;
		int index, new_start;
		for (int i = 0; i < count; i++) {
			flag = 0;
			for (int j = s; j < end[idx]; j++) {
				if (items[i] == p[j]) {
					if (p[j + 1] == -1) {
						index = i * total_row + idx;
						new_start = s + (j - s) + 2;
						projected_databse[index] = new_start;
						isItemset[index] = 0;
						//printf("item:%d row:%d start:%d\n", items[i],idx, s + (j - s) + 2);
					}
					else
					{
						index = i * total_row + idx;
						new_start = s + (j - s) + 1;
						projected_databse[index] = new_start;
						isItemset[index] = 1;
						//printf("item:%d row:%d start:%d\n", items[i], idx, s + (j - s) + 1);
					}
					flag = 1;
					break;
				}

			}
			if (flag != 1) {
				index = i * total_row + idx;
				new_start = end[idx];
				//printf("item:%d row:%d start:%d\n", items[i], idx, end[idx]);
				projected_databse[index] = new_start;
				isItemset[index] = 0;
			}
		}
	}
}

__global__ void get_projected_Database_for_prefix(int* __restrict__ p, int* start, int* __restrict__  end, int total_row, int* __restrict__ items, int count, int* __restrict__ projected_databse, int* __restrict__ isItemset, int* __restrict__ isItemset_ptr, int* __restrict__ last_itemset, int last_itemset_cnt) {
	int idx = threadIdx.x + blockIdx.x * blockDim.x;
	if (idx < total_row) {
		int s = start[idx];
		int flag, flag2;
		int index, new_start;
		int isitemset = isItemset_ptr[idx];
		for (int i = 0; i < count; i++) {
			flag = 0, flag2=0;
			if (items[i] > 0) {
				for (int j = s; j < end[idx]; j++) {
					if (p[j] == -1 && flag2 == 0) {
						flag2 = 1;
					}
					if (items[i] == p[j]) {
						if (isitemset == 1 && flag2 == 0) {
							continue;
						}
						else {
							if (p[j + 1] == -1) {
								index = i * total_row + idx;
								new_start = s + (j - s) + 2;
								projected_databse[index] = new_start;
								isItemset[index] = 0;
								//printf("item:%d row:%d start:%d\n", items[i],idx, s + (j - s) + 2);
							}
							else {
								index = i * total_row + idx;
								new_start = s + (j - s) + 1;
								projected_databse[index] = new_start;
								isItemset[index] = 1;
								//printf("item:%d row:%d start:%d\n", items[i], idx, s + (j - s) + 1);
							}
							flag = 1;
							break;
						}
					}
				}
				if (flag != 1) {
					index = i * total_row + idx;
					new_start = end[idx];
					//printf("item:%d row:%d start:%d\n", items[i], idx, end[idx]);
					projected_databse[index] = new_start;
					isItemset[index] = 0;
				}

			}
			else {
				int item = -items[i];
				for (int j = s; j < end[idx]; j++) {
					if (isitemset == 1 && flag2 == 0) {
						if (p[j] == item) {

							if (p[j + 1] == -1) {
								index = i * total_row + idx;
								new_start = s + (j - s) + 2;
								projected_databse[index] = new_start;
								isItemset[index] = 0;
								//printf("item:%d row:%d start:%d\n", items[i],idx, s + (j - s) + 2);
							}
							else {
								index = i * total_row + idx;
								new_start = s + (j - s) + 1;
								projected_databse[index] = new_start;
								isItemset[index] = 1;
								//printf("item:%d row:%d start:%d\n", items[i], idx, s + (j - s) + 1);
							}
							flag = 1;
							flag2 = 1;
							break;
						}
					}
					else {
						if (p[j] == last_itemset[0]) {
							int cnt = 0;
							int k;
							for (k = j; p[k] != -1; k++) {
								if (p[k] == last_itemset[cnt]) {
									cnt++;
								}
							}
							if (cnt == last_itemset_cnt) {
								if (p[k + 1] == -1) {
									index = i * total_row + idx;
									new_start = s + (k - s) + 2;
									projected_databse[index] = new_start;
									isItemset[index] = 0;
									//printf("item:%d row:%d start:%d\n", items[i],idx, s + (j - s) + 2);
								}
								else {
									index = i * total_row + idx;
									new_start = s + (k - s) + 1;
									projected_databse[index] = new_start;
									isItemset[index] = 1;
									//printf("item:%d row:%d start:%d\n", items[i], idx, s + (j - s) + 1);
								}
								flag = 1;
								flag2 = 1;
								break;
							}
						}
					}
				}
				if (flag != 1) {
					index = i * total_row + idx;
					new_start = end[idx];
					//printf("item:%d row:%d start:%d\n", items[i], idx, end[idx]);
					projected_databse[index] = new_start;
					isItemset[index] = 0;
				}

			}


		}

	}
}

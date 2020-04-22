#define Number_of_items 500
__device__ __constant__ int minmum_support = 219;
__device__ int frequent_item[Number_of_items];
__device__ int  FreqCount = 0;
__device__ int freq[Number_of_items] = { 0 };
__device__ int freq1[Number_of_items * 2] = { 0 };

__device__ char my_push_back_freq(int* item) {
	int insert_pt = atomicAdd(&FreqCount, 1);
	if (insert_pt < Number_of_items) {
		frequent_item[insert_pt] = *item;
		return insert_pt;
	}
	else return -1;
}

__global__ void init_freq1(){
	int idx = threadIdx.x + blockIdx.x * blockDim.x;
	if(idx<Number_of_items * 2){
		freq1[idx]=0;
	}
}
__global__ void init_frequent_itemset_in_PD(int* __restrict__ frequent_itemset_in_PD){
	int idx = threadIdx.x + blockIdx.x * blockDim.x;
	if(idx<(Number_of_items * 2)){
		frequent_itemset_in_PD[idx]=0;
	}
}

__global__ void findFrequentItemSet(int* __restrict__ p, int* __restrict__  start, int* __restrict__ end, int total_row) {
	int idx = threadIdx.x + blockIdx.x * blockDim.x;

	if (idx < total_row) {
		int ar[(Number_of_items+1)/32] = { 0 };
		int s = start[idx];
		for (int i = s; i < end[idx]; i++) {
			int num = p[i];
			if(num != -1 && num !=-2){

				if ((bool)(ar[num>>5] & (1<<num%32))==0) {
					atomicAdd(&freq[num], 1);

				}
				ar[num>>5]=ar[num>>5] | (1<<num & 31);


			}
	}
	}
	if (idx == total_row) {
		for (int i = 0; i < Number_of_items; i++) {
			if (freq[i] >= minmum_support) {
				my_push_back_freq(new int(i));
			}
		}
	}
}


__global__ void findFrequentItemSet_From_projected_database(int* __restrict__ p, int* __restrict__  start, int* __restrict__  end, int total_row, int* __restrict__ current_prefix, int frequent_items_count, int* __restrict__ isItemset_ptr, int* __restrict__ frequent_itemset_in_PD) {
	int idx = threadIdx.x + blockIdx.x * blockDim.x;
//	int minmum_support=min_sup;
	if (idx < total_row) {
		int ar1[(Number_of_items * 2 + 1)/32] = { 0 };
		int s = start[idx];
		int isitemset = isItemset_ptr[idx];
		int flag = 0,items;
		for (int i = s; i < end[idx]; i++) {
			int num = p[i];
			if(num != -1 && num !=-2){
				if (isitemset == 1 && flag == 0) {
					if (p[i] != -1) {////checking itemset with current prefix
						items=num+Number_of_items;
						if ((bool)(ar1[items>>5] & (1<<items%32)) == 0) {
							atomicAdd(&freq1[items], 1);
							if (freq1[items] >= minmum_support) {
							frequent_itemset_in_PD[items] = 1;
							}
						}
						ar1[items>>5]=ar1[items>>5] | (1<<items & 31);
						continue;
					}
					else
					{
						flag = 1;
					}
				}
				if (p[i] == current_prefix[0]) {
					if (p[i + 1] != -1) {
						for (int j = i + 1; p[j] != -1; j++) {
							int num2 = p[j];
							items=num2+Number_of_items;
							if ((bool)(ar1[items>>5] & (1<<items%32)) == 0) {
								atomicAdd(&freq1[items], 1);
								if (freq1[items] >= minmum_support) {
									frequent_itemset_in_PD[items] = 1;
								}
							}
								ar1[items>>5]=ar1[items>>5] | (1<<items & 31);
						//	ar1[num2 + Number_of_items] = 1;
						}
					}
				}
				if ((bool)(ar1[num>>5] & (1<<num%32)) == 0) {
					atomicAdd(&freq1[num], 1);
					if (freq1[num] >= minmum_support) {
						frequent_itemset_in_PD[num] = 1;
					//	frequent_itemset_in_PD[num + Number_of_items] = 1;
					}

				}
				ar1[num>>5]=ar1[num>>5] | (1<<num & 31);
			//	ar1[num] = 1;
			}

		}
	}
}

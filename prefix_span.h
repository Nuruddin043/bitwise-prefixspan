#define NUM_OF_BLOCKS 920
using namespace std;


thrust::device_vector<int> Device_projected_database;
thrust::device_vector<int> Device_isItemset;
thrust::device_vector<int> Device_prefix;
thrust::host_vector<int> Hostfreq_item;
int fcount;
class  prefix_task {
public:
	thrust::host_vector <int> prefix;
	thrust::host_vector <int> projected_database;
	thrust::host_vector<int> isItemset;
	thrust::host_vector <int> frequent_item;
};
stack<prefix_task> next_to_work;
#define gpuErrchk(ans) { gpuAssert((ans), __FILE__, __LINE__); }
inline void gpuAssert(cudaError_t code, const char *file, int line)
{
    if (code != cudaSuccess) {
        fprintf(stderr, "GPUassert: %s %s %d\n",
                cudaGetErrorString(code), file, line);
        exit(code);
    }
}


void prefix_Span(int* dptr, int* startPtr, int* endPtr, int total_row,vector < vector<int> > &sequential_patterns) {
	thrust::device_vector<int> device_freq_item;
	thrust::device_vector<int> last_itemset;
	thrust::host_vector<int> Host_last_itemset;
	int frequent_items_count, last_itemset_cnt;
	///---------------------------getting 1 freqeunt item projected_databse --------------------------------//

	findFrequentItemSet << < (total_row / NUM_OF_BLOCKS) + 1, NUM_OF_BLOCKS >> > (dptr, startPtr, endPtr, total_row);
	gpuErrchk(cudaThreadSynchronize());

	gpuErrchk(cudaMemcpyFromSymbol(&frequent_items_count, FreqCount, sizeof(int)));
	gpuErrchk(cudaMemcpyFromSymbol(&last_itemset_cnt, device_last_itemset_cnt, sizeof(int)));
	int fre_item[Number_of_items];
	gpuErrchk(cudaMemcpyFromSymbol(&fre_item, frequent_item, sizeof(int) * frequent_items_count));
	for (int i = 0; i < frequent_items_count; i++) {
		Hostfreq_item.push_back(fre_item[i]);
	}
	device_freq_item = Hostfreq_item;
	int* frq_item_ptr = raw_pointer_cast(&device_freq_item[0]);
	int* projDB, * isItemsetCopy, * frequent_itemset_in_PD;
	gpuErrchk(cudaMallocManaged(&projDB, total_row * frequent_items_count * sizeof(int)));
	gpuErrchk(cudaMallocManaged(&isItemsetCopy, total_row * frequent_items_count * sizeof(int)));
	int fre_size = (2 * Number_of_items);
	gpuErrchk(cudaMallocManaged(&frequent_itemset_in_PD, fre_size * sizeof(int)));
	init_freq1 << < (Number_of_items * 2 / NUM_OF_BLOCKS) + 1, NUM_OF_BLOCKS >> > ();
	get_projected_Database << < (total_row / NUM_OF_BLOCKS) + 1, NUM_OF_BLOCKS >> > (dptr, startPtr, endPtr, total_row, frq_item_ptr, frequent_items_count, projDB, isItemsetCopy);
	gpuErrchk(cudaDeviceSynchronize());
	int index;
	int* new_start_ptr, * current_prefix, * isItemset_ptr;
	vector<int> temp;
		///---------------------------for each 1 freqeunt item finding frequent_item and pushing into stuck--------------------------------//
	for (int i = 0; i < frequent_items_count; i++) {
		prefix_task p;
		temp.push_back(fre_item[i]);
		temp.push_back(-1);
		sequential_patterns.push_back(temp);
		p.prefix.push_back(fre_item[i]);
		p.prefix.push_back(-1);
		for (int j = 0; j < total_row; j++) {
			index = i * total_row + j;
			p.projected_database.push_back(projDB[index]);
			p.isItemset.push_back(isItemsetCopy[index]);
		}
		Device_projected_database = p.projected_database;
		Device_isItemset = p.isItemset;
		Device_prefix = p.prefix;
		new_start_ptr = raw_pointer_cast(&Device_projected_database[0]);
		current_prefix = raw_pointer_cast(&Device_prefix[0]);
		isItemset_ptr = raw_pointer_cast(&Device_isItemset[0]);
		findFrequentItemSet_From_projected_database << < (total_row / NUM_OF_BLOCKS) + 1, NUM_OF_BLOCKS >> > (dptr, new_start_ptr, endPtr, total_row, current_prefix, frequent_items_count, isItemset_ptr, frequent_itemset_in_PD);
		init_freq1 << < (Number_of_items * 2 / NUM_OF_BLOCKS) + 1, NUM_OF_BLOCKS >> > ();

		cudaDeviceSynchronize();
		for (int j = 0; j < Number_of_items; j++) {
			if (frequent_itemset_in_PD[j] == 1) {
				p.frequent_item.push_back(j);
			}

		}
		for (int j = Number_of_items; j < Number_of_items * 2 + 1; j++) {
			if (frequent_itemset_in_PD[j] == 1) {
				p.frequent_item.push_back(-j + Number_of_items);
			}

		}

		temp.clear();
		next_to_work.push(p);
		init_frequent_itemset_in_PD << < (Number_of_items * 2 / NUM_OF_BLOCKS) + 1, NUM_OF_BLOCKS >> > (frequent_itemset_in_PD);
		cudaDeviceSynchronize();
	}
///---------------------------for each prefix in stuck find projected database in parrallel way and find frequent item for that projDB-------------------------------//
	while (!next_to_work.empty())
	{
		prefix_task currentPrefix;
		currentPrefix = next_to_work.top();
		next_to_work.pop();
		int size = currentPrefix.frequent_item.size();
		vector<prefix_task> nextPrefix(size);
		int ps = currentPrefix.prefix.size() - 2;
		int  cnt = 0;
		for (int i = ps; i >= 0; i--) {
			if (currentPrefix.prefix[i] != -1) {
				Host_last_itemset.push_back(currentPrefix.prefix[i]);
				cnt++;
			}
			else {
				break;
			}
		}
		for (int i = 0; i < currentPrefix.frequent_item.size(); i++) {
			nextPrefix[i].prefix = currentPrefix.prefix;
			int in = nextPrefix[i].prefix.size() - 1;
			if (currentPrefix.frequent_item[i] > 0) {
				nextPrefix[i].prefix.push_back(currentPrefix.frequent_item[i]);
				nextPrefix[i].prefix.push_back(-1);
			}
			else {
				nextPrefix[i].prefix[in] = abs(currentPrefix.frequent_item[i]);
				nextPrefix[i].prefix.push_back(-1);
			}
			for (int j = 0; j < nextPrefix[i].prefix.size(); j++) {
				temp.push_back(nextPrefix[i].prefix[j]);
			}
			sequential_patterns.push_back(temp);
			temp.clear();
		}
		///---------------------------projected database for all frequent item for same prefix projected datbase--------------------------------//
		Device_projected_database = currentPrefix.projected_database;
		device_freq_item = currentPrefix.frequent_item;
		int* frq_item_ptr = raw_pointer_cast(&device_freq_item[0]);
		frequent_items_count = currentPrefix.frequent_item.size();
		new_start_ptr = raw_pointer_cast(&Device_projected_database[0]);
		Device_isItemset = currentPrefix.isItemset;
		isItemset_ptr = raw_pointer_cast(&Device_isItemset[0]);
		last_itemset = Host_last_itemset;
		int* last_itemset_ptr = raw_pointer_cast(&last_itemset[0]);
		last_itemset_cnt = cnt;

		get_projected_Database_for_prefix << < (total_row / NUM_OF_BLOCKS) + 1, NUM_OF_BLOCKS >> > (dptr, new_start_ptr, endPtr, total_row, frq_item_ptr, frequent_items_count, projDB, isItemsetCopy, isItemset_ptr, last_itemset_ptr, last_itemset_cnt);
		gpuErrchk(cudaDeviceSynchronize());
		for (int i = 0; i < currentPrefix.frequent_item.size(); i++) {

			for (int j = 0; j < total_row; j++) {
				index = i * total_row + j;
				nextPrefix[i].projected_database.push_back(projDB[index]);
				nextPrefix[i].isItemset.push_back(isItemsetCopy[index]);
			}

			///---------------------------finding all frequent item for all projected databse for current prefix--------------------------------//
			Device_projected_database = nextPrefix[i].projected_database;
			Device_isItemset = nextPrefix[i].isItemset;
			Device_prefix = nextPrefix[i].prefix;
			new_start_ptr = raw_pointer_cast(&Device_projected_database[0]);
			current_prefix = raw_pointer_cast(&Device_prefix[0]);
			isItemset_ptr = raw_pointer_cast(&Device_isItemset[0]);
			//findFrequentItemSet_From_projected_database << < (total_row / 920) + 1, 920 >> > (dptr, new_start_ptr, endPtr, total_row, current_prefix, isItemset_ptr, frequent_itemset_in_PD);
			findFrequentItemSet_From_projected_database << < (total_row / NUM_OF_BLOCKS) + 1, NUM_OF_BLOCKS >> > (dptr, new_start_ptr, endPtr, total_row, current_prefix, frequent_items_count, isItemset_ptr, frequent_itemset_in_PD);
			init_freq1 << < (Number_of_items * 2 / NUM_OF_BLOCKS) + 1, NUM_OF_BLOCKS >> > ();
			cudaDeviceSynchronize();
			for (int j = 0; j < Number_of_items; j++) {
				if (frequent_itemset_in_PD[j] == 1) {
					nextPrefix[i].frequent_item.push_back(j);
				}

			}
			for (int j = Number_of_items; j < Number_of_items * 2 + 1; j++) {
				if (frequent_itemset_in_PD[j] == 1) {
					nextPrefix[i].frequent_item.push_back(-j + Number_of_items);
				}

			}

			next_to_work.push(nextPrefix[i]);
			init_frequent_itemset_in_PD << < (Number_of_items * 2 / NUM_OF_BLOCKS) + 1, NUM_OF_BLOCKS >> > (frequent_itemset_in_PD);
			cudaDeviceSynchronize();
			/*for (int k = 0; k<nextPrefix[i].prefix.size(); k++) {
				cout << nextPrefix[i].prefix[k]<<" ";
			}
			cout << endl;
			for (int k = 0; k < nextPrefix[i].projected_database.size(); k++) {
				cout <<" "<< nextPrefix[i].projected_database[k] << " " << nextPrefix[i].isItemset[k];
			}
			cout << endl;
			for (int k = 0; k < nextPrefix[i].frequent_item.size(); k++) {
				cout << nextPrefix[i].frequent_item[k] << " ";
			}
			cout << endl;
			cout << " ----------------------------------------------------"<<endl;*/

		}








		Host_last_itemset.clear();
		nextPrefix.clear();
	}



	gpuErrchk(cudaFree(projDB));
	gpuErrchk(cudaFree(isItemsetCopy));
	gpuErrchk(cudaFree(frequent_itemset_in_PD));

}

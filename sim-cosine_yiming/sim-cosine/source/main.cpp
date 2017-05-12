/*
	Similarity calculation
	DB 2014-04-77
*/
#include <ctime>
#include <cmath>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <functional>
#include <thread>
#include <boost/program_options.hpp>

#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/timeb.h>
#include <map>
#include <typeinfo>
#include <sys/types.h>


#include "similarity.hpp" /// create headers

using namespace std;

namespace {
	const size_t SUCCESS = 0;
	const size_t ERROR_IN_COMMAND_LINE = 1;
	const size_t ERROR_UNHANDLED_EXCEPTION = 2;
}
#define _LINE_LENGTH 300
//#define const NUM_OF_THREADS 30
const int NUM_OF_THREADS = 50;

typedef struct itemstruct
{
	size_t otherItem;
	rating_t score;
}itemS;

typedef pair<size_t, itemS> PAIR;

//variable needed for time specification.
float cpu=0;
size_t mem=0;
//    size_t l_mem=0;
int pid=0;
int tid=-1;
long long start,end1,end_sim,end_push,end_sort,end_itend,begin_sort1,end_sort1;
//multimap<size_t,itemS> neighbors1,neighbors2,neighbors3,
//		neighbors4,neighbors5,neighbors6;
multimap<size_t,itemS> neighbors[NUM_OF_THREADS];
//vector<PAIR> neighbors_vec1,neighbors_vec2,neighbors_vec3,
//		neighbors_vec4,neighbors_vec5,neighbors_vec6;
vector<PAIR> neighbors_vec[NUM_OF_THREADS];



/*
  Parse command line parameters
*/
size_t parse_params(int argc, char *argv[], string& ratings_path, unsigned
int& num_users, unsigned int& num_items, unsigned int& nearest_neighbors,
					float& asymmetric_alpha, float& supp_threshold, float& conf_threshold, unsigned
					int& locality_param, string& targets_path) {
	namespace po = boost::program_options;
	po::options_description desc("Program parameters");
	desc.add_options()
			("help,?", "produce help message")
			("num_users,u", po::value<unsigned int>(&num_users)->required(),
			"number of users")
	("num_items,i", po::value<unsigned int>(&num_items)->required(),
			"number of items")
	("ratings_matrix,r", po::value<string>(&ratings_path)->required(),
			"ratings matrix path")
			("target_users,t", po::value<string>(&targets_path)->required(),
			 "target users ratings matrix path")
			("nearest_neighbors,k",
			 po::value<unsigned int>(&nearest_neighbors)->default_value(0),
			"output exclusively the k nearest neighbors for each user")
	("asymmetric_alpha,a", po::value<float>(&asymmetric_alpha)->default_value(0.5),
			"asymmetric cosine parameter -- default: symmetric (0.5)")
			("supp_threshold,s", po::value<float>(&supp_threshold)->default_value(UNDEFTH),
			 "support threshold")
			("conf_threshold,c", po::value<float>(&conf_threshold)->default_value(UNDEFTH),
			 "confiance threshold")
			("locality_param,q",
			 po::value<unsigned int>(&locality_param)->default_value(1),
			"transforms the similarity as follows: x->x^q")
	;
	po::variables_map vm;
	try {
		po::store(po::parse_command_line(argc, argv, desc), vm);
		if (vm.count("help")) {
			cerr << desc << endl;
			return ERROR_IN_COMMAND_LINE;
		}
		// if (vm.count("asymmetric_alpha") && (asymmetric_alpha < 0.0 || asymmetric_alpha > 1.0)) {
		// 	cerr << "Error: asymmetric_alpha value must be in [0,1].\n" << desc << endl;
		// 	return ERROR_IN_COMMAND_LINE;
		// }
		po::notify(vm);
	}
	catch(const std::exception& e) {
		cerr << "Error: " << e.what() << '\n' << desc << endl;
		return ERROR_IN_COMMAND_LINE;
	}
	return SUCCESS;
}

inline string errline(char const* msg, string& line) {
	return string(msg) + " -- Line: " + line;
}

/*
  Import a vector from a text file. Args: vector to fill, text file path.
*/
// template <typename val_t>
// void importIntVector(vector<val_t> &vec, string path) {
// 	val_t val;
// 	string line, eline;
// 	ifstream infile(path);

// 	if (!infile.is_open()) {
// 		throw string("Unable to open file: ")+path;
// 	}
// 	while (getline(infile, line)) {
// 		istringstream iss(line);
// 		if ( !(iss >> val) ) {
// 			throw errline("Unable to parse line",line);
// 		}
// 		vec.push_back(val);
// 	}
// 	infile.close();
// }


/*
 Import a sparse matrix from text file as a vector of hashes, each
 corresponding a matrix row.
 Args: vector to fill, text file path. Returns: number of entries read
*/
template <typename val_t, typename Trip>
int importTriplets(std::vector<Trip>& triplets, const string& path) {
	val_t val;
	long row, col, entries=0;
	string line;
	ifstream infile(path);

	if (!infile.is_open()) {
		throw string("Unable to open file: ")+path;
	}

	while (getline(infile, line)) {
		istringstream iss(line);
		if ( !(iss >> row >> col >> val) ) {
			throw errline("Unable to parse line",line);
		} else if (row < 0 || col < 0 ) {
			throw errline("Coordinates out of bound",line);
		} else {
			triplets.push_back(Trip((unsigned long)row,(unsigned long)col,val));
		}
		entries++;
	}
	infile.close();
	return entries;
}

/*
  Returns a string containing a timestamp in given format
*/
string now(const char* format = "%I:%M:%S %p %Z") {
	std::time_t t = time(NULL);
	char cstr[256];
	std::strftime(cstr, sizeof(cstr), format, std::localtime(&t));
	return cstr;
}

//template<template <typename> class P = std::less >
//struct compare_pair_second {
//	template<class T1, class T2> bool operator()
//	(const std::multimap<T1, T2>& left, const std::multimap<T1, T2>& right) {
//		return P<T2>()(left.second, right.second);
//	}
//};

//template<template <typename> class P = std::less >
//struct RankbyScore {
//	bool operator()
//	(const multimap<size_t, itemS>::iterator iter1, const multimap<size_t, itemS>::iterator iter2) {
//		return iter1->second.score > iter2->second.score;
//	}
//};

//bool RankbyScore(const multimap<size_t, itemS>::iterator iter1,
//                 const multimap<size_t, itemS>::iterator iter2) {
//        return iter1->second.score > iter2->second.score;
//};


bool RankbyScore(const PAIR left, const PAIR right) {
	if (left.first == right.first){
		return left.second.score > right.second.score;
	}
	else{
		return left.first < right.first;
	}
};




size_t get_executable_path( char* processdir,char* processname, size_t len)
{
	char* path_end;
	if(readlink("/proc/self/exe", processdir,len) <=0)
		return -1;
	path_end = strrchr(processdir,  '/');
	if(path_end == NULL)
		return -1;
	++path_end;
	strcpy(processname, path_end);
	*path_end = '\0';
	return (size_t)(path_end - processdir);
}


bool GetCpuMem(float &cpu,size_t &mem, int pid,int tid = -1)
{
	bool ret = false;
	char cmdline[100];
	sprintf(cmdline, "ps -o %%cpu,rss,%%mem,pid,tid -mp %d", pid);
	FILE *file;
	file = popen(cmdline, "r");
	if (file == NULL)
	{
		printf("file == NULL\n");
		return false;
	}

	char line[_LINE_LENGTH];
	float l_cpuPrec=0;
	int l_mem=0;
	float l_memPrec=0;
	int l_pid=0;
	int l_tid=0;
	if (fgets(line, _LINE_LENGTH, file) != NULL)
	{
		//  printf("1st line:%s",line);
		if (fgets(line, _LINE_LENGTH, file) != NULL)
		{
			//      printf("2nd line:%s",line);
			sscanf( line, "%f %d %f %d -", &l_cpuPrec, &l_mem, &l_memPrec, &l_pid );
			cpu = l_cpuPrec;
//            mem = l_mem/1024;
			mem = l_mem /1;
			if( tid == -1 )
				ret = true;
			else
			{
				while( fgets(line, _LINE_LENGTH, file) != NULL )
				{
					sscanf( line, "%f - - - %d", &l_cpuPrec, &l_tid );
					//              printf("other line:%s",line);
					//              cout<<l_cpuPrec<<'\t'<<l_tid<<endl;
					if( l_tid == tid )
					{
						printf("cpuVal is tid:%d\n",tid);
						cpu = l_cpuPrec;
						ret = true;
						break;
					}
				}
				if( l_tid != tid )
					printf("TID not exist\n");
			}
		}
		else
			printf("PID not exist\n");
	}
	else
		printf("Command or Parameter wrong\n");
	pclose(file);
	return ret;
}

long long getSystemTime() {
	struct timeb t;
	ftime(&t);
	return 1000 * t.time + t.millitm;
}

void compute_similarity(int division, int group,
						SparseXf ratings, SparseXf targets, unsigned locality_param,
						float asymmetric_alpha, float supp_threshold, unsigned int nearest_neighbors){
	cerr << "Computing Similarity......." << endl;

//	for (ptrdiff_t row=0; ((long(row % division) == long(group))&&(row < targets.outerSize())); ++row) {
	for (ptrdiff_t row=0; (row < targets.outerSize()); ++row) {
//		cerr << " " << row << " ";
//        cerr << "group is: " << group << ", division is: " << division << ", row % division is: " << row % division << endl;
//        cerr << "type of row % division: " << typeid(row % division).name() << ", type of group: " << typeid(long(group)).name() << endl;
		if (long(row % division) == long(group)){
			for (size_t k=0; k < ratings.outerSize(); ++k) {
//			 cerr << row << ' ' << k << ' ' << targets.innerVector(row).nonZeros()
//			  << ' ' <<  ratings.innerVector(k).nonZeros() << ' ' << sim << '\n';
				if (targets.innerVector(row).nonZeros() &&
					ratings.innerVector(k).nonZeros()) {
					if ((row == 10)&&(k == 10)) {
						start = getSystemTime();
					}

					innerVec tar = targets.innerVector(row), rat = ratings.innerVector(k);
					if ((row == 10)&&(k == 10)) {
						GetCpuMem(cpu, mem, pid, tid);
						cerr << "Compute tar and rat: " << "CPU: " << cpu << " " << "MEM: " << mem;
						end1 = getSystemTime();
						cerr << " Time for computing tar and rat: " << end1 - start << "ms" << endl;
					}

					rating_t sim = cosine_sim(tar,rat,
											  asymmetric_alpha, locality_param, supp_threshold);
//				cerr << "The " << row << "st round, Similarity is " << sim << endl;
					if ((row == 10)&&(k == 10)) {
						GetCpuMem(cpu, mem, pid, tid);
						cerr << "Similarities: " << "CPU: " << cpu << " " << "MEM: " << mem;
						end_sim = getSystemTime();
						cerr << " Time for computing Similarities: " << end_sim - end1 << "ms" << endl;
					}
					if (sim > 0.0) {
//					pair<size_t,size_t> newpair(k,sim);

//					std::pair<size_t, rating_t> newpair;
//					newpair = make_pair(k,sim);
//					neighbors.insert(row,newpair);

//					itemS newpair(k,sim);
						itemS newpair = {k,sim};
						for (int k = 1; k < NUM_OF_THREADS; k++){
							if (group == k){
								neighbors[k].insert(make_pair(row,newpair));
							}
						}
//						if (group == 1){
//							neighbors[1].insert(make_pair(row,newpair));
//						}
//						if (group == 2){
//							neighbors[2].insert(make_pair(row,newpair));
//						}
//						if (group == 3){
//							neighbors[3].insert(make_pair(row,newpair));
//						}
//						if (group == 4){
//							neighbors4.insert(make_pair(row,newpair));
//						}
//						if (group == 5){
//							neighbors5.insert(make_pair(row,newpair));
//						}
//						if (group == 0){
//							neighbors6.insert(make_pair(row,newpair));
//						}

//                        neighbors.insert(make_pair(row,newpair));
					}
					if ((row == 10)&&(k == 10)) {
						GetCpuMem(cpu, mem, pid, tid);
						cerr << "For pushing back(insert): " << "CPU: " << cpu << " " << "MEM: " << mem;
						end_push = getSystemTime();
						cerr << " Time for pushing back(insert): " << end_push - end_sim << "ms" << endl;
					}
				}
				// cout << row << ' ' << k << ' ' << targets.innerVector(row).nonZeros()
				//  << ' ' <<  ratings.innerVector(k).nonZeros() << ' ' << sim << '\n';
				// cout << targets.innerVector(row) << ratings.innerVector(k) << "\n\n";
			}

//			cerr << "Sort done." << endl;
		}

//		neighbors.clear(); //for single threads.
	}
    for (int i = 1; i < NUM_OF_THREADS; i++){
		if (group == i){
			begin_sort1 = getSystemTime();
			vector<PAIR> neighbors_vec_tmp(neighbors[i].begin(), neighbors[i].end());
//			typedef map <size_t,itemS> MapType;
//			for( MapType::iterator itm = neighbors[i].begin(); itm != neighbors[i].end(); ++itm ) {
//				neighbors_vec1.push_back(itm->second);
//			}

			sort(neighbors_vec_tmp.begin(),neighbors_vec_tmp.end(),RankbyScore);
//			for(auto k:neighbors_vec1){
//				neighbors_vec[i].push_back(k);
//			}
			neighbors_vec_tmp.swap(neighbors_vec[i]);

			GetCpuMem(cpu, mem, pid, tid);
			cerr << "Sorting for group " << i << ": CPU: " << cpu << " " << "MEM: " << mem;
			end_sort1 = getSystemTime();
			cerr << " Time for sorting group " << i <<" : " <<  end_sort1 - begin_sort1 << "ms" << endl;
		}
	}




//	if (group == 1){
//		begin_sort1 = getSystemTime();
//		vector<PAIR> neighbors_vec[1](neighbors1.begin(), neighbors1.end());
////				cerr << "Vector copy finishes." << endl;
//		sort(neighbors_vec1.begin(),neighbors_vec1.end(),RankbyScore);
//
//		GetCpuMem(cpu, mem, pid, tid);
//		cerr << "Sorting for group 1: " << "CPU: " << cpu << " " << "MEM: " << mem;
//		end_sort1 = getSystemTime();
//		cerr << " Time for sorting group 1: " << end_sort1 - begin_sort1 << "ms" << endl;
//	}
//	if (group == 2){
//		vector<PAIR> neighbors_vec2(neighbors2.begin(), neighbors2.end());
////				cerr << "Vector copy finishes." << endl;
//		sort(neighbors_vec2.begin(),neighbors_vec2.end(),RankbyScore);
//	}
//	if (group == 3){
//		vector<PAIR> neighbors_vec3(neighbors3.begin(), neighbors3.end());
////				cerr << "Vector copy finishes." << endl;
//		sort(neighbors_vec3.begin(),neighbors_vec3.end(),RankbyScore);
//	}
//	if (group == 4){
//		vector<PAIR> neighbors_vec4(neighbors4.begin(), neighbors4.end());
////				cerr << "Vector copy finishes." << endl;
//		sort(neighbors_vec4.begin(),neighbors_vec4.end(),RankbyScore);
//	}
//	if (group == 5){
//		vector<PAIR> neighbors_vec5(neighbors5.begin(), neighbors5.end());
////				cerr << "Vector copy finishes." << endl;
//		sort(neighbors_vec5.begin(),neighbors_vec5.end(),RankbyScore);
//
//		GetCpuMem(cpu, mem, pid, tid);
//		cerr << "Sorting for group 5: " << "CPU: " << cpu << " " << "MEM: " << mem;
//	}
//	if (group == 6){
//		vector<PAIR> neighbors_vec6(neighbors6.begin(), neighbors6.end());
////				cerr << "Vector copy finishes." << endl;
//		sort(neighbors_vec6.begin(),neighbors_vec6.end(),RankbyScore);
//	}

	cerr << endl << "Computing Similarity for group " << group << "Done." << endl;


}


void printResult(unsigned int nearest_neighbors){
	//print the result.
	cerr << "print result procedure." << endl;
	multimap<size_t,itemS>::iterator iter,iter1,iter2;
//	vector<PAIR> neighbors_vec(neighbors.begin(), neighbors.end());
//	cerr << "Vector copy finishes." << endl;
//	sort(neighbors_vec.begin(),neighbors_vec.end(),RankbyScore);
//	cerr << "Sort done." << endl;


	GetCpuMem(cpu, mem, pid, tid);
	cerr << "For Sorting: " << "CPU: " << cpu << " " << "MEM: " << mem;
	end_sort = getSystemTime();
	cerr << " Time for Sorting afterwards: " << end_sort - end_push << "ms" << endl;

//	auto itend = (nearest_neighbors && nearest_neighbors < neighbors.size())?
//				 neighbors.begin()+nearest_neighbors : neighbors.end();
	int neighbors_size_total = 0;
	for (int i =1; i < NUM_OF_THREADS; i++){
		neighbors_size_total += neighbors_vec[i].size();
	}
	cerr << "Number of neighbours size total is: " << neighbors_size_total;
	int itend_int = (nearest_neighbors && nearest_neighbors < neighbors_size_total)?
					nearest_neighbors : neighbors_size_total;
	cerr << "nearest neighbour is:" << nearest_neighbors << endl;
	cerr << "itend_int is: " << itend_int << endl;
//	for(auto kv = neighbors.begin(); kv != itend; ++kv) {
//
//	//Waiting to limit the output to nearest neighbors
////	for(auto kv = neighbors.begin(); kv != neighbors.end(); ++kv) {
////		if (row != (long)kv->first) {
//			cout << neighbors->first.first << ' ' << neighbors->second.first << ' ' << neighbors->second.second << '\n';
////		}
//	}


	cerr << "the size of neighbor is: " << neighbors_size_total << endl;
//	//Multimap output.
//	for(iter = neighbors.begin(); iter != neighbors.end(); ++iter)
//	{
//		cout << iter->first << ' ' << iter->second.otherItem << ' ' << iter->second.score << '\n';
//        cerr << iter->first << ' ' << iter->second.otherItem << ' ' << iter->second.score << '\n';
//		if (iter == neighbors.begin()){
//			cerr << iter->first << ' ' << iter->second.otherItem << ' ' << iter->second.score << '\n';
//		}
//
//	}


//	//This could be revised.
//	itend_int = 100;
//	//Multimap_vector output.
//	for (int j = 1; j < NUM_OF_THREADS; j++){
//		for (int i = 0; i != itend_int; ++i) {
//			cerr << "hello " << i;
//		if (neighbors_vec[j][i].first != neighbors_vec[j][i].second.otherItem){
//			cerr << "Not the same comparison.";
//			cerr << neighbors_vec[j][i].first << ' neighbors_vec[j][i].first finished ' << neighbors_vec[j][i].second.otherItem
//				 << ' neighbors_vec[j][i].second.otherItem finished ' << neighbors_vec[j][i].second.score << endl;
//		}
//		//Notice to change to cout
//		}
//	}





//	for (int i = 1; i != itend_int; ++i) {
//		cerr << "hello " << i;
////		if (neighbors_vec1[i].first != neighbors_vec1[i].second.otherItem){
//		cerr << "Not the same comparison.";
//		cerr << neighbors_vec1[i].first << ' neighbors_vec1[i].first finished ' << neighbors_vec1[i].second.otherItem
//			 << ' neighbors_vec1[i].second.otherItem finished ' << neighbors_vec1[i].second.score << endl;
////		}
//	}
//	cerr << "1";
//	for (int i = 0; i != itend_int; ++i) {
//		if (neighbors_vec2[i].first != neighbors_vec2[i].second.otherItem){
//			cout << neighbors_vec2[i].first << ' ' << neighbors_vec2[i].second.otherItem
//				 << ' ' << neighbors_vec2[i].second.score << endl;
//		}
//	}
//	cerr << "1";
//	for (int i = 0; i != itend_int; ++i) {
//		if (neighbors_vec3[i].first != neighbors_vec3[i].second.otherItem){
//			cout << neighbors_vec3[i].first << ' ' << neighbors_vec3[i].second.otherItem
//				 << ' ' << neighbors_vec3[i].second.score << endl;
//		}
//	}
//	cerr << "1";
//	for (int i = 0; i != itend_int; ++i) {
//		if (neighbors_vec4[i].first != neighbors_vec4[i].second.otherItem){
//			cout << neighbors_vec4[i].first << ' ' << neighbors_vec4[i].second.otherItem
//				 << ' ' << neighbors_vec4[i].second.score << endl;
//		}
//	}
//	cerr << "1";
//	for (int i = 0; i != itend_int; ++i) {
//		if (neighbors_vec5[i].first != neighbors_vec5[i].second.otherItem){
//			cout << neighbors_vec5[i].first << ' ' << neighbors_vec5[i].second.otherItem
//				 << ' ' << neighbors_vec5[i].second.score << endl;
//		}
//	}
//	cerr << "1";
//	for (int i = 0; i != itend_int; ++i) {
//		if (neighbors_vec6[i].first != neighbors_vec6[i].second.otherItem){
//			cout << neighbors_vec6[i].first << ' ' << neighbors_vec6[i].second.otherItem
//				 << ' ' << neighbors_vec6[i].second.score << endl;
//		}
//	}

//	if (row == 10) {
//		GetCpuMem(cpu, mem, pid, tid);
//		cerr << "For compute itend and check: " << "CPU: " << cpu << " " << "MEM: " << mem;
//		end_itend = getSystemTime();
//		cerr << " Time for itend and check: " << end_itend - end_sort << "ms" << endl;
//	}
}

int main (int argc, char *argv[]) {

	int division = NUM_OF_THREADS;
	int group;


	char path[PATH_MAX];
	char processname[1024];
	get_executable_path(path, processname, sizeof(path));
//    printf("directory:%s\nprocessname:%s\n",path,processname);
//    cerr << now() << "directory:%s\nprocessname:%s\n",path,processname;
	auto n_core = thread::hardware_concurrency();//获取cpu核心个数
	cerr << "Number of CPU core: " << n_core << endl;

	// Parameters
	unsigned int num_users, num_items, nearest_neighbors, locality_param;
	float asymmetric_alpha, supp_threshold, conf_threshold;
	string ratings_path, targets_path;
	pid = getpid();
	cerr << now() << " pid is:" << pid << endl;
	GetCpuMem( cpu, mem, pid, tid );
	cerr << now() << " Before parsing, " << "CPU: " << cpu << " " << "MEM: " << mem << endl;




	if (parse_params(argc, argv, ratings_path, num_users, num_items,
					 nearest_neighbors, asymmetric_alpha, supp_threshold, conf_threshold,
					 locality_param,targets_path)) {
		return ERROR_IN_COMMAND_LINE;
	}

	// setup I/O
	SparseXf ratings(num_users,num_items);
	SparseXf targets(num_users,num_items);
	try {
//        cerr << now() << " directory:%s\nprocessname:%s\n",path,processname;
		size_t entries = 0;
		std::vector<Triplet<rating_t> > triplets;

		cerr << now() << " Loading rating matrix..... ";
		entries = importTriplets<rating_t, Triplet<rating_t> >(triplets, ratings_path);
		ratings.setFromTriplets(triplets.begin(), triplets.end());
		cerr << entries << " entries loaded." << endl;
		GetCpuMem( cpu, mem, pid, tid );
		cerr << now() << " After loading entries, " << "CPU: " << cpu << " " << "MEM: " << mem << endl;

		triplets.clear();
		cerr << now() << " Loading target user rating matrix... ";
		entries = importTriplets<rating_t, Triplet<rating_t> >(triplets, targets_path);
		targets.setFromTriplets(triplets.begin(), triplets.end());
		cerr << entries << " entries loaded." << endl;
		GetCpuMem( cpu, mem, pid, tid );
		cerr << now() << " After loading target user rating matrix, " << "CPU: " << cpu << " " << "MEM: " << mem << endl;
	} catch (string errmsg) {
		cerr << "Error: " << errmsg << endl;
		terminate();
	}

//    CPU_OCCUPY cpu_stat1;
//    CPU_OCCUPY cpu_stat2;
//    MEM_OCCUPY mem_stat;
//    int cpu;

	//Print the mem used now.
//    cerr << now() << " Memory used now is:" << get_memoccupy ((MEM_OCCUPY *)&mem_stat) << endl;


	// generate similarity measures
	cerr << now() << " Generating similarity measures..." << endl;
//	rating_t sim;
//	vector< pair<size_t, rating_t> > neighbors;


	cerr << "Target outsize is: " << targets.outerSize() << endl;
	cerr << "Rating outsize is: " << ratings.outerSize() << endl;

//	std::thread t1(compute_similarity, division, 1,
//				   ratings, targets, locality_param,
//				   asymmetric_alpha, supp_threshold, nearest_neighbors);
//	std::thread t2(compute_similarity, division, 2,
//				   ratings, targets, locality_param,
//				   asymmetric_alpha, supp_threshold, nearest_neighbors);
//	std::thread t3(compute_similarity, division, 3,
//				   ratings, targets, locality_param,
//				   asymmetric_alpha, supp_threshold, nearest_neighbors);
//	std::thread t4(compute_similarity, division, 4,
//				   ratings, targets, locality_param,
//				   asymmetric_alpha, supp_threshold, nearest_neighbors);
//	std::thread t5(compute_similarity, division, 5,
//				   ratings, targets, locality_param,
//				   asymmetric_alpha, supp_threshold, nearest_neighbors);
////	std::thread t6(compute_similarity, division, 0, std::ref(neighbors),
////				   ratings, targets, locality_param,
////				   asymmetric_alpha, supp_threshold, nearest_neighbors);
//	t1.join();
//	t2.join();
//	t3.join();
//	t4.join();
//	t5.join();
////	t6.join();

	std::thread threads[NUM_OF_THREADS];
	for (int i = 0; i < NUM_OF_THREADS; i++) {
		threads[i] = std::thread(compute_similarity, division, i+1,
								 ratings, targets, locality_param,
								 asymmetric_alpha, supp_threshold, nearest_neighbors);
	}
	std::cout << "Done spawning threads! Now wait for them to join\n";
	for (auto& t: threads) {
		t.join();
	}




	cerr << now() << " Generating similarity measures done." << endl;

//	compute_similarity(division, 1, neighbors, ratings, targets, locality_param,
//					   asymmetric_alpha,supp_threshold,nearest_neighbors);
	printResult(nearest_neighbors);



	cerr << now() << " Done." << endl;

	// exit properly
	return SUCCESS;
}
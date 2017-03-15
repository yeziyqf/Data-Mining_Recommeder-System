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
#include <boost/program_options.hpp>

#include <limits.h>
#include <stdio.h>
#include <unistd.h>

#include "similarity.hpp" /// create headers

using namespace std;

namespace { 
  const size_t SUCCESS = 0; 
  const size_t ERROR_IN_COMMAND_LINE = 1; 
  const size_t ERROR_UNHANDLED_EXCEPTION = 2;  
}
#define _LINE_LENGTH 300
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

template<template <typename> class P = std::less >
struct compare_pair_second {
	template<class T1, class T2> bool operator()
	(const std::pair<T1, T2>& left, const std::pair<T1, T2>& right) {
		return P<T2>()(left.second, right.second);
	}
};

//typedef struct PACKED         //定义一个cpu occupy的结构体
//{
//    char name[20];      //定义一个char类型的数组名name有20个元素
//    unsigned int user; //定义一个无符号的int类型的user
//    unsigned int nice; //定义一个无符号的int类型的nice
//    unsigned int system;//定义一个无符号的int类型的system
//    unsigned int idle; //定义一个无符号的int类型的idle
//}CPU_OCCUPY;
//
//typedef struct PACKED         //定义一个mem occupy的结构体
//{
//    char name[20];      //定义一个char类型的数组名name有20个元素
//    unsigned long total;
//    char name2[20];
//    unsigned long free;
//}MEM_OCCUPY;
//
//void get_memoccupy (MEM_OCCUPY *mem) //对无类型get函数含有一个形参结构体类弄的指针O
//{
//    FILE *fd;
//    int n;
//    char buff[256];
//    MEM_OCCUPY *m;
//    m=mem;
//
//    fd = fopen ("/proc/meminfo", "r");
//
//    fgets (buff, sizeof(buff), fd);
//    fgets (buff, sizeof(buff), fd);
//    fgets (buff, sizeof(buff), fd);
//    fgets (buff, sizeof(buff), fd);
//    sscanf (buff, "%s %u %s", m->name, &m->total, m->name2);
//
//    fgets (buff, sizeof(buff), fd); //从fd文件中读取长度为buff的字符串再存到起始地址为buff这个空间里
//    sscanf (buff, "%s %u", m->name2, &m->free, m->name2);
//
//    fclose(fd);     //关闭文件fd
//}
//
//int cal_cpuoccupy (CPU_OCCUPY *o, CPU_OCCUPY *n)
//{
//    unsigned long od, nd;
//    unsigned long id, sd;
//    int cpu_use = 0;
//
//    od = (unsigned long) (o->user + o->nice + o->system +o->idle);//第一次(用户+优先级+系统+空闲)的时间再赋给od
//    nd = (unsigned long) (n->user + n->nice + n->system +n->idle);//第二次(用户+优先级+系统+空闲)的时间再赋给od
//
//    id = (unsigned long) (n->user - o->user);    //用户第一次和第二次的时间之差再赋给id
//    sd = (unsigned long) (n->system - o->system);//系统第一次和第二次的时间之差再赋给sd
//    if((nd-od) != 0)
//        cpu_use = (int)((sd+id)*10000)/(nd-od); //((用户+系统)乖100)除(第一次和第二次的时间差)再赋给g_cpu_used
//    else cpu_use = 0;
//    //printf("cpu: %u\n",cpu_use);
//    return cpu_use;
//}
//
//void get_cpuoccupy (CPU_OCCUPY *cpust) //对无类型get函数含有一个形参结构体类弄的指针O
//{
//    FILE *fd;
//    int n;
//    char buff[256];
//    CPU_OCCUPY *cpu_occupy;
//    cpu_occupy=cpust;
//
//    fd = fopen ("/proc/stat", "r");
//    fgets (buff, sizeof(buff), fd);
//
//    sscanf (buff, "%s %u %u %u %u", cpu_occupy->name, &cpu_occupy->user, &cpu_occupy->nice,&cpu_occupy->system, &cpu_occupy->idle);
//
//    fclose(fd);
//}

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


int main (int argc, char *argv[]) {
    float cpu=0;
    size_t mem=0;
    size_t l_mem=0;
    int pid=0;
    int tid=-1;

    char path[PATH_MAX];
    char processname[1024];
    get_executable_path(path, processname, sizeof(path));
//    printf("directory:%s\nprocessname:%s\n",path,processname);
    cerr << now() << "directory:%s\nprocessname:%s\n",path,processname;

	// Parameters
	unsigned int num_users, num_items, nearest_neighbors, locality_param;
	float asymmetric_alpha, supp_threshold, conf_threshold;
	string ratings_path, targets_path;
    pid = getpid();
    cerr << now() << " pid is:" << pid << endl;
    GetCpuMem( cpu, mem, pid, tid );
    cerr << now() << "Before parsing, " << "CPU:" << cpu << " " << "MEM:" << mem << endl;




    if (parse_params(argc, argv, ratings_path, num_users, num_items,
		nearest_neighbors, asymmetric_alpha, supp_threshold, conf_threshold,
		locality_param,targets_path)) {
		return ERROR_IN_COMMAND_LINE;
	}

	// setup I/O
	SparseXf ratings(num_users,num_items);
	SparseXf targets(num_users,num_items);
	try {
        cerr << now() << "directory:%s\nprocessname:%s\n",path,processname;
		size_t entries = 0;
		std::vector<Triplet<rating_t> > triplets;

		cerr << now() << " Loading rating matrix..... ";
		entries = importTriplets<rating_t, Triplet<rating_t> >(triplets, ratings_path);
		ratings.setFromTriplets(triplets.begin(), triplets.end());
		cerr << entries << " entries loaded." << endl;
        GetCpuMem( cpu, mem, pid, tid );
        cerr << now() << "After loading entries, " << "CPU:" << cpu << " " << "MEM:" << mem << endl;

		triplets.clear();
		cerr << now() << " Loading target user rating matrix... ";
		entries = importTriplets<rating_t, Triplet<rating_t> >(triplets, targets_path);
		targets.setFromTriplets(triplets.begin(), triplets.end());
		cerr << entries << " entries loaded." << endl;
        GetCpuMem( cpu, mem, pid, tid );
        cerr << now() << "After loading target user rating matrix, " << "CPU:" << cpu << " " << "MEM:" << mem << endl;
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
	rating_t sim;
	vector< pair<size_t, rating_t> > neighbors;
	for (ptrdiff_t row=0; row < targets.outerSize(); ++row) {
		for (ptrdiff_t k=0; k < ratings.outerSize(); ++k) {
			// cout << row << ' ' << k << ' ' << targets.innerVector(row).nonZeros()
			//  << ' ' <<  ratings.innerVector(k).nonZeros() << ' ' << sim << '\n';
			if (targets.innerVector(row).nonZeros() && 
				ratings.innerVector(k).nonZeros()) {
				innerVec tar=targets.innerVector(row), rat=ratings.innerVector(k);
				sim = cosine_sim(tar,rat,
							 asymmetric_alpha, locality_param, supp_threshold);
				if (sim > 0.0) {
					neighbors.push_back(pair<size_t, float>(k,sim));	
				}
			}
			// cout << row << ' ' << k << ' ' << targets.innerVector(row).nonZeros()
			//  << ' ' <<  ratings.innerVector(k).nonZeros() << ' ' << sim << '\n';
			// cout << targets.innerVector(row) << ratings.innerVector(k) << "\n\n";
		}
		sort(neighbors.begin(),neighbors.end(),compare_pair_second<greater>());
		auto itend = (nearest_neighbors && nearest_neighbors < neighbors.size())?
			neighbors.begin()+nearest_neighbors : neighbors.end();
		for(auto kv = neighbors.begin(); kv != itend; ++kv) {
			if (row != (long)kv->first) {
				cout << row << ' ' << kv->first << ' ' << kv->second << '\n';
			}
		}
        if (row == 10) {
            GetCpuMem( cpu, mem, pid, tid );
            cerr << now() << "Similarity " << row << "st round, "
                 << "CPU:" << cpu << " " << "MEM:" << mem << endl;
        }
		neighbors.clear();
	}
	cerr << now() << " Done." << endl;

	// exit properly
	return SUCCESS;
}

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

#include "similarity.hpp" /// create headers

using namespace std;

namespace { 
  const size_t SUCCESS = 0; 
  const size_t ERROR_IN_COMMAND_LINE = 1; 
  const size_t ERROR_UNHANDLED_EXCEPTION = 2;  
}

const double SIM_THRESHOLD = 0.00001;

/*
  Parse command line parameters
*/
size_t parse_params(int argc, char *argv[], string& ratings_path, unsigned
 int& num_users, unsigned int& num_items, unsigned int& nearest_neighbors,
 float& supp_threshold, float& conf_threshold, string& targets_path) {
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
		("supp_threshold,s", po::value<float>(&supp_threshold)->default_value(UNDEFTH),
			"support threshold")
		("conf_threshold,c", po::value<float>(&conf_threshold)->default_value(UNDEFTH),
			"confiance threshold")
	;
	po::variables_map vm;
	try {
		po::store(po::parse_command_line(argc, argv, desc), vm);
		if (vm.count("help")) {
			cerr << desc << endl;
			return ERROR_IN_COMMAND_LINE;
		}
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

int main (int argc, char *argv[]) {
	// Parameters
	unsigned int num_users, num_items, nearest_neighbors;
	float supp_threshold, conf_threshold;
	string ratings_path, targets_path;

	if (parse_params(argc, argv, ratings_path, num_users, num_items,
		nearest_neighbors, supp_threshold, conf_threshold,targets_path)) {
		return ERROR_IN_COMMAND_LINE;
	}

	// setup I/O
	SparseXf ratings(num_users,num_items);
	SparseXf targets(num_users,num_items);
	try {
		size_t entries = 0;
		std::vector<Triplet<rating_t> > triplets;

		cerr << now() << " Loading rating matrix... ";
		entries = importTriplets<rating_t, Triplet<rating_t> >(triplets, ratings_path);
		ratings.setFromTriplets(triplets.begin(), triplets.end());
		cerr << entries << " entries loaded." << endl;

		triplets.clear();
		cerr << now() << " Loading target user rating matrix... ";
		entries = importTriplets<rating_t, Triplet<rating_t> >(triplets, targets_path);
		targets.setFromTriplets(triplets.begin(), triplets.end());
		cerr << entries << " entries loaded." << endl;
	} catch (string errmsg) {
		cerr << "Error: " << errmsg << endl;
		terminate();
	}

	// generate similarity measures
	cerr << now() << " Generating similarity measures..." << endl;
	rating_t sim;
	vector< pair<size_t, rating_t> > neighbors;
	for (ptrdiff_t row=0; row < targets.outerSize(); ++row) {
		for (ptrdiff_t k=0; k < ratings.outerSize(); ++k) {
			if (targets.innerVector(row).nonZeros() && 
				ratings.innerVector(k).nonZeros()) {
				innerVec tar=targets.innerVector(row), rat=ratings.innerVector(k);
				sim = jaccard_sim(tar, rat, supp_threshold);
				if (sim > SIM_THRESHOLD) {
					neighbors.push_back(pair<size_t, float>(k,sim));	
				}
			}
		}
		sort(neighbors.begin(),neighbors.end(),compare_pair_second<greater>());
		auto itend = (nearest_neighbors && nearest_neighbors < neighbors.size())?
			neighbors.begin()+nearest_neighbors : neighbors.end();
		for(auto kv = neighbors.begin(); kv != itend; ++kv) {
			if (row != (long)kv->first) {
				cout << row << ' ' << kv->first << ' ' << kv->second << '\n';
			}
		}
		neighbors.clear();
	}
	cerr << now() << " Done." << endl;

	// exit properly
	return SUCCESS;
}

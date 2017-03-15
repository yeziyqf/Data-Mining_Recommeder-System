/*
	Social Filtering System

	DB 2014-04-07
*/
#include <algorithm>
#include <ctime>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <numeric>
#include <deque>
#include <functional>
#include <boost/program_options.hpp>
#include "socialfiltering.hpp"
#include "wgraph.h"

using std::max;
using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::vector;
using std::ifstream;
using std::ofstream;
using std::istringstream;

namespace { 
  const size_t SUCCESS = 0; 
  const size_t ERROR_IN_COMMAND_LINE = 1; 
  const size_t ERROR_UNHANDLED_EXCEPTION = 2;  
}

/*
  Parse command line parameters
*/
size_t parse_params(int argc, char *argv[], size_t& total_users, size_t& total_items,
					string& ratings_path, string& target_users_path, string&
					target_itemlist_path, string& graph_path, unsigned int& limit_rec,
					bool& item_based, string& weighted_avg_path) {
	namespace po = boost::program_options;
	po::options_description desc("Program parameters");
	desc.add_options()
		("help,?", "produce help message")
		("number_users,u", po::value<size_t>(&total_users)->required(),
			"total number of users")
		("number_items,i", po::value<size_t>(&total_items)->required(),
			"total number of items")
		("ratings_matrix,r", po::value<string>(&ratings_path)->required(), 
			"ratings matrix path (integer values)")
		("target_users,t", po::value<string>(&target_users_path)->required(), 
			"target users list path")
		("target_itemlist,l", po::value<string>(&target_itemlist_path)->required(), 
			"target users list path")
		("graph,g", po::value<string>(&graph_path)->required(),
			"similarity graph path")
		("limit_rec,k", po::value<unsigned int>(&limit_rec)->default_value(1),
			"max number of recommended items")
		("item_based,b", po::value<bool>(&item_based)->default_value(0),
			"0 for user-based recommendation, item-based otherwise")
		("output_weighted,a",
			po::value<string>(&weighted_avg_path)->default_value(string("")),
			"output recommendation with weighted average scores")
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
template <typename val_t>
void importIntVector(vector<val_t> &vec, string path) {
	val_t val;
	string line, eline;
	ifstream infile(path);

	if (!infile.is_open()) {
		throw string("Unable to open file: ")+path;
	}
	while (getline(infile, line)) {
		istringstream iss(line);
		if ( !(iss >> val) ) {
			throw errline("Unable to parse line",line);
		}
		vec.push_back(val);
	}
	infile.close();
}

/*
 Import a sparse matrix from text file as a vector of hashes, each corresponding a matrix row.
 Args: vector to fill, text file path. Returns: number of entries read
*/
template <typename val_t>
int importRowSparse(Ratings_smatrix& smatrix, string& path) {
	val_t val;
	int row, col, entries=0;
	string line;
	ifstream infile(path);

	if (!infile.is_open()) {
		throw string("Unable to open file: ")+path;
	}
	while (getline(infile, line)) {
		istringstream iss(line);
		if ( !(iss >> row >> col >> val) ) {
			throw errline("Unable to parse line",line);
		} else if (row < 0 || col < 0) {
			throw errline("Coordinates out of bound",line);
		} else {
			auto insertion_result = smatrix[row].emplace(col,val);
			if (!insertion_result.second) {
				// cerr << '\n' << row << ' ' << col << ' '
				// 	 << smatrix[row][col] << " <- " << val << '\n';
				throw errline("Unable to insert",line);
			}
		}
		entries++;
	}
	infile.close();
	return entries;
}

/*
 Import a sparse matrix from text file as a vector of hashes,
 each corresponding a matrix row. Fill a matrix and its transpose.
 Args: vector to fill, text file path. Returns: number of entries read.
*/
template <typename val_t>
int importRowSparse(Ratings_smatrix& smatrix, Ratings_smatrix& smatrixt, string& path) {
	val_t val;
	int row, col, entries=0;
	string line;
	ifstream infile(path);

	if (!infile.is_open()) {
		throw string("Unable to open file: ")+path;
	}
	while (getline(infile, line)) {
		istringstream iss(line);
		if ( !(iss >> row >> col >> val) ) {
			throw errline("Unable to parse line",line);
		} else if (row < 0 || col < 0) {
			throw errline("Coordinates out of bound",line);
		} else {
			auto insertion_result = smatrix[row].emplace(col,val);
			if (!insertion_result.second) {
				throw errline("Repeated insertion",line);
			}
			smatrixt[col].emplace(row,val);
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
	std::time_t t = time(nullptr);
	char cstr[256];
	std::strftime(cstr, sizeof(cstr), format, std::localtime(&t));
	return cstr;
}

int main (int argc, char *argv[]) {
	// Parameters
	bool item_based;
	unsigned int limit_rec;
	size_t total_users, total_items; 
	string ratings_path, target_users_path, target_itemlist_path, graph_path, weighted_avg_path;

	if (parse_params(argc, argv, total_users, total_items, ratings_path, target_users_path,
					 target_itemlist_path, graph_path, limit_rec, item_based,
					 weighted_avg_path)) {
		return ERROR_IN_COMMAND_LINE;
	}

	// setup I/O
	ofstream out_weighted_avg(weighted_avg_path);
	if (!weighted_avg_path.empty() && !out_weighted_avg.is_open()) {
		cerr << "Unable to open file: " << weighted_avg_path << endl;
		std::terminate();	
	}

	// potentially recommend all items
	vector<int> target_items(total_items);
	std::iota(target_items.begin(), target_items.end(), 0);

	vector<int> target_users;
	Ratings_smatrix target_itemlist(total_users); // ratings matrix of target users
	Ratings_smatrix ratings_matrix(max(total_users,total_items));  // ratings matrix (RM)
	Ratings_smatrix ratings_matrixt(max(total_users,total_items)); // transposed ratings matrix
	int entries = 0;
	try {
		cerr << now() << " Loading target users' item list... ";
		importIntVector<int>(target_users, target_users_path);
		cerr << target_users.size() << " users loaded." << endl;

		cerr << now() << " Loading rating matrix... ";
		if (item_based) {
			entries = importRowSparse<rating_t>(ratings_matrix, ratings_matrixt, ratings_path);
		} else {
			entries = importRowSparse<rating_t>(ratings_matrix, ratings_path);
		}
		cerr << entries << " entries loaded." << endl;

		cerr << now() << " Loading target users' rating matrix... ";
		entries = importRowSparse<rating_t>(target_itemlist, target_itemlist_path);
		cerr << entries << " entries loaded." << endl;
	} catch (string errmsg) {
		cerr << "\nError: " << errmsg << endl;
		std::terminate();
	}

	cerr << now() << " Loading user graph... ";
	Graph& graph = *wgraph_from_path(graph_path.c_str());
	cerr << graph.n << " nodes and " << graph.m << " links loaded." << endl;


	// generate recommendations for each user
	cerr << now() << " Generating " << (item_based? "item-based" : "user-based")
		 << " recommendations (up to " << limit_rec << "/user):" << endl;
	Ratings_smatrix& ratings_matrix2 = item_based? ratings_matrixt : ratings_matrix;

	SocialFiltering socialfiltering(limit_rec, target_items, ratings_matrix,
									ratings_matrix2, target_itemlist, graph);
	deque<pair<int,score_t> > rec_avg, rec_weighted_avg;
	for (auto user : target_users) {
		if (out_weighted_avg.is_open()) {
			if (item_based) {
				socialfiltering.recommend_ib(user, rec_avg, rec_weighted_avg);
			} else {
				socialfiltering.recommend_ub(user, rec_avg, rec_weighted_avg);
			}
			while (!rec_weighted_avg.empty()) {
				out_weighted_avg << user << ' ' << rec_weighted_avg.front().first
					 			 << ' ' << rec_weighted_avg.front().second << '\n';
				rec_weighted_avg.pop_front();
			}
		} else {
			if (item_based) {
				socialfiltering.recommend_ib(user, rec_avg, false);
			} else {
				socialfiltering.recommend_ub(user, rec_avg, false);
			}
		}
		while (!rec_avg.empty()) {
			cout << user << ' ' << rec_avg.front().first
				 << ' ' << rec_avg.front().second << '\n';
			rec_avg.pop_front();
		}
	}
	cerr << now() << " Done." << endl;

	// exit properly
	free_graph(&graph);
	return SUCCESS;
}

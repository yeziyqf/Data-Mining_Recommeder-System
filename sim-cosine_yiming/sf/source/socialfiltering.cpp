/*
  Social Filtering System, recommendation engine

  DB 2014-04-07
*/
#include <cassert>
#include <string>
#include <deque>
#include <queue>
#include <functional>
#include "wgraph.h"
#include "socialfiltering.hpp"

using namespace std;

const double SCORE_THRESHOLD = 0.00001;

void SocialFiltering::recommend_ub(
const int user, deque<pair<int,score_t> >& top_rec_avg, deque<pair<int,score_t> >& top_rec_weighted_avg) {
	// assert(top_rec_avg.empty());
	// assert(top_rec_weighted_avg.empty());
	// assert(recQ_avg.empty());
	// assert(recQ_weighted_avg.empty());
	for (auto item : target_items) {
		if (ratings_matrix[user].find(item) != ratings_matrix[user].end()) {
			continue; // eliminate items possessed by user
		}
		Scores scores = predict_scores(user,item);
		if (scores.weighted_avg > SCORE_THRESHOLD) {
			recQ_pushtop(item, scores.avg, recQ_avg);
			recQ_pushtop(item, scores.weighted_avg, recQ_weighted_avg);
		}
	}
	recQ_get_rec(top_rec_avg, recQ_avg);
	recQ_get_rec(top_rec_weighted_avg, recQ_weighted_avg);
}

void SocialFiltering::recommend_ib(
const int user, deque<pair<int,score_t> >& top_rec_avg, deque<pair<int,score_t> >& top_rec_weighted_avg) {
	// assert(top_rec_avg.empty());
	// assert(top_rec_weighted_avg.empty());
	// assert(recQ_avg.empty());
	// assert(recQ_weighted_avg.empty());
	for (auto item : target_items) {
		// if (ratings_matrix[user].find(item) != ratings_matrix[user].end()) {
		// 	continue; // eliminate items possessed by user
		// }
		if (target_itemlist[user].find(item) != target_itemlist[user].end()) {
			continue; // eliminate items possessed by user
		}
		Scores scores = predict_scores_ib(user,item);
		if (scores.weighted_avg > SCORE_THRESHOLD) {
			recQ_pushtop(item, scores.avg, recQ_avg);
			recQ_pushtop(item, scores.weighted_avg, recQ_weighted_avg);
		}
	}
	recQ_get_rec(top_rec_avg, recQ_avg);
	recQ_get_rec(top_rec_weighted_avg, recQ_weighted_avg);
}

void SocialFiltering::recommend_ub(
const int user, deque<pair<int,score_t> >& top_rec_metric, const bool weighted) {
	// assert(recQ_avg.empty() || weighted);
	// assert(recQ_weighted_avg.empty() || !weighted);
	SmallFirstQ& recQ = weighted? recQ_weighted_avg : recQ_avg;
	for (auto item : target_items) {
		if (ratings_matrix[user].find(item) != ratings_matrix[user].end()) {
			continue; // eliminate items possessed by user
		}
		Scores scores = predict_scores(user,item);
		// if (scores.avg) {
		// 	cerr << "score("<< user << ", " << item << ")= ";
		// 	cerr << scores.avg <<"\n";
		// }
		if (weighted) {
			recQ_pushtop(item, scores.weighted_avg, recQ);
		} else {
			recQ_pushtop(item, scores.avg, recQ);
		}
	}
	recQ_get_rec(top_rec_metric, recQ);
}

void SocialFiltering::recommend_ib(
const int user, deque<pair<int,score_t> >& top_rec_metric, const bool weighted) {
	assert(recQ_avg.empty() || weighted);
	assert(recQ_weighted_avg.empty() || !weighted);
	SmallFirstQ& recQ = weighted? recQ_weighted_avg : recQ_avg;
	for (auto item : target_items) {
		if (target_itemlist[user].find(item) != target_itemlist[user].end()) {
			continue; // eliminate items possessed by user
		}
		// if (ratings_matrix[user].find(item) != ratings_matrix[user].end()) {
		// 	continue; // eliminate items possessed by user
		// }
		// cerr << " ("<< user << ' ' << item << ")= ";
		Scores scores = predict_scores_ib(user,item);
		// cerr << scores.avg <<" <=\n";
		if (weighted) {
			recQ_pushtop(item, scores.weighted_avg, recQ);
		} else {
			recQ_pushtop(item, scores.avg, recQ);
		}
	}
	recQ_get_rec(top_rec_metric, recQ);
}

inline void SocialFiltering::recQ_pushtop(
int item, score_t score, SmallFirstQ& recQ) {
	if (score) {
		if (recQ.size() < limit_rec) {
			recQ.emplace(item,score);
		} else if (score > recQ.top().second) {
			recQ.emplace(item,score);
			recQ.pop();
		}
	}	
}

inline void SocialFiltering::recQ_get_rec(deque<pair<int,score_t> >& top_rec, SmallFirstQ& recQ) {
	while (!recQ.empty()) {
		top_rec.push_front(recQ.top());
		recQ.pop(); 
	}
}

Scores SocialFiltering::predict_scores(const int user, const int item) {
	int sum = 0; //, count = 0;
	double weighted_sum = 0.0; //, weighted_norm = 0.0;
	// cerr << graph.degrees[user] << "\n";
	for (int i = 0; i < graph.degrees[user]; i++) {
		int neighbor = graph.links[user][i].node;
		// cerr << ">> " << user << "->" << neighbor << endl;
		// cerr << "rating'(" << neighbor << ',' << item << ")" << endl;
		// cerr << "rating(" << item << ',' << neighbor << ")" << endl;
		auto rating_p = ratings_matrix2[neighbor].find(item);
		if (rating_p != ratings_matrix2[neighbor].end()) { // if neighbor has item
			double similarity = graph.links[user][i].weight;
			sum += (int)(rating_p->second);
			weighted_sum += (double)(rating_p->second)*similarity;
			// cerr << "sum: " << sum << endl;
			// cerr << "w sum: " << weighted_sum << endl;
			// count++;
			// weighted_norm += similarity;
			// cerr << "\t" << "score(" << user << ',' << item << ")+= sim("
			// << user << ',' << neighbor << ") x ratings(" << neighbor << ','
			// << item << ") = " << similarity << " x " << (double)(rating_p->second)
			// << " = " << (double)(rating_p->second)*similarity << '\n'; 
		}
		// cerr << endl; 
	}
	Scores scores = {0.0, 0.0};
	if (sum) {
		scores.avg = (double)sum; // /(double)count; 
		scores.weighted_avg = weighted_sum; // /weighted_norm;
	}
	return scores;	
}

Scores SocialFiltering::predict_scores_ibprime(const int user, const int item) {
	int sum = 0; //, count = 0;
	double weighted_sum = 0.0; //, weighted_norm = 0.0;
	// cerr << graph.degrees[user] << "\n";
	for (int i = 0; i < graph.degrees[user]; i++) {
		int neighbor = graph.links[user][i].node;
		// cerr << ">> " << user << "->" << neighbor << endl;
		// cerr << "rating'(" << neighbor << ',' << item << ")" << endl;
		// cerr << "rating(" << item << ',' << neighbor << ")" << endl;
		auto realuser = item;
		auto rating_p = target_itemlist[realuser].find(neighbor);
		if (rating_p != target_itemlist[realuser].end()) { // neighbor has item
			double similarity = graph.links[user][i].weight;
			sum += (int)(rating_p->second);
			weighted_sum += (double)(rating_p->second)*similarity;
			// cerr << "sum: " << sum << endl;
			// cerr << "w sum: " << weighted_sum << endl;
			// count++;
			// weighted_norm += similarity;
			// cerr << "\t" << "score(" << user << ',' << item << ")+= sim("
			// << user << ',' << neighbor << ") x ratings(" << neighbor << ','
			// << item << ") = " << similarity << " x " << (double)(rating_p->second)
			// << " = " << (double)(rating_p->second)*similarity << '\n'; 
		}
		// cerr << endl; 
	}
	Scores scores = {0.0, 0.0};
	if (sum) {
		scores.avg = (double)sum; // /(double)count; 
		scores.weighted_avg = weighted_sum; // /weighted_norm;
	}
	return scores;	
}

Scores SocialFiltering::predict_scores_ib(const int user, const int item) {
	return predict_scores_ibprime(item,user);
}
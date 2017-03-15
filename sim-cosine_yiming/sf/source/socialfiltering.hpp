/*
  Social Filtering System, recommendation engine

  DB 2014-04-07
*/
#ifndef SOCIALFILTERING_HPP
#define SOCIALFILTERING_HPP
#include <string>
#include <vector>
#include <queue>
#include <deque>
#include <unordered_map>
#include "wgraph.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <iostream>
#include <deque>
#include <functional>

using std::pair;
using std::vector;
using std::deque;
using std::unordered_map;
using std::priority_queue;

typedef int rating_t; // after normalisation can be char
typedef vector<unordered_map<int,rating_t> > Ratings_smatrix;

typedef double score_t;
struct Scores {
  score_t avg;
  score_t weighted_avg;
};

struct Bigger2nd { // Comparison/priorityQ: elements are sorted by second value, ascending
  int operator() ( const pair<int,score_t>& p1, const pair<int,score_t>& p2 ) {
    return p1.second > p2.second; }
};
typedef priority_queue<pair<int,score_t>, vector<pair<int,score_t> >, Bigger2nd> SmallFirstQ;

class SocialFiltering {
  unsigned int limit_rec;
  vector<int>& target_items;
  Ratings_smatrix& ratings_matrix;
  Ratings_smatrix& ratings_matrix2; // =ratings_matrix if UB, =transpose(ratings_matrix) if IB
  Ratings_smatrix& target_itemlist;
  Graph& graph;
  SmallFirstQ recQ_avg;
  SmallFirstQ recQ_weighted_avg;
  void recQ_pushtop(int item, score_t score, SmallFirstQ& recQ);
  void recQ_get_rec(deque<pair<int,score_t> >& top_rec, SmallFirstQ& recQ);
public:
  SocialFiltering(int lr,vector<int>& ti,Ratings_smatrix& rm,Ratings_smatrix& rmt,
    Ratings_smatrix& til, Graph& gr) :
    limit_rec(lr), target_items(ti), ratings_matrix(rm), ratings_matrix2(rmt),
    target_itemlist(til), graph(gr) {}
  void recommend_ib(const int user, deque<pair<int,score_t> >& top_rec_avg, deque<pair<int,score_t> >& top_rec_weighted_avg);
  void recommend_ub(const int user, deque<pair<int,score_t> >& top_rec_avg, deque<pair<int,score_t> >& top_rec_weighted_avg);
  void recommend_ub(const int user, deque<pair<int,score_t> >& top_rec_metric, const bool weighted);
  void recommend_ib(const int user, deque<pair<int,score_t> >& top_rec_metric, const bool weighted);
  Scores predict_scores(const int user, const int item);
  Scores predict_scores_ib(const int user, const int item);
  Scores predict_scores_ibprime(const int user, const int item);
};
#endif

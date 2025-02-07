#ifndef PLACER_H_
#define PLACER_H_

#include <stdlib.h> /* srand, rand */
#include <time.h>   /* time */

#include <cmath>
#include <nlohmann/json.hpp>
#include <thread>

#include "../PnRDB/datatype.h"
#include "Aplace.h"
#include "ConstGraph.h"
#include "ILP_solver.h"
#include "PlacerHyperparameters.h"
#include "SeqPair.h"
#include "design.h"
#ifdef PERFORMANCE_DRIVEN
#include <Python.h>
#endif
using std::cout;
using std::endl;
using namespace nlohmann;

//#define MAX_TIMEOUT 4300000 //4.3 seconds = 4300000 us

//#define MTMODE 1 // flag to turn on multi-threading

class Placer {
  private:
  struct Thread_data {
    int thread_id;
    design thread_designData;
    SeqPair thread_trial_sp;
    ConstGraph thread_trial_sol;
    int thread_mode;
    double thread_trial_cost = 0.0;
    bool thread_succeed = false;
  };
  // design designData;
  // PnRDB::hierNode node;
  bool GenerateValidSolution(design& mydesign, SeqPair& curr_sp, ConstGraph& curr_sol, int mode);
  void PlacementRegular(PnRDB::hierNode& node, string opath, int effort, PnRDB::Drc_info& drcInfo);  // do placement with simulated annealing
  void PlacementMixSA(PnRDB::hierNode& node, string opath, int effort, PnRDB::Drc_info& drcInfo);    // do placement with mix-sized simulated annealing
  void PlacementMixAP(PnRDB::hierNode& node, string opath, int effort, PnRDB::Drc_info& drcInfo);    // do placement with mix-sized analytical placement
  void ThreadFunc(Thread_data* MT);
  void PlacementCore(design& designData, SeqPair& curr_sp, ConstGraph& curr_sol, int mode, int effort);
  std::map<double, SeqPair> PlacementCoreAspectRatio(design& designData, SeqPair& curr_sp, ConstGraph& curr_sol, int mode, int nodeSize, int effort);
  // std::map<double, std::pair<SeqPair, ILP_solver>> PlacementCoreAspectRatio_ILP(design& designData, SeqPair& curr_sp, ILP_solver& curr_sol, int mode, int
  // nodeSize, int effort, PnRDB::Drc_info& drcInfo, PnRDB::hierNode& node);
  std::map<double, std::pair<SeqPair, ILP_solver>> PlacementCoreAspectRatio_ILP(design& designData, SeqPair& curr_sp, ILP_solver& curr_sol, int mode,
                                                                                int nodeSize, int effort, PnRDB::Drc_info& drcInfo, bool select_in_ILP);
  void ReshapeSeqPairMap(std::map<double, SeqPair>& spMap, int nodeSize);
  void ReshapeSeqPairMap(std::map<double, std::pair<SeqPair, ILP_solver>>& spMap, int nodeSize);
  void PlacementRegularAspectRatio(std::vector<PnRDB::hierNode>& nodeVec, string opath, int effort, PnRDB::Drc_info& drcInfo);
  void PlacementMixSAAspectRatio(std::vector<PnRDB::hierNode>& nodeVec, string opath, int effort, PnRDB::Drc_info& drcInfo);
  void PlacementMixAPAspectRatio(std::vector<PnRDB::hierNode>& nodeVec, string opath, int effort, PnRDB::Drc_info& drcInfo);
  void PlacementRegularAspectRatio_ILP(std::vector<PnRDB::hierNode>& nodeVec, string opath, int effort, PnRDB::Drc_info& drcInfo, bool select_in_ILP);
  void PlacementRegularAspectRatio_ILP_Analytical(std::vector<PnRDB::hierNode>& nodeVec, string opath, int effort, PnRDB::Drc_info& drcInfo,
                                                  bool select_in_ILP);
  void setPlacementInfoFromJson(std::vector<PnRDB::hierNode>& nodeVec, string opath, PnRDB::Drc_info& drcInfo);
  PlacerHyperparameters hyper;
  std::uniform_real_distribution<double> _rnd{0., 1.};
  static std::mt19937_64 _rng;

  public:
  Placer(PnRDB::hierNode& node, string opath, int effort, PnRDB::Drc_info& drcInfo, const PlacerHyperparameters& hyper_in);
  Placer(std::vector<PnRDB::hierNode>& nodeVec, string opath, int effort, PnRDB::Drc_info& drcInfo, const PlacerHyperparameters& hyper_in, bool select_in_ILP);
  // Placer(PnRDB::hierNode& input_node); // Constructor
  // PnRDB::hierNode CheckoutHierNode(); // Output hier Node after placement
};

#endif

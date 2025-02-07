#ifndef ILP_solver_h_
#define ILP_solver_h_

#include <stdio.h>
#include <stdlib.h> /* srand, rand */
#include <time.h>   /* time */

#include <algorithm>
#include <climits>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stack>
#include <string>
#include <utility>
#include <vector>

#include "../PnRDB/datatype.h"
#include "Aplace.h"
#include "ConstGraph.h"
#include "Pdatatype.h"
#include "SeqPair.h"
#include "design.h"
#include "lp_lib.h"

using std::cerr;
using std::cin;
using std::cout;
using std::make_pair;
using std::min;
using std::pair;
using std::stack;
using std::string;
using std::vector;

class ILP_solver {
  friend class Placer;

  private:
  struct Block {
    int x = 0, y = 0;            // LL of each block
    int H_flip = 0, V_flip = 0;  // flip along V axis and H axis
  };
  vector<Block> Blocks;
  placerDB::point LL, UR;
  double area = 0, HPWL = 0, HPWL_extend = 0, HPWL_extend_terminal = 0, ratio = 0, linear_const = 0, multi_linear_const = 0;
  double area_norm = 0, HPWL_norm = 0;
  double Aspect_Ratio_weight = 1000;
  double Aspect_Ratio[2] = {0, 100};
  double placement_box[2] = {-1.0, -1.0};
  typedef void(lphandlestr_func)(lprec* lp, void* userhandle, char* buf);
  static void lpsolve_logger(lprec* lp, void* userhandle, char* buf);
  vector<vector<int>> block_order;
  int x_pitch, y_pitch;

  inline void roundup(int& v, const int pitch) { v = pitch * ((v + pitch - 1) / pitch); }
  bool MoveBlocksUsingSlack(const std::vector<Block>& blockslocal, design& mydesign, SeqPair& curr_sp, PnRDB::Drc_info& drcInfo);
  bool FrameSolveILP(design& mydesign, SeqPair& curr_sp, PnRDB::Drc_info& drcInfo, bool flushlb = true, const vector<placerDB::point>* prev = nullptr);
  public:
  double cost = 0;
  double constraint_penalty = 0;
  ILP_solver();
  ILP_solver(design& mydesign, PnRDB::hierNode& node);
  ILP_solver(design& mydesign);
  ILP_solver(const ILP_solver& solver);
  ILP_solver& operator=(const ILP_solver& solver);
  double GenerateValidSolutionAnalytical(design& mydesign, PnRDB::Drc_info& drcInfo, PnRDB::hierNode& node);
  double GenerateValidSolution(design& mydesign, SeqPair& curr_sp, PnRDB::Drc_info& drcInfo);
  double GenerateValidSolution_select(design& mydesign, SeqPair& curr_sp, PnRDB::Drc_info& drcInfo);
  double CalculateCost(design& mydesign);
  double CalculateCost(design& mydesign, SeqPair& curr_sp);
  void WritePlacement(design& caseNL, SeqPair& curr_sp, string outfile);
  void PlotPlacement(design& mydesign, SeqPair& curr_sp, string outfile);
  void PlotPlacementAnalytical(design& caseNL, string outfile, bool plot_pin, bool plot_terminal, bool plot_net);
  std::vector<double> Calculate_Center_Point_feature(std::vector<std::vector<placerDB::point>>& temp_contact);
  void updateTerminalCenter(design& mydesign, SeqPair& curr_sp);
  void updateTerminalCenterAnalytical(design& mydesign);
  void UpdateHierNode(design& mydesign, SeqPair& curr_sp, PnRDB::hierNode& node, PnRDB::Drc_info& drcInfo);
  void UpdateHierNodeAnalytical(design& mydesign, PnRDB::hierNode& node, PnRDB::Drc_info& drcInfo);
  void UpdateBlockinHierNode(design& mydesign, placerDB::Omark ort, PnRDB::hierNode& node, int i, int sel, PnRDB::Drc_info& drcInfo);
  void UpdateTerminalinHierNode(design& mydesign, PnRDB::hierNode& node, PnRDB::Drc_info& drcInfo);
  void UpdateSymmetryNetInfoAnalytical(design& mydesign, PnRDB::hierNode& node, int i, int SBidx, placerDB::Smark axis_dir);
  void UpdateSymmetryNetInfo(design& mydesign, PnRDB::hierNode& node, int i, int SBidx, placerDB::Smark axis_dir, SeqPair& curr_sp);
  PnRDB::bbox ConvertBoundaryData(vector<placerDB::point> Bdata);
  PnRDB::point ConvertPointData(placerDB::point Pdata);
};

#endif

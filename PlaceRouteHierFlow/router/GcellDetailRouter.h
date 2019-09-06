#ifndef GCELLDETAILROUTER_H_
#define GCELLDETAILROUTER_H_

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <set>
#include <cmath>
#include <algorithm>
#include <limits.h>
#include <bitset>
#include <cstdlib> // system
#include <iterator>
#include <cctype>
#include <unistd.h> // getcwd
#include <map>
#include <set>
#include <utility>//std::pair, make_pair
#include "Grid.h"
#include "Graph.h"
#include "A_star.h"
#include "GlobalGrid.h"
#include "GlobalGraph.h"
#include "RawRouter.h"
#include "Rdatatype.h"
#include "GlobalRouter.h"
#include "GcellGlobalRouter.h"
#include "../PnRDB/datatype.h"

class GcellDetailRouter : public GcellGlobalRouter{

  friend class GlobalGrid;
  friend class Grid;
  friend class PowerRouter;

  private:
  GlobalGrid Gcell; 
/*
   int End_Metal_Flag = 1;
   
   struct DetailSegMark{
     //for each segment
     int overlapFlag_seg = -1; //-1 not routered, 0 nonoverlap, 1 overlapped
     std::vector<int> overlapFlag_metal; // 0 nonoverlap, 1 overlapped
     std::vector<int> iterMetals; //iter to the index of cleanMetals
     std::vector<std::vector<RouterDB::Metal> > cleanMetals; //clean metals for overlapped metal
   };
   
   struct DetailNetMark{
      std::vector<DetailSegMark> Mark_seg;
   };
   
   std::vector<DetailNetMark> DetailRouterMarks;
*/   
  public:
    
    GcellDetailRouter(); //initial Nets & Blocks with node data
    GcellDetailRouter(PnRDB::hierNode& HierNode, GcellGlobalRouter& GR, int path_number, int grid_scale);
    //void RecoverOverlap();
    //void Connection();
    //bool CheckOverlapped_part(int i, int j, int k);
    //bool CheckConnection_Source(RouterDB::Segment& temp_seg);
    //bool CheckConnection_Dest(RouterDB::Segment& temp_seg);
    void CreatePlistBlocks(std::vector<std::vector<RouterDB::point> >& plist, std::vector<RouterDB::Block>& Blocks);
    //void CreatePlistNets(std::vector<std::vector<RouterDB::point> >& plist, std::vector<RouterDB::Net>& Nets);
    void CreatePlistTerminals(std::vector<std::vector<RouterDB::point> >& plist, std::vector<RouterDB::terminal> Terminals);
    //void CreatePlistNets_DetailRouter(std::vector<std::vector<RouterDB::point> >& plist, std::vector<RouterDB::Net>& Nets);
    void UpdatePlistNets(std::vector<std::vector<RouterDB::Metal> > &physical_path, std::vector<std::vector<RouterDB::point> > & plist);
    void GetPhsical_Metal(std::vector<std::vector<RouterDB::Metal> > &physical_path);
    //void InsertPathToCand_Start(std::vector<std::vector<RouterDB::Metal> > &physical_path, RouterDB::Segment temp_seg);
    //void InsertPathToCand_Start(std::vector<std::vector<RouterDB::Metal> > &physical_path, RouterDB::Segment &temp_seg);
    //void InsertPathToCand_End(std::vector<std::vector<RouterDB::Metal> > &physical_path, RouterDB::Segment &temp_seg);
    void ConvertRect2GridPoints(std::vector<std::vector<RouterDB::point> >& plist, int mIdx, int LLx, int LLy, int URx, int URy);
    //void CheckOverlapMetals(std::set<RouterDB::SinkData, RouterDB::SinkDataComp> Set_x, std::vector<RouterDB::Net>& nets);
    void Physical_metal_via();
    //void AddMetalToPin();
    //void checkPathMetalToPin(int i, int j);

    void create_detailrouter();
    //std::vector<std::vector<RouterDB::SinkData> > findPins(RouterDB::Net temp_net);
    std::vector<RouterDB::Metal> findGlobalPath(RouterDB::Net temp_net);
    void splitPath(std::vector<std::vector<RouterDB::Metal> > temp_path, RouterDB::Net& temp_net);
    void lastmile_source(std::vector<std::vector<RouterDB::Metal> > &temp_path, std::vector<RouterDB::SinkData> temp_source);
    void lastmile_dest(std::vector<std::vector<RouterDB::Metal> > &temp_path, std::vector<RouterDB::SinkData> temp_source);
    void lastmile_source_new(std::vector<std::vector<RouterDB::Metal> > &temp_path, std::vector<RouterDB::SinkData> temp_source);
    void lastmile_dest_new(std::vector<std::vector<RouterDB::Metal> > &temp_path, std::vector<RouterDB::SinkData> temp_source);
    void updateSource(std::vector<std::vector<RouterDB::Metal> > temp_path, std::vector<RouterDB::SinkData>& temp_source);

    void NetToNodeNet(PnRDB::hierNode& HierNode, RouterDB::Net& net, int net_index);
    void NetToNodeInterMetal(PnRDB::hierNode& HierNode, RouterDB::Net& net);
    void NetToNodeBlockPins(PnRDB::hierNode& HierNode, RouterDB::Net& net);
    void returnPath(std::vector<std::vector<RouterDB::Metal> > temp_path, RouterDB::Net& temp_net);
    std::vector<RouterDB::Metal> CpSymPath(std::vector<RouterDB::Metal> temp_path, int H, int center);
    RouterDB::contact SymContact(RouterDB::contact &temp_contact, bool H, int center);
    RouterDB::SinkData Sym_contact(RouterDB::SinkData &temp_contact, bool H, int center);
    int Cover_Contact(RouterDB::SinkData &temp_contact, RouterDB::SinkData &sym_temp_contact, RouterDB::SinkData &cover_contact);
    std::vector<RouterDB::SinkData> FindCommon_Contact(std::vector<RouterDB::SinkData> &temp_contact, std::vector<RouterDB::SinkData> &sym_temp_contact, bool H, int center);
    int findPins_Sym(Grid& grid, RouterDB::Net &temp_net, RouterDB::Net &sym_temp_net, bool H, int center, std::vector<std::vector<RouterDB::SinkData> > &temp_pins, std::vector<std::vector<RouterDB::SinkData> > &sym_temp_pins ,std::vector<std::vector<RouterDB::SinkData> > &Common_contacts);
    std::vector<std::vector<RouterDB::SinkData> > findPins_new(Grid& grid, RouterDB::Net &temp_net);
    void SortPins(std::vector<std::vector<RouterDB::SinkData> > & temp_Pin);
    void CreatePlistSymBlocks(std::vector<std::set<RouterDB::point, RouterDB::pointXYComp> > &set_plist, RouterDB::point gridll, RouterDB::point gridur, int H, int center, RouterDB::point symgridll, RouterDB::point symgridur);
    void CreatePlistContact(std::vector<std::vector<RouterDB::point> >& plist, std::vector<RouterDB::contact>& Contacts);
    void CreatePlistSymNets(std::vector<std::set<RouterDB::point, RouterDB::pointXYComp> > &set_plist, RouterDB::point gridll, RouterDB::point gridur, bool H, int center, RouterDB::point symgridll, RouterDB::point symgridur);
    //std::vector<std::vector<RouterDB::SinkData> > findPins_new(Grid& grid, RouterDB::Net &temp_net);
    //std::vector<std::vector<RouterDB::SinkData> > findPins(RouterDB::Net temp_net);
    
    //void BlockInterMetalToNodeInterMetal(PnRDB::hierNode& HierNode);
    void ReturnHierNode(PnRDB::hierNode& HierNode);
    void printNetsInfo();
    //void ConvertToContactPnRDB_Placed_Origin(PnRDB::contact& pnr_contact,RouterDB::contact& router_contact);
    //void ConvertToContactPnRDB_Placed_Placed(PnRDB::contact& pnr_contact,RouterDB::contact& router_contact);
    //void ConvertToViaPnRDB_Placed_Origin(PnRDB::Via& temp_via, RouterDB::Via& router_via);
    //void ConvertToViaPnRDB_Placed_Placed(PnRDB::Via& temp_via, RouterDB::Via& router_via);
    //void TerminalToNodeTerminal(PnRDB::hierNode& HierNode);
    void GetPhsical_Metal_Via(int i);
    void BlockInterMetalToNodeInterMetal(PnRDB::hierNode& HierNode);
    void TerminalToNodeTerminal(PnRDB::hierNode& HierNode);
    void ConvertToViaPnRDB_Placed_Origin(PnRDB::Via& temp_via, RouterDB::Via& router_via);
    void ConvertToContactPnRDB_Placed_Origin(PnRDB::contact& pnr_contact,RouterDB::contact& router_contact);
    void ConvertToViaPnRDB_Placed_Placed(PnRDB::Via& temp_via, RouterDB::Via& router_via);
    void ConvertToContactPnRDB_Placed_Placed(PnRDB::contact& pnr_contact,RouterDB::contact& router_contact);
    void JudgeTileCoverage(std::vector<std::pair<int,int> > &tile_index, std::vector<std::vector<RouterDB::SinkData> > &temp_pins, GlobalGrid &Gcell);
    int Tile_Cover_Contact(RouterDB::SinkData &temp_contact, RouterDB::SinkData &sym_temp_contact);
    void CheckTile(RouterDB::Net &temp_net, GlobalGrid &Gcell);
    void GetPhsical_Via_contacts(std::vector<std::vector<RouterDB::Metal> >physical_path, std::vector<RouterDB::contact> &temp_via_contact);

};

#endif
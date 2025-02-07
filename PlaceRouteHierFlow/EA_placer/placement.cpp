#include "placement.h"
// #define DEBUG
Placement::Placement() {}

void Placement::Initilize_lambda() {
  Ppoint_F norm_wire_gradient;
  norm_wire_gradient.x = 0;
  norm_wire_gradient.y = 0;
  Ppoint_F norm_E_force_gradient;
  norm_E_force_gradient.x = 0;
  norm_E_force_gradient.y = 0;

  for (unsigned int i = 0; i < Blocks.size(); ++i) {
    norm_wire_gradient.x += abs(Blocks[i].Netforce.x);
    norm_wire_gradient.y += abs(Blocks[i].Netforce.y);
    norm_E_force_gradient.x += abs(Blocks[i].Eforce.x);
    norm_E_force_gradient.y += abs(Blocks[i].Eforce.y);
  }

  float lambda_x = beta * norm_wire_gradient.x / norm_E_force_gradient.x;
  float lambda_y = beta * norm_wire_gradient.y / norm_E_force_gradient.y;

  std::cout << "lambda_x " << lambda_x << " " << norm_wire_gradient.x / Blocks.size() << " " << norm_E_force_gradient.x / Blocks.size() << std::endl;
  std::cout << "lambda_y " << lambda_y << " " << norm_wire_gradient.y / Blocks.size() << " " << norm_E_force_gradient.y / Blocks.size() << std::endl;

  if (lambda_x < lambda_y) {
    lambda = lambda_x;
  } else {
    lambda = lambda_y;
  }
}

void Placement::Initilize_sym_beta() {
  Ppoint_F norm_wire_gradient;
  norm_wire_gradient.x = 0;
  norm_wire_gradient.y = 0;
  Ppoint_F norm_sym_force_gradient;
  norm_sym_force_gradient.x = 0;
  norm_sym_force_gradient.y = 0;

  for (unsigned int i = 0; i < Blocks.size(); ++i) {
    norm_wire_gradient.x += abs(Blocks[i].Netforce.x);
    norm_wire_gradient.y += abs(Blocks[i].Netforce.y);
    norm_sym_force_gradient.x += abs(Blocks[i].Symmetricforce.x);
    norm_sym_force_gradient.y += abs(Blocks[i].Symmetricforce.y);
  }

  float sym_beta_x = 0;
  float sym_beta_y = 0;

  if (norm_sym_force_gradient.x != 0) {
    sym_beta_x = beta * norm_wire_gradient.x / norm_sym_force_gradient.x;
  }
  if (norm_sym_force_gradient.y != 0) {
    float sym_beta_y = beta * norm_wire_gradient.y / norm_sym_force_gradient.y;
  }

  std::cout << "sym_beta_x " << sym_beta_x << " " << norm_wire_gradient.x / Blocks.size() << " " << norm_sym_force_gradient.x / Blocks.size() << std::endl;
  std::cout << "sym_beta_y " << sym_beta_y << " " << norm_wire_gradient.y / Blocks.size() << " " << norm_sym_force_gradient.y / Blocks.size() << std::endl;

  if (sym_beta_x < sym_beta_y) {
    sym_beta = 0.5 * sym_beta_y;
  } else {
    sym_beta = 0.5 * sym_beta_x;
  }
}

Placement::Placement(PnRDB::hierNode &current_node) {
  // step 1: transfroming info. of current_node to Blocks and Nets
  // create a small function for this
  float area, scale_factor;
  int max_block_number = 1000;
  int max_net_number = 100;
  int max_conection_number = 100;

  // for bins
  unit_x_bin = (float)1 / 16;
  unit_y_bin = (float)1 / 16;
  x_dimension_bin = 16;  // number of bin, number of pe
  y_dimension_bin = 16;  // number of bin, number of pe

  Bin_D.x = unit_x_bin;
  Bin_D.y = unit_y_bin;
  std::cout << "start reading node file" << std::endl;
  area = readInputNode(current_node);

  // for blocks
  unit_x = (float)1 / Blocks.size();
  unit_y = (float)1 / Blocks.size();
  x_dimension = Blocks.size();  // number of pe
  y_dimension = x_dimension;    // S number of pe
  Chip_D.x = (float)1;
  Chip_D.y = (float)1;

  for (unsigned int i = 0; i < x_dimension_bin; ++i) {
    vector<bin> temp_bins;
    for (unsigned int j = 0; j < y_dimension_bin; ++j) {
      bin temp_bin;
      temp_bin.Dpoint.x = unit_x_bin;
      temp_bin.Dpoint.y = unit_y_bin;
      temp_bin.Cpoint.x = i * unit_x_bin + unit_x_bin / 2;
      temp_bin.Cpoint.y = j * unit_y_bin + unit_y_bin / 2;
      temp_bin.Index.x = i;
      temp_bin.Index.y = j;
      temp_bins.push_back(temp_bin);
    }
    Bins.push_back(temp_bins);
  }

  // step 2: Given a initial position for each block
  // create a small function for this
  // need to estimate a area to do placement
  // scale into 1x1
  // initial position for each block
  std::cout << "Unify the block coordinate" << std::endl;
  scale_factor = 40.0;
  Unify_blocks(area, scale_factor);
  find_uni_cell();
  readCC();
  // Initilize_Placement(current_node);
  // PlotPlacement(600);
  splitNode_MS(uni_cell_Dpoint.y, uni_cell_Dpoint.x);
  int tol_diff = 3;
  addNet_after_split_Blocks(tol_diff, uni_cell_Dpoint.y, uni_cell_Dpoint.x);
  split_net();
  modify_symm_after_split(current_node);
  update_hiernode(current_node);

  // read alignment constrains
  read_alignment(current_node);
  read_order(current_node);
#ifdef quadratic_placement
  Initilize_Placement(current_node);
#else
  Initilize_Placement_Rand(current_node);
#endif

  print_blocks_nets();
  // step 3: call E_placer
  std::cout << "start ePlacement" << std::endl;
  PlotPlacement(602);
  // restore_MS();
  // PlotPlacement(601);
  E_Placer(current_node);
  bool isCompact = true;
  restore_CC_in_square(isCompact);

  // only for plot

  restore_MS();
  PlotPlacement(603);
  // setp 4: write back to HierNode
  writeback(current_node);
}

void Placement::place_ut(PnRDB::hierNode &current_node) {
  // step 1: transfroming info. of current_node to Blocks and Nets
  // create a small function for this
  float area, scale_factor;
  int max_block_number = 1000;
  int max_net_number = 100;
  int max_conection_number = 100;

  // for bins
  unit_x_bin = (float)1 / 16;
  unit_y_bin = (float)1 / 16;
  x_dimension_bin = 16;  // number of bin, number of pe
  y_dimension_bin = 16;  // number of bin, number of pe

  Bin_D.x = unit_x_bin;
  Bin_D.y = unit_y_bin;
  std::cout << "start reading node file" << std::endl;
  area = readInputNode(current_node);

  // for blocks
  unit_x = (float)1 / Blocks.size();
  unit_y = (float)1 / Blocks.size();
  x_dimension = Blocks.size();  // number of pe
  y_dimension = x_dimension;    // S number of pe
  Chip_D.x = (float)1;
  Chip_D.y = (float)1;

  for (unsigned int i = 0; i < x_dimension_bin; ++i) {
    vector<bin> temp_bins;
    for (unsigned int j = 0; j < y_dimension_bin; ++j) {
      bin temp_bin;
      temp_bin.Dpoint.x = unit_x_bin;
      temp_bin.Dpoint.y = unit_y_bin;
      temp_bin.Cpoint.x = i * unit_x_bin + unit_x_bin / 2;
      temp_bin.Cpoint.y = j * unit_y_bin + unit_y_bin / 2;
      temp_bin.Index.x = i;
      temp_bin.Index.y = j;
      temp_bins.push_back(temp_bin);
    }
    Bins.push_back(temp_bins);
  }

  // step 2: Given a initial position for each block
  // create a small function for this
  // need to estimate a area to do placement
  // scale into 1x1
  // initial position for each block
  for (unsigned int i = 0; i < originalBlockCNT; i++) {
    Blocks[i].original_Dpoint.x = current_node.Blocks[i].instance[0].width;
    Blocks[i].original_Dpoint.y = current_node.Blocks[i].instance[0].height;
  }
  std::cout << "Unify the block coordinate" << std::endl;
  scale_factor = 40.0;
  Unify_blocks(area, scale_factor);
  find_uni_cell();
  readCC();
  // Initilize_Placement(current_node);
  // PlotPlacement(600);
  splitNode_MS(uni_cell_Dpoint.y, uni_cell_Dpoint.x);
  int tol_diff = 3;
  addNet_after_split_Blocks(tol_diff, uni_cell_Dpoint.y, uni_cell_Dpoint.x);
  split_net();
  modify_symm_after_split(current_node);
  update_hiernode(current_node);

  // read alignment constrains
  read_alignment(current_node);
  read_order(current_node);
#ifdef quadratic_placement
  Initilize_Placement(current_node);
#else
  Initilize_Placement_Rand(current_node);
#endif

  print_blocks_nets();
  // step 3: call E_placer
  std::cout << "start ePlacement" << std::endl;
  PlotPlacement(602);
  // restore_MS();
  // PlotPlacement(601);
  UT_Placer();
  bool isCompact = true;
  restore_CC_in_square(isCompact);

  // only for plot

  // restore_MS();
  PlotPlacement(603);
  // setp 4: write back to HierNode
  writeback(current_node);
}

void Placement::place(PnRDB::hierNode &current_node) {
  auto logger = spdlog::default_logger()->clone("placer.Placement.place");
  // step 1: transfroming info. of current_node to Blocks and Nets
  // create a small function for this
  float area, scale_factor;
  int max_block_number = 1000;
  int max_net_number = 100;
  int max_conection_number = 100;
  circuit_name = current_node.name;

  // for bins
  unit_x_bin = (float)1 / 16;
  unit_y_bin = (float)1 / 16;
  x_dimension_bin = 16;  // number of bin, number of pe
  y_dimension_bin = 16;  // number of bin, number of pe

  Bin_D.x = unit_x_bin;
  Bin_D.y = unit_y_bin;
  std::cout << "start reading node file" << std::endl;
  area = readInputNode(current_node);

  // for blocks
  unit_x = (float)1 / Blocks.size();
  unit_y = (float)1 / Blocks.size();
  x_dimension = Blocks.size();  // number of pe
  y_dimension = x_dimension;    // S number of pe
  Chip_D.x = (float)1;
  Chip_D.y = (float)1;

  for (unsigned int i = 0; i < x_dimension_bin; ++i) {
    vector<bin> temp_bins;
    for (unsigned int j = 0; j < y_dimension_bin; ++j) {
      bin temp_bin;
      temp_bin.Dpoint.x = unit_x_bin;
      temp_bin.Dpoint.y = unit_y_bin;
      temp_bin.Cpoint.x = i * unit_x_bin + unit_x_bin / 2;
      temp_bin.Cpoint.y = j * unit_y_bin + unit_y_bin / 2;
      temp_bin.Index.x = i;
      temp_bin.Index.y = j;
      temp_bins.push_back(temp_bin);
    }
    Bins.push_back(temp_bins);
  }

  // step 2: Given a initial position for each block
  // create a small function for this
  // need to estimate a area to do placement
  // scale into 1x1
  // initial position for each block
  for (unsigned int i = 0; i < originalBlockCNT; i++) {
    Blocks[i].original_Dpoint.x = current_node.Blocks[i].instance[0].width;
    Blocks[i].original_Dpoint.y = current_node.Blocks[i].instance[0].height;
  }
  std::cout << "Unify the block coordinate" << std::endl;
  scale_factor = 40.0;
  Unify_blocks(area, scale_factor);
  find_uni_cell();
  readCC();
  // Initilize_Placement(current_node);
  // PlotPlacement(600);
  splitNode_MS(uni_cell_Dpoint.y, uni_cell_Dpoint.x);
  int tol_diff = 3;
  addNet_after_split_Blocks(tol_diff, uni_cell_Dpoint.y, uni_cell_Dpoint.x);
  split_net();
  modify_symm_after_split(current_node);
  update_hiernode(current_node);

  // read alignment constrains
  read_alignment(current_node);
  read_order(current_node);
  clock_t start, end;
  start = clock();
#ifdef quadratic_placement
  Initilize_Placement(current_node);
#else
  Initilize_Placement_Rand(current_node);
#endif
  end = clock();
  logger->info("initialize runtime: {0} s", (double)(end - start) / CLOCKS_PER_SEC);

  print_blocks_nets();
  // step 3: call E_placer
  std::cout << "start ePlacement" << std::endl;
  PlotPlacement(602);
  // restore_MS();
  // PlotPlacement(601);
  E_Placer(current_node);
  bool isCompact = true;
  restore_CC_in_square(isCompact);

  // only for plot

  // restore_MS();
  PlotPlacement(603);
  // setp 4: write back to HierNode
  writeback(current_node);
}

Placement::Placement(float chip_width, float chip_hight, float bin_width, float bin_hight) {
  this->Chip_D.x = chip_width;
  this->Chip_D.y = chip_hight;
  this->Bin_D.x = bin_width;
  this->Bin_D.x = bin_hight;
}

void Placement::generate_testing_data() {
#ifdef DEBUG
  std::cout << "generating test 1" << std::endl;
#endif
  Random_Generation_Block_Nets();
#ifdef DEBUG
  std::cout << "generating test 2" << std::endl;
#endif
  // Initilize_Placement();
#ifdef DEBUG
  std::cout << "generating test 3" << std::endl;
#endif
  PlotPlacement(0);
}

void Placement::Random_Generation_Block_Nets() {
  int max_block_number = 1000;
  int max_net_number = 100;
  int max_conection_number = 100;

  // for blocks
  unit_x = (float)1 / 64;
  unit_y = (float)1 / 64;
  x_dimension = 64;  // number of bin, number of pe
  y_dimension = 64;  // number of bin, number of pe

  // for bins
  unit_x_bin = (float)1 / 16;
  unit_y_bin = (float)1 / 16;
  x_dimension_bin = 16;  // number of bin, number of pe
  y_dimension_bin = 16;  // number of bin, number of pe

  Chip_D.x = (float)x_dimension * unit_x;
  Chip_D.y = (float)y_dimension * unit_y;

  Bin_D.x = unit_x_bin;
  Bin_D.y = unit_y_bin;

  for (unsigned int i = 0; i < max_block_number; ++i) {
    block temp_block;
    temp_block.Dpoint.x = unit_x;
    temp_block.Dpoint.y = unit_y;
    temp_block.index = i;
    Blocks.push_back(temp_block);
  }

  for (unsigned int i = 0; i < x_dimension_bin; ++i) {
    vector<bin> temp_bins;
    for (unsigned int j = 0; j < y_dimension_bin; ++j) {
      bin temp_bin;
      temp_bin.Dpoint.x = unit_x_bin;
      temp_bin.Dpoint.y = unit_y_bin;
      temp_bin.Cpoint.x = i * unit_x_bin + unit_x_bin / 2;
      temp_bin.Cpoint.y = j * unit_y_bin + unit_y_bin / 2;
      temp_bin.Index.x = i;
      temp_bin.Index.y = j;
      temp_bins.push_back(temp_bin);
    }
    Bins.push_back(temp_bins);
  }

  for (unsigned int i = 0; i < max_net_number; ++i) {
    set<int> connection_index;
    for (unsigned int j = 0; j < max_conection_number; ++j) {
      int random_block_index = rand() % max_block_number;
      connection_index.insert(random_block_index);
    }
    vector<int> connection_block_index;
    for (auto it = connection_index.begin(); it != connection_index.end(); ++it) {
      connection_block_index.push_back(*it);
      Blocks[*it].connected_net.push_back(i);
    }
    net temp_net;
    temp_net.connected_block = connection_block_index;
    temp_net.index = i;
    Nets.push_back(temp_net);
  }
}

void Placement::Create_Placement_Bins() {
  // according to the chip area, bin dimension, create a vector<bin> Bins
}

void Placement::Pull_back() {
  for (unsigned int i = 0; i < Blocks.size(); ++i) {
    if (Blocks[i].Cpoint.x + Blocks[i].Dpoint.x / 2 > Chip_D.x) {
      Blocks[i].Cpoint.x = Chip_D.x - Blocks[i].Dpoint.x / 2 - (1.5) * Bin_D.x / 2;
      // Blocks[i].Cpoint.x = Chip_D.x - Blocks[i].Dpoint.x/2;
    }
    if (Blocks[i].Cpoint.y + Blocks[i].Dpoint.y / 2 > Chip_D.y) {
      Blocks[i].Cpoint.y = Chip_D.y - Blocks[i].Dpoint.y / 2 - (1.5) * Bin_D.y / 2;
      // Blocks[i].Cpoint.y = Chip_D.y - Blocks[i].Dpoint.y/2;
    }
    if (Blocks[i].Cpoint.x - Blocks[i].Dpoint.x / 2 < 0) {
      Blocks[i].Cpoint.x = Blocks[i].Dpoint.x / 2 + (1.5) * Bin_D.x / 2;
      // Blocks[i].Cpoint.x = Blocks[i].Dpoint.x/2;
    }
    if (Blocks[i].Cpoint.y - Blocks[i].Dpoint.y / 2 < 0) {
      Blocks[i].Cpoint.y = Blocks[i].Dpoint.y / 2 + (1.5) * Bin_D.y / 2;
      // Blocks[i].Cpoint.y = Blocks[i].Dpoint.y/2;
    }
  }
}

void Placement::Pull_back_vector(vector<float> &temp_vector, bool x_or_y) {  // 1 is x, 0 is y

  for (unsigned int i = 0; i < temp_vector.size(); ++i) {
    if (x_or_y) {
      if (temp_vector[i] + Blocks[i].Dpoint.x / 2 > Chip_D.x) {
        temp_vector[i] = Chip_D.x - Blocks[i].Dpoint.x / 2 - (1.5) * Bin_D.x / 2;
        // temp_vector[i] = Chip_D.x - Blocks[i].Dpoint.x/2;
      }
      if (temp_vector[i] - Blocks[i].Dpoint.x / 2 < 0) {
        temp_vector[i] = Blocks[i].Dpoint.x / 2 + (1.5) * Bin_D.x / 2;
        // temp_vector[i] = Blocks[i].Dpoint.x/2;
      }
    } else {
      if (temp_vector[i] + Blocks[i].Dpoint.y / 2 > Chip_D.y) {
        temp_vector[i] = Chip_D.y - Blocks[i].Dpoint.y / 2 - (1.5) * Bin_D.y / 2;
        // temp_vector[i] = Chip_D.y - Blocks[i].Dpoint.y/2;
      }
      if (temp_vector[i] - Blocks[i].Dpoint.y / 2 < 0) {
        temp_vector[i] = Blocks[i].Dpoint.y / 2 + (1.5) * Bin_D.y / 2;
        // temp_vector[i] = Blocks[i].Dpoint.y/2;
      }
    }
  }
}

void Placement::Initilize_Placement(PnRDB::hierNode &current_node) {
  int MaxElement = originalBlockCNT + current_node.Terminals.size();
  int n, m;
  vector<vector<float>> xa(MaxElement, vector<float>(MaxElement + 1, 0));
  vector<vector<float>> ya(MaxElement, vector<float>(MaxElement + 1, 0));
  vector<pair<float, float>> port(current_node.Terminals.size());
  for (unsigned int i = 0; i < current_node.Port_Location.size(); i++) {
    switch (current_node.Port_Location[i].pos) {
      case PnRDB::TL:
        port[current_node.Port_Location[i].tid].first = 0;
        port[current_node.Port_Location[i].tid].second = 1;
        break;
      case PnRDB::TC:
        port[current_node.Port_Location[i].tid].first = 0.5;
        port[current_node.Port_Location[i].tid].second = 1;
        break;
      case PnRDB::TR:
        port[current_node.Port_Location[i].tid].first = 1;
        port[current_node.Port_Location[i].tid].second = 1;
        break;
      case PnRDB::BL:
        port[current_node.Port_Location[i].tid].first = 0;
        port[current_node.Port_Location[i].tid].second = 0;
        break;
      case PnRDB::BC:
        port[current_node.Port_Location[i].tid].first = 0.5;
        port[current_node.Port_Location[i].tid].second = 0;
        break;
      case PnRDB::BR:
        port[current_node.Port_Location[i].tid].first = 1;
        port[current_node.Port_Location[i].tid].second = 0;
        break;
      case PnRDB::RT:
        port[current_node.Port_Location[i].tid].first = 1;
        port[current_node.Port_Location[i].tid].second = 1;
        break;
      case PnRDB::RC:
        port[current_node.Port_Location[i].tid].first = 1;
        port[current_node.Port_Location[i].tid].second = 0.5;
        break;
      case PnRDB::RB:
        port[current_node.Port_Location[i].tid].first = 1;
        port[current_node.Port_Location[i].tid].second = 0;
        break;
      case PnRDB::LT:
        port[current_node.Port_Location[i].tid].first = 0;
        port[current_node.Port_Location[i].tid].second = 1;
        break;
      case PnRDB::LC:
        port[current_node.Port_Location[i].tid].first = 0;
        port[current_node.Port_Location[i].tid].second = 0.5;
        break;
      case PnRDB::LB:
        port[current_node.Port_Location[i].tid].first = 0;
        port[current_node.Port_Location[i].tid].second = 0;
        break;
      default:
        break;
    }
  }
  for (int i = 0; i < originalNetCNT; i++) {
    for (unsigned int j = 0; j < current_node.Nets[i].connected.size(); j++) {
      for (unsigned int k = j + 1; k < current_node.Nets[i].connected.size(); k++) {
        if (current_node.Nets[i].connected[j].iter2 >= 0 && current_node.Nets[i].connected[k].iter2 >= 0) {
          xa[current_node.Nets[i].connected[j].iter2][current_node.Nets[i].connected[k].iter2] -= 1;
          xa[current_node.Nets[i].connected[k].iter2][current_node.Nets[i].connected[j].iter2] -= 1;
          xa[current_node.Nets[i].connected[k].iter2][current_node.Nets[i].connected[k].iter2] += 1;
          xa[current_node.Nets[i].connected[j].iter2][current_node.Nets[i].connected[j].iter2] += 1;
          ya[current_node.Nets[i].connected[j].iter2][current_node.Nets[i].connected[k].iter2] -= 1;
          ya[current_node.Nets[i].connected[k].iter2][current_node.Nets[i].connected[j].iter2] -= 1;
          ya[current_node.Nets[i].connected[k].iter2][current_node.Nets[i].connected[k].iter2] += 1;
          ya[current_node.Nets[i].connected[j].iter2][current_node.Nets[i].connected[j].iter2] += 1;
        } else if (current_node.Nets[i].connected[j].iter2 == -1) {
          xa[current_node.Nets[i].connected[j].iter + originalBlockCNT][current_node.Nets[i].connected[j].iter + originalBlockCNT] = 1;
          xa[current_node.Nets[i].connected[j].iter + originalBlockCNT][MaxElement] = port[current_node.Nets[i].connected[j].iter].first;
          xa[current_node.Nets[i].connected[k].iter2][current_node.Nets[i].connected[k].iter2] += 1;
          xa[current_node.Nets[i].connected[k].iter2][MaxElement] += port[current_node.Nets[i].connected[j].iter].first;
          ya[current_node.Nets[i].connected[j].iter + originalBlockCNT][current_node.Nets[i].connected[j].iter + originalBlockCNT] = 1;
          ya[current_node.Nets[i].connected[j].iter + originalBlockCNT][MaxElement] = port[current_node.Nets[i].connected[j].iter].second;
          ya[current_node.Nets[i].connected[k].iter2][current_node.Nets[i].connected[k].iter2] += 1;
          ya[current_node.Nets[i].connected[k].iter2][MaxElement] += port[current_node.Nets[i].connected[j].iter].second;
        } else if (current_node.Nets[i].connected[k].iter2 == -1) {
          xa[current_node.Nets[i].connected[j].iter2][current_node.Nets[i].connected[j].iter2] += 1;
          xa[current_node.Nets[i].connected[j].iter2][MaxElement] += port[current_node.Nets[i].connected[k].iter].first;
          xa[current_node.Nets[i].connected[k].iter + originalBlockCNT][current_node.Nets[i].connected[k].iter + originalBlockCNT] = 1;
          xa[current_node.Nets[i].connected[k].iter + originalBlockCNT][MaxElement] = port[current_node.Nets[i].connected[k].iter].first;
          ya[current_node.Nets[i].connected[j].iter2][current_node.Nets[i].connected[j].iter2] += 1;
          ya[current_node.Nets[i].connected[j].iter2][MaxElement] += port[current_node.Nets[i].connected[k].iter].second;
          ya[current_node.Nets[i].connected[k].iter + originalBlockCNT][current_node.Nets[i].connected[k].iter + originalBlockCNT] = 1;
          ya[current_node.Nets[i].connected[k].iter + originalBlockCNT][MaxElement] = port[current_node.Nets[i].connected[k].iter].second;
        }
      }
    }
  }
  int i, j;
  n = MaxElement;
  for (j = 0; j < n; j++) {
    float max = 0;
    float imax = 0;
    for (i = j; i < n; i++) {
      if (imax < abs(xa[i][j])) {
        imax = abs(xa[i][j]);
        max = xa[i][j];
        m = i;
      }
    }
    if (abs(xa[j][j]) != max) {
      float b = 0;
      for (int k = j; k < n + 1; k++) {
        b = xa[j][k];
        xa[j][k] = xa[m][k];
        xa[m][k] = b;
      }
    }
    for (int r = j; r < n + 1; r++) {
      xa[j][r] = xa[j][r] / max;
    }
    for (i = j + 1; i < n; i++) {
      float c = xa[i][j];
      if (c == 0) continue;
      for (int s = j; s < n + 1; s++) {
        float tempdata = xa[i][s];
        xa[i][s] = xa[i][s] - xa[j][s] * c;
      }
    }
  }
  for (i = n - 2; i >= 0; i--) {
    for (j = i + 1; j < n; j++) {
      float tempData = xa[i][j];
      float data1 = xa[i][n];
      float data2 = xa[j][n];
      xa[i][n] = xa[i][n] - xa[j][n] * xa[i][j];
    }
  }
  for (j = 0; j < n; j++) {
    float max = 0;
    float imax = 0;
    for (i = j; i < n; i++) {
      if (imax < abs(ya[i][j])) {
        imax = abs(ya[i][j]);
        max = ya[i][j];
        m = i;
      }
    }
    if (abs(ya[j][j]) != max) {
      float b = 0;
      for (int k = j; k < n + 1; k++) {
        b = ya[j][k];
        ya[j][k] = ya[m][k];
        ya[m][k] = b;
      }
    }
    for (int r = j; r < n + 1; r++) {
      ya[j][r] = ya[j][r] / max;
    }
    for (i = j + 1; i < n; i++) {
      float c = ya[i][j];
      if (c == 0) continue;
      for (int s = j; s < n + 1; s++) {
        float tempdata = ya[i][s];
        ya[i][s] = ya[i][s] - ya[j][s] * c;
      }
    }
  }
  for (i = n - 2; i >= 0; i--) {
    for (j = i + 1; j < n; j++) {
      float tempData = ya[i][j];
      float data1 = ya[i][n];
      float data2 = ya[j][n];
      ya[i][n] = ya[i][n] - ya[j][n] * ya[i][j];
    }
  }
  for (unsigned int i = 0; i < originalBlockCNT; ++i) {
    Blocks[i].Cpoint.x = xa[i][MaxElement];
    Blocks[i].Cpoint.y = ya[i][MaxElement];
  }
  for (int i = originalBlockCNT; i < Blocks.size(); ++i) {
    int id = Blocks[i].splitedsource;
    Blocks[i].Cpoint.x = Blocks[id].Cpoint.x + (float)(rand() % 200) / 1000;
    Blocks[i].Cpoint.y = Blocks[id].Cpoint.y + (float)(rand() % 20) / 1000;
    // Blocks[i].Cpoint.x = 0.45 + (float)(rand() % 100) / 1000;
    // Blocks[i].Cpoint.y = 0.45 + (float)(rand() % 100) / 1000;
  }
  // refine_CC();
  restore_CC_in_square(true);
}

void Placement::Initilize_Placement_Rand(PnRDB::hierNode &current_node) {
  for (unsigned int i = 0; i < originalBlockCNT; ++i) {
    if (Blocks[i].Cpoint.x < 0.3 or Blocks[i].Cpoint.y < 0.3) {
      Blocks[i].Cpoint.x = 0.3 + (float)(rand() % 400) / 1000;
      Blocks[i].Cpoint.y = 0.3 + (float)(rand() % 400) / 1000;
    }
  }
  for (int i = originalBlockCNT; i < Blocks.size(); ++i) {
    int id = Blocks[i].splitedsource;
    Blocks[i].Cpoint.x = Blocks[id].Cpoint.x - 0.1 + (float)(rand() % 200) / 1000;
    Blocks[i].Cpoint.y = Blocks[id].Cpoint.y - 0.1 + (float)(rand() % 200) / 1000;
    // Blocks[i].Cpoint.x = 0.45 + (float)(rand() % 100) / 1000;
    // Blocks[i].Cpoint.y = 0.45 + (float)(rand() % 100) / 1000;
  }
  // refine_CC();
  restore_CC_in_square(true);
}

void Placement::Update_Bin_Density() {
  float unit_density = 1;

  for (unsigned int i = 0; i < Bins.size(); ++i) {
    for (unsigned int j = 0; j < Bins[i].size(); ++j) {
      Bins[i][j].density = 0.0;
    }
  }

  for (unsigned int i = 0; i < Bins.size(); ++i) {
    for (unsigned int j = 0; j < Bins[i].size(); ++j) {
      for (unsigned int k = 0; k < Blocks.size(); ++k) {
        float x_common_length = 0.0;
        bool x_common;
        x_common = Find_Common_Area(Blocks[k].Cpoint.x, Blocks[k].Dpoint.x, Bins[i][j].Cpoint.x, Bins[i][j].Dpoint.x, x_common_length);
        float y_common_length = 0.0;
        bool y_common;
        y_common = Find_Common_Area(Blocks[k].Cpoint.y, Blocks[k].Dpoint.y, Bins[i][j].Cpoint.y, Bins[i][j].Dpoint.y, y_common_length);

        if (x_common and y_common) {
          Bins[i][j].density += unit_density * x_common_length * y_common_length;
        }
      }

      Bins[i][j].density = Bins[i][j].density / (Bin_D.x * Bin_D.y);
    }
  }
}

bool Placement::Find_Common_Area(float x_center_block, float block_width, float x_center_bin, float bin_width, float &common_length) {
  float x_lower_block = x_center_block - block_width / 2;
  float x_upper_block = x_center_block + block_width / 2;
  float x_lower_bin = x_center_bin - bin_width / 2;
  float x_upper_bin = x_center_bin + bin_width / 2;

  float eqv_x_lower = max(x_lower_block, x_lower_bin);
  float eqv_x_upper = min(x_upper_block, x_upper_bin);

  common_length = eqv_x_upper - eqv_x_lower;

  if (common_length > 0) {
    return true;
  } else {
    return false;
  }
}

void Placement::Cal_Eforce_Block(int block_id) {
  // Q: should compare with replace's implementation
  Blocks[block_id].Eforce.x = 0.0;
  Blocks[block_id].Eforce.y = 0.0;

  for (unsigned int i = 0; i < Bins.size(); ++i) {
    for (unsigned int j = 0; j < Bins[i].size(); ++j) {
      float x_common_length;
      bool x_common;
      x_common = Find_Common_Area(Blocks[block_id].Cpoint.x, Blocks[block_id].Dpoint.x, Bins[i][j].Cpoint.x, Bins[i][j].Dpoint.x, x_common_length);
      float y_common_length;
      bool y_common;
      y_common = Find_Common_Area(Blocks[block_id].Cpoint.y, Blocks[block_id].Dpoint.y, Bins[i][j].Cpoint.y, Bins[i][j].Dpoint.y, y_common_length);

      if (x_common and y_common) {  // Q: should be x_common_length*y_common_length?
        Blocks[block_id].Eforce.x += (y_common_length * x_common_length / (Bin_D.x * Bin_D.y)) * Bins[i][j].Eforce.x;
        Blocks[block_id].Eforce.y += (y_common_length * x_common_length / (Bin_D.x * Bin_D.y)) * Bins[i][j].Eforce.y;
      }
    }
  }
  // #ifdef DEBUG
  std::cout << "blocks gradient " << Blocks[block_id].Eforce.x << " " << Blocks[block_id].Eforce.y << std::endl;
  // #endif
}

float Placement::Cal_HPWL() {
  float HPWL = 0;
  for (unsigned int i = 0; i < Nets.size(); ++i) {
    vector<float> x_value;
    vector<float> y_value;
    for (unsigned int j = 0; j < Nets[i].connected_block.size(); ++j) {
      int block_index = Nets[i].connected_block[j];
      x_value.push_back(Blocks[block_index].Cpoint.x);
      y_value.push_back(Blocks[block_index].Cpoint.y);
    }
    float max_x = x_value[0];
    float min_x = x_value[0];
    float max_y = y_value[0];
    float min_y = y_value[0];
    for (unsigned int j = 0; j < x_value.size(); ++j) {
      if (max_x < x_value[j]) max_x = x_value[j];
      if (min_x > x_value[j]) min_x = x_value[j];
      if (max_y < y_value[j]) max_y = y_value[j];
      if (min_y > y_value[j]) min_y = y_value[j];
    }
    HPWL += abs(max_x - min_x) + abs(max_y - min_y);
  }
  return HPWL;
}

void Placement::PlotPlacement(int index) {
  string outfile = to_string(index) + ".plt";
#ifdef DEBUG
  cout << "create gnuplot file" << endl;
#endif
  ofstream fout;
  fout.open(outfile.c_str());

  // set title
  fout << "#Use this file as a script for gnuplot\n#(See http://www.gnuplot.info/ for details)" << endl;
  fout << "\nset title\" #Blocks= " << Blocks.size() << ", #Nets= " << Nets.size() << ", Area=" << Chip_D.x * Chip_D.y << ", HPWL= " << Cal_HPWL() << " \""
       << endl;
  fout << "\nset nokey" << endl;
  fout << "#   Uncomment these two lines starting with \"set\"" << endl;
  fout << "#   to save an EPS file for inclusion into a latex document" << endl;
  fout << "# set terminal postscript eps color solid 20" << endl;
  fout << "# set output \"result.eps\"" << endl << endl;
  fout << "#   Uncomment these two lines starting with \"set\"" << endl;
  fout << "#   to save a PS file for printing" << endl;
  fout << "set term jpeg" << endl;
  fout << "set output \"" << to_string(index) + ".jpg"
       << "\"" << endl
       << endl;

  // set range
  float bias = 0;
  fout << "\nset xrange [" << 0.0 - bias << ":" << Chip_D.x + bias << "]" << endl;
  fout << "\nset yrange [" << 0.0 - bias << ":" << Chip_D.y + bias << "]" << endl;

  // set labels for blocks
  /*
    for(int i=0;i<(int)Blocks.size();++i) {
      fout<<"\nset label \""<<" B"+to_string(i)<<"\" at "<<Blocks[i].Cpoint.x<<" , "<<Blocks[i].Cpoint.y<<" center "<<endl;
    }
    */

  for (int i = 0; i < originalBlockCNT; ++i) {
    // fout << "\nset label \"" << Blocks[i].blockname << "\" at " << Blocks[i].Cpoint.x << " , " << Blocks[i].Cpoint.y << " center " << endl;
    fout << "\nset label \"" << Blocks[i].blockname << "\" at " << Blocks[i].Cpoint.x << " , " << Blocks[i].Cpoint.y << " center " << endl;
  }

  // fout << "\nplot[:][:] \'-\' with lines linestyle 3 lc 2,  \'-\' with lines linestyle 7 lc 2, "<<
  //       "\'-\' with lines linestyle 1 lc 2, \'-\' with lines linestyle 0 lc 2" << endl
  //      << endl;
  ;

  for (int i = 0; i < originalBlockCNT; ++i) {
    fout << "\nplot[:][:] \'-\' with lines linestyle 3 lc " << to_string(2 * i) << ",  \'-\' with lines linestyle 7 lc " << to_string(i * 2) << ", "
         << "\'-\' with lines linestyle 1 lc " << to_string(i * 2) << ", \'-\' with lines linestyle 0 lc " << to_string(i * 2) << "" << endl
         << endl;
    fout << "\t" << Blocks[i].Cpoint.x - Blocks[i].Dpoint.x / 2 << "\t" << Blocks[i].Cpoint.y - Blocks[i].Dpoint.y / 2 << endl;
    fout << "\t" << Blocks[i].Cpoint.x - Blocks[i].Dpoint.x / 2 << "\t" << Blocks[i].Cpoint.y + Blocks[i].Dpoint.y / 2 << endl;
    fout << "\t" << Blocks[i].Cpoint.x + Blocks[i].Dpoint.x / 2 << "\t" << Blocks[i].Cpoint.y + Blocks[i].Dpoint.y / 2 << endl;
    fout << "\t" << Blocks[i].Cpoint.x + Blocks[i].Dpoint.x / 2 << "\t" << Blocks[i].Cpoint.y - Blocks[i].Dpoint.y / 2 << endl;
    fout << "\t" << Blocks[i].Cpoint.x - Blocks[i].Dpoint.x / 2 << "\t" << Blocks[i].Cpoint.y - Blocks[i].Dpoint.y / 2 << endl;
    fout << endl;
  }
  fout << "\nEOF" << endl;
  /*
    // plot connections
    for(int i=0;i<Nets.size();i++){
      for(int j=0;j<Nets[i].connected_block.size()-1;j++){
        int first_block_index = Nets[i].connected_block[j];
        int second_block_index = Nets[i].connected_block[j+1];
        fout<<"\t"<<Blocks[first_block_index].Cpoint.x<<"\t"<<Blocks[first_block_index].Cpoint.y<<endl;
        fout<<"\t"<<Blocks[second_block_index].Cpoint.x<<"\t"<<Blocks[second_block_index].Cpoint.y<<endl;
        fout<<"\t"<<Blocks[first_block_index].Cpoint.x<<"\t"<<Blocks[first_block_index].Cpoint.y<<endl<<endl;
      }
      if(Nets[i].connected_block.size()-1>0) fout<<"\nEOF"<<endl;
    }
    */

  fout.close();
}

// WA model
// void Placement::Cal_WA_Net_Force()
// {

//   for (unsigned int i = 0; i < Nets.size(); ++i)
//   {

//     Nets[i].PSumNetforce.x = LSE_Net_SUM_P(i, 1);
//     Nets[i].PSumNetforce.y = LSE_Net_SUM_P(i, 0);
//     Nets[i].NSumNetforce.x = LSE_Net_SUM_N(i, 1);
//     Nets[i].NSumNetforce.y = LSE_Net_SUM_N(i, 0);

//     Nets[i].PSumNetforce_WA.x = WA_Net_SUM_P(i, 1);
//     Nets[i].PSumNetforce_WA.y = WA_Net_SUM_P(i, 0);
//     Nets[i].NSumNetforce_WA.x = WA_Net_SUM_N(i, 1);
//     Nets[i].NSumNetforce_WA.y = WA_Net_SUM_N(i, 0);
//   }

//   for (unsigned int i = 0; i < Blocks.size(); ++i)
//   {

//     Blocks[i].Net_block_force_P.x = LSE_block_P(i, 1);
//     Blocks[i].Net_block_force_P.y = LSE_block_P(i, 0);
//     Blocks[i].Net_block_force_N.x = LSE_block_N(i, 1);
//     Blocks[i].Net_block_force_N.y = LSE_block_N(i, 0);
//   }

//   for (unsigned int i = 0; i < Blocks.size(); ++i)
//   {
//     Blocks[i].Netforce.x = 0;
//     Blocks[i].Netforce.y = 0;
//     for (unsigned int j = 0; j < Blocks[i].connected_net.size(); j++)
//     {
//       int net_index = Blocks[i].connected_net[j];

//       Ppoint_F PSumNetforce = Nets[net_index].PSumNetforce;
//       Ppoint_F NSumNetforce = Nets[net_index].NSumNetforce;
//       Ppoint_F PSumNetforce_WA = Nets[net_index].PSumNetforce_WA;
//       Ppoint_F NSumNetforce_WA = Nets[net_index].NSumNetforce_WA;
//       float x_positive = ((1 + Blocks[i].Cpoint.x / gammar) * Blocks[i].Net_block_force_P.x * PSumNetforce.x - Blocks[i].Net_block_force_P.x *
//       PSumNetforce_WA.x) / (PSumNetforce.x * PSumNetforce.x); float x_nagative = ((1 + Blocks[i].Cpoint.x / gammar) * Blocks[i].Net_block_force_N.x *
//       NSumNetforce.x - Blocks[i].Net_block_force_N.x * NSumNetforce_WA.x) / (NSumNetforce.x * NSumNetforce.x); float y_positive = ((1 + Blocks[i].Cpoint.y /
//       gammar) * Blocks[i].Net_block_force_P.y * PSumNetforce.y - Blocks[i].Net_block_force_P.y * PSumNetforce_WA.y) / (PSumNetforce.y * PSumNetforce.y);
//       float y_nagative = ((1 + Blocks[i].Cpoint.y / gammar) * Blocks[i].Net_block_force_N.y * NSumNetforce.y - Blocks[i].Net_block_force_N.y *
//       NSumNetforce_WA.y) / (NSumNetforce.y * NSumNetforce.y); Blocks[i].Netforce.x += x_positive - x_nagative; Blocks[i].Netforce.y += y_positive -
//       y_nagative;
//     }
//   }
// }

void Placement::Cal_WA_Net_Force() {
  for (unsigned int i = 0; i < Nets.size(); ++i) {
    Nets[i].PSumNetforce.x = LSE_Net_SUM_P(i, 1);
    Nets[i].PSumNetforce.y = LSE_Net_SUM_P(i, 0);
    Nets[i].NSumNetforce.x = LSE_Net_SUM_N(i, 1);
    Nets[i].NSumNetforce.y = LSE_Net_SUM_N(i, 0);

    Nets[i].PSumNetforce_WA.x = WA_Net_SUM_P(i, 1);
    Nets[i].PSumNetforce_WA.y = WA_Net_SUM_P(i, 0);
    Nets[i].NSumNetforce_WA.x = WA_Net_SUM_N(i, 1);
    Nets[i].NSumNetforce_WA.y = WA_Net_SUM_N(i, 0);

    std::cout << "net sum " << i << " " << Nets[i].PSumNetforce.x << " " << Nets[i].PSumNetforce.y << " " << Nets[i].NSumNetforce.x << " "
              << Nets[i].NSumNetforce.y << " " << Nets[i].PSumNetforce_WA.x << " " << Nets[i].PSumNetforce_WA.y << " " << Nets[i].NSumNetforce_WA.x << " "
              << Nets[i].NSumNetforce_WA.y << std::endl;
  }

  for (unsigned int i = 0; i < Blocks.size(); ++i) {
    Blocks[i].Net_block_force_P.x = LSE_block_P(i, 1);
    Blocks[i].Net_block_force_P.y = LSE_block_P(i, 0);
    Blocks[i].Net_block_force_N.x = LSE_block_N(i, 1);
    Blocks[i].Net_block_force_N.y = LSE_block_N(i, 0);
    std::cout << "block single net force " << Blocks[i].Net_block_force_P.x << " " << Blocks[i].Net_block_force_P.y << " " << Blocks[i].Net_block_force_N.x
              << " " << Blocks[i].Net_block_force_N.y << std::endl;
  }

  for (unsigned int i = 0; i < Blocks.size(); ++i) {
    Blocks[i].Netforce.x = 0;
    Blocks[i].Netforce.y = 0;
    std::cout << "block " << i << std::endl;
    for (unsigned int j = 0; j < Blocks[i].connected_net.size(); j++) {
      int net_index = Blocks[i].connected_net[j];

      Ppoint_F PSumNetforce = Nets[net_index].PSumNetforce;
      Ppoint_F NSumNetforce = Nets[net_index].NSumNetforce;
      Ppoint_F PSumNetforce_WA = Nets[net_index].PSumNetforce_WA;
      Ppoint_F NSumNetforce_WA = Nets[net_index].NSumNetforce_WA;
      std::cout << "block info " << i << " net index " << net_index << " " << Nets[net_index].PSumNetforce.x << " " << Nets[net_index].PSumNetforce.y << " "
                << Nets[net_index].NSumNetforce.x << " " << Nets[net_index].NSumNetforce.y << " " << Nets[net_index].PSumNetforce_WA.x << " "
                << Nets[net_index].PSumNetforce_WA.y << " " << Nets[net_index].NSumNetforce_WA.x << " " << Nets[net_index].NSumNetforce_WA.y << std::endl;

      float x_positive =
          ((1 + Blocks[i].Cpoint.x / gammar) * Blocks[i].Net_block_force_P.x * PSumNetforce.x - Blocks[i].Net_block_force_P.x * PSumNetforce_WA.x) /
          (PSumNetforce.x * PSumNetforce.x);
      float x_nagative =
          ((1 + Blocks[i].Cpoint.x / gammar) * Blocks[i].Net_block_force_N.x * NSumNetforce.x - Blocks[i].Net_block_force_N.x * NSumNetforce_WA.x) /
          (NSumNetforce.x * NSumNetforce.x);
      float y_positive =
          ((1 + Blocks[i].Cpoint.y / gammar) * Blocks[i].Net_block_force_P.y * PSumNetforce.y - Blocks[i].Net_block_force_P.y * PSumNetforce_WA.y) /
          (PSumNetforce.y * PSumNetforce.y);
      float y_nagative =
          ((1 + Blocks[i].Cpoint.y / gammar) * Blocks[i].Net_block_force_N.y * NSumNetforce.y - Blocks[i].Net_block_force_N.y * NSumNetforce_WA.y) /
          (NSumNetforce.y * NSumNetforce.y);
      if (net_index >= originalNetCNT) {
        Blocks[i].Netforce.x += dummy_net_weight * (x_positive - x_nagative);
        Blocks[i].Netforce.y += dummy_net_weight * (y_positive - y_nagative);
      } else {
        Blocks[i].Netforce.x += (x_positive - x_nagative);
        Blocks[i].Netforce.y += (y_positive - y_nagative);
      }
    }
    std::cout << "block net force " << i << " force " << Blocks[i].Netforce.x << " " << Blocks[i].Netforce.y << std::endl;
  }
}

float Placement::WA_Net_SUM_P(int net_index, bool x_or_y) {
  // 1/r *( sum xi*exp(xi/r) )

  float result = 0.0;

  for (unsigned int i = 0; i < Nets[net_index].connected_block.size(); i++) {
    int block_index = Nets[net_index].connected_block[i];

    if (x_or_y) {  // 1 for x
      result += Blocks[block_index].Cpoint.x * Exp_Function(Blocks[block_index].Cpoint.x, gammar);
    } else {
      result += Blocks[block_index].Cpoint.y * Exp_Function(Blocks[block_index].Cpoint.y, gammar);
    }
  }

  return result / gammar;
}

float Placement::WA_Net_SUM_N(int net_index, bool x_or_y) {
  // 1/r *( sum xi*exp(-xi/r) )

  float result = 0.0;

  for (unsigned int i = 0; i < Nets[net_index].connected_block.size(); i++) {
    int block_index = Nets[net_index].connected_block[i];

    if (x_or_y) {  // 1 for x
      result += Blocks[block_index].Cpoint.x * Exp_Function(-Blocks[block_index].Cpoint.x, gammar);
    } else {
      result += Blocks[block_index].Cpoint.y * Exp_Function(-Blocks[block_index].Cpoint.y, gammar);
    }
  }

  return result / gammar;
}
// End WA model

void Placement::Cal_WA_Area_Force() {
  float Area_SUM_P_X = Area_SUM_P(1);
  float Area_SUM_P_X_2 = Area_SUM_P_X * Area_SUM_P_X;
  float Area_SUM_P_Y = Area_SUM_P(0);
  float Area_SUM_P_Y_2 = Area_SUM_P_Y * Area_SUM_P_Y;

  float Area_SUM_N_X = Area_SUM_N(1);
  float Area_SUM_N_X_2 = Area_SUM_N_X * Area_SUM_N_X;
  float Area_SUM_N_Y = Area_SUM_N(0);
  float Area_SUM_N_Y_2 = Area_SUM_N_Y * Area_SUM_N_Y;

  float Area_SUM_P_WA_X = Area_SUM_P_WA(1);
  float Area_SUM_P_WA_Y = Area_SUM_P_WA(0);
  float Area_SUM_N_WA_X = Area_SUM_N_WA(1);
  float Area_SUM_N_WA_Y = Area_SUM_N_WA(0);

  float WA_X = Area_SUM_P_WA_X / Area_SUM_P_X - Area_SUM_N_WA_X / Area_SUM_N_X;
  float WA_Y = Area_SUM_P_WA_Y / Area_SUM_P_Y - Area_SUM_N_WA_Y / Area_SUM_N_Y;

  for (unsigned int i = 0; i < Blocks.size(); ++i) {
    Blocks[i].Net_block_force_P.x = LSE_block_P(i, 1);
    Blocks[i].Net_block_force_P.y = LSE_block_P(i, 0);
    Blocks[i].Net_block_force_N.x = LSE_block_N(i, 1);
    Blocks[i].Net_block_force_N.y = LSE_block_N(i, 0);
  }

  for (unsigned int i = 0; i < Blocks.size(); ++i) {
    float x_positive = ((1 + Blocks[i].Cpoint.x / gammar) * Blocks[i].Net_block_force_P.x * Area_SUM_P_X - Blocks[i].Net_block_force_P.x * Area_SUM_P_WA_X) /
                       (Area_SUM_P_X * Area_SUM_P_X);
    float x_nagative = ((1 + Blocks[i].Cpoint.x / gammar) * Blocks[i].Net_block_force_N.x * Area_SUM_N_X - Blocks[i].Net_block_force_N.x * Area_SUM_N_WA_X) /
                       (Area_SUM_N_X * Area_SUM_N_X);
    float y_positive = ((1 + Blocks[i].Cpoint.y / gammar) * Blocks[i].Net_block_force_P.y * Area_SUM_P_Y - Blocks[i].Net_block_force_P.y * Area_SUM_P_WA_Y) /
                       (Area_SUM_P_Y * Area_SUM_P_Y);
    float y_nagative = ((1 + Blocks[i].Cpoint.y / gammar) * Blocks[i].Net_block_force_N.y * Area_SUM_N_Y - Blocks[i].Net_block_force_N.y * Area_SUM_N_WA_Y) /
                       (Area_SUM_N_Y * Area_SUM_N_Y);
    Blocks[i].Areaforce.x = (x_positive - x_nagative) * WA_Y;
    Blocks[i].Areaforce.y = (y_positive - y_nagative) * WA_X;
  }
}

// Area model
void Placement::Cal_LSE_Area_Force() {
  float Area_SUM_P_X = Area_SUM_P(1);
  float Area_SUM_P_Y = Area_SUM_P(0);

  float Area_SUM_N_X = Area_SUM_N(1);
  float Area_SUM_N_Y = Area_SUM_N(0);

  float LSE_X = gammar * (log((double)Area_SUM_P_X) - log((double)Area_SUM_N_X));
  float LSE_Y = gammar * (log((double)Area_SUM_P_Y) - log((double)Area_SUM_N_Y));

  for (unsigned int i = 0; i < Blocks.size(); ++i) {
    Blocks[i].Net_block_force_P.x = LSE_block_P(i, 1);
    Blocks[i].Net_block_force_P.y = LSE_block_P(i, 0);
    Blocks[i].Net_block_force_N.x = LSE_block_N(i, 1);
    Blocks[i].Net_block_force_N.y = LSE_block_N(i, 0);
  }

  for (unsigned int i = 0; i < Blocks.size(); ++i) {
    Blocks[i].Areaforce.x = (Blocks[i].Net_block_force_P.x / Area_SUM_P_X - Blocks[i].Net_block_force_N.x / Area_SUM_N_X) * LSE_Y;
    Blocks[i].Areaforce.y = (Blocks[i].Net_block_force_P.y / Area_SUM_P_Y - Blocks[i].Net_block_force_N.y / Area_SUM_N_Y) * LSE_X;
  }
}

// LSE model
void Placement::Cal_LSE_Net_Force() {
  for (unsigned int i = 0; i < Nets.size(); ++i) {
    Nets[i].PSumNetforce.x = LSE_Net_SUM_P(i, 1);
    Nets[i].PSumNetforce.y = LSE_Net_SUM_P(i, 0);
    Nets[i].NSumNetforce.x = LSE_Net_SUM_N(i, 1);
    Nets[i].NSumNetforce.y = LSE_Net_SUM_N(i, 0);
  }

  for (unsigned int i = 0; i < Blocks.size(); ++i) {
    Blocks[i].Net_block_force_P.x = LSE_block_P(i, 1);
    Blocks[i].Net_block_force_P.y = LSE_block_P(i, 0);
    Blocks[i].Net_block_force_N.x = LSE_block_N(i, 1);
    Blocks[i].Net_block_force_N.y = LSE_block_N(i, 0);
  }

  for (unsigned int i = 0; i < Blocks.size(); ++i) {
    Blocks[i].Netforce.x = 0;
    Blocks[i].Netforce.y = 0;
    for (unsigned int j = 0; j < Blocks[i].connected_net.size(); j++) {
      int net_index = Blocks[i].connected_net[j];
      Ppoint_F PSumNetforce = Nets[net_index].PSumNetforce;
      Ppoint_F NSumNetforce = Nets[net_index].NSumNetforce;
      Blocks[i].Netforce.x += Blocks[i].Net_block_force_P.x / PSumNetforce.x - Blocks[i].Net_block_force_N.x / NSumNetforce.x;
      Blocks[i].Netforce.y += Blocks[i].Net_block_force_P.y / PSumNetforce.y - Blocks[i].Net_block_force_N.y / NSumNetforce.y;
    }
  }
}

float Placement::Area_SUM_P(bool x_or_y) {
  float result = 0.0;

  for (unsigned int i = 0; i < Blocks.size(); ++i) {
    if (x_or_y) {
      result += Exp_Function(Blocks[i].Cpoint.x, gammar);
    } else {
      result += Exp_Function(Blocks[i].Cpoint.y, gammar);
    }
  }

  return result;
}

float Placement::Area_SUM_N(bool x_or_y) {
  float result = 0.0;

  for (unsigned int i = 0; i < Blocks.size(); ++i) {
    if (x_or_y) {
      result += Exp_Function(-Blocks[i].Cpoint.x, gammar);
    } else {
      result += Exp_Function(-Blocks[i].Cpoint.y, gammar);
    }
  }

  return result;
}

float Placement::Area_SUM_P_WA(bool x_or_y) {
  //( sum xi*exp(xi/r) )

  float result = 0.0;

  for (unsigned int i = 0; i < Blocks.size(); i++) {
    int block_index = i;

    if (x_or_y) {  // 1 for x
      result += Blocks[block_index].Cpoint.x * Exp_Function(Blocks[block_index].Cpoint.x, gammar);
    } else {
      result += Blocks[block_index].Cpoint.y * Exp_Function(Blocks[block_index].Cpoint.y, gammar);
    }
  }

  return result;
}

float Placement::Area_SUM_N_WA(bool x_or_y) {
  //( sum xi*exp(xi/r) )

  float result = 0.0;

  for (unsigned int i = 0; i < Blocks.size(); i++) {
    int block_index = i;

    if (x_or_y) {  // 1 for x
      result += Blocks[block_index].Cpoint.x * Exp_Function(-Blocks[block_index].Cpoint.x, gammar);
    } else {
      result += Blocks[block_index].Cpoint.y * Exp_Function(-Blocks[block_index].Cpoint.y, gammar);
    }
  }

  return result;
}

float Placement::LSE_Net_SUM_P(int net_index, bool x_or_y) {
  float result = 0.0;

  for (unsigned int i = 0; i < Nets[net_index].connected_block.size(); i++) {
    int block_index = Nets[net_index].connected_block[i];

    if (x_or_y) {  // 1 for x
      result += Exp_Function(Blocks[block_index].Cpoint.x, gammar);
#ifdef DEBUG
      std::cout << "lse exp result " << Exp_Function(Blocks[block_index].Cpoint.x, gammar) << std::endl;
#endif
    } else {
      result += Exp_Function(Blocks[block_index].Cpoint.y, gammar);
#ifdef DEBUG
      std::cout << "lse exp result " << Exp_Function(Blocks[block_index].Cpoint.x, gammar) << std::endl;
#endif
    }
  }
#ifdef DEBUG
  std::cout << "lse exp sum result " << result << std::endl;
#endif
  return result;
}

float Placement::LSE_Net_SUM_N(int net_index, bool x_or_y) {
  float result = 0.0;

  for (unsigned int i = 0; i < Nets[net_index].connected_block.size(); i++) {
    int block_index = Nets[net_index].connected_block[i];

    if (x_or_y) {  // 1 for x
      result += Exp_Function(-Blocks[block_index].Cpoint.x, gammar);
#ifdef DEBUG
      std::cout << "lse exp result " << Exp_Function(Blocks[block_index].Cpoint.x, gammar) << std::endl;
#endif
    } else {
      result += Exp_Function(-Blocks[block_index].Cpoint.y, gammar);
#ifdef DEBUG
      std::cout << "lse exp result " << Exp_Function(Blocks[block_index].Cpoint.x, gammar) << std::endl;
#endif
    }
  }
#ifdef DEBUG
  std::cout << "lse exp sum result " << result << std::endl;
#endif
  return result;
}

float Placement::LSE_block_P(int block_index, int x_or_y) {
  float result = 0.0;

  if (x_or_y) {  // 1 for x
    result += Exp_Function(Blocks[block_index].Cpoint.x, gammar);
  } else {
    result += Exp_Function(Blocks[block_index].Cpoint.y, gammar);
  }

  return result;
}

float Placement::LSE_block_N(int block_index, int x_or_y) {
  float result = 0.0;

  if (x_or_y) {  // 1 for x
    result += Exp_Function(-Blocks[block_index].Cpoint.x, gammar);
  } else {
    result += Exp_Function(-Blocks[block_index].Cpoint.y, gammar);
  }

  return result;
}

float Placement::Exp_Function(float x, float gammar) {
  // float result = exp(x/gammar);
  float offset = 0;
  // float result = Fast_Exp(x/gammar-offset);
  float result = exp(x / gammar - offset);
#ifdef DEBUG
  std::cout << "x " << x << "x/gammar " << x / gammar << " exp result " << result << std::endl;
#endif
  return result;
}

// Q: might need a fast exp cal function
// END LSE model

void Placement::Cal_Density_Eforce() {
#ifdef DEBUG
  cout << "start test fft functions" << endl;
#endif
#ifdef DEBUG
  std::cout << "Cal_Density_Eforce debug 0" << std::endl;
#endif
  int binCntX = x_dimension_bin;
  int binCntY = y_dimension_bin;
  float binSizeX = unit_x_bin;
  float binSizeY = unit_y_bin;
#ifdef DEBUG
  std::cout << "Cal_Density_Eforce debug 1" << std::endl;
#endif
  replace::FFT fft(binCntX, binCntY, binSizeX, binSizeY);
#ifdef DEBUG
  cout << "test flag 1" << endl;
  std::cout << "Cal_Density_Eforce debug 2" << std::endl;
#endif
  for (unsigned int i = 0; i < binCntX; ++i) {
    for (unsigned int j = 0; j < binCntY; j++) {
#ifdef DEBUG
      std::cout << "Bin: (" << i << ", " << j << ")" << std::endl;
      std::cout << "density:" << Bins[i][j].density << std::endl;
#endif
      fft.updateDensity(i, j, Bins[i][j].density);
    }
  }
#ifdef DEBUG
  std::cout << "Cal_Density_Eforce debug 3" << std::endl;
  cout << "test flag 2" << endl;
#endif
  fft.doFFT();
#ifdef DEBUG
  std::cout << "Cal_Density_Eforce debug 4" << std::endl;
  cout << "end test fft functions" << endl;
  std::cout << "Cal_Density_Eforce debug 5" << std::endl;
#endif
  for (unsigned int i = 0; i < binCntX; ++i) {
#ifdef DEBUG
    std::cout << "Cal_Density_Eforce debug 6" << std::endl;
#endif
    for (unsigned int j = 0; j < binCntY; ++j) {
      auto eForcePair = fft.getElectroForce(i, j);
      Bins[i][j].Eforce.x = eForcePair.first;
      Bins[i][j].Eforce.y = eForcePair.second;
#ifdef DEBUG
      std::cout << "Bin force " << Bins[i][j].Eforce.x << " " << Bins[i][j].Eforce.y << std::endl;
#endif
      float electroPhi = fft.getElectroPhi(i, j);
      Bins[i][j].Ephi = electroPhi;
    }
    // sumPhi_ += electroPhi*static_cast<float>(bin->nonPlaceArea()+bin->instPlacedArea()+bin->fillerArea());
  }
#ifdef DEBUG
  std::cout << "Cal_Density_Eforce debug 7" << std::endl;
#endif
  for (unsigned int i = 0; i < Blocks.size(); ++i) {
    Cal_Eforce_Block(i);
  }
#ifdef DEBUG
  std::cout << "Cal_Density_Eforce debug 8" << std::endl;
#endif
}

void Placement::Cal_Net_force() {
  // using lse or wa to calculated the force/gradient due to net
  // need a lse/wa kernel

  // lse functions?

  // wa functions?
}

void Placement::Cal_force() {
  for (unsigned int i = 0; i < Blocks.size(); ++i) {
    //  Blocks[i].Force.x = lambda*Blocks[i].Eforce.x - beta*Blocks[i].Netforce.x;
    //  Blocks[i].Force.y = lambda*Blocks[i].Eforce.y - beta*Blocks[i].Netforce.y;

    Blocks[i].Force.x = lambda * Blocks[i].Eforce.x - beta * Blocks[i].Netforce.x - sym_beta * Blocks[i].Symmetricforce.x - area_beta * Blocks[i].Areaforce.x;
    Blocks[i].Force.y = lambda * Blocks[i].Eforce.y - beta * Blocks[i].Netforce.y - sym_beta * Blocks[i].Symmetricforce.y - area_beta * Blocks[i].Areaforce.y;
    //  std::cout<<"symmetricforce/all"<<sym_beta*Blocks[i].Symmetricforce.x<<", "<<sym_beta*Blocks[i].Symmetricforce.y<<std::endl;
    //  if(isnan(Blocks[i].Force.x))
    //  {
    //    Blocks[i].Force.x = 0;
    //  }
    //  if(isnan(Blocks[i].Force.y))
    //  {
    //    Blocks[i].Force.y = 0;
    //  }
  }
}

bool Placement::Stop_Condition(float density, float &max_density) {
  // Pull_back();

  max_density = 0.0;
  for (unsigned int i = 0; i < Bins.size(); ++i) {
    for (unsigned int j = 0; j < Bins[i].size(); ++j) {
      if (Bins[i][j].density > max_density) {
        max_density = Bins[i][j].density;
      }
    }
  }
  std::cout << "max_density " << max_density << std::endl;
  if (max_density < density) {
    std::cout << "stop condition result: false" << std::endl;
    return false;
  } else {
    std::cout << "stop condition result: true" << std::endl;
    return true;
  }
}

float Placement::Cal_Overlap() {
  float max_overlap = 0.0f;

  for (unsigned int i = 0; i < Blocks.size(); ++i) {
    Blocks[i].overlap = 0.0f;

    for (unsigned int j = 0; j < Blocks.size(); ++j) {
      if (i != j) {
        float x_common_length = 0.0;
        bool x_common;
        x_common = Find_Common_Area(Blocks[i].Cpoint.x, Blocks[i].Dpoint.x, Blocks[j].Cpoint.x, Blocks[j].Dpoint.x, x_common_length);
        float y_common_length = 0.0;
        bool y_common;
        y_common = Find_Common_Area(Blocks[i].Cpoint.y, Blocks[i].Dpoint.y, Blocks[j].Cpoint.y, Blocks[j].Dpoint.y, y_common_length);

        if (x_common and y_common) {
          float overlap = x_common_length * y_common_length / (Blocks[i].Dpoint.x * Blocks[i].Dpoint.y);
          if (overlap > Blocks[i].overlap) {
            Blocks[i].overlap = overlap;
          }
          // Blocks[i].overlap += overlap;
        }
      }
    }
  }

  for (unsigned int i = 0; i < Blocks.size(); ++i) {
    if (max_overlap < Blocks[i].overlap) {
      max_overlap = Blocks[i].overlap;
    }
  }

  std::cout << "Max overlap " << max_overlap << std::endl;

  return max_overlap;
}

void Placement::Extract_Gradient(vector<float> &gradient, bool x_or_y) {
  if (x_or_y) {
    for (unsigned int i = 0; i < Blocks.size(); ++i) {
      gradient.push_back(Blocks[i].Force.x);
    }
  } else {
    for (unsigned int i = 0; i < Blocks.size(); ++i) {
      gradient.push_back(Blocks[i].Force.y);
    }
  }
}

void Placement::UT_Placer() {
  Cal_LSE_Net_Force();
  Cal_sym_Force();
  Cal_LSE_Area_Force();
  Cal_LSE_BND_Force();
  Cal_LSE_OL_Force();
  Cal_UT_Force();
  float step_size = 0.1f;
  float beta_1 = 0.9f;
  float beta_2 = 0.999f;
  // x direction
  vector<float> mlx(Blocks.size(), 0.0f);      // last 1st moment vector
  vector<float> vlx(Blocks.size(), 0.0f);      // last 2nd moment vector
  vector<float> mnx(Blocks.size(), 0.0f);      // next 1st moment vector
  vector<float> vnx(Blocks.size(), 0.0f);      // next 2nd moment vector
  vector<float> mn_hatx(Blocks.size(), 0.0f);  // next 1st moment vector
  vector<float> vn_hatx(Blocks.size(), 0.0f);  // next 2nd moment vector
  // y direction
  vector<float> mly(Blocks.size(), 0.0f);      // last 1st moment vector
  vector<float> vly(Blocks.size(), 0.0f);      // last 2nd moment vector
  vector<float> mny(Blocks.size(), 0.0f);      // next 1st moment vector
  vector<float> vny(Blocks.size(), 0.0f);      // next 2nd moment vector
  vector<float> mn_haty(Blocks.size(), 0.0f);  // next 1st moment vector
  vector<float> vn_haty(Blocks.size(), 0.0f);  // next 2nd moment vector
  // optimization or iteration algorithm
  // stop condition{
  // cal force or gradient
  int step = 0;
  float current_overlap = Cal_Overlap();
  float symmetricMin = 0.3;
  int upper_count_number = 200;
  while ((current_overlap > 0.3 or symCheck(symmetricMin)) and step < upper_count_number) {  // stop condition
    step++;
    current_overlap = Cal_Overlap();
    // cal gradient for the placement
    Cal_LSE_Net_Force();
    Cal_sym_Force();
    Cal_LSE_Area_Force();
    Cal_LSE_BND_Force();
    Cal_LSE_OL_Force();
    Cal_UT_Force();
    // performance ADAM algorithm
    vector<float> gradient_x;
    Extract_Gradient(gradient_x, 1);
    vector<float> gradient_y;
    Extract_Gradient(gradient_y, 1);

    vector<float> position_xl;
    vector<float> position_yl;
    Extract_Placement_Vectors(position_xl, 1);
    Extract_Placement_Vectors(position_yl, 0);
    vector<float> position_xn;
    vector<float> position_yn;
    Extract_Placement_Vectors(position_xn, 1);
    Extract_Placement_Vectors(position_yn, 0);
    Cal_mn(mnx, mlx, beta_1, gradient_x);
    Cal_mn_hat(mn_hatx, mnx, beta_1);
    Cal_vn(vnx, vlx, beta_2, gradient_x);
    Cal_vn_hat(vn_hatx, vnx, beta_2);
    Cal_new_position(mn_hatx, vn_hatx, step_size, position_xl, position_xn);
    Cal_mn(mny, mly, beta_1, gradient_y);
    Cal_mn_hat(mn_haty, mny, beta_1);
    Cal_vn(vny, vly, beta_2, gradient_y);
    Cal_vn_hat(vn_haty, vny, beta_2);
    Cal_new_position(mn_haty, vn_haty, step_size, position_yl, position_yn);
    Feedback_Placement_Vectors(position_xn, 1);
    Feedback_Placement_Vectors(position_yn, 1);
    mlx = mnx;
    vlx = vnx;
  }
}

void Placement::Cal_mn(vector<float> &mn, vector<float> &ml, float beta_1, vector<float> gradient) {
  for (unsigned int i = 0; i < gradient.size(); ++i) {
    mn[i] = beta_1 * ml[i] + (1.0 - beta_1) * gradient[i];
  }
}

void Placement::Cal_mn_hat(vector<float> &mn_hat, vector<float> &mn, float beta_1) {
  for (unsigned int i = 0; i < mn.size(); ++i) {
    mn_hat[i] = mn[i] / (1 - beta_1);
  }
}

void Placement::Cal_vn(vector<float> &vn, vector<float> &vl, float beta_2, vector<float> gradient) {
  for (unsigned int i = 0; i < gradient.size(); ++i) {
    vn[i] = beta_2 * vl[i] + (1.0 - beta_2) * gradient[i] * gradient[i];
  }
}

void Placement::Cal_vn_hat(vector<float> &vn_hat, vector<float> &vn, float beta_2) {
  for (unsigned int i = 0; i < vn.size(); ++i) {
    vn_hat[i] = vn[i] / (1 - beta_2);
  }
}

void Placement::Cal_new_position(vector<float> &mn_hat, vector<float> &vn_hat, float step_size, vector<float> position_old, vector<float> &position_new) {
  for (unsigned int i = 0; i < mn_hat.size(); ++i) {
    position_new[i] = position_old[i] + step_size * mn_hat[i] / (sqrt(vn_hat[i]) + 1e-8);
  }
}

void Placement::Cal_LSE_BND_Force() {
  Chip_BND.x = 1.0f;
  Chip_BND.y = 1.0f;

  for (unsigned int i = 0; i < Blocks.size(); ++i) {
    float first_term = Exp_Function(0.0f - Blocks[i].Cpoint.x + Blocks[i].Dpoint.x / 2, gammar) /
                       (Exp_Function(0.0f - Blocks[i].Cpoint.x + Blocks[i].Dpoint.x / 2, gammar) + 1);
    float second_term = Exp_Function(Blocks[i].Cpoint.x + Blocks[i].Dpoint.x / 2 - Chip_BND.x, gammar) /
                        (Exp_Function(Blocks[i].Cpoint.x + Blocks[i].Dpoint.x / 2 - Chip_BND.x, gammar) + 1);
    Blocks[i].BND_Force.x = first_term + second_term;
  }

  for (unsigned int i = 0; i < Blocks.size(); ++i) {
    float first_term = Exp_Function(0.0f - Blocks[i].Cpoint.y + Blocks[i].Dpoint.y / 2, gammar) /
                       (Exp_Function(0.0f - Blocks[i].Cpoint.y + Blocks[i].Dpoint.y / 2, gammar) + 1);
    float second_term = Exp_Function(Blocks[i].Cpoint.y + Blocks[i].Dpoint.y / 2 - Chip_BND.y, gammar) /
                        (Exp_Function(Blocks[i].Cpoint.y + Blocks[i].Dpoint.y / 2 - Chip_BND.y, gammar) + 1);
    Blocks[i].BND_Force.x = first_term + second_term;
  }
}

float Placement::Cal_OL_MIN_SUM(bool x_or_y, int i, int j) {
  float sum_min = 0.0f;
  if (x_or_y) {
    sum_min += Exp_Function(-(Blocks[i].Cpoint.x + Blocks[i].Dpoint.x / 2 - Blocks[j].Cpoint.x + Blocks[j].Dpoint.x / 2), gammar);
    sum_min += Exp_Function(-(Blocks[j].Cpoint.x + Blocks[j].Dpoint.x / 2 - Blocks[i].Cpoint.x + Blocks[i].Dpoint.x / 2), gammar);
    sum_min += Exp_Function(-Blocks[i].Dpoint.x, gammar);
    sum_min += Exp_Function(-Blocks[j].Dpoint.x, gammar);
  } else {
    sum_min += Exp_Function(-(Blocks[i].Cpoint.y + Blocks[i].Dpoint.y / 2 - Blocks[j].Cpoint.y + Blocks[j].Dpoint.y / 2), gammar);
    sum_min += Exp_Function(-(Blocks[j].Cpoint.y + Blocks[j].Dpoint.y / 2 - Blocks[i].Cpoint.y + Blocks[i].Dpoint.y / 2), gammar);
    sum_min += Exp_Function(-Blocks[i].Dpoint.y, gammar);
    sum_min += Exp_Function(-Blocks[j].Dpoint.y, gammar);
  }
  return sum_min;
}

float Placement::Cal_OL_Term(bool x_or_y, int i, int j) {
  float result = 0.0f;

  if (x_or_y) {
    float sum_min = Cal_OL_MIN_SUM(1, i, j);
    sum_min = -gammar * log(sum_min);
    result = gammar * log(Exp_Function(sum_min, gammar) + 1);
  } else {
    float sum_min = Cal_OL_MIN_SUM(0, i, j);
    sum_min = -gammar * log(sum_min);
    result = gammar * log(Exp_Function(sum_min, gammar) + 1);
  }
  return result;
}

float Placement::Cal_OL_Term_Gradient(bool x_or_y, int i, int j) {
  float result = 0.0f;

  if (x_or_y) {
    float sum_min = Cal_OL_MIN_SUM(1, i, j);
    sum_min = -gammar * log(sum_min);
    result = Exp_Function(sum_min, gammar) / (Exp_Function(sum_min, gammar) + 1);
  } else {
    float sum_min = Cal_OL_MIN_SUM(0, i, j);
    sum_min = -gammar * log(sum_min);
    result = Exp_Function(sum_min, gammar) / (Exp_Function(sum_min, gammar) + 1);
  }
  return result;
}

float Placement::Cal_OL_Gradient(bool x_or_y, int i, int j) {
  float result = 0.0f;

  if (x_or_y) {
    float first_term = Cal_OL_Term_Gradient(1, i, j);
    float sum_min = Cal_OL_MIN_SUM(1, i, j);
    float frac_term = Exp_Function(-(Blocks[i].Cpoint.x + Blocks[i].Dpoint.x / 2 - Blocks[j].Cpoint.x + Blocks[j].Dpoint.x / 2), gammar) -
                      Exp_Function(-(Blocks[j].Cpoint.x + Blocks[j].Dpoint.x / 2 - Blocks[i].Cpoint.x + Blocks[i].Dpoint.x / 2), gammar);
    result = first_term * (frac_term / sum_min);
  } else {
    float first_term = Cal_OL_Term_Gradient(0, i, j);
    float sum_min = Cal_OL_MIN_SUM(0, i, j);
    float frac_term = Exp_Function(-(Blocks[i].Cpoint.y + Blocks[i].Dpoint.y / 2 - Blocks[j].Cpoint.y + Blocks[j].Dpoint.y / 2), gammar) -
                      Exp_Function(-(Blocks[j].Cpoint.y + Blocks[j].Dpoint.y / 2 - Blocks[i].Cpoint.y + Blocks[i].Dpoint.y / 2), gammar);
    result = first_term * (frac_term / sum_min);
  }
  return result;
}

void Placement::Cal_LSE_OL_Force() {
  for (unsigned int i = 0; i < Blocks.size(); ++i) {
    float gradient = 0.0f;
    for (unsigned int j = 0; j < Blocks.size(); ++j) {
      if (i != j) gradient += Cal_OL_Gradient(1, i, j) * Cal_OL_Term(0, i, j);
    }
    Blocks[i].OL_Force.x = gradient;
  }

  for (unsigned int i = 0; i < Blocks.size(); ++i) {
    float gradient = 0.0f;
    for (unsigned int j = 0; j < Blocks.size(); ++j) {
      if (i != j) gradient += Cal_OL_Gradient(0, i, j) * Cal_OL_Term(1, i, j);
    }
    Blocks[i].OL_Force.y = gradient;
  }
}

void Placement::Cal_UT_Force() {
  for (unsigned int i = 0; i < Blocks.size(); ++i) {
    Blocks[i].Force.x = -beta * Blocks[i].Netforce.x - sym_beta * Blocks[i].Symmetricforce.x - area_beta * Blocks[i].Areaforce.x -
                        BND_beta * Blocks[i].BND_Force.x - OL_beta * Blocks[i].OL_Force.x;
    Blocks[i].Force.y = -beta * Blocks[i].Netforce.y - sym_beta * Blocks[i].Symmetricforce.y - area_beta * Blocks[i].Areaforce.y -
                        BND_beta * Blocks[i].BND_Force.y - OL_beta * Blocks[i].OL_Force.y;
  }
}

void Placement::E_Placer(PnRDB::hierNode &current_node) {
  int i = 0;
#ifdef DEBUG
  std::cout << "E_placer debug flage: 0" << std::endl;
#endif
  // force to align and order
  // force_alignment();
  vector<float> uc_y, vc_y, vl_y;
  vector<float> uc_x, vc_x, vl_x;
  force_order(vc_x, vl_x, vc_y, vl_y);
  force_alignment(vc_x, vl_x, vc_y, vl_y);

  Update_Bin_Density();
#ifdef DEBUG
  std::cout << "E_placer debug flage: 1" << std::endl;
#endif
  // gradient cal
  Cal_WA_Net_Force();
// Cal_LSE_Net_Force();
#ifdef DEBUG
  std::cout << "E_placer debug flage: 2" << std::endl;
#endif
  Cal_Density_Eforce();
#ifdef DEBUG
  std::cout << "E_placer debug flage: 3" << std::endl;
#endif

  Cal_sym_Force();
#ifdef DEBUG
  std::cout << "E_placer debug flage: 3.5" << std::endl;
#endif
  Cal_LSE_Area_Force();
  Cal_force();
#ifdef DEBUG
  std::cout << "E_placer debug flage: 4" << std::endl;
#endif

  float ac_x = 1.0f;
  vector<float> pre_vc_x, pre_vl_x;
  pre_conditioner(pre_vl_x, 1);  // 1 x direction
#ifdef DEBUG
  std::cout << "E_placer debug flage: 5" << std::endl;
#endif
  // vector<float> uc_x,vc_x,vl_x;
  Extract_Placement_Vectors(uc_x, 1);
#ifdef DEBUG
  std::cout << "E_placer debug flage: 6" << std::endl;
#endif
  Extract_Placement_Vectors(vc_x, 1);
#ifdef DEBUG
  std::cout << "E_placer debug flage: 7" << std::endl;
#endif
  Extract_Placement_Vectors(vl_x, 1);
#ifdef DEBUG
  std::cout << "E_placer debug flage: 8" << std::endl;
#endif

  float ac_y = 1.0f;
  vector<float> pre_vc_y, pre_vl_y;
  pre_conditioner(pre_vl_y, 0);  // 1 x direction
#ifdef DEBUG
  std::cout << "E_placer debug flage: 9" << std::endl;
#endif
  // vector<float> uc_y,vc_y,vl_y;
  Extract_Placement_Vectors(uc_y, 0);
#ifdef DEBUG
  std::cout << "E_placer debug flage: 10" << std::endl;
#endif
  Extract_Placement_Vectors(vc_y, 0);
#ifdef DEBUG
  std::cout << "E_placer debug flage: 11" << std::endl;
#endif
  Extract_Placement_Vectors(vl_y, 0);
#ifdef DEBUG
  std::cout << "E_placer debug flage: 12" << std::endl;
#endif
  bool start_flag = 1;
  Update_Bin_Density();
#ifdef DEBUG
  std::cout << "E_placer debug flage: 13" << std::endl;
#endif

  float stop_density = 0.01;
  float max_density = 1.0;
  float current_max_density = 10.0;
  int count_number = 0;
  int upper_count_number = 2000;
  float current_overlap = 1.0;
  float symmetricMin = 0.3;  // need to tune
  // initialize dummy net weight
  // dummy_net_weight = 0.001;
  // float dummy_net_weight_rate = dummy_net_weight_rate;
  // float dummy_net_target = 3.0;
  float dummy_net_weight_increase = cal_weight_init_increase(dummy_net_rate, dummy_net_weight, dummy_net_target, 100);
  vector<float> Density;
  vector<float> Overlap;
#ifdef DEBUG
  std::cout << "E_placer debug flage: 14" << std::endl;
#endif
  PlotPlacement(0);
  current_overlap = Cal_Overlap();
  // while((Stop_Condition(stop_density,current_max_density) or symCheck(symmetricMin)) and count_number<upper_count_number ){//Q: stop condition

#ifdef PERFORMANCE_DRIVEN
  Py_Initialize();
  if (!Py_IsInitialized()) std::cout << "Py_Initialize fails" << std::endl;
  PyRun_SimpleString("import sys");
  PyRun_SimpleString("sys.path.append('./')");
  PyObject *pModule = PyImport_ImportModule("calgrad");
  PyObject *pFun_initialization = PyObject_GetAttrString(pModule, "initialization");
  PyObject *pFun_cal_grad = PyObject_GetAttrString(pModule, "cal_grad");
  PyObject *pArgs_initialization = PyTuple_New(1);
  PyTuple_SetItem(pArgs_initialization, 0, PyUnicode_FromString(circuit_name.c_str()));
  PyObject *pyValue_initialization = PyEval_CallObject(pFun_initialization, pArgs_initialization);
  PyObject *sess = NULL, *X = NULL, *grads = NULL;
  PyArg_ParseTuple(pyValue_initialization, "O|O|O", &sess, &X, &grads);
  if (!sess) std::cout << "empty sess" << std::endl;
  if (!X) std::cout << "empty X" << std::endl;
  if (!grads) std::cout << "empty grads" << std::endl;
#endif

  while ((current_overlap > 0.3 or symCheck(symmetricMin)) and count_number < upper_count_number) {  // Q: stop condition
    // Initilize_lambda();
    // Initilize_sym_beta();
    // while(i<20){//Q: stop condition
    Density.push_back(current_max_density);
    current_overlap = Cal_Overlap();
    Overlap.push_back(current_overlap);
    if (current_max_density < max_density) {
      max_density = current_max_density;
#ifdef DEBUG
      std::cout << "E_placer debug flage: 16" << std::endl;
#endif
    } else if (current_max_density == Density.back()) {
#ifdef DEBUG
      std::cout << "E_placer debug flage: 17" << std::endl;
#endif
      count_number++;
    }
#ifdef DEBUG
    std::cout << "E_placer debug flage: 15" << std::endl;
#endif
    //  Density.push_back(current_max_density);
#ifdef DEBUG
    std::cout << "Iteration " << i << std::endl;
#endif
    // if(lambda<100)
    lambda = lambda * 1.1;
    beta = beta * 0.95;
    if (sym_beta < 0.1) {
      sym_beta = sym_beta * 1.05;
    }

    std::cout << "sym_beta:= " << sym_beta << std::endl;
    // force to align
    if (i % 10 == 0) {
      force_order(vc_x, vl_x, vc_y, vl_y);
      force_alignment(vc_x, vl_x, vc_y, vl_y);

      //  force_alignment();
    }

    // PlotPlacement(i);

    Update_Bin_Density();
    // gradient cal
    Cal_WA_Net_Force();
    cal_dummy_net_weight(dummy_net_weight, dummy_net_rate, dummy_net_weight_increase);
    // Cal_LSE_Net_Force();
    Cal_Density_Eforce();
#ifdef DEBUG
    std::cout << "E_placer debug flag: 18" << std::endl;
#endif
    Cal_sym_Force();
    Cal_LSE_Area_Force();
    Cal_force();

// WriteOut_Blocks(i);
// WriteOut_Bins(i);
// step size
// two direction x
#ifdef DEBUG
    std::cout << "test 1" << std::endl;
#endif
    pre_conditioner(pre_vc_x, 1);  // 1 x direction
#ifdef DEBUG
    std::cout << "test 1.1" << std::endl;
#endif
// pre_conditioner(pre_vl_x,1); //1 x direction
#ifdef DEBUG
    std::cout << "test 1.2" << std::endl;
#endif
    Nesterov_based_iteration(ac_x, uc_x, vc_x, vl_x, pre_vc_x, pre_vl_x, start_flag);
// two direction y
#ifdef DEBUG
    std::cout << "test 2" << std::endl;
#endif
    pre_conditioner(pre_vc_y, 0);  // 0 y direction
#ifdef DEBUG
    std::cout << "test 2.1" << std::endl;
#endif
// pre_conditioner(pre_vl_y,0); //0 y direction
#ifdef DEBUG
    std::cout << "test 2.1" << std::endl;
#endif
    Nesterov_based_iteration(ac_y, uc_y, vc_y, vl_y, pre_vc_y, pre_vl_y, start_flag);
    std::cout << "iteration " << i << "step size " << ac_x << " " << ac_y << std::endl;
#ifdef DEBUG
    std::cout << "test 3" << std::endl;
#endif
#ifdef PERFORMANCE_DRIVEN
    performance_gradient(uc_x, uc_y, pFun_cal_grad, sess, X, grads, 1 - current_overlap);
#endif
    Pull_back_vector(uc_x, 1);
    Pull_back_vector(uc_y, 0);
    Feedback_Placement_Vectors(uc_x, 1);
    Feedback_Placement_Vectors(uc_y, 0);
#ifdef hard_symmetry
    force_symmetry(current_node);
#endif
    Pull_back_vector(vc_x, 1);
    Pull_back_vector(vl_x, 1);
    Pull_back_vector(vc_y, 0);
    Pull_back_vector(vl_y, 0);
    // PlotPlacement(i);
// Pull_back();
#ifdef DEBUG
    std::cout << "test 4" << std::endl;
#endif
    start_flag = 0;
    i++;
  }
#ifdef PERFORMANCE_DRIVEN
  Py_Finalize();
#endif
  // exit(0);
  force_order(vc_x, vl_x, vc_y, vl_y);
  force_alignment(vc_x, vl_x, vc_y, vl_y);
  // restore_MS();
  // refine_CC();
  PlotPlacement(count_number);
  std::cout << "iter num when stop:=" << count_number << std::endl;
}

#ifdef PERFORMANCE_DRIVEN
void Placement::performance_gradient(vector<float> &uc_x, vector<float> &uc_y, PyObject *pFun_cal_grad, PyObject *sess, PyObject *X, PyObject *grads,
                                     float weight) {
  vector<float> uc_x_ori(originalBlockCNT, 0), uc_y_ori(originalBlockCNT, 0);
  // vector<float> uc_x_ori_move(originalBlockCNT,0), uc_y_ori_move(originalBlockCNT,0);
  for (int i = 0; i < originalBlockCNT; i++) {
    uc_x_ori[i] += uc_x[i];
    uc_y_ori[i] += uc_y[i];
    for (auto s : Blocks[i].spiltBlock) {
      uc_x_ori[i] += uc_x[s];
      uc_y_ori[i] += uc_y[s];
    }
    uc_x_ori[i] /= (Blocks[i].spiltBlock.size() + 1);
    uc_y_ori[i] /= (Blocks[i].spiltBlock.size() + 1);
  }
  PyObject *pArgs_cal_grad = PyTuple_New(6);
  PyObject *pyParams_x = PyList_New(0), *pyParams_y = PyList_New(0);
  for (int i = 0; i < originalBlockCNT; ++i) {
    PyList_Append(pyParams_x, Py_BuildValue("f", uc_x_ori[i]));
    PyList_Append(pyParams_y, Py_BuildValue("f", uc_y_ori[i]));
  }
  PyTuple_SetItem(pArgs_cal_grad, 0, sess);
  PyTuple_SetItem(pArgs_cal_grad, 1, PyUnicode_FromString(circuit_name.c_str()));
  PyTuple_SetItem(pArgs_cal_grad, 2, X);
  PyTuple_SetItem(pArgs_cal_grad, 3, grads);
  PyTuple_SetItem(pArgs_cal_grad, 4, pyParams_x);
  PyTuple_SetItem(pArgs_cal_grad, 5, pyParams_y);
  PyObject *pyValue_cal_grad = PyEval_CallObject(pFun_cal_grad, pArgs_cal_grad);
  vector<float> grad_x(originalBlockCNT, 0), grad_y(originalBlockCNT, 0);
  // int size = PyList_Size(pyValue_cal_grad);
  // std::cout<<"List size: "<<size<<std::endl;
  for (int i = 0; i < originalBlockCNT; ++i) {
    // PyObject *pRet = PyList_GetItem(pyValue_cal_grad, i);
    PyArg_Parse(PyList_GetItem(pyValue_cal_grad, i), "f", &grad_x[i]);
    // std::cout<<grad_x[i]<<std::endl;
  }
  for (int i = 0; i < originalBlockCNT; ++i) {
    // PyObject *pRet = PyList_GetItem(pyValue_cal_grad, i+originalBlockCNT);
    PyArg_Parse(PyList_GetItem(pyValue_cal_grad, i + originalBlockCNT), "f", &grad_y[i]);
    // std::cout<<grad_y[i]<<std::endl;
  }
  for (int i = 0; i < originalBlockCNT; i++) {
    uc_x[i] += weight * grad_x[i];
    uc_y[i] += weight * grad_y[i];
    for (auto s : Blocks[i].spiltBlock) {
      uc_x[s] += weight * grad_x[i];
      uc_y[s] += weight * grad_y[i];
    }
  }
}
#endif

void Placement::Extract_Placement_Vectors(vector<float> &temp_vector, bool x_or_y) {  // 1 is x, 0 is y

  temp_vector.clear();
  for (unsigned int i = 0; i < Blocks.size(); ++i) {
    if (x_or_y) {
      temp_vector.push_back(Blocks[i].Cpoint.x);
    } else {
      temp_vector.push_back(Blocks[i].Cpoint.y);
    }
  }
}

void Placement::Feedback_Placement_Vectors(vector<float> &temp_vector, bool x_or_y) {  // 1 is x, 0 is y

  for (unsigned int i = 0; i < Blocks.size(); ++i) {
    if (x_or_y) {
      Blocks[i].Cpoint.x = temp_vector[i];
    } else {
      Blocks[i].Cpoint.y = temp_vector[i];
    }
  }
}

void Placement::Nesterov_based_iteration(float &ac, vector<float> &uc, vector<float> &vc, vector<float> &vl, vector<float> &pre_vc, vector<float> &pre_vl,
                                         bool start_flag) {
  // Q:
  // Cal_WA_Net_Force();
  // Cal_LSE_Net_Force();
  // Cal_bin_force(); to be implemented
  // this function works for one direction, need to call twice x/y
  // Q:

  // start nesterov method
  // algorithm 1 of ePlace-MS: Electrostatics-Based Placement forMixed-Size Circuits
  float an;          // current/last step size
  vector<float> un;  // next/current/last vector u
  vector<float> vn;  // next/current/last vector u
// float ak = BkTrk(vc,vl,pre_vc,pre_vl); //Q: the port defination of BkTrk is not correct
#ifdef DEBUG
  std::cout << "Nesterov_based_iteration test 1" << std::endl;
#endif
  if (!start_flag) {
    // if(0){
    BkTrk(ac, an, uc, vc, vl, pre_vc, pre_vl);  // Q: the port defination of BkTrk is not correct
  }
// Q: BkTrk need to be revised since back tracing is not considered
#ifdef DEBUG
  std::cout << "Nesterov_based_iteration test 2" << std::endl;
  std::cout << "un size " << un.size() << " uc size " << uc.size() << " pre_vc size " << pre_vc.size() << std::endl;
#endif
  for (unsigned int i = 0; i < uc.size(); ++i) {
    // un.push_back(vc[i]-ac*pre_vc[i]); //QQQ:LIYG Huge change
    un.push_back(vc[i] + ac * pre_vc[i]);
  }
#ifdef DEBUG
  std::cout << "Nesterov_based_iteration test 3" << std::endl;
#endif
  an = (1 + sqrt(1 + 4 * ac * ac)) / 2;

  for (unsigned int i = 0; i < uc.size(); ++i) {
    vn.push_back(un[i] + (ac - 1) * (un[i] - uc[i]) / an);
  }
#ifdef DEBUG
  std::cout << "Nesterov_based_iteration test 4" << std::endl;
#endif
  // ac = an;
  uc = un;
  vl = vc;
  vc = vn;
  pre_vl = pre_vc;
}

void Placement::BkTrk(float &ac, float &an, vector<float> &uc, vector<float> &vc, vector<float> &vl, vector<float> &pre_vc, vector<float> &pre_vl) {
// algorithm 2 of ePlace-MS: Electrostatics-Based Placement forMixed-Size Circuits
#ifdef DEBUG
  std::cout << "BkTrk 1" << std::endl;
#endif
  float hat_ac = vector_fraction(vc, vl, pre_vc, pre_vl);
#ifdef DEBUG
  std::cout << "BkTrk 2" << std::endl;
#endif
  vector<float> hat_un;
  cal_hat_un(hat_ac, hat_un, vc, pre_vc);
#ifdef DEBUG
  std::cout << "BkTrk 3" << std::endl;
#endif
  vector<float> hat_vn;
  cal_hat_vn(ac, an, hat_vn, hat_un, uc);
#ifdef DEBUG
  std::cout << "BkTrk 4" << std::endl;
#endif
/*
   vector<float> pre_hat_vn; //Q this is not correct
   //this part could actually be ignored
   while(hat_ac>0.01*vector_fraction(hat_vn, vc, pre_hat_vn, pre_vc)){ //Q: what is stop condition Q://where is pre_hat_vn

     hat_ac = vector_fraction(hat_vn, vc, pre_hat_vn, pre_vc);
     cal_hat_un(hat_ac, hat_un, vc, pre_vc);
     cal_hat_vn(ac, an, hat_vn, hat_un, uc);
   }
   */

// this part could actually be ignored
#ifdef DEBUG
  std::cout << "BkTrk 5" << std::endl;
#endif
  ac = hat_ac;
  //  if(isnan(ac))
  //  {
  //    ac = 1;
  //  }
  //  else if(isnan(-ac))
  //  {
  //    ac = -1;
  //  }
}

float Placement::vector_fraction(vector<float> &vc, vector<float> &vl, vector<float> &pre_vc, vector<float> &pre_vl) {
  float sum_upper = 0.0;
  for (unsigned int i = 0; i < vc.size(); ++i) {
    sum_upper += (vc[i] - vl[i]) * (vc[i] - vl[i]);
  }
  sum_upper = sqrt(sum_upper);
  float sum_lower = 0.0;
  for (unsigned int i = 0; i < vc.size(); ++i) {
    sum_lower += (pre_vc[i] - pre_vl[i]) * (pre_vc[i] - pre_vl[i]);
  }
  sum_lower = sqrt(sum_lower);

  float hat_ac = sum_upper / sum_lower;
  return hat_ac;
}

void Placement::cal_hat_un(float &hat_ac, vector<float> &hat_un, vector<float> &vc, vector<float> &pre_vc) {
  for (unsigned int i = 0; i < vc.size(); ++i) {
    hat_un.push_back(vc[i] - hat_ac * pre_vc[i]);
  }
}

void Placement::cal_hat_vn(float &ac, float &an, vector<float> &hat_vn, vector<float> &hat_un, vector<float> &uc) {
  for (unsigned int i = 0; i < hat_un.size(); ++i) {
    hat_vn.push_back(hat_un[i] + (ac - 1) * (hat_un[i] - uc[i]) / an);
  }
}

// Calculating pre(vk) Q: also two directions
void Placement::pre_conditioner(vector<float> &Pre_Conditioner, bool x_or_y) {  // 1 is for x, 0 is for y

  vector<float> HPWL_Pre_Conditioner;
  // LSE_pre_conditioner(HPWL_Pre_Conditioner, x_or_y);
  WA_pre_conditioner(HPWL_Pre_Conditioner, x_or_y);
  // LSE_Pre_Conditioner
  // LSE_Pre_Conditioner
  // Yaguang (07/04/2021): found a bug - symmetry precondition pre_conditioner is missing actually. It can be calculated acctually

  vector<float> Desity_Pre_Conditioner;
  for (unsigned int i = 0; i < Blocks.size(); ++i) {
    if (x_or_y) {
      Desity_Pre_Conditioner.push_back(Blocks[i].Dpoint.x);
    } else {
      Desity_Pre_Conditioner.push_back(Blocks[i].Dpoint.y);
    }
  }

  Pre_Conditioner.clear();
  for (unsigned int i = 0; i < Blocks.size(); ++i) {
    if (x_or_y) {
      Pre_Conditioner.push_back(1 / (HPWL_Pre_Conditioner[i] + lambda * Desity_Pre_Conditioner[i]) * (Blocks[i].Force.x));
    } else {
      Pre_Conditioner.push_back(1 / (HPWL_Pre_Conditioner[i] + lambda * Desity_Pre_Conditioner[i]) * (Blocks[i].Force.y));
    }
  }
}

void Placement::LSE_pre_conditioner(vector<float> &HPWL_Pre_Conditioner, bool x_or_y) {
  HPWL_Pre_Conditioner.clear();

  for (unsigned int i = 0; i < Blocks.size(); ++i) {
    float sum = 0.0;
    for (unsigned int j = 0; j < Blocks[i].connected_net.size(); ++j) {
      int net_index = Blocks[i].connected_net[j];
      if (x_or_y) {  // 1 x, 0 y
        float term1 = Blocks[i].Net_block_force_P.x;
        float term2 = Nets[net_index].PSumNetforce.x;
        float term3 = Blocks[i].Net_block_force_N.x;
        float term4 = Nets[net_index].NSumNetforce.x;
        sum += term1 * (term2 - term1) / (gammar * term2 * term2) + term3 * (term4 - term3) / (gammar * term4 * term4);
      } else {
        float term1 = Blocks[i].Net_block_force_P.y;
        float term2 = Nets[net_index].PSumNetforce.y;
        float term3 = Blocks[i].Net_block_force_N.y;
        float term4 = Nets[net_index].NSumNetforce.y;
        sum += term1 * (term2 - term1) / (gammar * term2 * term2) + term3 * (term4 - term3) / (gammar * term4 * term4);
      }
    }
    HPWL_Pre_Conditioner.push_back(sum);
  }
}

void Placement::WA_pre_conditioner(vector<float> &HPWL_Pre_Conditioner, bool x_or_y) {
  HPWL_Pre_Conditioner.clear();

  for (unsigned int i = 0; i < Blocks.size(); ++i) {
    float sum = 0.0;
    sum = Blocks[i].connected_net.size();
    HPWL_Pre_Conditioner.push_back(sum);
  }
}

float Placement::Fast_Exp(float a) {
  a = 1.0f + a / 1024.0f;
  a *= a;
  a *= a;
  a *= a;
  a *= a;
  a *= a;
  a *= a;
  a *= a;
  a *= a;
  a *= a;
  a *= a;
  return a;
}

void Placement::WriteOut_Blocks(int iteration) {
  std::ofstream writoutfile;

  std::string file_name = to_string(iteration) + "_Iter_Blocks.txt";

  writoutfile.open(file_name);

  for (unsigned int i = 0; i < Blocks.size(); ++i) {
    writoutfile << Blocks[i].Cpoint.x << " " << Blocks[i].Cpoint.y << " " << Blocks[i].Dpoint.x << " " << Blocks[i].Dpoint.y << " " << Blocks[i].Eforce.x << " "
                << Blocks[i].Eforce.y << " " << Blocks[i].Force.x << " " << Blocks[i].Force.y << std::endl;
  }

  writoutfile.close();
}

void Placement::WriteOut_Bins(int iteration) {
  std::ofstream writoutfile;

  std::string file_name = to_string(iteration) + "_Iter_Bins.txt";

  writoutfile.open(file_name);

  for (unsigned int i = 0; i < Bins.size(); ++i) {
    for (unsigned int j = 0; j < Bins[i].size(); j++) {
      writoutfile << Bins[i][j].Cpoint.x << " " << Bins[i][j].Cpoint.y << " " << Bins[i][j].Dpoint.x << " " << Bins[i][j].Dpoint.y << " " << Bins[i][j].density
                  << " " << Bins[i][j].Ephi << " " << Bins[i][j].Eforce.x << " " << Bins[i][j].Eforce.y << std::endl;
    }
  }

  writoutfile.close();
}

// donghao start
// return the total area of all blocks
float Placement::readInputNode(PnRDB::hierNode &current_node) {
  int blockIndex = 0;
  float totalArea = 0;
  Blocks.clear();
  Nets.clear();
  std::cout << "start reading blocks file" << std::endl;
  int blockCNT = current_node.Blocks.size();
  // initialize sysmmtric matrix
  symmetric_force_matrix = vector<vector<Ppoint_F>>(blockCNT);
  for (int i = 0; i < blockCNT; ++i) {
    symmetric_force_matrix[i] = vector<Ppoint_F>(blockCNT);
    for (int j = 0; j < blockCNT; ++j) {
      symmetric_force_matrix[i][j].x = 0;
      symmetric_force_matrix[i][j].y = 0;
    }
  }

  for (vector<PnRDB::blockComplex>::iterator it = current_node.Blocks.begin(); it != current_node.Blocks.end(); ++it) {
    for (int i = 0; i < 1; ++i) {
      block tempblock;
      // update block name
      tempblock.blockname = it->instance[i].name;
      Ppoint_F tempPoint1, tempPoint2;
      // update center point
      tempPoint1.x = (float)it->instance[i].originCenter.x;
      tempPoint1.y = (float)it->instance[i].originCenter.y;

      // tempPoint1.x = 0.0;
      // tempPoint1.y = 0.0;
      tempblock.Cpoint = tempPoint1;

      // update height and width
      tempPoint2.x = (float)it->instance[i].width;
      tempPoint2.y = (float)it->instance[i].height;
      totalArea += tempPoint2.x * tempPoint2.y;
      tempblock.Dpoint = tempPoint2;

      // set the init force as zero
      tempblock.Force.x = 0;
      tempblock.Force.y = 0;
      tempblock.Netforce.x = 0;
      tempblock.Netforce.y = 0;
      tempblock.Eforce.x = 0;
      tempblock.Eforce.y = 0;
      // set the init NET_BLOCK_FORCE_P/N = 1
      tempblock.Net_block_force_N.x = 1;
      tempblock.Net_block_force_N.y = 1;
      tempblock.Net_block_force_P.x = 1;
      tempblock.Net_block_force_P.y = 1;
      tempblock.index = blockIndex;
      ++blockIndex;
      // connected net will be update later
      Blocks.push_back(tempblock);
    }
  }

  // update net information
  int netIndex = 0;
  std::cout << "total block number: " << blockIndex << std::endl;
  std::cout << "start reading net file" << std::endl;
  for (vector<PnRDB::net>::iterator it = current_node.Nets.begin(); it != current_node.Nets.end(); ++it) {
    net tempNet;
    std::cout << "current net id: " << netIndex << std::endl;
    // update name of net
    tempNet.netname = it->name;
    // based on my understanding, iter2 is the block id
    // I do not care about iter, which means block pin/terminal
    tempNet.connected_block.clear();
    for (int i = 0; i != it->connected.size(); ++i) {
      int iter2 = it->connected[i].iter2;
      std::cout << "connected block id: " << iter2 << std::endl;
      if (iter2 >= 0) {
        tempNet.connected_block.push_back(iter2);
        Blocks[iter2].connected_net.push_back(netIndex);
      }
    }
    // update net index
    tempNet.index = netIndex;
    ++netIndex;

    tempNet.NSumNetforce.x = 0;
    tempNet.NSumNetforce.y = 0;
    tempNet.NSumNetforce_WA.x = 0;
    tempNet.NSumNetforce_WA.y = 0;

    tempNet.PSumNetforce.x = 0;
    tempNet.PSumNetforce.y = 0;
    tempNet.PSumNetforce_WA.x = 0;
    tempNet.PSumNetforce_WA.y = 0;
    Nets.push_back(tempNet);
  }

  // read the symmtirc
  // #ifdef DEBUG
  std::cout << "number of sym constrain = " << current_node.SPBlocks.size() << endl;
  // #endif;
  for (vector<PnRDB::SymmPairBlock>::iterator it = current_node.SPBlocks.begin(); it != current_node.SPBlocks.end(); ++it) {
    // #ifdef DEBUG
    std::cout << "sym group start" << endl;
    std::cout << "self size = " << it->selfsym.size() << ", pair size = " << it->sympair.size() << endl;
    // #endif;

    SymmPairBlock tempSPB;

    tempSPB.selfsym.clear();
    tempSPB.sympair.clear();
    // tempSPB.selfsym = it->selfsym;
    // tempSPB.sympair = it->sympair;
    for (int i = 0; i < it->selfsym.size(); ++i) {
      tempSPB.selfsym.push_back(it->selfsym[i].first);
    }
    for (int i = 0; i < it->sympair.size(); ++i) {
      tempSPB.sympair.push_back(it->sympair[i]);
    }

    if (it->axis_dir == PnRDB::V) {
      // cond 1: only one sym pair
      tempSPB.horizon = 0;
      if (it->sympair.size() == 1 && it->selfsym.size() == 0) {
        int id0 = it->sympair[0].first;
        int id1 = it->sympair[0].second;
        // #ifdef DEBUG
        std::cout << "V: cond1, id0 = " << id0 << ", id1 = " << id1 << endl;
        // #endif;
        symmetric_force_matrix[id0][id0].y += 2;
        symmetric_force_matrix[id0][id1].y -= 2;
        symmetric_force_matrix[id1][id0].y -= 2;
        symmetric_force_matrix[id1][id1].y += 2;

        tempSPB.axis.first = id0;
        tempSPB.axis.second = id1;
      } else if (it->selfsym.size() > 0)  // exist self sym, consider the first self sym block center as axis = x0
      {
        int base = it->selfsym[0].first;
        tempSPB.axis.first = base;
        tempSPB.axis.second = base;
        // #ifdef DEBUG
        std::cout << "V: cond2, base = " << base << endl;
        // #endif;
        // for self sym (xi - x0)^2
        for (int i = 1; i < it->selfsym.size(); ++i) {
          int id = it->selfsym[i].first;
          std::cout << "V: cond2, id = " << id << endl;
          symmetric_force_matrix[id][id].x += 8;
          symmetric_force_matrix[id][base].x -= 8;
          symmetric_force_matrix[base][id].x -= 8;
          symmetric_force_matrix[base][base].x += 8;
        }
        // for pair sym (xi + xj - 2*x0)^2
        for (int i = 0; i < it->sympair.size(); ++i) {
          int id0 = it->sympair[i].first;
          int id1 = it->sympair[i].second;
          std::cout << "V: cond2, id0 = " << id0 << ", id1" << id1 << endl;
          //(yi - yj)^2
          symmetric_force_matrix[id0][id0].y += 2;
          symmetric_force_matrix[id0][id1].y -= 2;
          symmetric_force_matrix[id1][id0].y -= 2;
          symmetric_force_matrix[id1][id1].y += 2;

          //(xi + xj - 2*x0)^2
          symmetric_force_matrix[id0][id0].x += 2;
          symmetric_force_matrix[id0][id1].x += 2;
          symmetric_force_matrix[id0][base].x -= 4;

          symmetric_force_matrix[id1][id0].x += 2;
          symmetric_force_matrix[id1][id1].x += 2;
          symmetric_force_matrix[id1][base].x -= 4;

          symmetric_force_matrix[base][id0].x -= 2;
          symmetric_force_matrix[base][id1].x -= 2;
          symmetric_force_matrix[base][base].x += 4;
        }
      } else if (it->sympair.size() > 1)  // no self sym, consider the center of first sym pair of blocks as axis = 1/2*(x0.first + x0.second)
      {
        int idbase0 = it->sympair[0].first;
        int idbase1 = it->sympair[0].second;
        tempSPB.axis.first = idbase0;
        tempSPB.axis.second = idbase1;
        // #ifdef DEBUG
        std::cout << "V: cond3, idbase0 = " << idbase0 << ", idbase1 = " << idbase1 << endl;
        // #endif;
        symmetric_force_matrix[idbase0][idbase0].y += 2;
        symmetric_force_matrix[idbase0][idbase1].y -= 2;
        symmetric_force_matrix[idbase1][idbase0].y -= 2;
        symmetric_force_matrix[idbase1][idbase1].y += 2;
        for (int i = 1; i < it->sympair.size(); ++i) {
          int id0 = it->sympair[i].first;
          int id1 = it->sympair[i].second;
          // #ifdef DEBUG
          std::cout << "V: cond3, id0 = " << id0 << ", id1 = " << id1 << endl;
          // #endif;
          //(yi - yj)^2
          symmetric_force_matrix[id0][id0].y += 2;
          symmetric_force_matrix[id0][id1].y -= 2;
          symmetric_force_matrix[id1][id0].y -= 2;
          symmetric_force_matrix[id1][id1].y += 2;
          //(xi + xj - x0 - x1)^2
          symmetric_force_matrix[id0][id0].x += 2;
          symmetric_force_matrix[id0][id1].x += 2;
          symmetric_force_matrix[id0][idbase0].x -= 2;
          symmetric_force_matrix[id0][idbase1].x -= 2;

          symmetric_force_matrix[id1][id0].x += 2;
          symmetric_force_matrix[id1][id1].x += 2;
          symmetric_force_matrix[id1][idbase0].x -= 2;
          symmetric_force_matrix[id1][idbase1].x -= 2;

          symmetric_force_matrix[idbase0][id0].x -= 2;
          symmetric_force_matrix[idbase0][id1].x -= 2;
          symmetric_force_matrix[idbase0][idbase0].x += 2;
          symmetric_force_matrix[idbase0][idbase1].x += 2;

          symmetric_force_matrix[idbase1][id0].x -= 2;
          symmetric_force_matrix[idbase1][id1].x -= 2;
          symmetric_force_matrix[idbase1][idbase0].x += 2;
          symmetric_force_matrix[idbase1][idbase1].x += 2;
        }
      } else {
        continue;
      }
    } else  // axis : H
    {
      tempSPB.horizon = 1;
      // cond 1: only one sym pair
      if (it->sympair.size() == 1 && it->selfsym.size() == 0) {
        int id0 = it->sympair[0].first;
        int id1 = it->sympair[1].second;
        tempSPB.axis.first = id0;
        tempSPB.axis.second = id1;
        // #ifdef DEBUG
        std::cout << "H: cond1, id0 = " << id0 << ", idb1 = " << id1 << endl;
        // #endif;
        symmetric_force_matrix[id0][id0].x += 2;
        symmetric_force_matrix[id0][id1].x -= 2;
        symmetric_force_matrix[id1][id0].x -= 2;
        symmetric_force_matrix[id1][id1].x += 2;
      } else if (it->selfsym.size() > 0)  // exist self sym, consider the first self sym block center as axis = x0
      {
        int base = it->selfsym[0].first;
        tempSPB.axis.first = base;
        tempSPB.axis.second = base;
        // for self sym (yi - y0)^2
        // #ifdef DEBUG
        std::cout << "H: cond2, base = " << base << endl;
        // #endif;
        for (int i = 1; i < it->selfsym.size(); ++i) {
          int id = it->selfsym[i].first;
          // #ifdef DEBUG
          std::cout << "H: cond2, id = " << id << endl;
          // std::cout<<"matrix size:"<<symmetric_force_matrix.size()<<", "<<symmetric_force_matrix[0].size()<<endl;
          // #endif;
          symmetric_force_matrix[id][id].y += 8;
          symmetric_force_matrix[id][base].y -= 8;
          symmetric_force_matrix[base][id].y -= 8;
          symmetric_force_matrix[base][base].y += 8;
        }
        // for pair sym (xi + xj - 2*x0)^2
        for (int i = 0; i < it->sympair.size(); ++i) {
          int id0 = it->sympair[i].first;
          int id1 = it->sympair[i].second;
          // #ifdef DEBUG
          std::cout << "V: cond2, id0 = " << id0 << ", id1 = " << id1 << endl;
          // #endif;
          //(xi - xj)^2
          symmetric_force_matrix[id0][id0].x += 2;
          symmetric_force_matrix[id0][id1].x -= 2;
          symmetric_force_matrix[id1][id0].x -= 2;
          symmetric_force_matrix[id1][id1].x += 2;

          //(yi + yj - 2*y0)^2
          symmetric_force_matrix[id0][id0].y += 2;
          symmetric_force_matrix[id0][id1].y += 2;
          symmetric_force_matrix[id0][base].y -= 4;

          symmetric_force_matrix[id1][id0].y += 2;
          symmetric_force_matrix[id1][id1].y += 2;
          symmetric_force_matrix[id1][base].y -= 4;

          symmetric_force_matrix[base][id0].y -= 2;
          symmetric_force_matrix[base][id1].y -= 2;
          symmetric_force_matrix[base][base].y += 4;
        }
      } else if (it->sympair.size() > 1)  // no self sym, consider the center of first sym pair of blocks as axis = 1/2*(x0.first + x0.second)
      {
        int idbase0 = it->sympair[0].first;
        int idbase1 = it->sympair[0].second;
        tempSPB.axis.first = idbase0;
        tempSPB.axis.second = idbase1;
        // #ifdef DEBUG
        std::cout << "H: cond3, idbase0 = " << idbase0 << ", idbase1 = " << idbase1 << endl;
        // #endif;
        symmetric_force_matrix[idbase0][idbase0].x += 2;
        symmetric_force_matrix[idbase0][idbase1].x -= 2;
        symmetric_force_matrix[idbase1][idbase0].x -= 2;
        symmetric_force_matrix[idbase1][idbase1].x += 2;
        for (int i = 1; i < it->sympair.size(); ++i) {
          int id0 = it->sympair[i].first;
          int id1 = it->sympair[i].second;
          // #ifdef DEBUG
          std::cout << "H: cond3, id0 = " << id0 << ", id1 = " << id1 << endl;
          // #endif;
          //(xi - xj)^2
          symmetric_force_matrix[id0][id0].x += 2;
          symmetric_force_matrix[id0][id1].x -= 2;
          symmetric_force_matrix[id1][id0].x -= 2;
          symmetric_force_matrix[id1][id1].x += 2;
          //(yi + yj - y0 - y1)^2
          symmetric_force_matrix[id0][id0].y += 2;
          symmetric_force_matrix[id0][id1].y += 2;
          symmetric_force_matrix[id0][idbase0].y -= 2;
          symmetric_force_matrix[id0][idbase1].y -= 2;

          symmetric_force_matrix[id1][id0].y += 2;
          symmetric_force_matrix[id1][id1].y += 2;
          symmetric_force_matrix[id1][idbase0].y -= 2;
          symmetric_force_matrix[id1][idbase1].y -= 2;

          symmetric_force_matrix[idbase0][id0].y -= 2;
          symmetric_force_matrix[idbase0][id1].y -= 2;
          symmetric_force_matrix[idbase0][idbase0].y += 2;
          symmetric_force_matrix[idbase0][idbase1].y += 2;

          symmetric_force_matrix[idbase1][id0].y -= 2;
          symmetric_force_matrix[idbase1][id1].y -= 2;
          symmetric_force_matrix[idbase1][idbase0].y += 2;
          symmetric_force_matrix[idbase1][idbase1].y += 2;
        }
      } else {
        continue;
      }
    }
    SPBlocks.push_back(tempSPB);
  }
  // PRINT symmetric _force matrix
  std::cout << "symmetric_force matrix" << std::endl;
  for (int i = 0; i < blockCNT; ++i) {
    for (int j = 0; j < blockCNT; ++j) {
      std::cout << "(" << symmetric_force_matrix[i][j].x << ", " << symmetric_force_matrix[i][j].y << ")";
    }
    std::cout << std::endl;
  }
  // return the total area
  originalBlockCNT = Blocks.size();
  originalNetCNT = Nets.size();
  return totalArea;
}

void Placement::splitNode_MS(float uniHeight, float uniWidth) {
  std::cout << "split Node MS: debug 0" << std::endl;
  int original_block_num = originalBlockCNT;
  for (int i = 0; i < original_block_num; ++i) {
    // step 1: determine the x-direction Standard blocks num
    std::cout << "split Node MS: debug 1" << std::endl;
    Ppoint_F split_numF;
    Ppoint_I split_numI;
    split_numF.y = ceil(Blocks[i].Dpoint.y / uniHeight);
    split_numF.x = ceil(Blocks[i].Dpoint.x / uniWidth);
    split_numI.x = int(split_numF.x);
    split_numI.y = int(split_numF.y);
    // step 2: determine the y-direction standard blocks num
    int num_of_add_blocks = split_numI.y * split_numI.x - 1;
    if (num_of_add_blocks > 0) {
      Blocks[i].splited = 1;
      Blocks[i].Dpoint.x = uniWidth;
      Blocks[i].Dpoint.y = uniHeight;
      Blocks[i].split_shape = split_numF;  // save the information to restore
    }
    int id = Blocks.size();
    for (int j = 0; j < num_of_add_blocks; ++j) {
      std::cout << "split Node MS: debug 2" << std::endl;
      block temp;
      temp.blockname = Blocks[i].blockname + "_" + to_string(j + 1);
      temp.Dpoint.x = uniWidth;
      temp.Dpoint.y = uniHeight;
      temp.splited = 0;
      temp.splitedsource = i;
      temp.index = id;
      Blocks.push_back(temp);
      Blocks[i].spiltBlock.push_back(id);
      ++id;
    }
    // edit the splited module in original block, and add x*y-1 splited block into
    // push the
    std::cout << "split Node MS: debug 3" << std::endl;
    if (num_of_add_blocks > 0 and Blocks[i].commonCentroid == 0) {
      addNet_for_one_split_Blocks(i, split_numI);
    }

    std::cout << "split Node MS: debug 4" << std::endl;
  }
}

void Placement::addNet_for_one_split_Blocks(int blockID, Ppoint_I num) {
  // step 1: put all block id into a x by y 2d array
  std::cout << "add net for one splited blocks: debug 0" << std::endl;
  vector<vector<int>> ID_array;
  ID_array.clear();
  for (int i = 0; i < num.x; ++i) {
    int id = 0;
    vector<int> row;
    row.clear();
    for (int j = 0; j < num.y; ++j) {
      row.push_back(Blocks[blockID].spiltBlock[id]);
      ++id;
      if (id == Blocks[blockID].spiltBlock.size()) {
        break;
      }
    }
    ID_array.push_back(row);
  }
  std::cout << "add net for one splited blocks: debug 1" << std::endl;
  // put the source block into the center position
  Ppoint_I centerPoint;
  centerPoint.x = (num.x - 1) / 2;
  centerPoint.y = (num.y - 1) / 2;
  std::cout << "add net for one splited blocks: debug 2" << std::endl;
  ID_array[num.x - 1].push_back(ID_array[centerPoint.x][centerPoint.y]);
  ID_array[centerPoint.x][centerPoint.y] = blockID;

  std::cout << "add net for one splited blocks: debug 3" << std::endl;
  // add net for each block to connect the adjacent block
  int netID = Nets.size();
  for (int i = 0; i < num.x; ++i) {
    for (int j = 0; j < num.y; ++j) {
      std::cout << "add net for one splited blocks: debug 4" << std::endl;
      net temp1, temp2;
      if (i < num.x - 1) {
        std::cout << "add net for one splited blocks: debug 5" << std::endl;
        temp1.index = netID;
        std::cout << "add net for one splited blocks: debug 6" << std::endl;
        temp1.connected_block.push_back(ID_array[i][j]);
        std::cout << "add net for one splited blocks: debug 7" << std::endl;
        temp1.connected_block.push_back(ID_array[i + 1][j]);
        std::cout << "add net for one splited blocks: debug 8" << std::endl;
        Blocks[ID_array[i][j]].connected_net.push_back(netID);
        std::cout << "add net for one splited blocks: debug 9" << std::endl;
        std::cout << ID_array[i + 1][j] << std::endl;
        Blocks[ID_array[i + 1][j]].connected_net.push_back(netID);
        std::cout << "add net for one splited blocks: debug 10" << std::endl;
        ++netID;
        temp1.weight = dummy_net_weight;
        Nets.push_back(temp1);
      }
      if (j < num.y - 1) {
        std::cout << "add net for one splited blocks: debug 11" << std::endl;
        temp2.index = netID;
        temp2.connected_block.push_back(ID_array[i][j]);
        temp2.connected_block.push_back(ID_array[i][j + 1]);
        temp2.weight = dummy_net_weight;
        Blocks[ID_array[i][j]].connected_net.push_back(netID);
        Blocks[ID_array[i][j + 1]].connected_net.push_back(netID);
        Nets.push_back(temp2);
        ++netID;
      }
    }
  }
  std::cout << "add net for one splited blocks: debug 4" << std::endl;
}

void Placement::update_netlist_after_split_MS() {
  // step 1: for all original nets, split the
}
void Placement::Unify_blocks(float area, float scale_factor) {
  float height = sqrt(scale_factor * area);
  this->est_Size.x = height;
  this->est_Size.y = height;

  for (int i = 0; i < Blocks.size(); i++) {
    Blocks[i].Cpoint.x /= height;
    Blocks[i].Cpoint.y /= height;
    Blocks[i].Dpoint.x /= height;
    Blocks[i].Dpoint.y /= height;
  }
}

void Placement::find_uni_cell() {
  // Ppoint_F uni_cell_Dpoint;
  float min_x = Blocks[0].Dpoint.x, min_y = Blocks[0].Dpoint.y;
  float max_x = Blocks[0].Dpoint.x, max_y = Blocks[0].Dpoint.y;
  for (int i = 1; i < originalBlockCNT; ++i) {
    min_x = min(min_x, Blocks[i].Dpoint.x);
    min_y = min(min_y, Blocks[i].Dpoint.y);
    max_x = max(max_x, Blocks[i].Dpoint.x);
    max_y = max(max_y, Blocks[i].Dpoint.y);
  }
  uni_cell_Dpoint.x = (max_x / min_x <= 2) ? min_x : (max_x / 2);
  uni_cell_Dpoint.y = (max_y / min_y <= 2) ? min_y : (max_y / 2);
  /**
  uni_cell_Dpoint.x = Blocks[0].Dpoint.x;
  uni_cell_Dpoint.y = Blocks[0].Dpoint.y;
  int id = 0;
  for (int i = 1; i < originalBlockCNT; ++i)
  {
    if(Blocks[i].Dpoint.x < uni_cell_Dpoint.x)
    {
      uni_cell_Dpoint.x = Blocks[i].Dpoint.x;
    }
    if(Blocks[i].Dpoint.y < uni_cell_Dpoint.y)
    {
      uni_cell_Dpoint.y = Blocks[i].Dpoint.y;
    }
  }
  **/
  // uni_cell_Dpoint = Blocks[id].Dpoint;
  // return uni_cell_Dpoint;
}

void Placement::readCC() {
  for (int i = 0; i < originalBlockCNT; ++i) {
    int namelength = Blocks[i].blockname.size();
    string name = Blocks[i].blockname;
    std::size_t pos = name.find("_c");

    // int length_of_label = label.length();
    if ((pos = name.find("_c")) != string::npos || (pos = name.find("_C")) != string::npos) {
      std::cout << "find out cc in block" << name << std::endl;
      std::cout << "readCC: debug 0" << std::endl;
      string label = name.substr(pos);
      Blocks[i].commonCentroid = 1;
      int flag = 0;
      for (int j = 0; j < commonCentroids.size(); ++j) {
        std::cout << "readCC: debug 1" << std::endl;
        if (commonCentroids[j].label == label) {
          commonCentroids[j].blocks.push_back(i);
          flag = 1;
          break;
        }
      }
      if (flag == 0) {
        std::cout << "readCC: debug 2" << std::endl;
        commonCentroid temp;
        temp.label = label;
        temp.blocks.push_back(i);
        commonCentroids.push_back(temp);
      }
    }
  }
}
void Placement::addNet_after_split_Blocks(int tol_diff, float uniHeight, float uniWidth) {
  // determine the shape of commonCentroid
  std::cout << "add Net after split BLocks: debug 0" << std::endl;
  for (int i = 0; i < commonCentroids.size(); ++i) {
    std::cout << "add Net after split BLocks: debug 1" << std::endl;
    int cell_num = 0;
    for (int j = 0; j < commonCentroids[i].blocks.size(); ++j) {
      std::cout << "add Net after split BLocks: debug 2" << std::endl;
      int id = commonCentroids[i].blocks[j];

      cell_num += Blocks[id].spiltBlock.size() + 1;
    }
    std::cout << "add Net after split BLocks: debug 3" << std::endl;
    Ppoint_I shape = determineShape(cell_num, tol_diff);
    commonCentroids[i].shape = shape;
    addNet_commonCentroid(commonCentroids[i], cell_num, uniHeight, uniWidth);
  }
}

Ppoint_I Placement::determineShape(int cell_num, int tol_diff) {
  std::cout << "determine shape: debug 0" << std::endl;
  Ppoint_I shape, temp;
  shape.x = (int)ceil(sqrt(cell_num));
  shape.y = shape.x;
  int x_or_y = 0;
  int distance = shape.x * shape.y - cell_num;
  for (int i = (int)ceil(sqrt(cell_num)) - tol_diff; i < (int)ceil(sqrt(cell_num)) + tol_diff; ++i) {
    std::cout << "determine shape: debug 1" << std::endl;
    for (int j = (int)ceil(sqrt(cell_num)) - tol_diff; j < (int)ceil(sqrt(cell_num)) + tol_diff; ++j) {
      std::cout << "determine shape: debug 2" << std::endl;
      int tempDistance = i * j - cell_num;
      if (tempDistance >= 0 and tempDistance < distance) {
        shape.x = i;
        shape.y = j;
        distance = tempDistance;
      }
    }
  }
  return shape;
}

void Placement::addNet_commonCentroid(commonCentroid &CC, int cell_num, float uniHeight, float uniWidth) {
  std::cout << "addNet commonCentroid: debug 0" << std::endl;
  int dummyNum = CC.shape.x * CC.shape.y - cell_num;
  int id = 0;
  std::vector<std::vector<int>> ID_array;
  std::vector<int> ID_vector;
  // for (int i = 0; i < CC.blocks.size(); ++i)
  // {
  //   std::cout << "addNet commonCentroid: debug 1"  << std::endl;
  //   ID_vector.push_back(CC.blocks[i]);
  //   for (int j = 0; j < Blocks[CC.blocks[i]].spiltBlock.size(); ++j)
  //   {
  //     std::cout << "addNet commonCentroid: debug 2"  << std::endl;
  //     ID_vector.push_back(Blocks[CC.blocks[i]].spiltBlock[j]);
  //   }
  // }
  // for (int i = 0; i < dummyNum; ++i)
  // {
  //   std::cout << "addNet commonCentroid: debug 3"  << std::endl;
  //   ID_vector.push_back(-1);
  // }
  // for (int i = 0; i < CC.shape.x; ++i)
  // {
  //   std::cout << "addNet commonCentroid: debug 4"  << std::endl;
  //   std::vector<int> row;
  //   for (int j = 0; j < CC.shape.y; ++j)
  //   {
  //     row.push_back(ID_vector[id]);
  //     ++id;
  //   }
  //   ID_array.push_back(row);
  // }
  match_pairs(CC, dummyNum);
  // ID_array.swap(CC.fillin_matrix);
  int BlockID = Blocks.size();
  for (int i = 0; i < CC.shape.x; ++i) {
    for (int j = 0; j < CC.shape.y; ++j) {
      if (CC.fillin_matrix[i][j] < 0) {
        block temp;
        temp.blockname = CC.label + "dummy";
        temp.Dpoint.x = uniWidth;
        temp.Dpoint.y = uniHeight;
        temp.index = BlockID;
        Blocks.push_back(temp);
        CC.fillin_matrix[i][j] = BlockID;
        ++BlockID;
      }
    }
  }
  // add net
  int netID = Nets.size();
  for (int i = 0; i < CC.shape.x; ++i) {
    std::cout << "addNet commonCentroid: debug 5" << std::endl;
    for (int j = 0; j < CC.shape.y; ++j) {
      std::cout << "addNet commonCentroid: debug 6" << std::endl;
      if (i < CC.shape.x - 1) {
        int b1, b2;
        std::cout << "addNet commonCentroid: debug 6a" << std::endl;
        std::cout << "shape" << CC.shape.x << " " << CC.shape.y << std::endl;
        std::cout << "current pos" << i << " " << j << " " << std::endl;
        // b1 = ID_array[i][j];
        b1 = CC.fillin_matrix[i][j];
        std::cout << "addNet commonCentroid: debug 6b" << std::endl;
        // b2 = ID_array[i+1][j];
        b2 = CC.fillin_matrix[i + 1][j];
        std::cout << "addNet commonCentroid: debug 6c" << std::endl;
        if (b1 > 0 and b2 > 0) {
          std::cout << "addNet commonCentroid: debug 7"
                    << " " << b1 << " " << b2 << std::endl;
          net temp;
          temp.index = netID;
          temp.connected_block.push_back(b1);
          temp.connected_block.push_back(b2);
          Blocks[b1].connected_net.push_back(netID);
          Blocks[b2].connected_net.push_back(netID);
          temp.weight = dummy_net_weight;
          Nets.push_back(temp);
          netID++;
        }
      }
      if (j < CC.shape.y - 1) {
        std::cout << "addNet commonCentroid: debug 8" << std::endl;
        int b1, b2;
        b1 = CC.fillin_matrix[i][j];
        b2 = CC.fillin_matrix[i][j + 1];
        if (b1 > 0 and b2 > 0) {
          net temp;
          temp.index = netID;
          temp.connected_block.push_back(b1);
          temp.connected_block.push_back(b2);
          Blocks[b1].connected_net.push_back(netID);
          Blocks[b2].connected_net.push_back(netID);
          temp.weight = dummy_net_weight;
          Nets.push_back(temp);
          netID++;
        }
      }
    }
  }
  // CC.fillin_matrix.swap(ID_array);
}

void Placement::match_pairs(commonCentroid &CC, int dummyNum) {
  // read the shape from CC
  std::cout << "match pairs: debug 0" << std::endl;
  Ppoint_I shape = CC.shape;
  vector<vector<int>> fillin_matrix;  // to store the relative position of Standard cells
  // determine the center point
  // but if we consider the (x,y) -> (2x,2y), then in the new map, the center is (x-1,y-1)
  // For example, if shape = (2,3) then the new map shape is (4,6), (0,0) (2,0);(0,2),(2,2);(0,4),(2,4)
  // center is (1,2)
  Ppoint_I center = CC.shape;
  center.x -= 1;
  center.y -= 1;

  // calculate the manhattan distance
  vector<pair<pair<int, int>, int>> position_q;
  std::cout << "match pairs: debug 1" << std::endl;
  for (int i = 0; i < shape.x; ++i) {
    vector<int> row;
    std::cout << "match pairs: debug 2" << std::endl;
    for (int j = 0; j < shape.y; ++j) {
      std::cout << "match pairs: debug 3" << std::endl;
      int dis;
      dis = abs(2 * i - center.x) + abs(2 * j - center.y);
      row.push_back(-1);  //-1 means dummy as default
      pair<pair<int, int>, int> position_info;
      pair<int, int> temp;
      temp.first = i;
      temp.second = j;
      position_info.first = temp;
      position_info.second = dis;
      position_q.push_back(position_info);
    }
    std::cout << "match pairs: debug 4" << std::endl;
    fillin_matrix.push_back(row);
  }
  std::cout << "match pairs: debug 5" << std::endl;
  sort(position_q.begin(), position_q.end(), comp_position);
  std::cout << "match pairs: debug 6" << std::endl;
  // match the blocks
  vector<pair<int, int>> block_pairs;
  vector<int> block_q;
  // first find out the original block which be divided into odd number of pieces
  for (int i = 0; i < CC.blocks.size(); ++i) {
    int id = CC.blocks[i];
    std::cout << "match pairs: debug 7" << Blocks[id].spiltBlock.size() << std::endl;

    if (Blocks[id].spiltBlock.size() % 2 == 0) {
      block_q.push_back(id);
      std::cout << "match pairs: debug 7b"
                << " ,id:" << id << std::endl;
    }
  }
  std::cout << "match pairs: debug 8" << std::endl;
  match_vector_into_pairs(block_q, block_pairs);
  std::cout << "match pairs: debug 9" << std::endl;
  // write the pairs into fillin_matrix along the position_q
  int filled_num = 0;
  std::cout << "match pairs: debug 10" << std::endl;
  for (int i = 0; i < block_pairs.size(); ++i) {
    // find out the top element in position q
    std::cout << "match pairs: debug 11" << std::endl;
    pair<int, int> pos;
    pos = position_q[0].first;
    // you may ask what if we have odd number of positions and odd number of blocks
    // that's the reason why I allocate the second element firstly
    // if we have odd number of blocks, the second element is dummy (-2)
    // and the first element will share the same position and cover the dummy
    fillin_matrix[shape.x - 1 - pos.first][shape.y - 1 - pos.second] = block_pairs[i].second;
    fillin_matrix[pos.first][pos.second] = block_pairs[i].first;
    // find out the mirror pos in position_q with 4 steps
    position_q.erase(position_q.begin());
    if (shape.x - 1 - pos.first != pos.first or shape.y - 1 - pos.second != pos.second) {
      std::cout << "match pairs: debug 12" << std::endl;
      for (int j = 0; j < position_q.size(); ++j) {
        std::cout << "match pairs: debug 13" << std::endl;
        if (position_q[j].first.first == shape.x - 1 - pos.first and position_q[j].first.second == shape.y - 1 - pos.second) {
          position_q.erase(position_q.begin() + j);
          break;
        }
      }
    }
  }
  // deal with the remainder blocks
  block_pairs.clear();
  std::cout << "match pairs: debug 14"
            << " " << CC.blocks.size() << std::endl;
  for (int i = 0; i < CC.blocks.size(); ++i) {
    int id = CC.blocks[i];
    std::cout << "match pairs: debug 15"
              << ", " << i << " " << Blocks[id].spiltBlock.size() << std::endl;

    vector<pair<int, int>> temp;
    temp.clear();
    for (int j = 0; (j + 1) < (Blocks[id].spiltBlock.size()); j += 2) {
      std::cout << "match pairs: debug 16"
                << " " << j << std::endl;
      pair<int, int> cur_pair;
      cur_pair.first = Blocks[id].spiltBlock[j];
      cur_pair.second = Blocks[id].spiltBlock[j + 1];
      temp.push_back(cur_pair);
    }
    if (Blocks[id].spiltBlock.size() % 2 == 1)
    // match the last piece of Standard cell and the center piece of standard cell into one pair
    {
      pair<int, int> cur_pair;
      cur_pair.first = Blocks[id].spiltBlock.back();
      cur_pair.second = id;
      temp.push_back(cur_pair);
    }
    std::cout << "match pairs: debug 16a"
              << " " << block_pairs.size() << " " << temp.size() << std::endl;
    for (int j = 0; j < temp.size(); ++j) {
      std::cout << "temp element:" << temp[j].first << " ," << temp[j].second << std::endl;
    }
    merge_two_vectors(block_pairs, temp);
    std::cout << "match pairs: debug 16b"
              << " " << block_pairs.size() << " " << temp.size() << std::endl;
    for (int j = 0; j < block_pairs.size(); ++j) {
      std::cout << "block pairs element:" << block_pairs[j].first << " ," << block_pairs[j].second << std::endl;
    }
  }

  // allocate the position
  for (int i = 0; i < block_pairs.size(); ++i) {
    // find out the top element in position q
    std::cout << "match pairs: debug 17" << std::endl;
    pair<int, int> pos;
    pos = position_q[0].first;
    std::cout << "pos" << pos.first << " " << pos.second << std::endl;
    std::cout << "pos mirror" << shape.x - 1 - pos.first << " " << shape.y - 1 - pos.second << std::endl;
    fillin_matrix[shape.x - 1 - pos.first][shape.y - 1 - pos.second] = block_pairs[i].second;
    fillin_matrix[pos.first][pos.second] = block_pairs[i].first;
    // find out the mirror pos in position_q with 4 steps
    position_q.erase(position_q.begin());
    if (shape.x - 1 - pos.first != pos.first or shape.y - 1 - pos.second != pos.second) {
      for (int j = 0; j < position_q.size(); ++j) {
        std::cout << "match pairs: debug 18" << std::endl;
        if (position_q[j].first.first == shape.x - 1 - pos.first and position_q[j].first.second == shape.y - 1 - pos.second) {
          std::cout << "match pairs: debug 19" << std::endl;
          position_q.erase(position_q.begin() + j);
          break;
        }
      }
    }
  }
  CC.fillin_matrix.swap(fillin_matrix);
  for (int i = 0; i < shape.x; ++i) {
    for (int j = 0; j < shape.y; ++j) {
      std::cout << CC.fillin_matrix[i][j] << " ";
    }
    std::cout << std::endl;
  }
}
void Placement::merge_two_vectors(vector<pair<int, int>> &v1, vector<pair<int, int>> &v2) {
  std::cout << "merge 2 vectors: debug 0a" << std::endl;
  vector<pair<int, int>> A, B;
  std::cout << "merge 2 vectors: debug 0b" << std::endl;
  if (v1.size() > v2.size()) {
    // A.swap(v1);
    // B.swap(v2);
  } else {
    v1.swap(v2);
  }
  std::cout << "merge 2 vectors: debug 1" << std::endl;
  // calculate the period
  int period, sizeA, sizeB, pos;
  sizeA = v1.size();
  sizeB = v2.size();
  pos = 0;
  if (sizeB != 0) {
    period = sizeA / sizeB + 1;
    pos = 1;
  }

  std::cout << "merge 2 vectors: debug 2" << std::endl;
  for (int i = 0; i < v2.size(); ++i) {
    std::cout << "merge 2 vectors: debug 3" << std::endl;
    v1.insert(v1.begin() + pos, v2[i]);
    pos += period;
    std::cout << "merge 2 vectors: debug 4" << std::endl;
  }
  // save the result into v1
  // v1.swap(A);
  // v2.swap(B);
  std::cout << "merge 2 vectors: debug 5" << std::endl;
}
void Placement::match_vector_into_pairs(vector<int> &q, vector<pair<int, int>> &pairs) {
  pairs.clear();
  int i = 0;
  if (q.size() % 2 == 1) {
    pair<int, int> temp;
    temp.first = q[0];
    temp.second = -2;  //-2 means dummy ;-1 means empty;>0 means occupy
    pairs.push_back(temp);
    ++i;
  }
  for (; i + 1 < q.size(); i = i + 2) {
    pair<int, int> temp;
    temp.first = q[i];
    temp.second = q[i + 1];  //-2 means dummy ;-1 means empty;>0 means occupy
    pairs.push_back(temp);
  }
}
bool Placement::comp_position(pair<pair<int, int>, int> p1, pair<pair<int, int>, int> p2) { return p1.second < p2.second; }
void Placement::restore_MS() {
  for (int i = 0; i < originalBlockCNT; ++i) {
    // restore the shape

    // restore the center
    if (Blocks[i].commonCentroid == 0 and Blocks[i].splited == 1) {
      Ppoint_F split_shape = Blocks[i].split_shape;
      Blocks[i].Dpoint.x *= split_shape.x;
      Blocks[i].Dpoint.y *= split_shape.y;
      for (int j = 0; j < Blocks[i].spiltBlock.size(); ++j) {
        Blocks[i].Cpoint.x += Blocks[Blocks[i].spiltBlock[j]].Cpoint.x;
        Blocks[i].Cpoint.y += Blocks[Blocks[i].spiltBlock[j]].Cpoint.y;

        Blocks[Blocks[i].spiltBlock[j]].Cpoint.x = 0;
        Blocks[Blocks[i].spiltBlock[j]].Cpoint.y = 0;
      }
      Blocks[i].Cpoint.x /= Blocks[i].spiltBlock.size() + 1;
      Blocks[i].Cpoint.y /= Blocks[i].spiltBlock.size() + 1;
    }
  }
}

void Placement::refine_CC() {
  for (int i = 0; i < commonCentroids.size(); ++i) {
    std::cout << "refine_CC: debug 0" << std::endl;
    Ppoint_F center;
    int id0, id1, id2, id3;
    Ppoint_I index0, index1;
    if (commonCentroids[i].shape.x % 2 == 0) {
      std::cout << "refine_CC: debug 1" << std::endl;
      index0.x = commonCentroids[i].shape.x / 2;
      index1.x = index0.x - 1;

      // center.x = 0;
    } else {
      std::cout << "refine_CC: debug 2" << std::endl;
      index0.x = (commonCentroids[i].shape.x - 1) / 2;
      index1.x = index0.x;
    }
    if (commonCentroids[i].shape.y % 2 == 0) {
      std::cout << "refine_CC: debug 3" << std::endl;
      index0.y = commonCentroids[i].shape.y / 2;
      index1.y = index0.y - 1;

      // center.x = 0;
    } else {
      std::cout << "refine_CC: debug 4" << std::endl;
      index0.y = (commonCentroids[i].shape.y - 1) / 2;
      index1.y = index0.y;
    }
    std::cout << "refine_CC: debug 5" << std::endl;
    id0 = commonCentroids[i].fillin_matrix[index0.x][index0.y];
    id1 = commonCentroids[i].fillin_matrix[index0.x][index1.y];
    id2 = commonCentroids[i].fillin_matrix[index1.x][index1.y];
    id3 = commonCentroids[i].fillin_matrix[index1.x][index0.y];
    std::cout << "refine_CC: debug 6" << std::endl;
    center.x = Blocks[id0].Cpoint.x + Blocks[id1].Cpoint.x + Blocks[id2].Cpoint.x + Blocks[id3].Cpoint.x;

    center.y = Blocks[id0].Cpoint.y + Blocks[id1].Cpoint.y + Blocks[id2].Cpoint.y + Blocks[id3].Cpoint.y;
    center.x /= 4;
    center.y /= 4;
    std::cout << "refine_CC: debug 7" << commonCentroids[i].shape.x << " " << commonCentroids[i].shape.y << std::endl;
    // push every pair of element to match the center
    for (int j = 0; j < commonCentroids[i].shape.x; ++j) {
      for (int k = 0; k < commonCentroids[i].shape.y; ++k) {
        std::cout << "refine_CC: debug 8" << std::endl;
        // find mirror pos
        Ppoint_I pos;
        pos.x = commonCentroids[i].shape.x - 1 - j;
        pos.y = commonCentroids[i].shape.y - 1 - k;
        Ppoint_F pair_center;
        int temp_id0, temp_id1;
        temp_id0 = commonCentroids[i].fillin_matrix[j][k];
        temp_id1 = commonCentroids[i].fillin_matrix[pos.x][pos.y];
        pair_center.x = Blocks[temp_id1].Cpoint.x + Blocks[temp_id0].Cpoint.x;
        pair_center.y = Blocks[temp_id1].Cpoint.y + Blocks[temp_id0].Cpoint.y;
        pair_center.x /= 2;
        pair_center.y /= 2;
        Ppoint_F offset;
        std::cout << "refine_CC: debug 9" << std::endl;
        offset.x = center.x - pair_center.x;
        offset.y = center.y - pair_center.y;
        Blocks[temp_id0].Cpoint.x += offset.x;
        Blocks[temp_id0].Cpoint.y += offset.y;
        Blocks[temp_id1].Cpoint.x += offset.x;
        Blocks[temp_id1].Cpoint.y += offset.y;
      }
    }
  }
}

void Placement::restore_CC_in_square(bool isCompact) {
  for (int i = 0; i < commonCentroids.size(); ++i) {
    // find X_MAX, Y_MAX, X_MIN, Y_MIN
    float X_MAX, Y_MAX, X_MIN, Y_MIN;
    int index = commonCentroids[i].blocks[0];
    X_MAX = X_MIN = Blocks[index].Cpoint.x;
    Y_MAX = Y_MIN = Blocks[index].Cpoint.y;

    for (int ii = 0; ii < commonCentroids[i].shape.x; ++ii) {
      for (int jj = 0; jj < commonCentroids[i].shape.y; ++jj) {
        index = commonCentroids[i].fillin_matrix[ii][jj];
        if (index >= 0) {
          Ppoint_F center = Blocks[index].Cpoint;
          X_MIN = center.x < X_MIN ? center.x : X_MIN;
          X_MAX = center.x > X_MAX ? center.x : X_MAX;
          Y_MIN = center.y < Y_MIN ? center.y : Y_MIN;
          Y_MAX = center.y > Y_MAX ? center.y : Y_MAX;
        }
      }
    }

    // relocate all blocks

    Ppoint_F inteval;
    if (commonCentroids[i].shape.x > 1) {
      if (isCompact) {
        inteval.x = (X_MAX - X_MIN) / (commonCentroids[i].shape.x - 1) / 2;
      } else {
        inteval.x = (X_MAX - X_MIN) / (commonCentroids[i].shape.x - 1);
      }

    } else {
      inteval.x = 0;
    }

    if (commonCentroids[i].shape.y > 1) {
      // inteval.y = (Y_MAX - Y_MIN)/(commonCentroids[i].shape.y-1);
      if (isCompact) {
        inteval.y = (Y_MAX - Y_MIN) / (commonCentroids[i].shape.y - 1) / 2;
      } else {
        inteval.y = (Y_MAX - Y_MIN) / (commonCentroids[i].shape.y - 1);
      }

    } else {
      inteval.y = 0;
    }

    for (int ii = 0; ii < commonCentroids[i].shape.x; ++ii) {
      for (int jj = 0; jj < commonCentroids[i].shape.y; ++jj) {
        index = commonCentroids[i].fillin_matrix[ii][jj];
        if (index >= 0) {
          Blocks[index].Cpoint.x = X_MIN + ii * inteval.x;
          Blocks[index].Cpoint.y = Y_MIN + jj * inteval.y;
        }
      }
    }
  }
}

void Placement::restore_MS(PnRDB::hierNode &current_node) {
  std::cout << "restore ms debug:0" << std::endl;
  current_node.isFirstILP = 0;
  PnRDB::hierNode copy_node(current_node);
  current_node.Blocks.erase(current_node.Blocks.end() - (Blocks.size() - originalBlockCNT), current_node.Blocks.end());
  std::cout << "restore ms debug:1" << std::endl;
  current_node.Nets.erase(current_node.Nets.end() - (Nets.size() - originalNetCNT), current_node.Nets.end());

  for (int i = 0; i < current_node.SPBlocks.size(); ++i) {
    int j = 0;
    while (j < current_node.SPBlocks[i].selfsym.size() && current_node.SPBlocks[i].selfsym[j].first < originalBlockCNT) {
      ++j;
    }

    if (j < current_node.SPBlocks[i].selfsym.size()) {
      current_node.SPBlocks[i].selfsym.erase(current_node.SPBlocks[i].selfsym.begin() + j, current_node.SPBlocks[i].selfsym.end());
    }

    j = 0;
    while (j < current_node.SPBlocks[i].sympair.size() && current_node.SPBlocks[i].sympair[j].first < originalBlockCNT &&
           current_node.SPBlocks[i].sympair[j].second < originalBlockCNT) {
      ++j;
    }

    if (j < current_node.SPBlocks[i].sympair.size()) {
      current_node.SPBlocks[i].sympair.erase(current_node.SPBlocks[i].sympair.begin() + j, current_node.SPBlocks[i].sympair.end());
    }
  }
  std::cout << "restore ms debug:3" << std::endl;
  // restore the size of block
  int idx = 0;
  for (int i = 0; i < current_node.Nets.size(); ++i) {
    current_node.Nets[i].weight = 1.0;
  }
  std::cout << "restore ms debug:4" << std::endl;
  for (int i = 0; i < current_node.Blocks.size(); ++i) {
    for (int j = 0; j < 1; ++j) {
      if (Blocks[idx].splited) {
        current_node.Blocks[i].instance[j].height *= (int)ceil(Blocks[idx].split_shape.y);
        current_node.Blocks[i].instance[j].width *= (int)ceil(Blocks[idx].split_shape.x);
        current_node.Blocks[i].instance[j].originBox.UR.y *= (int)ceil(Blocks[idx].split_shape.y);
        current_node.Blocks[i].instance[j].originBox.UR.x *= (int)ceil(Blocks[idx].split_shape.x);
        current_node.Blocks[i].instance[j].originCenter.y *= (int)ceil(Blocks[idx].split_shape.y);
        current_node.Blocks[i].instance[j].originCenter.x *= (int)ceil(Blocks[idx].split_shape.x);
      }
      ++idx;
    }
  }

  std::cout << "restore ms debug:5" << std::endl;
  // merge CC block
  // make origin block in CC size to zero
  int id_new_block = originalBlockCNT;
  // Ppoint_F uni_cell_Dpoint;
  // uni_cell_Dpoint.x = Blocks[0].Dpoint.x;
  // uni_cell_Dpoint.y = Blocks[0].Dpoint.y;
  Ppoint_I uni_cell_shape;
  uni_cell_shape.x = (int)(est_Size.x * uni_cell_Dpoint.x);
  uni_cell_shape.y = (int)(est_Size.y * uni_cell_Dpoint.y);
  for (int i = 0; i < commonCentroids.size(); ++i) {
    vector<int> to_connect;
    PnRDB::block tempBlock;
    tempBlock.name = "CC_merge_cell" + commonCentroids[i].label;
    tempBlock.orient = PnRDB::N;
    cc_name_to_id_map[tempBlock.name] = i;

    tempBlock.height = Blocks[commonCentroids[i].blocks[0]].original_Dpoint.y;
    tempBlock.width = Blocks[commonCentroids[i].blocks[0]].original_Dpoint.x;
    tempBlock.originBox.UR.x = tempBlock.width;
    tempBlock.originBox.UR.y = tempBlock.height;
    tempBlock.originCenter.x = tempBlock.width / 2;
    tempBlock.originCenter.y = tempBlock.height / 2;
    for (int j = 0; j < commonCentroids[i].blocks.size(); ++j) {
      tempBlock.placedCenter.x += copy_node.Blocks[commonCentroids[i].blocks[j]].instance[0].placedCenter.x;
      tempBlock.placedCenter.y += copy_node.Blocks[commonCentroids[i].blocks[j]].instance[0].placedCenter.y;
    }
    tempBlock.placedCenter.x /= float(commonCentroids[i].blocks.size());
    tempBlock.placedCenter.y /= float(commonCentroids[i].blocks.size());
    tempBlock.placedBox.LL.x = tempBlock.placedCenter.x - tempBlock.width / 2;
    tempBlock.placedBox.UR.x = tempBlock.placedCenter.x + tempBlock.width / 2;
    tempBlock.placedBox.LL.y = tempBlock.placedCenter.y - tempBlock.height / 2;
    tempBlock.placedBox.UR.y = tempBlock.placedCenter.y + tempBlock.height / 2;
    tempBlock.gdsFile = copy_node.Blocks[commonCentroids[i].blocks[0]].instance[0].gdsFile;
    tempBlock.PowerNets = copy_node.Blocks[commonCentroids[i].blocks[0]].instance[0].PowerNets;
    tempBlock.blockPins = copy_node.Blocks[commonCentroids[i].blocks[0]].instance[0].blockPins;
    tempBlock.interMetals = copy_node.Blocks[commonCentroids[i].blocks[0]].instance[0].interMetals;
    tempBlock.interVias = copy_node.Blocks[commonCentroids[i].blocks[0]].instance[0].interVias;
    tempBlock.master = copy_node.Blocks[commonCentroids[i].blocks[0]].instance[0].master;
    tempBlock.lefmaster = copy_node.Blocks[commonCentroids[i].blocks[0]].instance[0].lefmaster;
    tempBlock.dummy_power_pin = copy_node.Blocks[commonCentroids[i].blocks[0]].instance[0].dummy_power_pin;
    tempBlock.GuardRings = copy_node.Blocks[commonCentroids[i].blocks[0]].instance[0].GuardRings;

    PnRDB::blockComplex tempBlockComplex;

    tempBlockComplex.instNum = 1;
    tempBlockComplex.instance.push_back(tempBlock);
    current_node.Blocks.push_back(tempBlockComplex);
    new_to_original_block_map[current_node.Blocks.size() - 1] = commonCentroids[i].blocks[0];
    original_to_new_block_map[commonCentroids[i].blocks[0]] = current_node.Blocks.size() - 1;
    std::cout << "restore ms debug:6" << std::endl;
    for (int j = 0; j < commonCentroids[i].blocks.size(); ++j) {
      int id = commonCentroids[i].blocks[j];
      int cur_id = 0;
      // find that id
      for (int ii = 0; ii < current_node.Blocks.size(); ++ii) {
        for (int jj = 0; jj < 1; ++jj) {
          if (id == cur_id) {
            current_node.Blocks[ii].instance[jj].height = 0;
            current_node.Blocks[ii].instance[jj].width = 0;
            current_node.Blocks[ii].instance[jj].originBox.UR.x = 0;
            current_node.Blocks[ii].instance[jj].originBox.UR.y = 0;
            current_node.Blocks[ii].instance[jj].originBox.LL.x = 0;
            current_node.Blocks[ii].instance[jj].originBox.LL.y = 0;
            current_node.Blocks[ii].instance[jj].originCenter.x = 0;
            current_node.Blocks[ii].instance[jj].originCenter.y = 0;
            current_node.Blocks[ii].instance[jj].placedBox.UR.x = 0;
            current_node.Blocks[ii].instance[jj].placedBox.UR.y = 0;
            current_node.Blocks[ii].instance[jj].placedBox.LL.x = 0;
            current_node.Blocks[ii].instance[jj].placedBox.LL.y = 0;
            current_node.Blocks[ii].instance[jj].placedCenter.x = 0;
            current_node.Blocks[ii].instance[jj].placedCenter.y = 0;
            current_node.Blocks[ii].instance[jj].isRead = false;
          }
        }
        if (id == cur_id) {
          break;
        } else {
          ++cur_id;
        }
      }
      std::cout << "restore ms debug:7" << std::endl;
      for (int k = 0; k < Blocks[id].connected_net.size(); ++k) {
        int netid = Blocks[id].connected_net[k];
        // current_node.Nets[netid].weight=0;
        if (netid < originalNetCNT) {
          PnRDB::connectNode tempNode;
          tempNode.iter2 = id_new_block;
          tempNode.type = PnRDB::Block;
          tempNode.iter = 0;
          // current_node.Nets[netid].connected.push_back(tempNode);

          for (int ii = 0; ii < current_node.Nets[netid].connected.size(); ++ii) {
            if (current_node.Nets[netid].connected[ii].iter2 == id) {
              current_node.Nets[netid].connected[ii].iter2 = id_new_block;
              break;
            }
          }
        }
      }
    }
    std::cout << "restore ms debug:8" << std::endl;
    id_new_block++;
  }
  for (auto &s : current_node.SPBlocks) {
    for (auto &p : s.sympair) {
      if (original_to_new_block_map.find(p.first) != original_to_new_block_map.end()) p.first = original_to_new_block_map[p.first];
      if (original_to_new_block_map.find(p.second) != original_to_new_block_map.end()) p.second = original_to_new_block_map[p.second];
    }
    for (auto &p : s.selfsym) {
      if (original_to_new_block_map.find(p.first) != original_to_new_block_map.end()) p.first = original_to_new_block_map[p.first];
    }
  }
  for (auto &a : current_node.Align_blocks) {
    for (auto &b : a.blocks) {
      if (original_to_new_block_map.find(b) != original_to_new_block_map.end()) b = original_to_new_block_map[b];
    }
  }
  for (auto &order : current_node.Ordering_Constraints) {
    for (auto &b : order.first) {
      if (original_to_new_block_map.find(b) != original_to_new_block_map.end()) b = original_to_new_block_map[b];
    }
  }
  PlotPlacement(601);
}
// donghao end

void Placement::print_blocks_nets() {
  std::cout << "print information about blocks" << std::endl;
  for (int i = 0; i < Blocks.size(); ++i) {
    std::cout << "block id" << Blocks[i].index;
    std::cout << "block position: (" << Blocks[i].Cpoint.x << ", " << Blocks[i].Cpoint.y << ")"
              << "d:(" << Blocks[i].Dpoint.x << ", " << Blocks[i].Dpoint.y << ")" << std::endl;

    std::cout << "connect net:";
    for (int j = 0; j < Blocks[i].connected_net.size(); ++j) {
      std::cout << Blocks[i].connected_net[j] << " ";
    }
    std::cout << std::endl;
  }

  std::cout << "print information about nets" << std::endl;
  for (int i = 0; i < Nets.size(); ++i) {
    std::cout << "net id" << Nets[i].index;
    // std::cout << "block position: (" << Blocks[i].Cpoint.x << ", " << Blocks[i].Cpoint.y << ")"
    //           << "d:(" << Blocks[i].Dpoint.x << ", " << Blocks[i].Dpoint.y << ")" << std::endl;

    std::cout << "connect block:";
    for (int j = 0; j < Nets[i].connected_block.size(); ++j) {
      std::cout << Nets[i].connected_block[j] << " ";
    }
    std::cout << std::endl;
  }
}

void Placement::Cal_sym_Force() {
#ifdef DEBUG
  std::cout << "Cal_sym_Force debug flag: 1" << std::endl;
#endif
  for (int i = 0; i < symmetric_force_matrix.size(); ++i) {
    Blocks[i].Symmetricforce.x = 0;
    Blocks[i].Symmetricforce.y = 0;
    for (int j = 0; j < symmetric_force_matrix[i].size(); ++j) {
#ifdef DEBUG
      std::cout << "Cal_sym_Force debug flag: 3" << std::endl;
      std::cout << "force x=" << symmetric_force_matrix[i][j].x << ", force y=" << symmetric_force_matrix[i][j].y;
      std::cout << "center x=" << Blocks[j].Cpoint.x << ", center y=" << Blocks[j].Cpoint.y << std::endl;
#endif
      Blocks[i].Symmetricforce.x += symmetric_force_matrix[i][j].x * Blocks[j].Cpoint.x;
      Blocks[i].Symmetricforce.y += symmetric_force_matrix[i][j].y * Blocks[j].Cpoint.y;
#ifdef DEBUG
      std::cout << "Cal_sym_Force debug flag: 4" << std::endl;
#endif
    }
    if (isnan(Blocks[i].Symmetricforce.x)) {
#ifdef DEBUG
      std::cout << "Cal_sym_Force debug flag: 5" << std::endl;
#endif
      Blocks[i].Symmetricforce.x = 0;
    }
    if (isnan(Blocks[i].Symmetricforce.y)) {
#ifdef DEBUG
      std::cout << "Cal_sym_Force debug flag: 6" << std::endl;
#endif
      Blocks[i].Symmetricforce.y = 0;
    }
  }
#ifdef DEBUG
  std::cout << "Cal_sym_Force debug flag: 2" << std::endl;
#endif
}

void Placement::read_alignment(PnRDB::hierNode &current_node) {
  //###############################################
  // old version using struct Alignment//
  //###############################################
  // float height = this->est_Size.y;
  // float weight = this->est_Size.x;
  // Alignment_blocks.clear();

  // for(int i = 0;i < current_node.Alignment_blocks.size();++i)
  // {

  // Alignment temp;
  // //find the larger blocks
  // float s1,s2;
  // int id1,id2;
  // id1 = current_node.Alignment_blocks[i].blockid1;
  // id2 = current_node.Alignment_blocks[i].blockid2;

  // s1 = Blocks[id1].Dpoint.x * Blocks[id1].Dpoint.y;
  // s2 = Blocks[id2].Dpoint.x * Blocks[id2].Dpoint.y;
  // if(s2 > s1)
  // {
  //   temp.blockid1 = id2;
  //   temp.blockid2 = id1;
  // }
  // else
  // {
  //   temp.blockid1 = id1;
  //   temp.blockid2 = id2;
  // }

  // temp.horizon = current_node.Alignment_blocks[i].horizon;
  // if(temp.horizon == 1)//LL1.x = LL2.x ->c1.x - d1.x/2 = c2.x - d2.x/2
  // //distance = c2.x - c1.x = d2.x/2 - d1.x/2
  // {
  //   temp.distance = 0.5*(Blocks[temp.blockid2].Dpoint.x - Blocks[temp.blockid1].Dpoint.x);
  // }
  // else
  // {
  //   temp.distance = 0.5*(Blocks[temp.blockid2].Dpoint.y - Blocks[temp.blockid1].Dpoint.y);
  // }

  // Alignment_blocks.push_back(temp);

  //###############################################
  // new version using struct AlignBLock//
  //###############################################

  // }
  AlignBlocks.clear();
  for (int i = 0; i < current_node.Align_blocks.size(); ++i) {
    AlignBlock temp;
    PnRDB::AlignBlock *curAlign = &current_node.Align_blocks[i];
    temp.blocks.clear();
    temp.blocks = curAlign->blocks;
    temp.horizon = curAlign->horizon;
    // find the largest blocks and put it into begin()
    AlignBlocks.push_back(temp);
  }
}

void Placement::force_alignment(vector<float> &vc_x, vector<float> &vl_x, vector<float> &vc_y, vector<float> &vl_y) {
  //###############################################
  // old version using struct Alignment//
  //###############################################

  // for(int i = 0;i < Alignment_blocks.size();++i)
  // {
  //   int id1 = Alignment_blocks[i].blockid1;
  //   int id2 = Alignment_blocks[i].blockid2;
  //   float distance = Alignment_blocks[i].distance;
  //   if(Alignment_blocks[i].horizon == 1)
  //   {
  //     Blocks[id2].Cpoint.x = Blocks[id1].Cpoint.x + distance;
  //   }
  //   else
  //   {
  //     Blocks[id2].Cpoint.y = Blocks[id1].Cpoint.y + distance;
  //   }
  // }
  std::cout << "force align begin" << std::endl;
  for (int i = 0; i < AlignBlocks.size(); ++i) {
    int headIdx = AlignBlocks[i].blocks[0];
    Ppoint_F head_pos = Blocks[headIdx].Cpoint;
    Ppoint_F head_dem = Blocks[headIdx].Dpoint;
    if (AlignBlocks[i].horizon) {
      for (int j = 1; j < AlignBlocks[i].blocks.size(); ++j) {
        int cur_idx = AlignBlocks[i].blocks[j];
        Ppoint_F cur_dem = Blocks[cur_idx].Dpoint;

        float distance = 1 / 2 * (cur_dem.y - head_dem.y);
        Blocks[cur_idx].Cpoint.y = head_pos.y + distance;
        // update vl and vc
        if (vl_y.size() > cur_idx) {
          // vl_y[cur_idx] = Blocks[cur_idx].Cpoint.y;
          vc_y[cur_idx] = Blocks[cur_idx].Cpoint.y;
        }
      }
    } else {
      for (int j = 1; j < AlignBlocks[i].blocks.size(); ++j) {
        int cur_idx = AlignBlocks[i].blocks[j];
        Ppoint_F cur_dem = Blocks[cur_idx].Dpoint;

        float distance = 1 / 2 * (cur_dem.x - head_dem.x);
        Blocks[cur_idx].Cpoint.x = head_pos.x + distance;
        // update vl and vc
        if (vl_x.size() > cur_idx) {
          // vl_x[cur_idx] = Blocks[cur_idx].Cpoint.x;
          vc_x[cur_idx] = Blocks[cur_idx].Cpoint.x;
        }
      }
    }
  }
  std::cout << "force align finish" << std::endl;
}

void Placement::read_order(PnRDB::hierNode &current_node) {
  Ordering_Constraints = current_node.Ordering_Constraints;
  std::cout << "ordering constraints size: " << Ordering_Constraints.size() << std::endl;
}

void Placement::force_symmetry(PnRDB::hierNode &current_node) {
  for (auto symmetry : current_node.SPBlocks) {
    if (symmetry.axis_dir == PnRDB::V) {
      // set<int> center_y_set;
      // set<int> distance_set;
      float center_x = 0;
      for (auto i_selfsym : symmetry.selfsym) {
        center_x += Blocks[i_selfsym.first].Cpoint.x;
      }
      for (auto i_sympair : symmetry.sympair) {
        center_x += Blocks[i_sympair.first].Cpoint.x;
        center_x += Blocks[i_sympair.second].Cpoint.x;
      }
      center_x /= (symmetry.selfsym.size() + symmetry.sympair.size() * 2);

      for (auto i_selfsym : symmetry.selfsym) {
        /**
        while(center_y_set.find(node.Blocks[i_selfsym.first].instance[0].placedCenter.y)!=center_y_set.end())
          node.Blocks[i_selfsym.first].instance[0].placedCenter.y++;
        center_y_set.insert(node.Blocks[i_selfsym.first].instance[0].placedCenter.y);
        **/
        Blocks[i_selfsym.first].Cpoint.x = center_x;
      }

      for (auto i_sympair : symmetry.sympair) {
        float diff = center_x - (Blocks[i_sympair.first].Cpoint.x + Blocks[i_sympair.second].Cpoint.x) / 2;
        Blocks[i_sympair.first].Cpoint.x += diff;
        Blocks[i_sympair.second].Cpoint.x += diff;
        /**
         * while(distance_set.find(abs(node.Blocks[i_sympair.first].instance[0].placedCenter.x-node.Blocks[i_sympair.second].instance[0].placedCenter.x))!=distance_set.end()){
          node.Blocks[i_sympair.first].instance[0].placedCenter.x--;
          node.Blocks[i_sympair.second].instance[0].placedCenter.x++;
        }
        distance_set.insert(abs(node.Blocks[i_sympair.first].instance[0].placedCenter.x - node.Blocks[i_sympair.second].instance[0].placedCenter.x));
        int center_y = (node.Blocks[i_sympair.first].instance[0].placedCenter.y + node.Blocks[i_sympair.second].instance[0].placedCenter.y) / 2;
        while (center_y_set.find(center_y) != center_y_set.end()) center_y++;
        center_y_set.insert(center_y);
        node.Blocks[i_sympair.first].instance[0].placedCenter.y = center_y;
        node.Blocks[i_sympair.second].instance[0].placedCenter.y = center_y;
        **/
        float center_y = (Blocks[i_sympair.first].Cpoint.y + Blocks[i_sympair.second].Cpoint.y) / 2;
        Blocks[i_sympair.first].Cpoint.y = center_y;
        Blocks[i_sympair.second].Cpoint.y = center_y;
      }
    } else {
      // set<int> center_x_set;
      // set<int> distance_set;
      float center_y = 0;
      for (auto i_selfsym : symmetry.selfsym) {
        center_y += Blocks[i_selfsym.first].Cpoint.y;
      }
      for (auto i_sympair : symmetry.sympair) {
        center_y += Blocks[i_sympair.first].Cpoint.y;
        center_y += Blocks[i_sympair.second].Cpoint.y;
      }
      center_y /= (symmetry.selfsym.size() + symmetry.sympair.size() * 2);
      for (auto i_selfsym : symmetry.selfsym) {
        /**while(center_x_set.find(node.Blocks[i_selfsym.first].instance[0].placedCenter.x)!=center_x_set.end())
          node.Blocks[i_selfsym.first].instance[0].placedCenter.x++;
        center_x_set.insert(node.Blocks[i_selfsym.first].instance[0].placedCenter.x);
        **/
        Blocks[i_selfsym.first].Cpoint.y = center_y;
      }
      for (auto i_sympair : symmetry.sympair) {
        float diff = center_y - (Blocks[i_sympair.first].Cpoint.y + Blocks[i_sympair.second].Cpoint.y) / 2;
        Blocks[i_sympair.first].Cpoint.y += diff;
        Blocks[i_sympair.second].Cpoint.y += diff;
        /**
        while(distance_set.find(abs(node.Blocks[i_sympair.first].instance[0].placedCenter.y-node.Blocks[i_sympair.second].instance[0].placedCenter.y))!=distance_set.end()){
          node.Blocks[i_sympair.first].instance[0].placedCenter.y--;
          node.Blocks[i_sympair.second].instance[0].placedCenter.y++;
        }
        distance_set.insert(abs(node.Blocks[i_sympair.first].instance[0].placedCenter.y - node.Blocks[i_sympair.second].instance[0].placedCenter.y));
        int center_x = (node.Blocks[i_sympair.first].instance[0].placedCenter.x + node.Blocks[i_sympair.second].instance[0].placedCenter.x) / 2;
        while (center_x_set.find(center_x) != center_x_set.end()) center_x++;
        center_x_set.insert(center_x);
        node.Blocks[i_sympair.first].instance[0].placedCenter.x = center_x;
        node.Blocks[i_sympair.second].instance[0].placedCenter.x = center_x;
        **/
        float center_x = (Blocks[i_sympair.first].Cpoint.x + Blocks[i_sympair.second].Cpoint.x) / 2;
        Blocks[i_sympair.first].Cpoint.x = center_x;
        Blocks[i_sympair.second].Cpoint.x = center_x;
      }
    }
  }
}

void Placement::force_order(vector<float> &vc_x, vector<float> &vl_x, vector<float> &vc_y, vector<float> &vl_y) {
  // step 1: put the Cpoint into verctor
  for (int i = 0; i < Ordering_Constraints.size(); ++i) {
    vector<Ppoint_F> Centers = vector<Ppoint_F>();
    for (int j = 0; j < Ordering_Constraints[i].first.size(); ++j) {
      std::cout << "ordering id before sort: " << Ordering_Constraints[i].first[j];
      Centers.push_back(Blocks[Ordering_Constraints[i].first[j]].Cpoint);
      std::cout << "pos:" << Centers[j].x << ", " << Centers[j].y << std::endl;
    }
    // step 2: sort the Cpoint vector
    if (Ordering_Constraints[i].second == PnRDB::H) {
      sort(Centers.begin(), Centers.end(), comp_x);
    } else {
      sort(Centers.begin(), Centers.end(), comp_y);
    }
    // step 3: assign the sorted cpoint

    for (int j = 0; j < Ordering_Constraints[i].first.size(); ++j) {
      int id = Ordering_Constraints[i].first[j];
      std::cout << "ordering id after sort: " << id;
      if (Ordering_Constraints[i].second == PnRDB::H) {
        Blocks[id].Cpoint.x = Centers[j].x;
        if (vl_x.size() > id) {
          // vl_x[id] = Blocks[id].Cpoint.x;
          vc_x[id] = Blocks[id].Cpoint.x;
        }
      } else {
        Blocks[id].Cpoint.y = Centers[j].y;
        if (vl_y.size() > id) {
          // vl_y[id] = Blocks[id].Cpoint.y;
          vc_y[id] = Blocks[id].Cpoint.y;
        }
      }

      std::cout << "pos:" << Centers[j].x << ", " << Centers[j].y << std::endl;
    }
  }
}

bool Placement::comp_x(Ppoint_F c1, Ppoint_F c2) { return c1.x < c2.x; }

bool Placement::comp_y(Ppoint_F c1, Ppoint_F c2) { return c1.y > c2.y; }

void Placement::writeback(PnRDB::hierNode &current_node) {
  int idx = 0;
  for (vector<PnRDB::blockComplex>::iterator it = current_node.Blocks.begin(); it != current_node.Blocks.end(); ++it) {
    for (int i = 0; i < 1; ++i) {
      it->instance[i].placedCenter.x = (int)(est_Size.x * Blocks[idx].Cpoint.x);
      it->instance[i].placedCenter.y = (int)(est_Size.y * Blocks[idx].Cpoint.y);

      it->instance[i].placedBox.LL.x = (int)(est_Size.x * Blocks[idx].Cpoint.x) - it->instance[i].width / 2;
      it->instance[i].placedBox.LL.y = (int)(est_Size.y * Blocks[idx].Cpoint.y) - it->instance[i].height / 2;

      it->instance[i].placedBox.UR.x = (int)(est_Size.x * Blocks[idx].Cpoint.x) + it->instance[i].width / 2;
      it->instance[i].placedBox.UR.y = (int)(est_Size.y * Blocks[idx].Cpoint.y) + it->instance[i].height / 2;
      it->instance[i].orient = PnRDB::N;
      ++idx;
    }
  }
}

bool Placement::symCheck(float tol) {
  float tot_bias = 0;
  for (int i = 0; i < SPBlocks.size(); ++i) {
    if (SPBlocks[i].horizon) {
      // step 1: find the axis
      float axis, axis_double;
      int baseid0, baseid1;
      baseid0 = SPBlocks[i].axis.first;
      baseid1 = SPBlocks[i].axis.second;
      axis = 0.5 * (Blocks[baseid0].Cpoint.y + Blocks[baseid1].Cpoint.y);
      axis_double = axis * 2;
      // step 2: evalue all modules in sympair
      for (int j = 0; j < SPBlocks[i].sympair.size(); ++j) {
        int id0 = SPBlocks[i].sympair[j].first;
        int id1 = SPBlocks[i].sympair[j].second;
        tot_bias += fabs(Blocks[id0].Cpoint.y + Blocks[id1].Cpoint.y - axis_double);
      }
      // step 3: evalue all modules in selfs0.211138m0.211138
      for (int j = 0; j < SPBlocks[i].selfsym.size(); ++j) {
        int id0 = SPBlocks[i].selfsym[j];
        tot_bias += fabs(Blocks[id0].Cpoint.y - axis);
      }
    } else {
      // step 1: find the axis
      float axis, axis_double;
      int baseid0, baseid1;
      baseid0 = SPBlocks[i].axis.first;
      baseid1 = SPBlocks[i].axis.second;
      axis = 0.5 * (Blocks[baseid0].Cpoint.x + Blocks[baseid1].Cpoint.x);
      axis_double = axis * 2;
      // step 2: evalue all modules in sympair
      for (int j = 0; j < SPBlocks[i].sympair.size(); ++j) {
        int id0 = SPBlocks[i].sympair[j].first;
        int id1 = SPBlocks[i].sympair[j].second;
        tot_bias += fabs(Blocks[id0].Cpoint.x + Blocks[id1].Cpoint.x - axis_double);
      }
      // step 3: evalue all modules in selfsym
      for (int j = 0; j < SPBlocks[i].selfsym.size(); ++j) {
        int id0 = SPBlocks[i].selfsym[j];
        tot_bias += fabs(Blocks[id0].Cpoint.x - axis);
      }
    }
  }
  std::cout << "tot_symmetric bias = " << tot_bias << std::endl;
  return tot_bias > tol;
}

void Placement::update_hiernode(PnRDB::hierNode &current_node) {
  // update blocks
  Ppoint_I uni_cell_shape;
  uni_cell_shape.x = (int)(est_Size.x * uni_cell_Dpoint.x);
  uni_cell_shape.y = (int)(est_Size.y * uni_cell_Dpoint.y);
  for (int i = originalBlockCNT; i < Blocks.size(); ++i) {
    // add blockcomplex
    PnRDB::block tempBlock;
    tempBlock.name = Blocks[i].blockname;
    tempBlock.orient = PnRDB::N;
    tempBlock.height = uni_cell_shape.y;
    tempBlock.width = uni_cell_shape.x;
    tempBlock.originBox.LL.x = 0, tempBlock.originBox.LL.y = 0;
    tempBlock.originBox.UR.x = tempBlock.width, tempBlock.originBox.LL.y = tempBlock.height;

    PnRDB::blockComplex tempBlockComplex;
    tempBlockComplex.instNum = 1;
    tempBlockComplex.instance.push_back(tempBlock);
    current_node.Blocks.push_back(tempBlockComplex);
  }
  // update netlist
  for (int i = originalNetCNT; i < Nets.size(); ++i) {
    PnRDB::net tempNet;
    tempNet.weight = Nets[i].weight;
    for (int j = 0; j < Nets[i].connected_block.size(); ++j) {
      PnRDB::connectNode tempNode;
      tempNode.iter2 = Nets[i].connected_block[j];
      tempNode.type = PnRDB::Block;
      tempNode.iter = 0;
      tempNet.connected.push_back(tempNode);
    }
    current_node.Nets.push_back(tempNet);
  }
  // update ordering and alignment
  // set a flag?

  // update the size of block
  int idx = 0;
  for (int i = 0; i < current_node.Blocks.size(); ++i) {
    for (int j = 0; j < 1; ++j) {
      if (Blocks[idx].splited) {
        current_node.Blocks[i].instance[j].height /= int(ceil(Blocks[i].split_shape.y));
        current_node.Blocks[i].instance[j].width /= int(ceil(Blocks[i].split_shape.x));
        current_node.Blocks[i].instance[j].originBox.UR.y /= int(ceil(Blocks[i].split_shape.y));
        current_node.Blocks[i].instance[j].originBox.UR.x /= int(ceil(Blocks[i].split_shape.x));
        current_node.Blocks[i].instance[j].originCenter.y /= int(ceil(Blocks[i].split_shape.y));
        current_node.Blocks[i].instance[j].originCenter.x /= int(ceil(Blocks[i].split_shape.x));
      }
      ++idx;
    }
  }
  current_node.isFirstILP = true;
}

void Placement::split_net() {
  for (int i = 0; i < originalNetCNT; ++i) {
    vector<int> chosen_block;
    float weight = 1;
    for (int j = 0; j < Nets[i].connected_block.size(); ++j) {
      int id = Nets[i].connected_block[j];
      chosen_block.push_back(0);
      weight /= (float)(Blocks[id].spiltBlock.size() + 1);
    }
    int id0 = Nets[i].connected_block[0];
    Nets[i].weight = 0;
    while (chosen_block[0] <= Blocks[id0].spiltBlock.size()) {
      net tempNet;
      tempNet.index = Nets.size();
      for (int j = 0; j < Nets[i].connected_block.size(); ++j) {
        int id = Nets[i].connected_block[j];
        int chosenID;
        if (chosen_block[j] < Blocks[id].spiltBlock.size()) {
          chosenID = Blocks[id].spiltBlock[chosen_block[j]];
        } else {
          chosenID = id;
        }
        tempNet.connected_block.push_back(chosenID);
        Blocks[chosenID].connected_net.push_back(tempNet.index);
      }
      tempNet.weight = weight;
      Nets.push_back(tempNet);

      // update chosen block vector
      chosen_block[chosen_block.size() - 1] += 1;
      for (int j = Nets[i].connected_block.size() - 1; j >= 0; --j) {
        int id = Nets[i].connected_block[j];
        if (chosen_block[j] > Blocks[id].spiltBlock.size() and j > 0) {
          chosen_block[j] = 0;
          chosen_block[j - 1] += 1;
        }
      }
    }
  }
}

// void Placement::compact_cc()
// {
//   for(int i = 0;i < commonCentroids.size();++i)
//   {
//     Ppoint_F center;
//     for(int)
//   }
// }

void Placement::modify_symm_after_split(PnRDB::hierNode &current_node) {
  for (int i = 0; i < current_node.SPBlocks.size(); ++i) {
    // check all pair symmtirc blocks
    int pair_symm_num = current_node.SPBlocks[i].sympair.size();
    for (int j = 0; j < pair_symm_num; ++j) {
      int originid0 = current_node.SPBlocks[i].sympair[j].first;
      int originid1 = current_node.SPBlocks[i].sympair[j].second;

      Ppoint_F shape = Blocks[originid0].split_shape;
      int xlen = (int)ceil(shape.x);
      for (int k = 0; k < Blocks[originid0].spiltBlock.size(); ++k) {
        pair<int, int> temp;
        temp.first = Blocks[originid0].spiltBlock[k];
        temp.second = Blocks[originid1].spiltBlock[k];
        current_node.SPBlocks[i].sympair.push_back(temp);
      }
    }
    // check all self symmtric blocks
    int self_symm_num = current_node.SPBlocks[i].selfsym.size();
    PnRDB::Smark dir = current_node.SPBlocks[i].axis_dir;
    for (int j = 0; j < self_symm_num; ++j) {
      int id = current_node.SPBlocks[i].selfsym[j].first;
      for (int k = 0; k < Blocks[id].spiltBlock.size(); ++k) {
        pair<int, PnRDB::Smark> temp;
        temp.first = Blocks[id].spiltBlock[k];
        temp.second = dir;
        current_node.SPBlocks[i].selfsym.push_back(temp);
      }
    }
  }

  // modify the sym_matrix
  for (int i = 0; i < symmetric_force_matrix.size(); ++i) {
    while (symmetric_force_matrix[i].size() < Blocks.size()) {
      Ppoint_F temp;
      temp.x = 0;
      temp.y = 0;
      symmetric_force_matrix[i].push_back(temp);
    }
  }
  while (symmetric_force_matrix.size() < Blocks.size()) {
    vector<Ppoint_F> temp;
    for (int i = 0; i < Blocks.size(); ++i) {
      Ppoint_F temp_point;
      temp_point.x = 0;
      temp_point.y = 0;
      temp.push_back(temp_point);
    }
    symmetric_force_matrix.push_back(temp);
  }
}

float Placement::cal_weight_init_increase(float &rate, float &init_val, float &target_val, float target_converge_iter) {
  float qn = pow(rate, target_converge_iter);
  float a1 = target_val * (1 - rate) / (1 - qn);
  return a1;
}

void Placement::cal_dummy_net_weight(float &weight, float &rate, float &increase) {
  weight += increase;
  increase *= rate;
  std::cout << "dummy_net weight:= " << weight << std::endl;
}

void Placement::set_dummy_net_weight(float init_weight, float rate, float targe) { dummy_net_weight = init_weight; }

void Placement::break_merged_cc(PnRDB::hierNode &current_node) {
  update_pos(current_node);
  std::cout << "restore ms debug:0" << std::endl;

  string mark_of_cc = "CC_merge_cell";
  for (int i = 0; i < current_node.Blocks.size(); ++i) {
    std::cout << "restore ms debug:1" << std::endl;
    for (int j = 0; j < 1; ++j) {
      std::cout << "restore ms debug:2" << std::endl;
      if (current_node.Blocks[i].instance[j].name.find(mark_of_cc) != string::npos) {
        std::cout << "restore ms debug:3" << std::endl;
        // break the cc into its shape
        int pos = current_node.Blocks[i].instance[j].name.find(mark_of_cc);  // bug 1

        char ccID = current_node.Blocks[i].instance[j].name[pos + 15];  // bug 1
        int ccID_int = cc_name_to_id_map[current_node.Blocks[i].instance[j].name];
        // int ccID_int = ccID - 48 - 1;  // bug 2
        std::cout << "restore ms debug:4" << std::endl;
        // determine the period and center
        Ppoint_F center, period, LL;
        center.x = (float)current_node.Blocks[i].instance[j].placedCenter.x / est_Size.x;
        center.y = (float)current_node.Blocks[i].instance[j].placedCenter.y / est_Size.y;

        // LL.x = center.x - 1/2*(float)current_node.Blocks[i].instance[j].width/est_Size.x;
        // LL.y = center.y - 1/2*(float)current_node.Blocks[i].instance[j].height/est_Size.y;
        LL.x = (float)(current_node.Blocks[i].instance[j].placedCenter.x - current_node.Blocks[i].instance[j].width / 2) / est_Size.x;   // bug
        LL.y = (float)(current_node.Blocks[i].instance[j].placedCenter.y - current_node.Blocks[i].instance[j].height / 2) / est_Size.y;  // bug

        // LL.x = center.x - 1/2 *uni_cell.x * commonCentroids[ccID_int].shape.x;
        // LL.y = center.y - 1/2 *uni_cell.y * commonCentroids[ccID_int].shape.y;
        std::cout << "width of CC " << ccID_int << " =" << current_node.Blocks[i].instance[j].width << endl;    // bug
        std::cout << "height of CC " << ccID_int << " =" << current_node.Blocks[i].instance[j].height << endl;  // bug

        std::cout << "LL:=" << LL.x * est_Size.x << ", " << LL.y * est_Size.y << endl;
        // std::cout<<"width *0.5:= center -LL "<<current_node.Blocks[i].instance[j].placedCenter.x - <<", "<<center.y-LL.y<<endl;

        if (commonCentroids[ccID_int].shape.x > 1) {
          std::cout << "restore ms debug:5" << std::endl;
          period.x = (float)current_node.Blocks[i].instance[j].width / est_Size.x / (commonCentroids[ccID_int].shape.x);  // bug
          // period.x = 0;
        } else {
          period.x = 0;
        }
        if (commonCentroids[ccID_int].shape.y > 1) {
          std::cout << "restore ms debug:6" << std::endl;
          period.y = (float)current_node.Blocks[i].instance[j].height / est_Size.y / (commonCentroids[ccID_int].shape.y);  // bug
          // period.y = 0;
        } else {
          period.y = 0;
        }
        // period = uni_cell;
        std::cout << "restore ms debug:7:" << ccID_int << std::endl;
        // range the pos of each cell
        for (int ii = 0; ii < commonCentroids[ccID_int].shape.x; ++ii) {
          for (int jj = 0; jj < commonCentroids[ccID_int].shape.y; ++jj) {
            // if(commonCentroids[ccID_int].fillin_matrix[ii][jj]>=0)
            // {
            int id = commonCentroids[ccID_int].fillin_matrix[ii][jj];
            std::cout << "restore ms debug:7a:" << id << std::endl;
            Blocks[id].Cpoint.x = LL.x + ii * period.x + 0.5 * period.x;  // bug 3
            Blocks[id].Cpoint.y = LL.y + jj * period.y + 0.5 * period.y;
            ;
            Blocks[id].Dpoint.x = period.x;
            Blocks[id].Dpoint.y = period.y;
            // }
          }
        }
      }
    }
  }
  for (auto b : new_to_original_block_map) {
    current_node.Blocks[b.first].instance[0].name = current_node.Blocks[b.second].instance[0].name;
  }
  for (auto b : original_to_new_block_map) {
    current_node.Blocks[b.first].instance[0].originBox = current_node.Blocks[b.second].instance[0].originBox;
    current_node.Blocks[b.first].instance[0].originCenter = current_node.Blocks[b.second].instance[0].originCenter;
    current_node.Blocks[b.first].instance[0].placedBox = current_node.Blocks[b.second].instance[0].placedBox;
    current_node.Blocks[b.first].instance[0].placedCenter = current_node.Blocks[b.second].instance[0].placedCenter;
    current_node.Blocks[b.first].instance[0].orient = current_node.Blocks[b.second].instance[0].orient;
    current_node.Blocks[b.first].instance[0].PowerNets = current_node.Blocks[b.second].instance[0].PowerNets;
    current_node.Blocks[b.first].instance[0].blockPins = current_node.Blocks[b.second].instance[0].blockPins;
    current_node.Blocks[b.first].instance[0].interMetals = current_node.Blocks[b.second].instance[0].interMetals;
    current_node.Blocks[b.first].instance[0].interVias = current_node.Blocks[b.second].instance[0].interVias;
    current_node.Blocks[b.first].instance[0].dummy_power_pin = current_node.Blocks[b.second].instance[0].dummy_power_pin;
    current_node.Blocks[b.first].instance[0].GuardRings = current_node.Blocks[b.second].instance[0].GuardRings;
    current_node.Blocks[b.first].instance[0].width = current_node.Blocks[b.second].instance[0].width;
    current_node.Blocks[b.first].instance[0].height = current_node.Blocks[b.second].instance[0].height;
  }
  for (auto &n : current_node.Nets) {
    for (auto &c : n.connected) {
      if (c.type == PnRDB::Block && c.iter2 >= originalBlockCNT) {
        c.iter2 = new_to_original_block_map[c.iter2];
      }
    }
  }
  current_node.Blocks.erase(current_node.Blocks.begin() + originalBlockCNT, current_node.Blocks.end());
  /**for (auto b = current_node.Blocks.begin(); b != current_node.Blocks.end();) {
    if(b->instance[0].isRead==false){
      b = current_node.Blocks.erase(b);
    }else{
      ++b;
    }
  }**/
  PlotPlacement(604);
}

void Placement::update_pos(PnRDB::hierNode &current_node) {
  int idx = 0;

  for (int i = 0; i < current_node.Blocks.size(); ++i) {
    for (int j = 0; j < 1; ++j) {
      if (current_node.Blocks[i].instance[j].isRead) {
        Blocks[idx].Cpoint.x = (float)current_node.Blocks[i].instance[j].placedCenter.x / est_Size.x;
        Blocks[idx].Cpoint.y = (float)current_node.Blocks[i].instance[j].placedCenter.y / est_Size.y;
        std::cout << "update_pos: " << Blocks[idx].blockname << " Cpoint:=" << current_node.Blocks[i].instance[j].placedCenter.x << " "
                  << current_node.Blocks[i].instance[j].placedCenter.y << endl;
      }

      // Blocks[idx].Cpoint.x = 0;
      // Blocks[idx].Cpoint.y = 0;
      idx++;
      if (idx == originalBlockCNT)  // bug
      {
        return;  // bug
      }
    }
  }
}

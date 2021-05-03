#ifndef SEQPAIR_h_
#define SEQPAIR_h_

#include <set>
#include <vector>
#include <algorithm>
#include <chrono>
#include <random>
#include <functional>
#include <utility>
#include <string>
#include <iostream>
#include <memory>
#include <stdlib.h>     /* srand, rand */
#include "../PnRDB/readfile.h"
#include "Pdatatype.h"
#include "design.h"

using std::cerr;
using std::cout;
using std::endl;
using std::make_pair;
using std::pair;
using std::set;
using std::string;
using std::swap;
using std::vector;


class SeqPairEnumerator
{
  private:
    vector<int> _posPair, _negPair, _selected;
    std::pair<size_t, size_t> _enumIndex; //first : pos, second : neg
    vector<int> _maxSelected;
    size_t _maxEnum;
    int _maxSize;
    unsigned _exhausted : 1;
  public:
    SeqPairEnumerator(const vector<int>& pair, design& casenl);
    void Permute();
    const vector<int>& PosPair() const { return _posPair; }
    const vector<int>& NegPair() const { return _negPair; }
    const vector<int>& Selected() const { return _selected; }
    const bool EnumExhausted() const { return _exhausted; }
    const bool IncrementSelected();
};


class SeqPair 
{
  private:
    friend class ILP_solver;
    vector<int> posPair;
    vector<int> negPair;
    vector<placerDB::Omark> orient;
    vector<placerDB::Smark> symAxis;
    vector<int> selected, selectedOptTaps, selectedNoTaps;
    std::shared_ptr<SeqPairEnumerator> _seqPairEnum;
    vector<int> FindShortSeq(design& caseNL, vector<int>& seq, int idx);
    int GetVertexIndexinSeq(vector<int>& seq, int v);
    bool MoveAsymmetricBlockUnit(design& caseNL, vector<int>& seq, int anode);
    vector<int> GetVerticesIndexinSeq(vector<int>& seq, vector<int>& L);
    vector<int> SwapTwoListinSeq(vector<int>& Alist, vector<int>& Blist, vector<int>& seq);
    void InsertCommonSBlock(design& originNL, design& reducedNL, int originIdx);
    void InsertNewSBlock(design& originNL, int originIdx);

  public:
    SeqPair();
    SeqPair(int blockSize);
    SeqPair(string pos, string neg);
    SeqPair(const SeqPair& sp);
    SeqPair(design& caseNL, const size_t maxIter = 0);
    SeqPair& operator=(const SeqPair& sp);
    SeqPair(design& originNL, design& reducedNL, SeqPair& reducedSP);
    static size_t Factorial(const size_t& t);
    bool Enumerate() const { return _seqPairEnum ? true : false; }
    const bool EnumExhausted() const { return _seqPairEnum ? _seqPairEnum->EnumExhausted() : false; }
    vector<int> GetBlockIndex(int blockNo);
    vector<int> GetRightBlock(int blockNo);
    vector<int> GetLeftBlock(int blockNo);
    vector<int> GetAboveBlock(int blockNo);
    vector<int> GetBelowBlock(int blockNo);
    placerDB::Omark GetBlockOrient(int blockNo);
    void PrintSeqPair();
    void ChangeOrient(int blockNo, placerDB::Omark ort );
    void FlipOrient(int blockNo);
    void AdjustOrient(int blockNo, placerDB::Omark ort);
    void SwitchSingleSequence(int b1, int b2, bool flag);
    void SwitchDoubleSequence(int b1, int b2);
    bool FastInitialScan(design& caseNL);
    placerDB::Smark GetSymmBlockAxis(int SBidx);
    bool MoveAsymmetricBlockposPair(design& caseNL);
    bool MoveAsymmetricBlocknegPair(design& caseNL);
    bool MoveAsymmetricBlockdoublePair(design& caseNL);
    bool SwapTwoBlocksofSameGroup(design& caseNL);
    bool SwapMultiBlocksofSameGroup(design& caseNL);
    bool SwapTwoSymmetryGroup(design& caseNL);
    bool ChangeAsymmetricBlockOrient(design& caseNL);
    bool RotateSymmetryGroup(design& caseNL);
    bool ChangeSymmetryGroupOrient(design& caseNL);
    bool ChangeSymmetryBlockOrient(design& caseNL);
    void Perturbation(design& caseNL);
    void PerturbationNew(design& caseNL);
    void TestSwap();
    int GetBlockSelected(int blockNo);
    bool ChangeSelectedBlock(design& caseNL);
    void KeepOrdering(design& caseNL);
    void BackupSelected(const bool opt = true) 
    {
      if (opt) selectedOptTaps = selected;
      else selectedNoTaps = selected;
    }
    void RestoreSelected(const bool opt = true)
    {
      if (opt) {
        if (!selectedOptTaps.empty()) std::swap(selected, selectedOptTaps);
      } else {
        if (!selectedNoTaps.empty()) std::swap(selected, selectedNoTaps);
      }
    }
};

#endif

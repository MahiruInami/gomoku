#ifndef MCTSTREE_H
#define MCTSTREE_H

#include "mctsnode.h"
#include "bitfield.h"
#include "common.h"

class MCTSTree
{
public:
    MCTSTree(short evalColor);

    void selectChild(short x, short y);
    void update(const BitField* const rootState);

    std::vector<AIMoveData> getNodesData() const;
    std::vector<AIMoveData> getBestPlayout(short x, short y) const;
    unsigned getTotalPlayouts() const { return _root ? _root->getRealPlayouts() : 0; }

    unsigned getThreadsCount() const { return _maxTreads; }
private:
    void expand(MCTSNode* root, const BitField* const rootState);
    float playout(const BitField* const rootState, short rootColor);
    void explore(MCTSNode* root, const BitField* const rootState);

    MCTSNode* selectBestChild(MCTSNode* root, const BitField* const rootState) const;
private:
    MCTSNode* _root;
    BitField* _currentState;

    short _evalColor = 0;
    unsigned _maxTreads = 1;

    std::array<unsigned, BOARD_LENGTH> _nodePlayouts;
public:
    static unsigned NODE_EXPLORATIONS_TO_EXPAND;
};

#endif // MCTSTREE_H

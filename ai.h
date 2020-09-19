#pragma once

#include <memory>
#include <vector>
#include <list>
#include "mctnode.h"
#include "field.h"
#include "aidomainknowledge.h"

class AI
{
public:
    enum class NodePruneStrategy {
        NONE,
        HARD,
        SOFT
    };
    static constexpr unsigned NODE_VISIT_FOR_SAFE_DELETE = 8;

    AI(short aiPlayerColor);
    ~AI();

    void update();
    void goToNode(short position);

    void setPruneStrategy(NodePruneStrategy strategy) { NODE_PRUNE_STRATEGY = strategy; }
    bool goTreeForward(short position);
    bool goBack();

    short getPlayerColor() const { return _playerColor; }

    unsigned getRootPlayoursCount() const { return _root->getNodeExplorations(); }
    AIMoveData getBestNodePosition() const;
    std::vector<AIMoveData> getPossibleMoves() const;

    Field* getBoardState() const { return _board; }
private:
    void expand(MCTNode* node, IField* field, AIDomainKnowledge* domainKnowledge);
    void select(MCTNode* node, IField* field, AIDomainKnowledge* domainKnowledge);
    void explore(MCTNode* node, IField* field, AIDomainKnowledge* domainKnowledge);
    void simulatePlayout(MCTNode* node, IField* field, AIDomainKnowledge* domainKnowledge);

    bool partialPrune(MCTNode* node);
    void _partialPrune(MCTNode* node);
private:
    MCTNode* _root;
    MCTNode* _realRoot;
    Field* _board;
    AIDomainKnowledge* _domainKnowledge;

    std::list<MCTNode*> _nodesToPrune;
    std::list<MCTNode*> _partialPruneQueue;
    int _nodesToPartialPrune = 0;
    unsigned _nodesPruned = 0;

    short _playerColor;

    NodePruneStrategy NODE_PRUNE_STRATEGY = NodePruneStrategy::SOFT;
};


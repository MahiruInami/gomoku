#pragma once

#include <memory>
#include <vector>
#include <list>
#include "mctnode.h"
#include "field.h"
#include "aidomainknowledge.h"

struct AIMoveData {
    short position;
    short x;
    short y;
    short color;

    float scores;
    float minMaxScores;

    unsigned nodeVisits;
};

class AI
{
public:
    AI(short aiPlayerColor);
    ~AI();

    void update();
    void goToNode(short position);

    AIMoveData getBestNodePosition() const;
    std::vector<AIMoveData> getPossibleMoves() const;
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
};


#pragma once

#include <memory>
#include <vector>
#include "field.h"

struct AIMoveData {
    short position;
    short x;
    short y;
    short color;

    float scores;
    float minMaxScores;

    unsigned nodeVisits;
};

class MCTNode {
public:
    MCTNode();
    ~MCTNode();

    bool isExpanded() const { return _isExpanded; }
    void setExpanded(bool value) { _isExpanded = value; }

    const FieldMove& getMove() const { return _move; }

    void setPosition(short position) { _move.position = position; }
    short getPosition() const { return _move.position; }

    void setColor(short color) { _move.color = color; }
    short getColor() const { return _move.color; }

    void setParent(MCTNode* parent) { _parent = parent; }
    MCTNode* getParent() const { return _parent; }

    void addChild(MCTNode* child);
    bool hasChild(short hashedMove) const;
    MCTNode* getChild(short hashedMove) const;
    const std::vector<MCTNode*>& getChildren() { return _children; }
    void clearChildren() { _children.clear(); _hashedChildren.clear(); }

    void setExplored(bool value) { _isExplored = value; }
    bool isExplored() const { return _isExplored; }

    void setEndpoint(bool value) { _isEndpoint = value; }
    bool isEndpoint() const { return _isEndpoint; }

    void setTerminal(bool value) { _isTerminal = value; }
    bool isTerminal() const { return _isTerminal; }

    void backpropagate(float scores);

    void setMinMaxScore(float value) { _minMaxScore = value; }
    float getMinMaxScore() const { return _minMaxScore; }

    void setScores(float value) { _scores = value; }
    float getScores() const { return _scores; }

    unsigned getNodeVisits() const { return _nodeVisits; }

    float getNodeSelectionScore() const;
    float getNodeEvaluationScore() const;
private:
    FieldMove _move;

    bool _isExpanded = false;
    bool _isExplored = false;
    bool _isEndpoint = false;
    bool _isTerminal = false;

    bool _isWinningNode = false;

    float _scores = 0.f;
    float _minMaxScore = 0.f;

    unsigned _nodeVisits = 0;

    MCTNode* _parent = nullptr;
    std::vector<MCTNode*> _children;
    std::unordered_set<short> _hashedChildren;
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
    void expand(MCTNode* node, Field* field);
    void select(MCTNode* node, Field* field);
    void explore(MCTNode* node, Field* field);
    void simulatePlayout(MCTNode* node, Field* field);
private:
    MCTNode* _root;
    MCTNode* _realRoot;
    Field* _board;

    short _playerColor;
};

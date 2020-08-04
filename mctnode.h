#pragma once

#include "IField.h"

class MCTNode {
public:
    friend class AI;
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
    std::vector<MCTNode*>& getChildren() { return _children; }
    void clearChildren() { _children.clear(); _hashedChildren.clear(); }

    void setExplored(bool value) { _isExplored = value; }
    bool isExplored() const { return _isExplored; }

    void setEndpoint(bool value) { _isEndpoint = value; }
    bool isEndpoint() const { return _isEndpoint; }

    void setTerminal(bool value) { _isTerminal = value; }
    bool isTerminal() const { return _isTerminal; }

    void addNodeExploration() { _nodeExplorations++; }
    void backpropagate(float scores, MCTNode* child);

    void setScores(float value) { _scores = value; }
    float getScores() const { return _scores; }

    unsigned getNodeVisits() const { return _nodeVisits; }
    unsigned getNodeExplorations() const { return _nodeExplorations; }

    float getNodeSelectionScore(short playingColor) const;
    float getNodeEvaluationScore(short playingColor) const;

    IField* getDebugState() { return _debugState; }
private:
    FieldMove _move;

    bool _isExpanded = false;
    bool _isExplored = false;
    bool _isEndpoint = false;
    bool _isTerminal = false;

    float _scores = 0.f;
    float _tacticalScore = 0.f;

    unsigned _nodeVisits = 0;
    unsigned _nodeExplorations = 0;

    MCTNode* _parent = nullptr;
    std::vector<MCTNode*> _children;
    std::unordered_set<short> _hashedChildren;

    IField* _debugState = nullptr;
};

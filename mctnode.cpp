#include "mctnode.h"
#include <math.h>

MCTNode::MCTNode() {

}

MCTNode::~MCTNode() {
    _parent = nullptr;
    _hashedChildren.clear();

    for (auto& child : _children) {
        delete child;
    }
    _children.clear();
}

void MCTNode::addChild(MCTNode *child) {
    if (hasChild(child->getPosition())) {
        return;
    }

    child->setParent(this);
    _children.push_back(child);
    _hashedChildren.insert(child->getPosition());
}

bool MCTNode::hasChild(short hashedMove) const {
    return _hashedChildren.find(hashedMove) != _hashedChildren.end();
}

MCTNode* MCTNode::getChild(short hashedMove) const {
    for (auto& child : _children) {
        if (child->getPosition() == hashedMove) {
            return child;
        }
    }

    return nullptr;
}

float MCTNode::getNodeSelectionScore(short playingColor) const {
    float visits = _nodeExplorations > 0 ? _nodeExplorations : 1.f;

    float visitRatio = _parent && _parent->getNodeExplorations() > 0 ? std::sqrt(std::log(_parent->getNodeExplorations()) / visits) * 1.5f : 0.f;
    float score = getNodeEvaluationScore(playingColor);
    return score + visitRatio;
}


float MCTNode::getNodeEvaluationScore(short playingColor) const {
    if (_nodeVisits == 0) {
        return 0.f;
    }

    float score = _scores / _nodeVisits;
    if (getColor() != playingColor) { score = -score; }
    return score;
}

void MCTNode::backpropagate(float scores, MCTNode* /*child*/) {
    _scores += scores;
    scores += _tacticalScore;
    _tacticalScore = 0.f;

    _nodeExplorations++;
    _nodeVisits++;

    if (_parent) {
        _parent->backpropagate(scores, this);
    }
}

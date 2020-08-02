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

float MCTNode::getNodeSelectionScore() const {
    float visits = _nodeVisits > 0 ? _nodeVisits : 1.f;

    float visitRatio = _parent ? std::sqrt(std::log2(_parent->getNodeVisits()) / visits) : 0.f;
    return _scores / visits + visitRatio;
}


float MCTNode::getNodeEvaluationScore() const {
    if (_nodeVisits == 0) {
        return 0.f;
    }

    return _scores / _nodeVisits;
}

void MCTNode::backpropagate(float scores, MCTNode* child) {
    _scores += scores;
    scores += _tacticalScore;
    _tacticalScore = 0.f;

    if (child == nullptr || _nodeVisits == 0) {
        _minMaxScore = scores;
    } else {
        if (getColor() == BLACK_PIECE_COLOR) _minMaxScore = -1.f;
        else _minMaxScore = 1.f;

        for (auto& ch : _children) {
            if (getColor() == BLACK_PIECE_COLOR) _minMaxScore = std::max(_minMaxScore, ch->getMinMaxScore());
            else _minMaxScore = std::min(_minMaxScore, ch->getMinMaxScore());
        }
    }

    _nodeVisits++;

    if (_parent) {
        _parent->backpropagate(scores, this);
    }
}

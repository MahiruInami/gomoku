#include "ai.h"
#include <algorithm>
#include <math.h>
#include "debug.h"

#include <QDebug>
#include <QElapsedTimer>

enum class NodePruneStrategy {
    NONE,
    HARD,
    SOFT
} NODE_PRUNE_STRATEGY = NodePruneStrategy::SOFT;
constexpr unsigned NODE_VISIT_FOR_SAFE_DELETE = 8;

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
    if (_nodeVisits == 0) {
        return 0.f;
    }

    float visitRatio = _parent ? std::sqrt(std::log(_parent->getNodeVisits()) / _nodeVisits) * 1.4142135f : 0.f;
    return _scores / _nodeVisits + visitRatio;
}


float MCTNode::getNodeEvaluationScore() const {
    if (_nodeVisits == 0) {
        return 0.f;
    }

    return _scores / _nodeVisits;
}

void MCTNode::backpropagate(float scores) {
    _scores += scores;
    scores += _tacticalScore;
    _tacticalScore = 0.f;
    _nodeVisits++;

    if (getColor() == BLACK_PIECE_COLOR) {
        _minMaxScore = std::max(_minMaxScore, scores);
    } else {
        _minMaxScore = std::min(_minMaxScore, scores);
    }

    if (_parent) {
        _parent->backpropagate(scores);
    }
}

AI::AI(short aiPlayerColor)
    : _playerColor(aiPlayerColor)
{
    _board = new Field();
    _board->clear();

    _realRoot = new MCTNode();
    _root = _realRoot;
    _root->setParent(nullptr);

    if (NODE_PRUNE_STRATEGY != NodePruneStrategy::NONE) {
        _realRoot = nullptr;
    }
}

AI::~AI() {
    if (NODE_PRUNE_STRATEGY == NodePruneStrategy::NONE) {
        _root = nullptr;
        delete _realRoot;
    } else if (NODE_PRUNE_STRATEGY == NodePruneStrategy::HARD) {
        delete _root;
        _root = nullptr;
    } else if (NODE_PRUNE_STRATEGY == NodePruneStrategy::SOFT) {
        delete _root;
        _root = nullptr;

        for (auto& node : _nodesToPrune) {
            delete node;
        }
        _nodesToPrune.clear();
    }

    delete _board;
}

AIMoveData AI::getBestNodePosition() const {
    MCTNode* bestNode = nullptr;
    float bestNodeScores = -2.;
    for (auto& child : _root->getChildren()) {
        float nodeScores = child->getNodeEvaluationScore();
        if (!bestNode || bestNodeScores < nodeScores) {
            bestNode = child;
            bestNodeScores = nodeScores;
        }
    }

    if (!bestNode) {
        return {};
    }

    AIMoveData moveData;
    moveData.position = bestNode->getPosition();
    moveData.scores = bestNode->getNodeEvaluationScore();
    moveData.color = bestNode->getColor();
    moveData.nodeVisits = bestNode->getNodeVisits();
    moveData.minMaxScores = bestNode->getMinMaxScore();

    return moveData;
}

std::vector<AIMoveData> AI::getPossibleMoves() const {
    std::vector<AIMoveData> result;
    for (auto& child : _root->getChildren()) {
        AIMoveData moveData;
        moveData.position = child->getPosition();
        moveData.scores = child->getNodeEvaluationScore();
        moveData.color = child->getColor();
        moveData.nodeVisits = child->getNodeVisits();
        moveData.minMaxScores = child->getMinMaxScore();

        result.push_back(moveData);
    }

    return result;
}

void AI::_partialPrune(MCTNode *node) {
    if (_nodesToPartialPrune <= 0) {
        return;
    }

    for (auto childrenIt = node->getChildren().begin(); childrenIt != node->getChildren().end();) {
        if ((*childrenIt)->getNodeVisits() < static_cast<unsigned>(_nodesToPartialPrune) || (*childrenIt)->getChildren().empty()) {
            auto nodeForDelete = (*childrenIt);
            childrenIt = node->getChildren().erase(childrenIt);

            _nodesToPartialPrune -= nodeForDelete->getNodeVisits();
            _nodesPruned += nodeForDelete->getNodeVisits();
            auto parent = nodeForDelete->getParent();
            while (parent) { parent->_nodeVisits -= nodeForDelete->getNodeVisits(); parent = parent->getParent(); }

            nodeForDelete->setParent(nullptr);
            delete nodeForDelete;
        } else {
            auto nodeToCheck = *childrenIt;
            ++childrenIt;
            _partialPrune(nodeToCheck);
        }
    }
}

bool AI::partialPrune(MCTNode* node) {
    _partialPrune(node);
    return node->getNodeVisits() == 0 || node->getChildren().empty();
}

void AI::update() {
    if (NODE_PRUNE_STRATEGY == NodePruneStrategy::SOFT && !_nodesToPrune.empty()) {
        _nodesToPartialPrune = NODE_VISIT_FOR_SAFE_DELETE;
        _nodesPruned = 0;

        auto node = _nodesToPrune.front();
        if (node->getNodeVisits() < NODE_VISIT_FOR_SAFE_DELETE || node->getChildren().empty()) {
            _nodesToPrune.pop_front();
            _nodesToPartialPrune -= node->getNodeVisits();
            _nodesPruned += node->getNodeVisits();
            node->setParent(nullptr);
            delete node;
        } else if (partialPrune(node)) {
            _nodesToPrune.pop_front();
            node->setParent(nullptr);
            delete  node;
        }

        qDebug() << "Partial prune: " << _nodesToPartialPrune << _nodesPruned;
    }

    if (_board->getFieldStatus() != FieldStatus::IN_PROGRESS) {
        return;
    }

    Debug::getInstance().startTrack(DebugTimeTracks::AI_UPDATE);

    Field* field = new Field(*_board);

    if (!_root->isExpanded()) {
        expand(_root, field);
    }

    select(_root, field);
    delete field;

    Debug::getInstance().stopTrack(DebugTimeTracks::AI_UPDATE);
}

void AI::goToNode(short position) {
    if (_root->hasChild(position)) {
        _root = _root->getChild(position);
        _board->placePiece(_root->getMove());

        if (NODE_PRUNE_STRATEGY == NodePruneStrategy::HARD) {
            auto parent = _root->getParent();
            _root->setParent(nullptr);
            auto nodes = parent->getChildren();
            parent->clearChildren();
            delete parent;
            for (auto& node : nodes) {
                if (node == _root) {
                    continue;
                }

                delete node;
            }
        } else if (NODE_PRUNE_STRATEGY == NodePruneStrategy::SOFT) {
            auto parent = _root->getParent();
            _root->setParent(nullptr);
            auto nodes = parent->getChildren();
            parent->clearChildren();

            for (auto& node : nodes) {
                if (node == _root) {
                    continue;
                }

                _nodesToPrune.push_back(node);
            }
        }
        return;
    }

    MCTNode* child = new MCTNode();
    child->setPosition(position);
    child->setColor(FieldMove::getNextColor(_root->getColor()));

    if (NODE_PRUNE_STRATEGY == NodePruneStrategy::HARD) {
        delete _root;
        _root = child;
    } else if (NODE_PRUNE_STRATEGY == NodePruneStrategy::SOFT) {
        _nodesToPrune.push_back(_root);
        _root = child;
    } else {
        _root->addChild(child);
        _root = child;
    }

    _board->placePiece(child->getMove());
}

void AI::expand(MCTNode* node, Field* field) {
    auto& moves = field->getAvailableMoves();

    for (auto& move : moves) {
        if (node->hasChild(move)) {
            continue;
        }

        MCTNode* child = new MCTNode();
        child->setPosition(move);
        child->setColor(FieldMove::getNextColor(node->getColor()));

        node->addChild(child);
    }

    node->setExpanded(true);
}

void AI::select(MCTNode* node, Field* field) {
    // select best node
    MCTNode* bestNode = nullptr;
    float bestNodeScores = 0.f;


    bool isAllTerminal = true;
    for (auto& child : node->getChildren()) {
        if (child->isTerminal()) {
            continue;
        }

        isAllTerminal = false;
        float nodeSelectionScores = child->getNodeSelectionScore();
        if (!bestNode || bestNodeScores < nodeSelectionScores) {
            bestNode = child;
            bestNodeScores = nodeSelectionScores;
        }
    }

    node->setExplored(true);
    if (isAllTerminal) {
        node->setTerminal(true);
        return;
    }

    explore(bestNode, field);
}

void AI::explore(MCTNode* node, Field* field) {
    field->placePiece(node->getMove());
    node->setExplored(true);

    if (field->getFieldStatus() != FieldStatus::IN_PROGRESS) {
        // game ended
        node->setEndpoint(true);
        node->setTerminal(true);

        return;
    }

    auto& moves = field->getAvailableMoves();
    auto rIndex = rand() % moves.size();
    auto moveItr = moves.begin();
    while (rIndex > 0) { moveItr++; rIndex--; }
    auto rMove = *moveItr;

    if (node->getChildren().size() == moves.size()) {
        // check if we explored all nodes
        bool isAllTerminal = true;
        for (auto& move : moves) {
            if (!node->hasChild(move) || !node->getChild(move)->isTerminal()) {
                isAllTerminal = false;
                break;
            }
        }

        if (isAllTerminal) {
            node->setTerminal(true);

            return;
        }
    }

    if (node->hasChild(rMove) && node->getChild(rMove)->isTerminal()) {
        for (auto& move : moves) {
            if (!node->hasChild(move) || !node->getChild(move)->isTerminal()) {
                rMove = move;
                break;
            }
        }
    }

    if (node->hasChild(rMove)) {
        explore(node->getChild(rMove), field);
        return;
    }

    MCTNode* child = new MCTNode();
    child->setPosition(rMove);
    child->setColor(FieldMove::getNextColor(node->getColor()));

    node->addChild(child);
    simulatePlayout(child, field);
}

void AI::simulatePlayout(MCTNode* node, Field* field) {
    field->placePiece(node->getMove());
    node->setExplored(true);

    if (field->getFieldStatus() != FieldStatus::IN_PROGRESS) {
        // game ended
        node->setEndpoint(true);
        node->setTerminal(true);
        short winningPlayerColor = static_cast<short>(field->getFieldStatus());
        float scores = 0.f;
        if (winningPlayerColor == _playerColor) {
            scores = 1.f;
        } else if (field->getFieldStatus() != FieldStatus::DRAW) {
            scores = -1.f;
        }

        node->backpropagate(scores);
        return;
    }


    auto& moves = field->getAvailableMoves();
    auto rIndex = rand() % moves.size(); // not _really_ random
    auto moveItr = moves.begin();
    while (rIndex > 0) { moveItr++; rIndex--; }

    auto rMove = *moveItr;

    if (node->hasChild(rMove) && node->getChild(rMove)->isTerminal()) {
        for (auto& move : moves) {
            if (!node->hasChild(move) || !node->getChild(move)->isTerminal()) {
                rMove = move;
                break;
            }
        }
    }

    if (node->hasChild(rMove)) {
        explore(node->getChild(rMove), field);
        return;
    }

    MCTNode* child = new MCTNode();
    child->setPosition(rMove);
    child->setColor(FieldMove::getNextColor(node->getColor()));

    node->addChild(child);
    simulatePlayout(child, field);
}

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
    }

    if (_board->getFieldStatus() != FieldStatus::IN_PROGRESS) {
        return;
    }

    Debug::getInstance().startTrack(DebugTimeTracks::AI_UPDATE);

    Field* field = new Field(*_board);
    AIDomainKnowledge* domainKnowledge = new AIDomainKnowledge(*_domainKnowledge);

    if (!_root->isExpanded()) {
        expand(_root, field, domainKnowledge);
    }

    select(_root, field, domainKnowledge);
    delete field;
    delete domainKnowledge;

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

void AI::expand(MCTNode* node, IField* field, AIDomainKnowledge* /*domainKnowledge*/) {
    auto& moves = field->getAvailableMoves(FieldMove::getNextColor(node->getColor()));

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

void AI::select(MCTNode* node, IField* field, AIDomainKnowledge* domainKnowledge) {
    // select best node
    MCTNode* bestNode = nullptr;
    float bestNodeScores = 0.f;


    bool isAllTerminal = true;
//    qDebug() << "Select: " << node->getColor() << "xy: " << FieldMove::getXFromPosition(node->getPosition()) << " " << FieldMove::getYFromPosition(node->getPosition());
    for (auto& child : node->getChildren()) {
        if (child->isTerminal()) {
//            qDebug() << "Skip terminal node: " << child->getColor() << "xy: " << FieldMove::getXFromPosition(child->getPosition()) << " " << FieldMove::getYFromPosition(child->getPosition()) << " " << child->getNodeSelectionScore() << " " << child->getNodeEvaluationScore();
            continue;
        }

        isAllTerminal = false;
        float nodeSelectionScores = child->getNodeSelectionScore();
//        qDebug() << "check Node: " << child->getColor() << "xy: " << FieldMove::getXFromPosition(child->getPosition()) << " " << FieldMove::getYFromPosition(child->getPosition()) << " " << nodeSelectionScores << " " << child->getNodeEvaluationScore();
//        if (!bestNode || (bestNodeScores < nodeSelectionScores && node->getColor() != _playerColor) || (bestNodeScores > nodeSelectionScores && node->getColor() == _playerColor)) {
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

//    qDebug() << "Best node: " << bestNode->getColor() << "xy: " << FieldMove::getXFromPosition(bestNode->getPosition()) << " " << FieldMove::getYFromPosition(bestNode->getPosition()) << " " << bestNode->getNodeSelectionScore() << " " << bestNode->getNodeEvaluationScore();
    explore(bestNode, field, domainKnowledge);
}

void AI::explore(MCTNode* node, IField* field, AIDomainKnowledge* domainKnowledge) {
    field->placePiece(node->getMove());
    node->setExplored(true);

    if (field->getFieldStatus() != FieldStatus::IN_PROGRESS) {
        // game ended
        node->setEndpoint(true);
        node->setTerminal(true);

        return;
    }

//    int pickExisting = rand() % 10;
//    if (node->isExpanded() || (node->getChildren().size() > 2 && pickExisting < 6)) {
//        select(node, field);
//        return;
//    }

    auto& moves = field->getAvailableMoves(FieldMove::getNextColor(node->getColor()));
    if (node->getChildren().size() == moves.size()) {
        // check if we explored all nodes
        bool isAllTerminal = true;
        bool isExpanded = true;
        for (auto& move : moves) {
            if (!node->hasChild(move)) {
                isExpanded = false;
                isAllTerminal = false;
                break;
            }

            if (!node->getChild(move)->isTerminal()) {
                isAllTerminal = false;
            }
        }

        if (isAllTerminal) {
            node->setTerminal(true);
            return;
        }

        if (isExpanded) {
            node->setExpanded(true);
            select(node, field, domainKnowledge);
            return;
        }
    }

    auto rIndex = rand() % moves.size();
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
        explore(node->getChild(rMove), field, domainKnowledge);
        return;
    }

    MCTNode* child = new MCTNode();
    child->setPosition(rMove);
    child->setColor(FieldMove::getNextColor(node->getColor()));

    node->addChild(child);
    simulatePlayout(child, field, domainKnowledge);
}

void AI::simulatePlayout(MCTNode* node, IField* field, AIDomainKnowledge* domainKnowledge) {
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

        node->backpropagate(scores, nullptr);
        return;
    }

    //


    auto& moves = field->getAvailableMoves(FieldMove::getNextColor(node->getColor()));
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
        explore(node->getChild(rMove), field, domainKnowledge);
        return;
    }

    MCTNode* child = new MCTNode();
    child->setPosition(rMove);
    child->setColor(FieldMove::getNextColor(node->getColor()));

    node->addChild(child);
    simulatePlayout(child, field, domainKnowledge);
}

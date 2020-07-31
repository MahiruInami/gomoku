#include "ai.h"
#include <algorithm>
#include <math.h>
#include "debug.h"

#include <QDebug>
#include <QElapsedTimer>

#define PRUNE_HANGING_NODES 1

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
    QElapsedTimer timerMain;
    timerMain.start();
    for (auto& child : _children) {
        if (child->getPosition() == hashedMove) {
            Debug::getInstance().getChildTime += timerMain.nsecsElapsed();
            return child;
        }
    }

    Debug::getInstance().getChildTime += timerMain.nsecsElapsed();
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
    _nodeVisits++;

    if (getColor() == BLACK_PIECE_COLOR) {
        _minMaxScore = std::max(_minMaxScore, scores);
    } else {
        _minMaxScore = std::min(_minMaxScore, scores);
    }

    if (_parent) {
        _parent->backpropagate(_scores);
    }
}

AI::AI(short aiPlayerColor)
    : _playerColor(aiPlayerColor)
{
    _board = new Field();
    _board->clear();

    _realRoot = new MCTNode();
    _root = _realRoot;
}

AI::~AI() {
    _root = nullptr;
    delete _realRoot;
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

void AI::update() {
    if (_board->getFieldStatus() != FieldStatus::IN_PROGRESS) {
        return;
    }

    QElapsedTimer timerMain;
    timerMain.start();

    QElapsedTimer timerSelect;
    timerSelect.start();
    Field* field = new Field(*_board);

    if (!_root->isExpanded()) {
        expand(_root, field);
    }

    select(_root, field);
    delete field;

    qDebug() << "Update time: " << timerMain.elapsed();
}

void AI::goToNode(short position) {
    if (_root->hasChild(position)) {
        _root = _root->getChild(position);
        _realRoot = _root;
        _board->placePiece(_root->getMove());

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
        return;
    }

    MCTNode* child = new MCTNode();
    child->setPosition(position);
    child->setColor(FieldMove::getNextColor(_root->getColor()));

    delete _root;
//    _root->addChild(child);

    _root = child;
    _realRoot = _root;
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

    Debug::getInstance().selectVisits++;

    // select best node
    QElapsedTimer timerMain;
    timerMain.start();

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

    Debug::getInstance().selectTime += timerMain.nsecsElapsed();
    node->setExplored(true);
    if (isAllTerminal) {
        node->setTerminal(true);
        return;
    }

    explore(bestNode, field);
}

void AI::explore(MCTNode* node, Field* field) {

    Debug::getInstance().exploreVisits++;

    QElapsedTimer timerMain1;
    timerMain1.start();

    field->placePiece(node->getMove());
    node->setExplored(true);

    if (field->getFieldStatus() != FieldStatus::IN_PROGRESS) {
        // game ended
        node->setEndpoint(true);
        node->setTerminal(true);

        Debug::getInstance().exploreTime += timerMain1.nsecsElapsed();
        return;
    }

    QElapsedTimer timerMain;
    timerMain.start();

    auto& moves = field->getAvailableMoves();
    auto rIndex = rand() % moves.size();
    auto moveItr = moves.begin();
    while (rIndex > 0) { moveItr++; rIndex--; }
    auto rMove = *moveItr;

    Debug::getInstance().randomMoveTime += timerMain.nsecsElapsed();

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

            Debug::getInstance().exploreTime += timerMain1.nsecsElapsed();
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
        Debug::getInstance().exploreTime += timerMain1.nsecsElapsed();

        explore(node->getChild(rMove), field);
        return;
    }

    MCTNode* child = new MCTNode();
    child->setPosition(rMove);
    child->setColor(FieldMove::getNextColor(node->getColor()));

    node->addChild(child);

    Debug::getInstance().exploreTime += timerMain1.nsecsElapsed();
    simulatePlayout(child, field);
}

void AI::simulatePlayout(MCTNode* node, Field* field) {

    Debug::getInstance().simulationVisits++;

    QElapsedTimer timerMain1;
    timerMain1.start();

    QElapsedTimer timerMain2;
    timerMain2.start();

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

        QElapsedTimer timerMaine;
        timerMaine.start();

        node->backpropagate(scores);

        Debug::getInstance().simulationTime += timerMain1.nsecsElapsed();
        Debug::getInstance().simulationBoardTime += timerMain2.nsecsElapsed();
        Debug::getInstance().backpropagateTime += timerMaine.nsecsElapsed();

        return;
    }

    Debug::getInstance().simulationBoardTime += timerMain1.nsecsElapsed();

    QElapsedTimer timerMain;
    timerMain.start();

    auto& moves = field->getAvailableMoves();
    auto rIndex = rand() % moves.size(); // not _really_ random
    auto moveItr = moves.begin();
    while (rIndex > 0) { moveItr++; rIndex--; }

    auto rMove = *moveItr;

    Debug::getInstance().randomMoveTime += timerMain.elapsed();

    if (node->hasChild(rMove) && node->getChild(rMove)->isTerminal()) {
        for (auto& move : moves) {
            if (!node->hasChild(move) || !node->getChild(move)->isTerminal()) {
                rMove = move;
                break;
            }
        }
    }

    if (node->hasChild(rMove)) {
        Debug::getInstance().simulationTime += timerMain1.nsecsElapsed();

        explore(node->getChild(rMove), field);
        return;
    }

    MCTNode* child = new MCTNode();
    child->setPosition(rMove);
    child->setColor(FieldMove::getNextColor(node->getColor()));

    node->addChild(child);

    Debug::getInstance().simulationTime += timerMain1.nsecsElapsed();
    simulatePlayout(child, field);
}

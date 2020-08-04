#include "ai.h"
#include <algorithm>
#include <math.h>
#include "debug.h"

#include <QDebug>
#include <QElapsedTimer>

constexpr int CHANCE_TO_DROP_DEVELOPMENT_ATTACK = 70;
constexpr int CHANCE_TO_DROP_DEVELOPMENT_DEFENCE = 80;

AI::AI(short aiPlayerColor)
    : _playerColor(aiPlayerColor)
{
    _board = new Field();
    _board->clear();

    _domainKnowledge = new AIDomainKnowledge();

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
        float nodeScores = child->getNodeEvaluationScore(_playerColor);
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
    moveData.scores = bestNode->getNodeEvaluationScore(_playerColor);
    moveData.color = bestNode->getColor();
    moveData.nodeVisits = bestNode->getNodeVisits();
    moveData.selectionScore = bestNode->getNodeSelectionScore(_playerColor);

    return moveData;
}

std::vector<AIMoveData> AI::getPossibleMoves() const {
    std::vector<AIMoveData> result;
    for (auto& child : _root->getChildren()) {
        AIMoveData moveData;
        moveData.position = child->getPosition();
        moveData.scores = child->getNodeEvaluationScore(_playerColor);
        moveData.color = child->getColor();
        moveData.nodeVisits = child->getNodeExplorations();
        moveData.selectionScore = child->getNodeSelectionScore(_playerColor);

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

bool AI::goTreeForward(short position) {
    if (_root->hasChild(position)) {
        if (!_root->_debugState) _root->_debugState = new Field(*_board);

        _root = _root->getChild(position);
        _board->placePiece(_root->getMove());
        AIDomainKnowledge::updateDomainKnowledge(_domainKnowledge, _root->getMove().position);

        short currentColor = FieldMove::getNextColor(_root->getColor());
        if (_board->getMoves().size() >= 2) {
            short lastMove = *(++_board->getMoves().rbegin());
            AIDomainKnowledge::generateAttackMoves(_board, lastMove, currentColor, _domainKnowledge->getPatternStore(currentColor));
        }
        AIDomainKnowledge::generateDefensiveMoves(_board, _root->getMove().position, _root->getColor(), _domainKnowledge->getPatternStore(currentColor));

        return true;
    }

    return false;
}

bool AI::goBack() {
    if (_root && _root->getParent()) {
        _root = _root->getParent();
        if (_root->_debugState) {
            delete _board;
            _board = new Field(*(static_cast<Field*>(_root->_debugState)));
        }
        return true;
    }

    return false;
}


void AI::goToNode(short position) {
    if (_root->hasChild(position)) {
        _root = _root->getChild(position);
        _board->placePiece(_root->getMove());

        AIDomainKnowledge::updateDomainKnowledge(_domainKnowledge, _root->getMove().position);

        short currentColor = FieldMove::getNextColor(_root->getColor());
        if (_board->getMoves().size() >= 2) {
            short lastMove = *(++_board->getMoves().rbegin());
            AIDomainKnowledge::generateAttackMoves(_board, lastMove, currentColor, _domainKnowledge->getPatternStore(currentColor));
        }
        AIDomainKnowledge::generateDefensiveMoves(_board, _root->getMove().position, _root->getColor(), _domainKnowledge->getPatternStore(currentColor));

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

    _board->placePiece(child->getMove());

    AIDomainKnowledge::updateDomainKnowledge(_domainKnowledge, child->getMove().position);

    short currentColor = FieldMove::getNextColor(child->getColor());
    if (_board->getMoves().size() >= 2) {
        short lastMove = *(++_board->getMoves().rbegin());
        AIDomainKnowledge::generateAttackMoves(_board, lastMove, currentColor, _domainKnowledge->getPatternStore(currentColor));
    }
    AIDomainKnowledge::generateDefensiveMoves(_board, child->getMove().position, child->getColor(), _domainKnowledge->getPatternStore(currentColor));

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

void AI::expand(MCTNode* node, IField* field, AIDomainKnowledge* domainKnowledge) {
    short currentColor = FieldMove::getNextColor(node->getColor());
    if (field->getMoves().size() >= 2) {
        short lastMove = *(++field->getMoves().rbegin());
        AIDomainKnowledge::generateAttackMoves(field, lastMove, currentColor, domainKnowledge->getPatternStore(currentColor));
    }
    AIDomainKnowledge::generateDefensiveMoves(field, node->getMove().position, node->getColor(), domainKnowledge->getPatternStore(currentColor));

    bool usedTacticalMoves = false, usedOnlyDevelopmentMoves = true, usedFirstPriorityMoves = false;
    unsigned patternCategoryIndex = 0;
    for (auto& patternCategory : domainKnowledge->getPatternStore(currentColor).patterns) {
        if (patternCategory.empty()) {
            patternCategoryIndex++;
            continue;
        }

        if (patternCategoryIndex >= AIPatternsStore::DEVELOPMENT_ATTACK && usedFirstPriorityMoves) {
            break;
        }

        for (auto& pattern : patternCategory) {
            for (auto& move : pattern.moves) {
                usedTacticalMoves = true;
                usedFirstPriorityMoves = !(pattern.type == AITacticalPatternType::DEVELOPMENT_ATTACK || pattern.type == AITacticalPatternType::DEVELOPMENT_DEFENCE);
                usedOnlyDevelopmentMoves = usedOnlyDevelopmentMoves && (pattern.type == AITacticalPatternType::DEVELOPMENT_ATTACK || pattern.type == AITacticalPatternType::DEVELOPMENT_DEFENCE);

                if (node->hasChild(move)) {
                    continue;
                }

                MCTNode* child = new MCTNode();
                child->setPosition(move);
                child->setColor(FieldMove::getNextColor(node->getColor()));

                node->addChild(child);
            }
        }

        node->setExpanded(true);
        patternCategoryIndex++;
    }

    if (usedTacticalMoves && !usedOnlyDevelopmentMoves) {
        return;
    }

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
    for (auto& child : node->getChildren()) {
        if (child->isTerminal()) {
            continue;
        }

        isAllTerminal = false;
        float nodeSelectionScores = child->getNodeSelectionScore(_playerColor);
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

    explore(bestNode, field, domainKnowledge);
}

void AI::explore(MCTNode* node, IField* field, AIDomainKnowledge* domainKnowledge) {
    field->placePiece(node->getMove());
    AIDomainKnowledge::updateDomainKnowledge(domainKnowledge, node->getMove().position);
    node->setExplored(true);

    if (field->getFieldStatus() != FieldStatus::IN_PROGRESS) {
        // game ended

        node->setEndpoint(true);
        node->setTerminal(true);

        if (node->getNodeVisits() == 0) {
            short winningPlayerColor = static_cast<short>(field->getFieldStatus());
            float scores = 0.f;
            if (winningPlayerColor == _playerColor) {
                scores = 1.f;
            } else if (field->getFieldStatus() != FieldStatus::DRAW) {
                scores = -1.f;
            }

            node->backpropagate(scores, nullptr);
        } else {
            while (node) {
                node->addNodeExploration();
                node = node->getParent();
            }
        }

        return;
    }

    auto& moves = field->getAvailableMoves(FieldMove::getNextColor(node->getColor()));

    short rMove = 0;
    short currentColor = FieldMove::getNextColor(node->getColor());
    if (field->getMoves().size() >= 2) {
        short lastMove = *(++field->getMoves().rbegin());
        AIDomainKnowledge::generateAttackMoves(field, lastMove, currentColor, domainKnowledge->getPatternStore(currentColor));
    }
    AIDomainKnowledge::generateDefensiveMoves(field, node->getMove().position, node->getColor(), domainKnowledge->getPatternStore(currentColor));

    int patternIndex = 0;
    for (auto& patternCategory : domainKnowledge->getPatternStore(currentColor).patterns) {
        if (patternCategory.empty()) {
            patternIndex++;
            continue;
        }

        auto moveDecision = rand() % 100;
        if (moveDecision > 70 && (patternIndex == AIPatternsStore::DEVELOPMENT_ATTACK || patternIndex == AIPatternsStore::DEVELOPMENT_DEFENCE)) {
            break;
        }

        auto randomPatternIndexx = rand() % patternCategory.size(); // not _really_ random
        auto patternIt = patternCategory.begin();
        while (randomPatternIndexx > 0) { ++patternIt; --randomPatternIndexx; }
        auto rMoveIndex = rand() % (*patternIt).moves.size();
        auto moveItr = (*patternIt).moves.begin();
        while (rMoveIndex > 0) { moveItr++; rMoveIndex--; }

        rMove = *moveItr;
        if (node->hasChild(rMove) && node->getChild(rMove)->isTerminal()) {
            bool isExit = false;
            for (auto& pattern : patternCategory) {
                for (auto& move : pattern.moves) {
                    if (!node->hasChild(move) || !node->getChild(move)->isTerminal()) {
                        rMove = move;
                        isExit = true;
                        break;
                    }
                }

                if (isExit) {
                    break;
                }
            }
        }

        break;
    }

    if (rMove == 0) {
        auto rIndex = rand() % moves.size();
        auto moveItr = moves.begin();
        while (rIndex > 0) { moveItr++; rIndex--; }
        rMove = *moveItr;

//        if (node->hasChild(rMove) && node->getChild(rMove)->isTerminal()) {
//            for (auto& move : moves) {
//                if (!node->hasChild(move) || !node->getChild(move)->isTerminal()) {
//                    rMove = move;
//                    break;
//                }
//            }
//        }
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
    AIDomainKnowledge::updateDomainKnowledge(domainKnowledge, node->getMove().position);
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



    short rMove = 0;
    short currentColor = FieldMove::getNextColor(node->getColor());
    if (field->getMoves().size() >= 2) {
        short lastMove = *(++field->getMoves().rbegin());
        AIDomainKnowledge::generateAttackMoves(field, lastMove, currentColor, domainKnowledge->getPatternStore(currentColor));
    }
    AIDomainKnowledge::generateDefensiveMoves(field, node->getMove().position, node->getColor(), domainKnowledge->getPatternStore(currentColor));

    int patternIndex = 0;
    for (auto& patternCategory : domainKnowledge->getPatternStore(currentColor).patterns) {
        if (patternCategory.empty()) {
            patternIndex++;
            continue;
        }

        auto moveDecision = rand() % 100;
        if (moveDecision > CHANCE_TO_DROP_DEVELOPMENT_ATTACK && patternIndex == AIPatternsStore::DEVELOPMENT_ATTACK) {
            break;
        }
        if (moveDecision > CHANCE_TO_DROP_DEVELOPMENT_DEFENCE && patternIndex == AIPatternsStore::DEVELOPMENT_DEFENCE) {
            break;
        }

        auto randomPatternIndexx = rand() % patternCategory.size(); // not _really_ random
        auto patternIt = patternCategory.begin();
        while (randomPatternIndexx > 0) { ++patternIt; --randomPatternIndexx; }
        auto rMoveIndex = rand() % (*patternIt).moves.size();
        auto moveItr = (*patternIt).moves.begin();
        while (rMoveIndex > 0) { moveItr++; rMoveIndex--; }

        rMove = *moveItr;

        if (node->hasChild(rMove) && node->getChild(rMove)->isTerminal()) {
            bool isExit = false;
            for (auto& pattern : patternCategory) {
                for (auto& move : pattern.moves) {
                    if (!node->hasChild(move) || !node->getChild(move)->isTerminal()) {
                        rMove = move;
                        isExit = true;
                        break;
                    }
                }

                if (isExit) {
                    break;
                }
            }
        }

        break;
    }

    if (rMove == 0) {
        auto& moves = field->getAvailableMoves(FieldMove::getNextColor(node->getColor()));
        auto rIndex = rand() % moves.size();
        auto moveItr = moves.begin();
        while (rIndex > 0) { moveItr++; rIndex--; }
        rMove = *moveItr;

//        if (node->hasChild(rMove) && node->getChild(rMove)->isTerminal()) {
//            for (auto& move : moves) {
//                if (!node->hasChild(move) || !node->getChild(move)->isTerminal()) {
//                    rMove = move;
//                    break;
//                }
//            }
//        }
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

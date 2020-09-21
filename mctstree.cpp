#include "mctstree.h"
#include <algorithm>
#include <thread>
#include <future>
#include <math.h>
#include <QDebug>
#include "debug.h"

unsigned MCTSTree::NODE_EXPLORATIONS_TO_EXPAND = 32;

MCTSTree::MCTSTree(short evalColor) {
    _root = new MCTSNode();
    _root->setParent(nullptr);

    _evalColor = evalColor;

    unsigned int n = std::thread::hardware_concurrency();
    _maxTreads = std::min(std::max(n, _maxTreads), 24u);
}

void MCTSTree::selectChild(short x, short y) {
    unsigned long userDataBlack = 0;
    userDataBlack = writePositionX(x, userDataBlack);
    userDataBlack = writePositionY(y, userDataBlack);
    userDataBlack = writeColorData(BLACK_PIECE_COLOR, userDataBlack);

    unsigned long userDataWhite = 0;
    userDataWhite = writePositionX(x, userDataWhite);
    userDataWhite = writePositionY(y, userDataWhite);
    userDataWhite = writeColorData(WHITE_PIECE_COLOR, userDataWhite);

    MCTSNode* node = _root->getChildHead();
    while (node) {
        if (node->getUserData() == userDataBlack || node->getUserData() == userDataWhite) {
            break;
        }
        node = node->getNextNode();
    }

    if (!node) {
        short color = extractColorData(_root->getUserData());
        unsigned long userData = 0;
        userData = writePositionX(x, userData);
        userData = writePositionY(y, userData);
        userData = writeColorData(getNextPlayerColor(color), userData);

        node = new MCTSNode();
        node->setUserData(userData);
        node->__x = x;
        node->__y = y;
        node->__color = getNextPlayerColor(color);
        _root->addChild(node);
    }

    _root = node;
}

std::vector<AIMoveData> MCTSTree::getNodesData() const {
    std::vector<AIMoveData> result;
    MCTSNode* node = _root->getChildHead();
    float rootVisits = _root->getPlayouts();

    while (node) {
        auto nodeUserData = node->getUserData();
        short x = extractPositionX(nodeUserData);
        short y = extractPositionY(nodeUserData);

        float nodeAddScore = node->getPlayouts() > 0 && rootVisits > 0 ? std::sqrt(std::log(rootVisits) / sqrt(node->getPlayouts())) : rootVisits;
        float nodeScore = node->getPlayouts() > 0 ? node->getScore() / node->getPlayouts() : 0.f;

        AIMoveData moveData;
        moveData.position = getHashedPosition(x, y);
        moveData.scores = nodeScore;
        moveData.color = extractColorData(nodeUserData);
        moveData.nodeVisits = node->getPlayouts();
        moveData.selectionScore = nodeScore + nodeAddScore;


        result.push_back(moveData);
        node = node->getNextNode();
    }

    return result;
}

std::vector<AIMoveData> MCTSTree::getBestPlayout(short x, short y) const {
    std::vector<AIMoveData> result;
    auto node = _root->getChildHead();
    while (node) {
        auto nodeUserData = node->getUserData();
        short nodeX = extractPositionX(nodeUserData);
        short nodeY = extractPositionY(nodeUserData);
        if (nodeX == x && nodeY == y) {
            break;
        }

        node = node->getNextNode();
    }

    unsigned topIndex = node ? node->__depth : 0;
    while (node) {
        auto nodeUserData = node->getUserData();
        short x = extractPositionX(nodeUserData);
        short y = extractPositionY(nodeUserData);

        float nodeScore = node->getPlayouts() > 0 ? node->getScore() / node->getPlayouts() : 0.f;

        AIMoveData moveData;
        moveData.position = getHashedPosition(x, y);
        moveData.scores = nodeScore;
        moveData.color = extractColorData(nodeUserData);
        moveData.nodeVisits = node->getPlayouts();
        moveData.selectionScore = 0.f;
        moveData.moveIndex = node->__depth - topIndex + 1;

        result.push_back(moveData);

        auto child = node->getChildHead();
        auto bestChild = child;
        float bestScore = -100.f;
        while (child) {
            float childScore = child->getPlayouts() > 0 ? child->getScore() / child->getPlayouts() : 0.f;
            if (childScore >= bestScore) {
                bestScore = childScore;
                bestChild = child;
            }
            child = child->getNextNode();
        }
        node = bestChild;
    }

    return result;
}

void MCTSTree::update(const BitField* const rootState) {
    explore(_root, rootState);
}

void MCTSTree::expand(MCTSNode* root, const BitField* const rootState) {
    // create nodes
    if (rootState->getGameStatus() != 0) {
        return;
    }

    short color = extractColorData(root->getUserData());

    const auto& moves = rootState->getBestMoves(getNextPlayerColor(color));
    for (const auto& move : moves) {
        MCTSNode* child = new MCTSNode();

        unsigned long userData = 0;
        userData = writePositionX(extractHashedPositionX(move), userData);
        userData = writePositionY(extractHashedPositionY(move), userData);
        userData = writeColorData(getNextPlayerColor(color), userData);

        child->setUserData(userData);
        child->__x = extractHashedPositionX(move);
        child->__y = extractHashedPositionY(move);
        child->__color = getNextPlayerColor(color);
        root->addChild(child);
    }
}

void MCTSTree::explore(MCTSNode* root, const BitField* const rootState) {
    Debug::getInstance().startTrack(DebugTimeTracks::NODE_SELECTION);
    MCTSNode* node = root;
    BitField field(*rootState);
    while (node->getChildHead()) {
        MCTSNode* nextNode = selectBestChild(node, &field);
        node = nextNode;

        short x = extractPositionX(node->getUserData());
        short y = extractPositionY(node->getUserData());
        short color = extractColorData(node->getUserData());

        field.makeMove(x, y, color);
    }

    if (field.getGameStatus() == BLACK_PIECE_COLOR || field.getGameStatus() == WHITE_PIECE_COLOR) {
        node->setTerminal();
    }

    Debug::getInstance().stopTrack(DebugTimeTracks::NODE_SELECTION);

    Debug::getInstance().startTrack(DebugTimeTracks::AI_UPDATE);
    float playoutScore = 0;
    unsigned playouts = 1;
    if (field.getGameStatus() != 0) {
        if (field.getGameStatus() == _evalColor) {
            playoutScore = 1.f;
        } else if (field.getGameStatus() == getNextPlayerColor(_evalColor)) {
            playoutScore = -1.f;
        }
    } else {
        short moveColor = extractColorData(node->getUserData());
        std::vector<std::future<float>> threads;
        for (unsigned i = 0; i < _maxTreads; ++i) {
            threads.push_back(std::async([this](const BitField* const rootState, short rootColor) {
                return playout(rootState, rootColor);
            }, &field, moveColor));
        }

        for (auto& result : threads) {
            playoutScore += result.get();
        }

        playoutScore = playoutScore / static_cast<float>(_maxTreads);
        playouts = _maxTreads;

//        playoutScore = playout(&field, moveColor);
    }

    Debug::getInstance().stopTrack(DebugTimeTracks::AI_UPDATE);

    Debug::getInstance().startTrack(DebugTimeTracks::TRAVERSE_AND_EXPAND);
    MCTSNode* traversBackNode = node;
    while (traversBackNode) {
        short color = extractColorData(traversBackNode->getUserData());

        traversBackNode->addScore(color == _evalColor ? playoutScore : -playoutScore);
        traversBackNode->addPlayout();
        traversBackNode->addRealPlayouts(playouts);

        traversBackNode = traversBackNode->getParent();
    }

    if (node->isLeaf() && node->getRealPlayouts() >= NODE_EXPLORATIONS_TO_EXPAND) {
        expand(node, &field);
    }
    Debug::getInstance().stopTrack(DebugTimeTracks::TRAVERSE_AND_EXPAND);
}

MCTSNode* MCTSTree::selectBestChild(MCTSNode* root, const BitField* const rootState) const {
    unsigned long rootVisits = root->getPlayouts();
    float bestScore = -1000.f;
    MCTSNode* bestNode = root->getChildHead();
    MCTSNode* node = root->getChildHead();

    double p = 1.f - std::min(root->getPlayouts() / 500000.0, 1.0);
    double rndHit = ((double) rand() / (RAND_MAX));
    int rndChildIndex = -1;
    if (p < rndHit) {
        rndChildIndex = rand() % root->getChildrenCount();

        MCTSNode* randomNode = root->getChildHead();
        while (randomNode && rndChildIndex > 0) {
            rndChildIndex--;

            randomNode = randomNode->getNextNode();
        }

        return randomNode;
    }

    while (node && rootVisits > 0) {
        float nodeAddScore = node->getPlayouts() > 0 ? std::sqrt(std::log(rootVisits) / sqrt(node->getPlayouts())) : rootVisits;
        float nodeScore = node->getPlayouts() > 0 ? node->getScore() / node->getPlayouts() : 0.f;

//        short x = extractPositionX(node->getUserData());
//        short y = extractPositionY(node->getUserData());
//        short color = extractColorData(node->getUserData());
        float nodePriority = 0;//rootState->getMovePriority(getHashedPosition(x, y), color) / 5.f;
        if (node->isTerminal()) {
            nodeScore += 100;
        }

        float totalScore = nodeScore + nodeAddScore + nodePriority;
        if (totalScore >= bestScore) {
            bestScore = totalScore;
            bestNode = node;
        }

        node = node->getNextNode();
    }

    return bestNode;
}

float MCTSTree::playout(const BitField* const rootState, short rootColor) {
    short x = 0;
    short y = 0;
    short color = rootColor;

    BitField field(*rootState);
    while (field.getGameStatus() == 0) {
        // find move position
        color = getNextPlayerColor(color);

        auto move = field.getMoveByPriority(color);
        x = extractHashedPositionX(move);
        y = extractHashedPositionY(move);

        if (!field.makeMove(x, y, color)) {
            break;
        }
    }

    if (field.getGameStatus() == _evalColor) {
        return 1.f;
    } else if (field.getGameStatus() == getNextPlayerColor(_evalColor)) {
        return -1.f;
    }

    return 0.f;
}

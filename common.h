#pragma once

#include <vector>
#include <array>
#include <unordered_set>
#include <unordered_map>

static constexpr unsigned long DATA_MASK_SIZE = 8;
static constexpr unsigned long DOUBLE_DATA_MASK_SIZE = DATA_MASK_SIZE * 2;
static constexpr unsigned long TRIPPLE_DATA_MASK_SIZE = DATA_MASK_SIZE * 3;
static constexpr unsigned long COLOR_DATA_MASK_SIZE = DATA_MASK_SIZE * 2;
static constexpr unsigned long DATA_MOVE_MASK = 0xFF;
static constexpr unsigned long DATA_MOVE_MASK_SHIFTED = 0xFF << DATA_MASK_SIZE;
static constexpr unsigned long DATA_MOVE_MASK_DOUBLE_SHIFTED = 0xFF << DOUBLE_DATA_MASK_SIZE;
static constexpr unsigned long DATA_MOVE_MASK_TRIPPLE_SHIFTED = 0xFF << TRIPPLE_DATA_MASK_SIZE;
static constexpr short BLACK_PIECE_COLOR = 1;
static constexpr short WHITE_PIECE_COLOR = 2;
static constexpr short FIRST_MOVE_COLOR = BLACK_PIECE_COLOR;
static constexpr short MOVES_IN_ROW_TO_WIN = 5;
static constexpr short BOARD_SIZE = 19;
static constexpr short BOARD_LENGTH = BOARD_SIZE * BOARD_SIZE;
static constexpr unsigned long FILLED_ROW_BITS = (1 << BOARD_SIZE) - 1;
static constexpr std::array<unsigned, 9> FFS_TABLE = {0, 1, 2, 0, 3, 0, 0, 0, 4};

static constexpr short extractPositionX(unsigned long userData) {
    return userData & DATA_MOVE_MASK;
}

static constexpr short extractPositionY(unsigned long userData) {
    return (userData >> DATA_MASK_SIZE) & DATA_MOVE_MASK;
}

static constexpr short extractColorData(unsigned long userData) {
    return (userData >> COLOR_DATA_MASK_SIZE) & DATA_MOVE_MASK;
}

static constexpr unsigned long writePositionX(short x, unsigned long userData) {
    return (userData & (~DATA_MOVE_MASK)) | x;
}

static constexpr unsigned long writePositionY(short y, unsigned long userData) {
    return (userData & (~(DATA_MOVE_MASK << DATA_MASK_SIZE))) | (y << DATA_MASK_SIZE);
}

static constexpr unsigned long writeColorData(short color, unsigned long userData) {
    return (userData & (~(DATA_MOVE_MASK << COLOR_DATA_MASK_SIZE))) | (color << COLOR_DATA_MASK_SIZE);
}

static constexpr short getHashedPosition(short x, short y) {
    return x + y * BOARD_SIZE;
}

static constexpr short extractHashedPositionX(unsigned pos) {
    return pos % BOARD_SIZE;
}

static constexpr short extractHashedPositionY(unsigned pos) {
    return pos / BOARD_SIZE;
}

static constexpr short getNextPlayerColor(short color) {
    if (color == -1 || color == 0) {
        return FIRST_MOVE_COLOR;
    }

    return color == BLACK_PIECE_COLOR ? WHITE_PIECE_COLOR : BLACK_PIECE_COLOR;
}

static std::array<short, BOARD_LENGTH> distanceToLeftEdge() {
    std::array<short, BOARD_LENGTH> result;
    for (int y = 0; y < BOARD_SIZE; ++y) {
        for (int x = 0; x < BOARD_SIZE; ++x) {
            result[getHashedPosition(x, y)] = BOARD_SIZE - x - 1;
        }
    }

    return result;
}

static std::array<short, BOARD_LENGTH> distanceToRightEdge() {
    std::array<short, BOARD_LENGTH> result;
    for (int y = 0; y < BOARD_SIZE; ++y) {
        for (int x = 0; x < BOARD_SIZE; ++x) {
            result[getHashedPosition(x, y)] = x;
        }
    }

    return result;
}

static std::array<short, BOARD_LENGTH> distanceToTopEdge() {
    std::array<short, BOARD_LENGTH> result;
    for (int y = 0; y < BOARD_SIZE; ++y) {
        for (int x = 0; x < BOARD_SIZE; ++x) {
            result[getHashedPosition(x, y)] = BOARD_SIZE - y - 1;
        }
    }

    return result;
}

static std::array<short, BOARD_LENGTH> distanceToBottomEdge() {
    std::array<short, BOARD_LENGTH> result;
    for (int y = 0; y < BOARD_SIZE; ++y) {
        for (int x = 0; x < BOARD_SIZE; ++x) {
            result[getHashedPosition(x, y)] = y;
        }
    }

    return result;
}

static const std::array<short, BOARD_LENGTH> BOARD_DISTANCE_RIGHT = distanceToRightEdge();
static const std::array<short, BOARD_LENGTH> BOARD_DISTANCE_LEFT = distanceToLeftEdge();
static const std::array<short, BOARD_LENGTH> BOARD_DISTANCE_TOP = distanceToTopEdge();
static const std::array<short, BOARD_LENGTH> BOARD_DISTANCE_BOTTOM = distanceToBottomEdge();

struct AIMoveData {
    short position = 0;
    short x = 0;
    short y = 0;
    short color = 0;

    float scores = 0.f;
    float selectionScore = 0.f;

    unsigned nodeVisits = 0;
};

static constexpr short AI_PATTERN_DEFENCES_COUNT = 4;
struct AIPattern {
    short index = 0;
    long pattern = 0;
    long emptyPattern = 0;
    long enemyPattern = 0;

    short patternShift = 0;
    short emptyShift = 0;
    short enemyShift = 0;

    unsigned attackPriority = 0;
    unsigned miaiId = 0;

    std::array<short, AI_PATTERN_DEFENCES_COUNT> defence = {0};
    std::array<short, AI_PATTERN_DEFENCES_COUNT> defencePriorities = {0};
};

static constexpr unsigned PATTERNS_COUNT = 31;
static constexpr std::array<AIPattern, PATTERNS_COUNT> generatePatterns() {
//    short id = 0;
    std::array<AIPattern, PATTERNS_COUNT> result = {{
                                                        {0, 15, 1, 0, -4, 0, 0, 8, 1, {0}, {8}},
                                                        {1, 23, 1, 0, -3, 0, 0, 8, 2, {0}, {8}},
                                                        {2, 27, 1, 0, -2, 0, 0, 8, 3, {0}, {8}},
                                                        {3, 29, 1, 0, -1, 0, 0, 8, 4, {0}, {8}},
                                                        {4, 15, 1, 0, 1, 0, 0, 8, 5, {0}, {8}},
                                                        {5, 7, 35, 0, 1, -1, 0, 6, 6, {0, 4}, {6, 6}},
                                                        {6, 7, 49, 0, -3, -4, 0, 6, 6, {0, -4}, {6, 6}},
                                                        {7, 13, 37, 0, -1, -2, 0, 6, 7, {0, 3, -2}, {6, 6, 6}},
                                                        {8, 11, 41, 0, -2, -3, 0, 6, 8, {0, 2, -3}, {6, 6, 6}},
                                                        {9, 19, 3, 0, -2, 1, 0, 4, 16, {0, 1}, {4, 4}},
                                                        {10, 19, 3, 0, -3, -1, 0, 4, 16, {0, -1}, {4, 4}},
                                                        {11, 25, 3, 0, -2, -1, 0, 4, 17, {0, -1}, {4, 4}},
                                                        {12, 25, 3, 0, -1, 1, 0, 4, 17, {0, 1}, {4, 4}},
                                                        {13, 11, 41, 0, -4, -5, 0, 4, 7, {0, -2, -5}, {4, 4, 4}},
                                                        {14, 11, 41, 0, 1, 3, 0, 4, 7, {0, 3, 5}, {4, 4, 4}},
                                                        {15, 13, 37, 0, -4, -5, 0, 4, 8, {0, -3, -5}, {4, 4, 4}},
                                                        {16, 13, 37, 0, 1, 2, 0, 4, 8, {0, 2, 5}, {4, 4, 4}},
                                                        {17, 13, 5, 1, -1, -2, 3, 4, 9, {0, -2}, {4, 4}},
                                                        {18, 13, 5, 1, 1, 2, 5, 4, 9, {0, 2}, {4, 4}},
                                                        {19, 13, 9, 1, -1, 3, -2, 4, 10, {0, 3}, {4, 4}},
                                                        {20, 13, 9, 1, -4, -3, -5, 4, 10, {0, -3}, {4, 4}},
                                                        {21, 11, 9, 1, -2, -3, 2, 4, 11, {0, -3}, {4, 4}},
                                                        {22, 11, 9, 1, 1, 3, 5, 4, 11, {0, 3}, {4, 4}},
                                                        {23, 11, 5, 1, -2, 2, -3, 4, 12, {0, 2}, {4, 4}},
                                                        {24, 11, 5, 1, -4, -2, -5, 4, 12, {0, -2}, {4, 4}},
                                                        {25, 7, 3, 1, 1, -1, 4, 4, 13, {0, -1}, {4, 4}},
                                                        {26, 7, 3, 1, 2, 1, 5, 4, 13, {0, 1}, {4, 4}},
                                                        {27, 7, 3, 1, -3, 1, -4, 4, 14, {0, 1}, {4, 4}},
                                                        {28, 7, 3, 1, -4, -1, -5, 4, 14, {0, -1}, {4, 4}},
                                                        {29, 7, 17, 65, -3, -4, -5, 4, 15, {0, -4}, {4, 4}},
                                                        {30, 7, 17, 65, 1, 4, -1, 4, 15, {0, 4}, {4, 4}},
    }};

    return result;
}

struct MOVE_PRIORITIES {
    static constexpr short IMMIDIATE = 8;
    static constexpr short URGENT = 6;
    static constexpr short HIGH = 4;
};

static constexpr std::array<AIPattern, PATTERNS_COUNT> MOVE_PATTERNS = generatePatterns();

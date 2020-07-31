#pragma once

#include <vector>
#include <unordered_set>
#include <unordered_map>

static constexpr short BLACK_PIECE_COLOR = 1;
static constexpr short WHITE_PIECE_COLOR = 2;
static constexpr short FIRST_MOVE_COLOR = BLACK_PIECE_COLOR;
static constexpr short MOVES_IN_ROW_TO_WIN = 5;
static constexpr short BOARD_SIZE = 9;

struct FieldMove {
    static constexpr int OFFSET = 8;
    static constexpr int MIN_X_VALUE = 1 << OFFSET;
    static constexpr int MIN_Y_VALUE = 1;
    static constexpr int BASE = 0xFF;

    short position = -1;
    short color = -1;

    static short getXFromPosition(short position) {
        return position >> OFFSET;
    }

    static short getYFromPosition(short position) {
        return position & BASE;
    }

    static short getPositionFromPoint(short x, short y) {
        return (x << OFFSET) + y;
    }

    static short getNextColor(short color) {
        if (color == -1) {
            return FIRST_MOVE_COLOR;
        }

        return color == BLACK_PIECE_COLOR ? WHITE_PIECE_COLOR : BLACK_PIECE_COLOR;
    }
};

enum class FieldStatus {
    IN_PROGRESS = 5,
    DRAW = 6,
    BLACK_WIN = BLACK_PIECE_COLOR,
    WHITE_WIN = WHITE_PIECE_COLOR
};

class IField {
public:
    virtual const std::unordered_set<short>& getAvailableMoves() const = 0;
    virtual const std::unordered_map<short, short>& getCurrentPieces() const = 0;
    virtual bool placePiece(short x, short y, short color) = 0;
    virtual bool placePiece(const FieldMove& move) = 0;
    virtual FieldStatus getFieldStatus() const = 0;
    virtual void printField() const = 0;

    virtual void clear() = 0;
};

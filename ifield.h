#pragma once

#include <vector>
#include <unordered_set>
#include <unordered_map>

static constexpr short BLACK_PIECE_COLOR = 1;
static constexpr short WHITE_PIECE_COLOR = 2;
static constexpr short FIRST_MOVE_COLOR = BLACK_PIECE_COLOR;
static constexpr short MOVES_IN_ROW_TO_WIN = 5;
static constexpr short BOARD_SIZE = 9;
static constexpr short BOARD_LENGTH = BOARD_SIZE * BOARD_SIZE;

struct FieldMove {
    static constexpr short BASE = BOARD_SIZE;

    static constexpr short MIN_X_VALUE = 1;
    static constexpr short MIN_Y_VALUE = BASE;

    short position = -1;
    short color = -1;

    static short getXFromPosition(short position) {
        return position % BASE;
    }

    static short getYFromPosition(short position) {
        return position / BASE;
    }

    static short getPositionFromPoint(short x, short y) {
        return y * BASE + x;
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
    virtual const std::array<short, BOARD_SIZE * BOARD_SIZE>& getBoardState() const = 0;
    virtual const std::vector<short> getMoves() const = 0;
    virtual bool placePiece(short x, short y, short color) = 0;
    virtual bool placePiece(short position, short color) = 0;
    virtual bool placePiece(const FieldMove& move) = 0;
    virtual FieldStatus getFieldStatus() const = 0;
    virtual void printField() const = 0;

    virtual void clear() = 0;
};

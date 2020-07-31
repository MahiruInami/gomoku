#include "field.h"
#include "debug.h"
#include <QElapsedTimer>

Field::Field()
    : _status(FieldStatus::IN_PROGRESS)
{
}


Field::~Field()
{
}

Field::Field(const Field& field) {
    _availableMoves = field._availableMoves;
    _board = field._board;
    _currentMoves = field._currentMoves;
    _status = field._status;
}


const std::unordered_set<short>& Field::getAvailableMoves() const {
    return _availableMoves;
}

const std::array<short, BOARD_SIZE * BOARD_SIZE>& Field::getBoardState() const {
    return _board;
}

const std::vector<short> Field::getMoves() const {
    return _currentMoves;
}

bool Field::placePiece(short x, short y, short color) {
    FieldMove move;
    move.position = FieldMove::getPositionFromPoint(x, y);
    move.color = color;
    return placePiece(move);
}

bool Field::placePiece(short position, short color) {
    if (_board[position] != 0) {
        return false;
    }

    _board[position] = color;
    _currentMoves.push_back(position);

    updateStatus(position, color);
    return true;
}

bool Field::placePiece(const FieldMove& move) {
    return placePiece(move.position, move.color);
}

void Field::updateStatus(short position, short color) {
    short originalX = FieldMove::getXFromPosition(position);
    short originalY = FieldMove::getYFromPosition(position);

    //==========================================================
    // up-down
    short piecesAbove = 0;
    for (piecesAbove = 0; piecesAbove < MOVES_IN_ROW_TO_WIN; ++piecesAbove) {
        short pos = position + FieldMove::MIN_Y_VALUE * (piecesAbove + 1);
        if (pos >= BOARD_LENGTH) {
            break;
        }

        if (_board[pos] != color) {
            break;
        }
    }
    short piecesBelow = 0;
    for (piecesBelow = 0; piecesBelow < MOVES_IN_ROW_TO_WIN; ++piecesBelow) {
        short pos = position - FieldMove::MIN_Y_VALUE * (piecesBelow + 1);
        if (pos < 0) {
            break;
        }

        if (_board[pos] != color) {
            break;
        }
    }

    if (piecesAbove + piecesBelow + 1 >= MOVES_IN_ROW_TO_WIN) {
        _status = static_cast<FieldStatus>(color);
        return;
    }

    //==========================================================
    // left-right
    short piecesAtLeft = 0;
    for (piecesAtLeft = 0; piecesAtLeft < MOVES_IN_ROW_TO_WIN; ++piecesAtLeft) {
        short xOffset = FieldMove::MIN_X_VALUE * (piecesAtLeft + 1);
        short pos = position + xOffset;
        if (originalX + xOffset >= BOARD_SIZE) {
            break;
        }

        if (_board[pos] != color) {
            break;
        }
    }
    short piecesAtRight = 0;
    for (piecesAtRight = 0; piecesAtRight < MOVES_IN_ROW_TO_WIN; ++piecesAtRight) {
        short xOffset = FieldMove::MIN_X_VALUE * (piecesAtRight + 1);
        short pos = position - xOffset;
        if (originalX - xOffset < 0) {
            break;
        }
        if (_board[pos] != color) {
            break;
        }
    }

    if (piecesAtLeft + piecesAtRight + 1 >= MOVES_IN_ROW_TO_WIN) {
        _status = static_cast<FieldStatus>(color);
        return;
    }

    //==========================================================
    // left-top right-down diagonal
    short piecesTopLeft = 0;
    for (piecesTopLeft = 0; piecesTopLeft < MOVES_IN_ROW_TO_WIN; ++piecesTopLeft) {
        short xOffset = FieldMove::MIN_X_VALUE * (piecesTopLeft + 1);
        short pos = position - xOffset + FieldMove::MIN_Y_VALUE * (piecesTopLeft + 1);
        if (pos >= BOARD_LENGTH || originalX - xOffset < 0) {
            break;
        }

        if (_board[pos] != color) {
            break;
        }
    }
    short piecesDownRight = 0;
    for (piecesDownRight = 0; piecesDownRight < MOVES_IN_ROW_TO_WIN; ++piecesDownRight) {
        short xOffset = FieldMove::MIN_X_VALUE * (piecesDownRight + 1);
        short pos = position + xOffset - FieldMove::MIN_Y_VALUE * (piecesDownRight + 1);
        if (pos < 0 || originalX + xOffset >= BOARD_SIZE) {
            break;
        }

        if (_board[pos] != color) {
            break;
        }
    }

    if (piecesTopLeft + piecesDownRight + 1 >= MOVES_IN_ROW_TO_WIN) {
        _status = static_cast<FieldStatus>(color);
        return;
    }

    //==========================================================
    // right-top left-down diagonal
    short piecesTopRight = 0;
    for (piecesTopRight = 0; piecesTopRight < MOVES_IN_ROW_TO_WIN; ++piecesTopRight) {
        short xOffset = FieldMove::MIN_X_VALUE * (piecesTopRight + 1);
        short pos = position + xOffset + FieldMove::MIN_Y_VALUE * (piecesTopRight + 1);
        if (pos >= BOARD_LENGTH || originalX + xOffset >= BOARD_SIZE) {
            break;
        }

        if (_board[pos] != color) {
            break;
        }
    }
    short piecesDownLeft = 0;
    for (piecesDownLeft = 0; piecesDownLeft < MOVES_IN_ROW_TO_WIN; ++piecesDownLeft) {
        short xOffset = FieldMove::MIN_X_VALUE * (piecesDownLeft + 1);
        short pos = position - xOffset - FieldMove::MIN_Y_VALUE * (piecesDownLeft + 1);
        if (pos < 0 || originalX - xOffset < 0) {
            break;
        }

        if (_board[pos] != color) {
            break;
        }
    }

    if (piecesTopRight + piecesDownLeft + 1 >= MOVES_IN_ROW_TO_WIN) {
        _status = static_cast<FieldStatus>(color);
        return;
    }

    // remove current move
    if (_availableMoves.find(position) != _availableMoves.end()) {
        _availableMoves.erase(position);
    }

    for (short y = -1; y <= 1; ++y) {
        for (short x = -1; x <= 1; ++x) {
            short newX = originalX + x;
            short newY = originalY + y;

            if (newX < 0 || newX >= BOARD_SIZE || newY < 0 || newY >= BOARD_SIZE) {
                continue;
            }

            short newPosition = FieldMove::getPositionFromPoint(newX, newY);
            if (_board[newPosition] != 0) {
                continue;
            }

            if (_availableMoves.find(newPosition) != _availableMoves.end()) {
                continue;
            }

            _availableMoves.insert(newPosition);
         }
    }

    if (_availableMoves.empty()) {
        _status = FieldStatus::DRAW;
    }
}

void Field::updateStatus(const FieldMove& move) {
   updateStatus(move.position, move.color);
}

FieldStatus Field::getFieldStatus() const {
    return _status;
}

void Field::clear() {
    _availableMoves.clear();
    _board.fill(0);
    _currentMoves.clear();
    _status = FieldStatus::IN_PROGRESS;

    short fieldCenter = BOARD_SIZE / 2;
    _availableMoves.insert(fieldCenter * FieldMove::MIN_X_VALUE  + fieldCenter * FieldMove::MIN_Y_VALUE);

//    for (short y = 0; y < BOARD_SIZE; ++y) {
//        for (short x = 0; x < BOARD_SIZE; ++x) {
//            short newPosition = x * FieldMove::MIN_X_VALUE  + y * FieldMove::MIN_Y_VALUE;
//            _availableMoves.insert(newPosition);
//         }
//    }
}

void Field::printField() const {

}

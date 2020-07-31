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
    _moves = field._moves;
    _status = field._status;
}


const std::unordered_set<short>& Field::getAvailableMoves() const {
    return _availableMoves;
}

const std::unordered_map<short, short>& Field::getCurrentPieces() const {
    return _moves;
}

bool Field::placePiece(short x, short y, short color) {
    FieldMove move;
    move.position = FieldMove::getPositionFromPoint(x, y);
    move.color = color;
    return placePiece(move);
}

bool Field::placePiece(const FieldMove& move) {
    QElapsedTimer timerMain;
    timerMain.start();

    if (_moves.find(move.position) != _moves.end()) {
        Debug::getInstance().fieldMoveTime += timerMain.nsecsElapsed();
        return false;
    }

    _moves[move.position] = move.color;

    Debug::getInstance().fieldMoveTime += timerMain.nsecsElapsed();

    updateStatus(move);
    return true;
}

void Field::updateStatus(const FieldMove& move) {
    QElapsedTimer timerMain;
    timerMain.start();

    //==========================================================
    // up-down
    short piecesAbove = 0;
    for (piecesAbove = 0; piecesAbove < MOVES_IN_ROW_TO_WIN; ++piecesAbove) {
        short checkPosition = move.position + (piecesAbove + 1) * FieldMove::MIN_Y_VALUE;
        if (_moves.find(checkPosition) == _moves.end()) {
            break;
        }

        if (_moves[checkPosition] != move.color) {
            break;
        }
    }
    short piecesBelow = 0;
    for (piecesBelow = 0; piecesBelow < MOVES_IN_ROW_TO_WIN; ++piecesBelow) {
        short checkPosition = move.position - (piecesBelow + 1) * FieldMove::MIN_Y_VALUE;
        if (_moves.find(checkPosition) == _moves.end()) {
            break;
        }

        if (_moves[checkPosition] != move.color) {
            break;
        }
    }

    if (piecesAbove + piecesBelow + 1 >= MOVES_IN_ROW_TO_WIN) {
        _status = static_cast<FieldStatus>(move.color);
        Debug::getInstance().updateStatusTime += timerMain.nsecsElapsed();
        return;
    }

    //==========================================================
    // left-right
    short piecesAtLeft = 0;
    for (piecesAtLeft = 0; piecesAtLeft < MOVES_IN_ROW_TO_WIN; ++piecesAtLeft) {
        short checkPosition = move.position - (piecesAtLeft + 1) * FieldMove::MIN_X_VALUE;
        if (_moves.find(checkPosition) == _moves.end()) {
            break;
        }

        if (_moves[checkPosition] != move.color) {
            break;
        }
    }
    short piecesAtRight = 0;
    for (piecesAtRight = 0; piecesAtRight < MOVES_IN_ROW_TO_WIN; ++piecesAtRight) {
        short checkPosition = move.position + (piecesAtRight + 1) * FieldMove::MIN_X_VALUE;
        if (_moves.find(checkPosition) == _moves.end()) {
            break;
        }

        if (_moves[checkPosition] != move.color) {
            break;
        }
    }

    if (piecesAtLeft + piecesAtRight + 1 >= MOVES_IN_ROW_TO_WIN) {
        _status = static_cast<FieldStatus>(move.color);
        Debug::getInstance().updateStatusTime += timerMain.nsecsElapsed();
        return;
    }

    //==========================================================
    // left-top right-down diagonal
    short piecesTopLeft = 0;
    for (piecesTopLeft = 0; piecesTopLeft < MOVES_IN_ROW_TO_WIN; ++piecesTopLeft) {
        short checkPosition = move.position - (piecesTopLeft + 1) * FieldMove::MIN_X_VALUE  + (piecesTopLeft + 1) * FieldMove::MIN_Y_VALUE;
        if (_moves.find(checkPosition) == _moves.end()) {
            break;
        }

        if (_moves[checkPosition] != move.color) {
            break;
        }
    }
    short piecesDownRight = 0;
    for (piecesDownRight = 0; piecesDownRight < MOVES_IN_ROW_TO_WIN; ++piecesDownRight) {
        short checkPosition = move.position + (piecesDownRight + 1) * FieldMove::MIN_X_VALUE  - (piecesDownRight + 1) * FieldMove::MIN_Y_VALUE;
        if (_moves.find(checkPosition) == _moves.end()) {
            break;
        }

        if (_moves[checkPosition] != move.color) {
            break;
        }
    }

    if (piecesTopLeft + piecesDownRight + 1 >= MOVES_IN_ROW_TO_WIN) {
        _status = static_cast<FieldStatus>(move.color);
        Debug::getInstance().updateStatusTime += timerMain.nsecsElapsed();
        return;
    }

    //==========================================================
    // right-top left-down diagonal
    short piecesTopRight = 0;
    for (piecesTopRight = 0; piecesTopRight < MOVES_IN_ROW_TO_WIN; ++piecesTopRight) {
        short checkPosition = move.position + (piecesTopRight + 1) * FieldMove::MIN_X_VALUE  + (piecesTopRight + 1) * FieldMove::MIN_Y_VALUE;
        if (_moves.find(checkPosition) == _moves.end()) {
            break;
        }

        if (_moves[checkPosition] != move.color) {
            break;
        }
    }
    short piecesDownLeft = 0;
    for (piecesDownLeft = 0; piecesDownLeft < MOVES_IN_ROW_TO_WIN; ++piecesDownLeft) {
        short checkPosition = move.position - (piecesDownLeft + 1) * FieldMove::MIN_X_VALUE  - (piecesDownLeft + 1) * FieldMove::MIN_Y_VALUE;
        if (_moves.find(checkPosition) == _moves.end()) {
            break;
        }

        if (_moves[checkPosition] != move.color) {
            break;
        }
    }

    if (piecesTopRight + piecesDownLeft + 1 >= MOVES_IN_ROW_TO_WIN) {
        _status = static_cast<FieldStatus>(move.color);
        Debug::getInstance().updateStatusTime += timerMain.nsecsElapsed();
        return;
    }

    Debug::getInstance().updateStatusTime += timerMain.nsecsElapsed();

    QElapsedTimer timerMain1;
    timerMain1.start();

    // remove current move
    if (_availableMoves.find(move.position) != _availableMoves.end()) {
        _availableMoves.erase(move.position);
    }

//    for (short y = -1; y <= 1; ++y) {
//        for (short x = -1; x <= 1; ++x) {
//            short newPosition = move.position + x * FieldMove::MIN_X_VALUE  + y * FieldMove::MIN_Y_VALUE;

//            short posX = FieldMove::getXFromPosition(newPosition);
//            short posY = FieldMove::getYFromPosition(newPosition);
//            if (posX < 0 || posX >= BOARD_SIZE || posY < 0 || posY >= BOARD_SIZE) {
//                continue;
//            }

//            if (_moves.find(newPosition) != _moves.end()) {
//                continue;
//            }

//            _availableMoves.insert(newPosition);
//         }
//    }

    if (_availableMoves.empty()) {
        _status = FieldStatus::DRAW;
    }

    Debug::getInstance().moveGenerationTime += timerMain1.nsecsElapsed();
}

FieldStatus Field::getFieldStatus() const {
    return _status;
}

void Field::clear() {
    _availableMoves.clear();
    _moves.clear();
    _status = FieldStatus::IN_PROGRESS;

//    short fieldCenter = BOARD_SIZE / 2;
//    _availableMoves.insert(fieldCenter * FieldMove::MIN_X_VALUE  + fieldCenter * FieldMove::MIN_Y_VALUE);

    for (short y = 0; y < BOARD_SIZE; ++y) {
        for (short x = 0; x < BOARD_SIZE; ++x) {
            short newPosition = x * FieldMove::MIN_X_VALUE  + y * FieldMove::MIN_Y_VALUE;
            _availableMoves.insert(newPosition);
         }
    }
}

void Field::printField() const {

}

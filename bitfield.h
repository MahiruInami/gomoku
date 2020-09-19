#pragma once

#include "common.h"
#include <bitset>
#include "movepattern.h"

class BitField
{
public:
    BitField();

    bool makeMove(short x, short y, short color);
    bool unmakeMove();

    short getRandomMove() const;
    short getMoveByPriority(short color) const;
    short getMovePriority(short hashedPosition, short color) const;
    short getMoveDefencePriority(short hashedPosition, short color) const;
    std::vector<short> getBestMoves(short color) const;

    int getGameStatus() const { return _gameStatus; }
    const std::vector<short>& getAvailableMoves() const { return _availableMoves; }


    void clear() {
        _availableMovesHash.reset();
        _availableMoves.clear();

        short fieldCenter = BOARD_SIZE / 2;
        _availableMovesHash.set(getHashedPosition(fieldCenter, fieldCenter));
        _availableMoves.push_back(getHashedPosition(fieldCenter, fieldCenter));

        _horizontals.fill(0);
        _verticals.fill(0);
        _diagonal_right.fill(0);
        _diagonal_left.fill(0);
        _attackingMovesPriority.fill(0);
        _defensiveMovesPriority.fill({});

        _filledHorizontals = 0;
        _filledVerticals = 0;
        _filledDiagonalsRight = 0;
        _filledDiagonalsLeft = 0;

        _history.clear();

        _gameStatus = 0;
    }

    static unsigned long getDiagonalRightIndex(short x, short y);
    static unsigned long getDiagonalLeftIndex(short x, short y);
private:
    void incrementalUpdate(short x, short y, short color);
    void updateMovePriority(short x, short y, std::vector<short>& newMoves);
    void createTemplate(short x, short y, short colorShift, short patternId, short attackId, short directionId, short miaiId, short priority, std::vector<short>& newMoves);
private:
    std::bitset<BOARD_LENGTH> _availableMovesHash;
    std::vector<short> _availableMoves;

    std::array<unsigned long, BOARD_SIZE * 2> _horizontals = {0};
    std::array<unsigned long, BOARD_SIZE * 2> _verticals = {0};
    std::array<unsigned long, BOARD_SIZE * 2 * 2> _diagonal_right = {0};
    std::array<unsigned long, BOARD_SIZE * 2 * 2> _diagonal_left = {0};

    std::array<unsigned long, BOARD_LENGTH * 2> _attackingMovesPriority = {0};

    struct DefensiveMove {
        unsigned short priority = 0;
        std::vector<unsigned long> patterns;

        void updatePriorityFast(unsigned short newPriority) {
            priority = std::max(newPriority, priority);
        }
    };

    std::array<DefensiveMove, BOARD_LENGTH * 2> _defensiveMovesPriority = {{}};
    /*
     * diagonal_right:
     *       *
     *     *
     *   *
     * *
     * diagonal_left:
     * *
     *   *
     *     *
     *       *
     *         *
     */

    unsigned long _filledHorizontals = 0;
    unsigned long _filledVerticals = 0;
    unsigned long _filledDiagonalsRight = 0;
    unsigned long _filledDiagonalsLeft = 0;

    std::vector<std::tuple<short, short, short>> _history;

    int _gameStatus = 0;
};


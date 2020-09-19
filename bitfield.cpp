#include "bitfield.h"
#include <QDebug>
#include "debug.h"
#include <math.h>

BitField::BitField()
    : _gameStatus(0)
{

}

unsigned long BitField::getDiagonalRightIndex(short x, short y) {
    return x + y;
}

unsigned long BitField::getDiagonalLeftIndex(short x, short y) {
    return BOARD_SIZE - x + y - 1;
}

long getPatternRight(long& pattern, long value, long opponentValue, long index) {
    long shifts = 0;
    while ((value & index) && !(opponentValue & index)) {
        index = index >> 1;
        pattern = (pattern << 1) + 1;
        shifts++;
    }

    return shifts;
}

long getPatternLeft(long& pattern, long value, long opponentValue, long index) {
    long shifts = 0;
    while ((value & index) && !(opponentValue & index)) {
        index = index << 1;
        pattern = (pattern << 1) | pattern;
        shifts++;
    }

    return shifts;
}

long getConsecutiveBitsCount(long value) {
    int count = 0;
    while (value > 0) {
       value = (value & (value << 1));
       count++;
    }

    return count;
}

short getMoveHash(short x, short y) {
    return y * BOARD_SIZE + x;
}

constexpr long PATTERN_ID_MASK = 0x3F;
constexpr long ATTACK_ID_MASK = 0x1C0;
constexpr long DIRECTION_ID_MASK = 0xE00;
constexpr long MIAI_ID_MASK = 0x3F0000;

constexpr short ATTACK_ID_SHIFT = 7;
constexpr short DIRECTION_ID_SHIFT = 9;
constexpr short MIAI_ID_SHIFT = 16;

long getPackedPriority(short patternId, short attackId, short direction, short miaiId) {
    return patternId + (attackId << ATTACK_ID_SHIFT) + (direction << DIRECTION_ID_SHIFT) + (miaiId << MIAI_ID_SHIFT);
}

short BitField::getRandomMove() const {
    auto rIndex = rand() % _availableMoves.size();
    auto moveItr = _availableMoves.begin();
    while (rIndex > 0) { moveItr++; rIndex--; }

    return *moveItr;
}

short BitField::getMovePriority(short hashedPosition, short color) const {
    int priorityShift = (color - 1) * BOARD_LENGTH;

    auto myPriority = _attackingMovesPriority[hashedPosition + priorityShift];
    return myPriority;
}

short BitField::getMoveDefencePriority(short hashedPosition, short color) const {
    int priorityShift = (color - 1) * BOARD_LENGTH;

    auto myPriority = _defensiveMovesPriority[hashedPosition + priorityShift];
    return myPriority.priority;
}

short BitField::getMoveByPriority(short color) const {
    std::array<std::vector<short>, MOVE_PRIORITIES::IMMIDIATE + 1> attackPriorities;
    std::array<std::vector<short>, MOVE_PRIORITIES::IMMIDIATE + 1> defencePriorities;
    short maxDefencePriority = 0, minDefencePriority = 1000;
    short maxAttackPriority = 0, minAttackPriority = 1000;
    for (auto& move : _availableMoves) {
        short attackPriority = getMovePriority(move, color);
        short defencePriority = getMoveDefencePriority(move, color);
        attackPriorities[attackPriority].push_back(move);
        defencePriorities[defencePriority].push_back(move);

        minDefencePriority = std::max(minDefencePriority, defencePriority);
        maxDefencePriority = std::max(maxDefencePriority, defencePriority);
        minAttackPriority = std::max(minAttackPriority, attackPriority);
        maxAttackPriority = std::max(maxAttackPriority, attackPriority);
    }

    if (maxAttackPriority >= MOVE_PRIORITIES::IMMIDIATE) {
        auto rIndex = rand() % attackPriorities[maxAttackPriority].size();
        auto moveItr = attackPriorities[maxAttackPriority].begin();
        while (rIndex > 0) { moveItr++; rIndex--; }

        return *moveItr;
    }

    if (maxDefencePriority >= MOVE_PRIORITIES::IMMIDIATE) {
        auto rIndex = rand() % defencePriorities[maxDefencePriority].size();
        auto moveItr = defencePriorities[maxDefencePriority].begin();
        while (rIndex > 0) { moveItr++; rIndex--; }

        return *moveItr;
    }

    if (maxAttackPriority >= MOVE_PRIORITIES::URGENT && maxDefencePriority < MOVE_PRIORITIES::HIGH) {
        auto rIndex = rand() % attackPriorities[maxAttackPriority].size();
        auto moveItr = attackPriorities[maxAttackPriority].begin();
        while (rIndex > 0) { moveItr++; rIndex--; }

        return *moveItr;
    }

    if (maxDefencePriority >= MOVE_PRIORITIES::URGENT && maxAttackPriority < MOVE_PRIORITIES::HIGH) {
        auto rIndex = rand() % defencePriorities[maxDefencePriority].size();
        auto moveItr = defencePriorities[maxDefencePriority].begin();
        while (rIndex > 0) { moveItr++; rIndex--; }

        return *moveItr;
    }

    short attackDefenceRoll = rand() % 100;
    if (maxDefencePriority >= MOVE_PRIORITIES::URGENT && maxAttackPriority >= MOVE_PRIORITIES::HIGH) {
        if (attackDefenceRoll < 50) {
            auto rIndex = rand() % defencePriorities[maxDefencePriority].size();
            auto moveItr = defencePriorities[maxDefencePriority].begin();
            while (rIndex > 0) { moveItr++; rIndex--; }

            return *moveItr;
        } else {
            auto rIndex = rand() % attackPriorities[maxAttackPriority].size();
            auto moveItr = attackPriorities[maxAttackPriority].begin();
            while (rIndex > 0) { moveItr++; rIndex--; }

            return *moveItr;
        }
    }

    short move = 0;
    if (attackDefenceRoll < 50) {
        short totalPriority = maxDefencePriority + minDefencePriority;
        short randomHit = totalPriority == 0 ? 0 : rand() % totalPriority;
        short selectedPriority = maxDefencePriority;
        for (int i = 0; i <= maxDefencePriority; ++i) {
            if (defencePriorities[i].empty()) {
                continue;;
            }

            if (randomHit <= i) {
                selectedPriority = i;
                break;
            }
        }

        auto rIndex = rand() % defencePriorities[selectedPriority].size();
        auto moveItr = defencePriorities[selectedPriority].begin();
        while (rIndex > 0) { moveItr++; rIndex--; }

        move = *moveItr;
    } else {
        short totalPriority = maxAttackPriority + minAttackPriority;
        short randomHit = totalPriority == 0 ? 0 : rand() % totalPriority;
        short selectedPriority = maxAttackPriority;
        for (int i = 0; i <= maxAttackPriority; ++i) {
            if (attackPriorities[i].empty()) {
                continue;;
            }

            if (randomHit <= i) {
                selectedPriority = i;
                break;
            }
        }

        auto rIndex = rand() % attackPriorities[selectedPriority].size();
        auto moveItr = attackPriorities[selectedPriority].begin();
        while (rIndex > 0) { moveItr++; rIndex--; }

        move = *moveItr;
    }

    return move;
}

std::vector<short> BitField::getBestMoves(short color) const {
    std::vector<short> result;

    std::array<std::vector<short>, MOVE_PRIORITIES::IMMIDIATE + 1> attackPriorities;
    std::array<std::vector<short>, MOVE_PRIORITIES::IMMIDIATE + 1> defencePriorities;
    short maxDefencePriority = 0, minDefencePriority = 1000;
    short maxAttackPriority = 0, minAttackPriority = 1000;
    for (auto& move : _availableMoves) {
        short attackPriority = getMovePriority(move, color);
        short defencePriority = getMoveDefencePriority(move, color);
        attackPriorities[attackPriority].push_back(move);
        defencePriorities[defencePriority].push_back(move);

        minDefencePriority = std::min(minDefencePriority, defencePriority);
        maxDefencePriority = std::max(maxDefencePriority, defencePriority);
        minAttackPriority = std::min(minAttackPriority, attackPriority);
        maxAttackPriority = std::max(maxAttackPriority, attackPriority);
    }

    if (maxAttackPriority >= MOVE_PRIORITIES::IMMIDIATE) {
        return attackPriorities[maxAttackPriority];
    }

    if (maxDefencePriority >= MOVE_PRIORITIES::IMMIDIATE) {
        return defencePriorities[maxDefencePriority];
    }

    if (maxAttackPriority >= MOVE_PRIORITIES::URGENT && maxDefencePriority < MOVE_PRIORITIES::HIGH) {
        return attackPriorities[maxAttackPriority];
    }

    if (maxDefencePriority >= MOVE_PRIORITIES::URGENT && maxAttackPriority < MOVE_PRIORITIES::HIGH) {
        return defencePriorities[maxDefencePriority];
    }

    if (maxDefencePriority >= MOVE_PRIORITIES::URGENT && maxAttackPriority >= MOVE_PRIORITIES::HIGH) {
        std::copy(attackPriorities[maxAttackPriority].begin(), attackPriorities[maxAttackPriority].end(), std::back_inserter(result));
        std::copy(defencePriorities[maxDefencePriority].begin(), defencePriorities[maxDefencePriority].end(), std::back_inserter(result));

        return result;
    }

    return _availableMoves;
}

bool BitField::makeMove(short x, short y, short color) {
    if (_gameStatus != 0) {
        return false;
    }

    Debug::getInstance().startTrack(DebugTimeTracks::MAKE_MOVE);

    Debug::getInstance().trackCall(DebugCallTracks::MAKE_MOVE);

    int colorShift = (color - 1) * BOARD_SIZE;
    long horizontal = 1 << x;
    long vertical = 1 << y;
    unsigned long fullBoard = (_horizontals[y] | _horizontals[y + BOARD_SIZE]);

    if (fullBoard & horizontal) {
        return false;
    }

    _horizontals[y + colorShift] = _horizontals[y + colorShift] | horizontal;
    _verticals[x + colorShift] = _verticals[x + colorShift] | vertical;

    auto leftDiagonal = getDiagonalLeftIndex(x, y);
    _diagonal_left[leftDiagonal + colorShift * 2] = _diagonal_left[leftDiagonal + colorShift * 2] | vertical;

    auto rightDiagonal = getDiagonalRightIndex(x, y);
    _diagonal_right[rightDiagonal + colorShift * 2] = _diagonal_right[rightDiagonal + colorShift * 2] | vertical;

    _history.push_back(std::make_tuple(x, y, color));

    if ((_horizontals[y] | _horizontals[y + BOARD_SIZE]) == FILLED_ROW_BITS) {
        _filledHorizontals = _filledHorizontals | (1 << y);
    }

    if (getConsecutiveBitsCount(_horizontals[y + colorShift]) >= MOVES_IN_ROW_TO_WIN) {
        _gameStatus = color;
        return true;
    }

    if (getConsecutiveBitsCount(_verticals[x + colorShift]) >= MOVES_IN_ROW_TO_WIN) {
        _gameStatus = color;
        return true;
    }

    if (getConsecutiveBitsCount(_diagonal_left[leftDiagonal + colorShift * 2]) >= MOVES_IN_ROW_TO_WIN) {
        _gameStatus = color;
        return true;
    }

    if (getConsecutiveBitsCount(_diagonal_right[rightDiagonal + colorShift * 2]) >= MOVES_IN_ROW_TO_WIN) {
        _gameStatus = color;
        return true;
    }

    if (_filledHorizontals == FILLED_ROW_BITS) {
        _gameStatus = -1;
        return true;
    }

    Debug::getInstance().startTrack(DebugTimeTracks::INCREMENTAL_UPDATE);
    incrementalUpdate(x, y, color);
    Debug::getInstance().stopTrack(DebugTimeTracks::INCREMENTAL_UPDATE);

    Debug::getInstance().stopTrack(DebugTimeTracks::MAKE_MOVE);
    return true;
}

bool BitField::unmakeMove() {
    if (_history.empty()) {
        return false;
    }

    auto move = _history.back();
    _history.pop_back();

    short x = std::get<0>(move);
    short y = std::get<1>(move);
    short color = std::get<2>(move);

    int colorShift = (color -1) * BOARD_SIZE;
    long horizontal = ~(1 << x);
    long vertical = ~(1 << y);

    _horizontals[y + colorShift] = _horizontals[y + colorShift] & horizontal;
    _verticals[x + colorShift] = _verticals[x + colorShift] & vertical;

    auto leftDiagonal = getDiagonalLeftIndex(x, y);
    _diagonal_left[leftDiagonal + colorShift * 2] = _diagonal_left[leftDiagonal + colorShift * 2] & vertical;

    auto rightDiagonal = getDiagonalRightIndex(x, y);
    _diagonal_right[rightDiagonal + colorShift * 2] = _diagonal_right[rightDiagonal + colorShift * 2] & vertical;

    return true;
}

void BitField::incrementalUpdate(short x, short y, short color) {
    Debug::getInstance().startTrack(DebugTimeTracks::ERASE_AVAILABLE_MOVES);
    auto moveHash = getMoveHash(x, y);
    if (_availableMovesHash.test(moveHash)) {
        _availableMovesHash.flip(moveHash);

        auto it =  std::find(_availableMoves.begin(), _availableMoves.end(), moveHash);
        if (it != _availableMoves.end()) {
            _availableMoves.erase(it);
        }
    }
    Debug::getInstance().stopTrack(DebugTimeTracks::ERASE_AVAILABLE_MOVES);

    Debug::getInstance().startTrack(DebugTimeTracks::CLEAR_TEMPLATES);
    // clear defensive moves
    int priorityShift = (color - 1) * BOARD_LENGTH;
    short parentDefensiveHash = moveHash + priorityShift;
    for (auto patternIt = _defensiveMovesPriority[parentDefensiveHash].patterns.begin(); patternIt != _defensiveMovesPriority[parentDefensiveHash].patterns.end();) {
        long pattern = *patternIt;
        short patternId = (pattern & PATTERN_ID_MASK);
        short patternDirection = (pattern & DIRECTION_ID_MASK) >> DIRECTION_ID_SHIFT;
        short attackIndex = (pattern & ATTACK_ID_MASK) >> ATTACK_ID_SHIFT;
        short currentShift = MOVE_PATTERNS[patternId].defence[attackIndex];

//        qDebug() << "Deleting pattern " << pattern << patternId;
        bool shouldDeleteKey = false;
        for (int i = 0; i < AI_PATTERN_DEFENCES_COUNT; ++i) {
            if (MOVE_PATTERNS[patternId].defence[i] == 0 && i > 0) {
//                qDebug() << "break" << patternId << i;
                break;
            }

            short shift = MOVE_PATTERNS[patternId].defence[i];
            short resetShift = shift - currentShift;

            short resetX = x;
            short resetY = y;

            if (patternDirection == 0) {
                resetX += resetShift;
            } else if (patternDirection == 1) {
                resetY += resetShift;
            } else if (patternDirection == 2) {
                resetX += resetShift;
                resetY += resetShift;
            } else {
                resetX -= resetShift;
                resetY += resetShift;
            }

            if (resetX < 0 || resetY < 0 || resetX >= BOARD_SIZE || resetY >= BOARD_SIZE) {
//                qDebug() << "out bounds to delete " << resetX << resetY << pattern << i << currentShift << shift;
                continue;
            }

            short resetHash = getMoveHash(resetX, resetY);
            long resetPatternHash = (pattern & (PATTERN_ID_MASK | MIAI_ID_MASK | DIRECTION_ID_MASK)) | (i << ATTACK_ID_SHIFT);
            short key = resetHash + priorityShift;
            auto it = std::find_if(_defensiveMovesPriority[key].patterns.begin(), _defensiveMovesPriority[key].patterns.end(), [resetPatternHash](auto& value) {
                return (value & DIRECTION_ID_MASK) == (resetPatternHash & DIRECTION_ID_MASK) && (value & MIAI_ID_MASK) == (resetPatternHash & MIAI_ID_MASK);
            });
            if (it == _defensiveMovesPriority[key].patterns.end()) {
//                qDebug() << "failed to delete " << resetHash << resetX << resetY << pattern << resetPatternHash << currentShift << shift;
                continue;
            }

//            qDebug() << "deleted " << resetHash << resetX << resetY << pattern << resetPatternHash << currentShift << shift;
            if (key != parentDefensiveHash) _defensiveMovesPriority[key].patterns.erase(it);
            else shouldDeleteKey = true;
//            qDebug() << "erase pattern from " << resetHash;

            unsigned maxPriority = 0;
            for (auto& resetPattern : _defensiveMovesPriority[key].patterns) {
                short moveId = (resetPattern & PATTERN_ID_MASK);
                maxPriority = std::max(MOVE_PATTERNS[moveId].attackPriority, maxPriority);
            }
            _defensiveMovesPriority[key].priority = maxPriority;
//            qDebug() << "new pattern priority from " << resetHash << maxPriority;
        }

        if (shouldDeleteKey) patternIt = _defensiveMovesPriority[parentDefensiveHash].patterns.erase(patternIt);
        else ++patternIt;
    }

    Debug::getInstance().stopTrack(DebugTimeTracks::CLEAR_TEMPLATES);

    Debug::getInstance().startTrack(DebugTimeTracks::ADD_NEW_MOVES);
    constexpr std::array<std::pair<short, short>, 8> DIRECTIONS = {{
        {1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {-1, -1}, {1, -1}, {-1, 1}
    }};

    std::vector<short> generatedMoves;
    for (const auto& dir : DIRECTIONS) {
        for (int jump = 1; jump <= 2; ++jump) {
            short newX = x + dir.first * jump;
            short newY = y + dir.second * jump;
            if (newX < 0 || newY < 0 || newX >= BOARD_SIZE || newY >= BOARD_SIZE) {
                continue;
            }

            generatedMoves.clear();

            auto newMoveHash = getMoveHash(newX, newY);
            unsigned long fullBoard = (_horizontals[newY] | _horizontals[newY + BOARD_SIZE]);
            bool isPositionEmpty = (fullBoard & (1 << newX)) == 0;
            if (isPositionEmpty) {
                updateMovePriority(newX, newY, generatedMoves);
            } else if (jump == 2) {
                for (int i = jump + 1; i <= MOVES_IN_ROW_TO_WIN + jump + 1; ++i) {
                    short raycastX = x + dir.first * i;
                    short raycastY = y + dir.second * i;
                    if (raycastX < 0 || raycastY < 0 || raycastX >= BOARD_SIZE || raycastY >= BOARD_SIZE) {
                        break;
                    }

                    unsigned long raycastFullBoard = (_horizontals[raycastY] | _horizontals[raycastY + BOARD_SIZE]);
                    if ((raycastFullBoard & (1 << raycastX)) != 0) {
                        continue;
                    }

                    updateMovePriority(raycastX, raycastY, generatedMoves);
                    break;
                }
            }

            if (jump == 2 && generatedMoves.empty()) {
                continue;
            }

            if (isPositionEmpty) {
                generatedMoves.push_back(newMoveHash);
            }

            for (auto& move : generatedMoves) {
                if (_availableMovesHash.test(move)) {
                    continue;
                }

                unsigned long fullBoard2 = (_horizontals[extractHashedPositionY(move)] |_horizontals[extractHashedPositionY(move) + BOARD_SIZE]);
                bool isPositionEmpty2 = (fullBoard2 & (1 << extractHashedPositionX(move))) == 0;
                if (!isPositionEmpty2) {
                    continue;
                }

                _availableMovesHash.set(move);
                _availableMoves.push_back(move);
            }
        }
    }

    Debug::getInstance().stopTrack(DebugTimeTracks::ADD_NEW_MOVES);
}

void BitField::createTemplate(short x, short y, short colorShift, short patternId, short attackId, short directionId, short miaiId, short priority, std::vector<short>& newMoves) {
    if (x < 0 || y < 0 || x >= BOARD_SIZE || y >= BOARD_SIZE) {
        return;
    }

    Debug::getInstance().startTrack(DebugTimeTracks::CREATE_TEMPLATE);
    Debug::getInstance().trackCall(DebugCallTracks::CREATE_TEMPLATE);

    short defenceMove = getHashedPosition(x, y);
    short key = defenceMove + colorShift;
    unsigned long patternValue = getPackedPriority(patternId, attackId, directionId, miaiId);
    if (std::find_if(_defensiveMovesPriority[key].patterns.begin(), _defensiveMovesPriority[key].patterns.end(), [patternValue](auto& value) {
        return (value & DIRECTION_ID_MASK) == (patternValue & DIRECTION_ID_MASK) && (value & MIAI_ID_MASK) == (patternValue & MIAI_ID_MASK);
    }) != _defensiveMovesPriority[key].patterns.end()) {
        return;
    }
    _defensiveMovesPriority[key].patterns.push_back(patternValue);
    _defensiveMovesPriority[key].updatePriorityFast(priority);
    newMoves.push_back(defenceMove);

    Debug::getInstance().stopTrack(DebugTimeTracks::CREATE_TEMPLATE);

//    qDebug() << "add defence pattern " << colorShift << x << y << defenceMove << priority << patternId << attackId << patternValue;
}

void BitField::updateMovePriority(short x, short y, std::vector<short>& newMoves) {
    Debug::getInstance().startTrack(DebugTimeTracks::UPDATE_TEMPLATES);
    Debug::getInstance().trackCall(DebugCallTracks::UPDATE_TEMPLATES);

    int blackPriorityShift = (BLACK_PIECE_COLOR - 1) * BOARD_LENGTH;
    int whitePriorityShift = (WHITE_PIECE_COLOR - 1) * BOARD_LENGTH;

    int blackColorShift = (BLACK_PIECE_COLOR - 1) * BOARD_SIZE;
    int whiteColorShift = (WHITE_PIECE_COLOR - 1) * BOARD_SIZE;

    auto moveHash = getMoveHash(x, y);

    unsigned blackPriority = 0;
    unsigned whitePriority = 0;

    short updatedPatterns = 0;

    static constexpr short BLACK_HORIZONTAL = 1;
    static constexpr short WHITE_HORIZONTAL = 2;
    static constexpr short BLACK_VERTICAL = 4;
    static constexpr short WHITE_VERTICAL = 8;
    static constexpr short BLACK_DIAGONAL_LEFT = 16;
    static constexpr short WHITE_DIAGONAL_LEFT = 32;
    static constexpr short BLACK_DIAGONAL_RIGHT = 64;
    static constexpr short WHITE_DIAGONAL_RIGHT = 128;
    static constexpr short FULL_PATTERN_MASK = 255;

    for (const auto& pattern : MOVE_PATTERNS) {

        Debug::getInstance().trackCall(DebugCallTracks::UPDATE_TEMPLATES_INLINE);

        bool fitHorizontal = (x + pattern.patternShift) >= 0 && (x + pattern.emptyShift) >= 0;
        bool fitVertical = (y + pattern.patternShift) >= 0 && (y + pattern.emptyShift) >= 0;

        long stringValueBlack = 0;
        long stringValueWhite = 0;
        long emptyStringValue = 0;

        long shiftedPattern = 0;
        long shiftedEmptyPattern = 0;
        long shiftedEnemyPattern = 0;

        if (fitHorizontal) {
            stringValueBlack = _horizontals[y + blackColorShift];
            stringValueWhite = _horizontals[y + whiteColorShift];

            emptyStringValue = ~(stringValueBlack | stringValueWhite);

            shiftedPattern = pattern.pattern << (x + pattern.patternShift);
            shiftedEmptyPattern = pattern.emptyPattern << (x + pattern.emptyShift);
            shiftedEnemyPattern = pattern.enemyPattern << (x + pattern.enemyShift);

            if (!(updatedPatterns & BLACK_HORIZONTAL) && (shiftedPattern & stringValueBlack) == shiftedPattern && (emptyStringValue & shiftedEmptyPattern) == shiftedEmptyPattern && (shiftedEnemyPattern & stringValueWhite) == shiftedEnemyPattern) {
                blackPriority = blackPriority < pattern.attackPriority ? pattern.attackPriority : blackPriority;

                updatedPatterns = updatedPatterns | BLACK_HORIZONTAL;
                for (int i = 0; i < AI_PATTERN_DEFENCES_COUNT; ++i) {
                    if (pattern.defence[i] == 0 && i > 0) {
                        break;
                    }
                    short defX = x + pattern.defence[i], defY = y;
                    if (defX < 0 || defY < 0 || defX >= BOARD_SIZE || defY >= BOARD_SIZE) {
                        continue;
                    }

                    createTemplate(defX, defY, whitePriorityShift, pattern.index, i, 0, pattern.miaiId, pattern.attackPriority, newMoves);
                }
            }
            if (!(updatedPatterns & WHITE_HORIZONTAL) && (shiftedPattern & stringValueWhite) == shiftedPattern && (emptyStringValue & shiftedEmptyPattern) == shiftedEmptyPattern && (shiftedEnemyPattern & stringValueBlack) == shiftedEnemyPattern) {
                whitePriority = whitePriority < pattern.attackPriority ? pattern.attackPriority : whitePriority;

                updatedPatterns = updatedPatterns | WHITE_HORIZONTAL;
                for (int i = 0; i < AI_PATTERN_DEFENCES_COUNT; ++i) {
                    if (pattern.defence[i] == 0 && i > 0) {
                        break;
                    }
                    short defX = x + pattern.defence[i], defY = y;
                    if (defX < 0 || defY < 0 || defX >= BOARD_SIZE || defY >= BOARD_SIZE) {
                        continue;
                    }

                    createTemplate(defX, defY, blackPriorityShift, pattern.index, i, 0, pattern.miaiId, pattern.attackPriority, newMoves);
                }
            }
        }

        if (fitVertical) {
            stringValueBlack = _verticals[x + blackColorShift];
            stringValueWhite = _verticals[x + whiteColorShift];

            emptyStringValue = ~(stringValueBlack | stringValueWhite);

            shiftedPattern = pattern.pattern << (y + pattern.patternShift);
            shiftedEmptyPattern = pattern.emptyPattern << (y + pattern.emptyShift);
            shiftedEnemyPattern = pattern.enemyPattern << (y + pattern.enemyShift);

            if (!(updatedPatterns & BLACK_VERTICAL) && (shiftedPattern & stringValueBlack) == shiftedPattern && (emptyStringValue & shiftedEmptyPattern) == shiftedEmptyPattern && (shiftedEnemyPattern & stringValueWhite) == shiftedEnemyPattern) {
                blackPriority = blackPriority < pattern.attackPriority ? pattern.attackPriority : blackPriority;

                updatedPatterns = updatedPatterns | BLACK_VERTICAL;
                for (int i = 0; i < AI_PATTERN_DEFENCES_COUNT; ++i) {
                    if (pattern.defence[i] == 0 && i > 0) {
                        break;
                    }
                    short defX = x, defY = y + pattern.defence[i];
                    if (defX < 0 || defY < 0 || defX >= BOARD_SIZE || defY >= BOARD_SIZE) {
                        continue;
                    }

                    createTemplate(defX, defY, whitePriorityShift, pattern.index, i, 1, pattern.miaiId, pattern.attackPriority, newMoves);
                }
            }
            if (!(updatedPatterns & WHITE_VERTICAL) && (shiftedPattern & stringValueWhite) == shiftedPattern && (emptyStringValue & shiftedEmptyPattern) == shiftedEmptyPattern && (shiftedEnemyPattern & stringValueBlack) == shiftedEnemyPattern) {
                whitePriority = whitePriority < pattern.attackPriority ? pattern.attackPriority : whitePriority;

                updatedPatterns = updatedPatterns | WHITE_VERTICAL;
                for (int i = 0; i < AI_PATTERN_DEFENCES_COUNT; ++i) {
                    if (pattern.defence[i] == 0 && i > 0) {
                        break;
                    }
                    short defX = x, defY = y + pattern.defence[i];
                    if (defX < 0 || defY < 0 || defX >= BOARD_SIZE || defY >= BOARD_SIZE) {
                        continue;
                    }

                    createTemplate(defX, defY, blackPriorityShift, pattern.index, i, 1, pattern.miaiId, pattern.attackPriority, newMoves);
                }
            }
        }

        if (fitVertical) {
            auto leftDiagonal = getDiagonalLeftIndex(x, y);

            stringValueBlack = _diagonal_left[leftDiagonal + blackColorShift * 2];
            stringValueWhite = _diagonal_left[leftDiagonal + whiteColorShift * 2];

            emptyStringValue = ~(stringValueBlack | stringValueWhite);

            shiftedPattern = pattern.pattern << (y + pattern.patternShift);
            shiftedEmptyPattern = pattern.emptyPattern << (y + pattern.emptyShift);
            shiftedEnemyPattern = pattern.enemyPattern << (y + pattern.enemyShift);

            if (!(updatedPatterns & BLACK_DIAGONAL_LEFT) && (shiftedPattern & stringValueBlack) == shiftedPattern && (emptyStringValue & shiftedEmptyPattern) == shiftedEmptyPattern && (shiftedEnemyPattern & stringValueWhite) == shiftedEnemyPattern) {
                blackPriority = blackPriority < pattern.attackPriority ? pattern.attackPriority : blackPriority;

                updatedPatterns = updatedPatterns | BLACK_DIAGONAL_LEFT;
                for (int i = 0; i < AI_PATTERN_DEFENCES_COUNT; ++i) {
                    if (pattern.defence[i] == 0 && i > 0) {
                        break;
                    }
                    short defX = x + pattern.defence[i], defY = y + pattern.defence[i];
                    if (defX < 0 || defY < 0 || defX >= BOARD_SIZE || defY >= BOARD_SIZE) {
                        continue;
                    }

                    createTemplate(defX, defY, whitePriorityShift, pattern.index, i, 2, pattern.miaiId, pattern.attackPriority, newMoves);
                }
            }
            if (!(updatedPatterns & WHITE_DIAGONAL_LEFT) && (shiftedPattern & stringValueWhite) == shiftedPattern && (emptyStringValue & shiftedEmptyPattern) == shiftedEmptyPattern && (shiftedEnemyPattern & stringValueBlack) == shiftedEnemyPattern) {
                whitePriority = whitePriority < pattern.attackPriority ? pattern.attackPriority : whitePriority;

                updatedPatterns = updatedPatterns | WHITE_DIAGONAL_LEFT;
                for (int i = 0; i < AI_PATTERN_DEFENCES_COUNT; ++i) {
                    if (pattern.defence[i] == 0 && i > 0) {
                        break;
                    }
                    short defX = x + pattern.defence[i], defY = y + pattern.defence[i];
                    if (defX < 0 || defY < 0 || defX >= BOARD_SIZE || defY >= BOARD_SIZE) {
                        continue;
                    }

                    createTemplate(defX, defY, blackPriorityShift, pattern.index, i, 2, pattern.miaiId, pattern.attackPriority, newMoves);
                }
            }
        }

        if (fitVertical) {
            auto rightDiagonal = getDiagonalRightIndex(x, y);

            stringValueBlack = _diagonal_right[rightDiagonal + blackColorShift * 2];
            stringValueWhite = _diagonal_right[rightDiagonal + whiteColorShift * 2];

            emptyStringValue = ~(stringValueBlack | stringValueWhite);

            shiftedPattern = pattern.pattern << (y + pattern.patternShift);
            shiftedEmptyPattern = pattern.emptyPattern << (y + pattern.emptyShift);
            shiftedEnemyPattern = pattern.enemyPattern << (y + pattern.enemyShift);

            if (!(updatedPatterns & BLACK_DIAGONAL_RIGHT) && (shiftedPattern & stringValueBlack) == shiftedPattern && (emptyStringValue & shiftedEmptyPattern) == shiftedEmptyPattern && (shiftedEnemyPattern & stringValueWhite) == shiftedEnemyPattern) {
                blackPriority = blackPriority < pattern.attackPriority ? pattern.attackPriority : blackPriority;

                updatedPatterns = updatedPatterns | BLACK_DIAGONAL_RIGHT;
                for (int i = 0; i < AI_PATTERN_DEFENCES_COUNT; ++i) {
                    if (pattern.defence[i] == 0 && i > 0) {
                        break;
                    }
                    short defX = x - pattern.defence[i], defY = y + pattern.defence[i];
                    if (defX < 0 || defY < 0 || defX >= BOARD_SIZE || defY >= BOARD_SIZE) {
                        continue;
                    }

                    createTemplate(defX, defY, whitePriorityShift, pattern.index, i, 3, pattern.miaiId, pattern.attackPriority, newMoves);
                }
            }
            if (!(updatedPatterns & WHITE_DIAGONAL_RIGHT) && (shiftedPattern & stringValueWhite) == shiftedPattern && (emptyStringValue & shiftedEmptyPattern) == shiftedEmptyPattern && (shiftedEnemyPattern & stringValueBlack) == shiftedEnemyPattern) {
                whitePriority = whitePriority < pattern.attackPriority ? pattern.attackPriority : whitePriority;

                updatedPatterns = updatedPatterns | WHITE_DIAGONAL_RIGHT;
                for (int i = 0; i < AI_PATTERN_DEFENCES_COUNT; ++i) {
                    if (pattern.defence[i] == 0 && i > 0) {
                        break;
                    }
                    short defX = x - pattern.defence[i], defY = y + pattern.defence[i];
                    if (defX < 0 || defY < 0 || defX >= BOARD_SIZE || defY >= BOARD_SIZE) {
                        continue;
                    }

                    createTemplate(defX, defY, blackPriorityShift, pattern.index, i, 3, pattern.miaiId, pattern.attackPriority, newMoves);
                }
            }
        }

        if (updatedPatterns & FULL_PATTERN_MASK) {
            break;
        }
    }

    _attackingMovesPriority[moveHash + blackPriorityShift] = blackPriority;
    _attackingMovesPriority[moveHash + whitePriorityShift] = whitePriority;

    Debug::getInstance().stopTrack(DebugTimeTracks::UPDATE_TEMPLATES);
}

#include "aidomainknowledge.h"
#include <QDebug>

AIDomainKnowledge::AIDomainKnowledge()
{
}

AIDomainKnowledge::AIDomainKnowledge(const AIDomainKnowledge& domainKnowledge) {
    _domain = domainKnowledge._domain;
}

bool AIDomainKnowledge::canMoveInDirection(short position, SearchDirection direction, bool moveBackwards) {
    if (position < 0 || position >= BOARD_LENGTH) {
        return false;
    }

    if (direction == SearchDirection::HORIZONTAL) {
        return moveBackwards ? BOARD_DISTANCE_RIGHT[position] > 0 : BOARD_DISTANCE_LEFT[position] > 0;
    }
    if (direction == SearchDirection::VERTICAL) {
        return moveBackwards ? BOARD_DISTANCE_BOTTOM[position] > 0 : BOARD_DISTANCE_TOP[position] > 0;
    }
    if (direction == SearchDirection::DIAGONAL) {
        return moveBackwards ? BOARD_DISTANCE_BOTTOM[position] > 0 && BOARD_DISTANCE_RIGHT[position] > 0 : BOARD_DISTANCE_TOP[position] > 0 && BOARD_DISTANCE_LEFT[position] > 0;
    }
    if (direction == SearchDirection::DIAGONAL_REVERSE) {
        return moveBackwards ? BOARD_DISTANCE_BOTTOM[position] > 0 && BOARD_DISTANCE_LEFT[position] > 0 : BOARD_DISTANCE_TOP[position] > 0 && BOARD_DISTANCE_RIGHT[position] > 0;
    }

    return false;
}

short AIDomainKnowledge::getNextMoveInDirection(short position, AIDomainKnowledge::SearchDirection direction, short step, bool moveBackwards) {
    if (step == 0) {
        return position;
    }

    if (direction == SearchDirection::HORIZONTAL) {
        short offset = FieldMove::MIN_X_VALUE * step;
        short nextPosition = position + (moveBackwards ? -offset : offset);
        return nextPosition;
    }

    if (direction == SearchDirection::VERTICAL) {
        short offset = FieldMove::MIN_Y_VALUE * step;
        short nextPosition = position + (moveBackwards ? -offset : offset);
        return nextPosition;
    }

    if (direction == SearchDirection::DIAGONAL) {
        short offsetX = FieldMove::MIN_X_VALUE * step;
        short offsetY = FieldMove::MIN_Y_VALUE * step;
        short nextPosition = position + (moveBackwards ? -offsetX - offsetY : offsetX + offsetY);
        return nextPosition;
    }

    if (direction == SearchDirection::DIAGONAL_REVERSE) {
        short offsetX = FieldMove::MIN_X_VALUE * step;
        short offsetY = FieldMove::MIN_Y_VALUE * step;
        short nextPosition = position + (moveBackwards ? offsetX - offsetY : -offsetX + offsetY);
        return nextPosition;
    }

    return position;
}

AIMovePattern AIDomainKnowledge::generatePattern(IField* field, short move, short color, AIDomainKnowledge::SearchDirection direction) {
    const std::array<short, BOARD_LENGTH>& board = field->getBoardState();
    short blockColor = FieldMove::getNextColor(color);

    short leftIndex = 0, rightIndex = 0;
    short pattern = 1, blankPattern = 0;
    while (canMoveInDirection(getNextMoveInDirection(move, direction, leftIndex, false), direction, false)) {
        short nextPosition = getNextMoveInDirection(move, direction, leftIndex + 1, false);
        if (board[nextPosition] != color) {
            break;
        }

        leftIndex++;
        pattern = pattern << 1;
        pattern++;
    }
    while (canMoveInDirection(getNextMoveInDirection(move, direction, rightIndex, true), direction, true)) {
        short nextPosition = getNextMoveInDirection(move, direction, rightIndex + 1, true);
        if (board[nextPosition] != color) {
            break;
        }

        rightIndex++;
        pattern = pattern << 1;
        pattern++;
    }

    short sequence = rightIndex + leftIndex + 1;

    short counter = 1;
    short leftPosition = getNextMoveInDirection(move, direction, leftIndex, false);
    short blankOffset = 0;
    short additionalPattern = 0;
    short patternLeftOffset = 0;
    while (sequence < 4 && canMoveInDirection(getNextMoveInDirection(leftPosition, direction, counter - 1, false), direction, false)) {
        short nextPosition = getNextMoveInDirection(leftPosition, direction, counter, false);
        short nextColor = board[nextPosition];
        if (nextColor == blockColor) {
            break;
        }

        if (nextColor == 0) {
            if (additionalPattern > 0) {
                break;
            }
            if (additionalPattern == 0 && counter == 2) {
                break;
            }
            patternLeftOffset++;
        } else if (nextColor == color) {
            if (additionalPattern == 0) {
                blankOffset += (counter - 1);
            }

            additionalPattern = additionalPattern << 1;
            additionalPattern++;
            patternLeftOffset++;

            if (counter >= (5 - sequence)) {
                break;
            }
        }

        counter++;
    }

    if (additionalPattern > 0) pattern = pattern + (additionalPattern << (sequence + blankOffset));
    if (additionalPattern > 0) leftIndex += patternLeftOffset;

    counter = 1;
    blankOffset = 0;
    additionalPattern = 0;
    short patternRightOffset = 0;
    short additionalPatternCounter = 0;
    short rightPosition = getNextMoveInDirection(move, direction, rightIndex, true);
    while (sequence < 4 && canMoveInDirection(getNextMoveInDirection(rightPosition, direction, counter - 1, true), direction, true)) {
        short nextPosition = getNextMoveInDirection(rightPosition, direction, counter, true);
        short nextColor = board[nextPosition];
        if (nextColor == blockColor) {
            break;
        }

        if (nextColor == 0) {
            if (additionalPattern > 0) {
                break;
            }
            if (additionalPattern == 0 && counter == 2) {
                break;
            }
            patternRightOffset++;
        } else if (nextColor == color) {
            if (additionalPattern == 0) {
                blankOffset += (counter - 1);
            }

            additionalPattern = additionalPattern << 1;
            additionalPattern++;
            additionalPatternCounter++;
            patternRightOffset++;

            if (counter >= (5 - sequence)) {
                break;
            }
        }

        counter++;
    }

    if (additionalPattern > 0) pattern = (pattern << (blankOffset + additionalPatternCounter)) + additionalPattern;
    if (additionalPattern > 0) rightIndex += patternRightOffset;

    blankOffset = 0;
    counter = 1;
    short leftBlank = 0, rightBlank = 0;
    while (canMoveInDirection(getNextMoveInDirection(move, direction, leftIndex + counter - 1, false), direction, false) && counter <= 2) {
        short nextPosition = getNextMoveInDirection(move, direction, leftIndex + counter, false);
        if (board[nextPosition] != 0) {
            break;
        }

        leftBlank = leftBlank << 1;
        leftBlank++;
        counter++;
    }

    counter = 1;
    while (canMoveInDirection(getNextMoveInDirection(move, direction, rightIndex + counter - 1, true), direction, true) && counter <= 2) {
        short nextPosition = getNextMoveInDirection(move, direction, rightIndex + counter, true);
        if (board[nextPosition] != 0) {
            break;
        }

        rightBlank = rightBlank << 1;
        rightBlank++;
        blankOffset++;
        counter++;
    }

    blankPattern = (leftBlank << (leftIndex + rightIndex + 1 + blankOffset)) + rightBlank;

    AIMovePattern movePattern;
    movePattern.patternStart = rightIndex;
    movePattern.patternEnd = leftIndex;
    movePattern.pattern = pattern;
    movePattern.blankPattern = blankPattern;

    return movePattern;
}

void AIDomainKnowledge::generateAttackMoves(IField* field, short move, short color, AIPatternsStore& store) {
    std::vector<AIMovePattern> patterns;
    patterns.push_back(generatePattern(field, move, color, SearchDirection::HORIZONTAL));
    patterns.push_back(generatePattern(field, move, color, SearchDirection::VERTICAL));
    patterns.push_back(generatePattern(field, move, color, SearchDirection::DIAGONAL));
    patterns.push_back(generatePattern(field, move, color, SearchDirection::DIAGONAL_REVERSE));
    //========================
    // must to defend patterns

    int index = -1;
    static constexpr std::array<SearchDirection, 4> directions {SearchDirection::HORIZONTAL, SearchDirection::VERTICAL, SearchDirection::DIAGONAL, SearchDirection::DIAGONAL_REVERSE};
    for (auto& pattern : patterns) {
        index++;
        // 11011 - pattern
        if (pattern.pattern == 27) {
            short attack = getNextMoveInDirection(move, directions[index], pattern.patternStart - 2, true);
            AITacticalPattern attackPattern;
            attackPattern.type = AITacticalPatternType::CRITICAL_ATTACK;
            attackPattern.moves.insert(attack);
            store.patterns[AIPatternsStore::CRITICAL_ATTACK].push_back(attackPattern);
        }

        // 10111 - pattern
        if (pattern.pattern == 23) {
            short attack = getNextMoveInDirection(move, directions[index], pattern.patternEnd - 1, false);
            AITacticalPattern attackPattern;
            attackPattern.type = AITacticalPatternType::CRITICAL_ATTACK;
            attackPattern.moves.insert(attack);
            store.patterns[AIPatternsStore::CRITICAL_ATTACK].push_back(attackPattern);
        }

        // 11101 - pattern
        if (pattern.pattern == 29) {
            short attack = getNextMoveInDirection(move, directions[index], pattern.patternStart - 1, true);
            AITacticalPattern attackPattern;
            attackPattern.type = AITacticalPatternType::CRITICAL_ATTACK;
            attackPattern.moves.insert(attack);
            store.patterns[AIPatternsStore::CRITICAL_ATTACK].push_back(attackPattern);
        }

        // 1111 - pattern
        if (pattern.pattern == 15) {
            // have nothing at right
            // pattern is 1111 and blank is xxxx11
            AITacticalPattern attackPattern;
            attackPattern.type = AITacticalPatternType::CRITICAL_ATTACK;

            if ((pattern.blankPattern & 3) > 0) {
                short attack = getNextMoveInDirection(move, directions[index], pattern.patternStart + 1, true);
                attackPattern.moves.insert(attack);
            }

            if ((pattern.blankPattern & 16) > 0) {
                short attack = getNextMoveInDirection(move, directions[index], pattern.patternEnd + 1, false);
                attackPattern.moves.insert(attack);
            }

            if (!attackPattern.moves.empty()) {
                store.patterns[AIPatternsStore::CRITICAL_ATTACK].push_back(attackPattern);
            }
        }
        // 111 - pattern with blank pattern
        if (pattern.pattern == 7) {
            // blank patterns 1100011 - 99, 110001 - 49
            AITacticalPattern attackPattern7;
            attackPattern7.type = AITacticalPatternType::VITAL_ATTACK;

            if (pattern.blankPattern == 99 || pattern.blankPattern == 49) {
                short attack = getNextMoveInDirection(move, directions[index], pattern.patternEnd + 1, false);
                attackPattern7.moves.insert(attack);
            }

            // blank patterns 1100011 - 99, 100011 - 35
            if (pattern.blankPattern == 99 || pattern.blankPattern == 35) {
                short attack = getNextMoveInDirection(move, directions[index], pattern.patternStart + 1, true);
                attackPattern7.moves.insert(attack);
            }

            if (!attackPattern7.moves.empty()) {
                store.patterns[AIPatternsStore::VITAL_ATTACK].push_back(attackPattern7);
            }

            // tactical move with pattern 111 and blank pattern 11000
            if (pattern.blankPattern == 24) {
                AITacticalPattern attackPattern;
                attackPattern.type = AITacticalPatternType::DEVELOPMENT_ATTACK;

                short attack = getNextMoveInDirection(move, directions[index], pattern.patternEnd + 1, false);
                attackPattern.moves.insert(attack);

                short secondAttack = getNextMoveInDirection(move, directions[index], pattern.patternEnd + 2, false);
                attackPattern.moves.insert(secondAttack);

                store.patterns[AIPatternsStore::DEVELOPMENT_ATTACK].push_back(attackPattern);
            }

            // tactical move with pattern 111 and blank pattern 00011
            if (pattern.blankPattern == 3) {
                AITacticalPattern attackPattern;
                attackPattern.type = AITacticalPatternType::DEVELOPMENT_ATTACK;

                short attack = getNextMoveInDirection(move, directions[index], pattern.patternStart + 1, true);
                attackPattern.moves.insert(attack);

                short secondAttack = getNextMoveInDirection(move, directions[index], pattern.patternStart + 2, true);
                attackPattern.moves.insert(secondAttack);

                store.patterns[AIPatternsStore::DEVELOPMENT_ATTACK].push_back(attackPattern);
            }
        }

        // 1011 - pattern with blank pattern
        if (pattern.pattern == 11) {
            if (pattern.blankPattern == 33 || pattern.blankPattern == 67 || pattern.blankPattern == 97 || pattern.blankPattern == 195) {
                AITacticalPattern attackPattern;
                attackPattern.type = AITacticalPatternType::VITAL_ATTACK;

                short attack = getNextMoveInDirection(move, directions[index], pattern.patternEnd - 1, false);
                attackPattern.moves.insert(attack);

                store.patterns[AIPatternsStore::VITAL_ATTACK].push_back(attackPattern);
            }
        }

        // 1101 - pattern with blank pattern
        if (pattern.pattern == 13) {
            if (pattern.blankPattern == 33 || pattern.blankPattern == 67 || pattern.blankPattern == 97 || pattern.blankPattern == 195) {
                AITacticalPattern attackPattern;
                attackPattern.type = AITacticalPatternType::VITAL_ATTACK;

                short attack = getNextMoveInDirection(move, directions[index], pattern.patternStart - 1, true);
                attackPattern.moves.insert(attack);

                store.patterns[AIPatternsStore::VITAL_ATTACK].push_back(attackPattern);
            }
        }

        // 11 - tactical pattern
        if (pattern.pattern == 3) {
            // blank - 11001 or 110011
            AITacticalPattern attackPattern;
            attackPattern.type = AITacticalPatternType::DEVELOPMENT_ATTACK;

            if (pattern.blankPattern == 25 || pattern.blankPattern == 51) {
                short attack = getNextMoveInDirection(move, directions[index], pattern.patternEnd + 1, false);
                attackPattern.moves.insert(attack);

                attack = getNextMoveInDirection(move, directions[index], pattern.patternEnd + 2, false);
                attackPattern.moves.insert(attack);
            }

            // blank - 10011 or 110011
            if (pattern.blankPattern == 19 || pattern.blankPattern == 51) {
                short attack = getNextMoveInDirection(move, directions[index], pattern.patternStart + 1, true);
                attackPattern.moves.insert(attack);

                attack = getNextMoveInDirection(move, directions[index], pattern.patternStart + 2, true);
                attackPattern.moves.insert(attack);
            }

            if (!attackPattern.moves.empty()) {
                store.patterns[AIPatternsStore::DEVELOPMENT_ATTACK].push_back(attackPattern);
            }
        }

        // 101 - tactical pattern
        if (pattern.pattern == 5) {
            AITacticalPattern attackPattern;
            attackPattern.type = AITacticalPatternType::DEVELOPMENT_ATTACK;

            // blank - 1100011
            if (pattern.blankPattern == 99) {
                short attack = getNextMoveInDirection(move, directions[index], pattern.patternEnd + 1, false);
                attackPattern.moves.insert(attack);

                attack = getNextMoveInDirection(move, directions[index], pattern.patternEnd + 2, false);
                attackPattern.moves.insert(attack);

                attack = getNextMoveInDirection(move, directions[index], pattern.patternStart + 2, true);
                attackPattern.moves.insert(attack);

                attack = getNextMoveInDirection(move, directions[index], pattern.patternStart + 1, true);
                attackPattern.moves.insert(attack);

                attack = getNextMoveInDirection(move, directions[index], pattern.patternStart - 1, true);
                attackPattern.moves.insert(attack);
            }

            // blank - 110001
            if (pattern.blankPattern == 49) {
                short attack = getNextMoveInDirection(move, directions[index], pattern.patternEnd + 1, false);
                attackPattern.moves.insert(attack);

                attack = getNextMoveInDirection(move, directions[index], pattern.patternEnd + 2, false);
                attackPattern.moves.insert(attack);

                attack = getNextMoveInDirection(move, directions[index], pattern.patternStart - 1, true);
                attackPattern.moves.insert(attack);
            }

            // blank - 100011
            if (pattern.blankPattern == 35) {
                short attack = getNextMoveInDirection(move, directions[index], pattern.patternStart + 1, true);
                attackPattern.moves.insert(attack);

                attack = getNextMoveInDirection(move, directions[index], pattern.patternStart + 2, true);
                attackPattern.moves.insert(attack);

                attack = getNextMoveInDirection(move, directions[index], pattern.patternStart - 1, true);
                attackPattern.moves.insert(attack);
            }

            if (!attackPattern.moves.empty()) {
                store.patterns[AIPatternsStore::DEVELOPMENT_ATTACK].push_back(attackPattern);
            }
        }
    }

    std::unordered_map<short, std::vector<AITacticalPattern>> tacticalMap;
    for (auto& developmentPattern : store.patterns[AIPatternsStore::DEVELOPMENT_ATTACK]) {
        for (auto& move : developmentPattern.moves) {
            tacticalMap[move].push_back(developmentPattern);
        }
    }

    for (auto& tacticalPoint : tacticalMap) {
        if (tacticalPoint.second.size() >= 2) {
            AITacticalPattern attackPattern;
            attackPattern.type = AITacticalPatternType::VITAL_ATTACK;

            for (auto& pattern : tacticalPoint.second) {
                for (auto& move : pattern.moves) {
                    attackPattern.moves.insert(move);
                }
            }
            store.patterns[AIPatternsStore::VITAL_ATTACK].push_back(attackPattern);
        }
    }
}

void AIDomainKnowledge::generateDefensiveMoves(IField* field, short move, short color, AIPatternsStore& store) {
    std::vector<AIMovePattern> patterns;
    patterns.push_back(generatePattern(field, move, color, SearchDirection::HORIZONTAL));
    patterns.push_back(generatePattern(field, move, color, SearchDirection::VERTICAL));
    patterns.push_back(generatePattern(field, move, color, SearchDirection::DIAGONAL));
    patterns.push_back(generatePattern(field, move, color, SearchDirection::DIAGONAL_REVERSE));
    //========================
    // must to defend patterns

    int index = -1;
    static constexpr std::array<SearchDirection, 4> directions {SearchDirection::HORIZONTAL, SearchDirection::VERTICAL, SearchDirection::DIAGONAL, SearchDirection::DIAGONAL_REVERSE};
    for (auto& pattern : patterns) {
        index++;
        // 11011 - pattern
        if (pattern.pattern == 27) {
            AITacticalPattern defencePattern;
            defencePattern.type = AITacticalPatternType::CRITICAL_DEFENCE;

            short defence = getNextMoveInDirection(move, directions[index], pattern.patternStart - 2, true);
            defencePattern.moves.insert(defence);

            store.patterns[AIPatternsStore::CRITICAL_DEFENCE].push_back(defencePattern);
        }

        // 10111 - pattern
        if (pattern.pattern == 23) {
            AITacticalPattern defencePattern;
            defencePattern.type = AITacticalPatternType::CRITICAL_DEFENCE;

            short defence = getNextMoveInDirection(move, directions[index], pattern.patternEnd - 1, false);
            defencePattern.moves.insert(defence);

            store.patterns[AIPatternsStore::CRITICAL_DEFENCE].push_back(defencePattern);
        }

        // 11101 - pattern
        if (pattern.pattern == 29) {
            AITacticalPattern defencePattern;
            defencePattern.type = AITacticalPatternType::CRITICAL_DEFENCE;

            short defence = getNextMoveInDirection(move, directions[index], pattern.patternStart - 1, true);
            defencePattern.moves.insert(defence);

            store.patterns[AIPatternsStore::CRITICAL_DEFENCE].push_back(defencePattern);
        }

        // 1111 - pattern
        if (pattern.pattern == 15) {
            // have nothing at right
            // pattern is 1111 and blank is xxxx11
            AITacticalPattern defencePattern;
            defencePattern.type = AITacticalPatternType::CRITICAL_DEFENCE;

            if ((pattern.blankPattern & 3) > 0) {
                short defence = getNextMoveInDirection(move, directions[index], pattern.patternStart + 1, true);
                defencePattern.moves.insert(defence);
            }

            if ((pattern.blankPattern & 16) > 0) {
                short defence = getNextMoveInDirection(move, directions[index], pattern.patternEnd + 1, false);
                defencePattern.moves.insert(defence);
            }

            if (!defencePattern.moves.empty()) {
                store.patterns[AIPatternsStore::CRITICAL_DEFENCE].push_back(defencePattern);
            }
        }

        // 111 - pattern with blank pattern
        if (pattern.pattern == 7) {
            if (pattern.blankPattern == 99 || pattern.blankPattern == 49 || pattern.blankPattern == 35) {
                AITacticalPattern defencePattern;
                defencePattern.type = AITacticalPatternType::VITAL_DEFENCE;

                short defence = getNextMoveInDirection(move, directions[index], pattern.patternStart + 1, true);
                defencePattern.moves.insert(defence);

                short secondDefence = getNextMoveInDirection(move, directions[index], pattern.patternEnd + 1, false);
                defencePattern.moves.insert(secondDefence);

                store.patterns[AIPatternsStore::VITAL_DEFENCE].push_back(defencePattern);
            }

            // tactical move with pattern 111 and blank pattern 11000
            if (pattern.blankPattern == 24) {
                AITacticalPattern defencePattern;
                defencePattern.type = AITacticalPatternType::DEVELOPMENT_DEFENCE;

                short defence = getNextMoveInDirection(move, directions[index], pattern.patternEnd + 1, false);
                defencePattern.moves.insert(defence);

                defence = getNextMoveInDirection(move, directions[index], pattern.patternEnd + 2, false);
                defencePattern.moves.insert(defence);

                store.patterns[AIPatternsStore::DEVELOPMENT_DEFENCE].push_back(defencePattern);
            }

            // tactical move with pattern 111 and blank pattern 00011
            if (pattern.blankPattern == 3) {
                AITacticalPattern defencePattern;
                defencePattern.type = AITacticalPatternType::DEVELOPMENT_DEFENCE;

                short defence = getNextMoveInDirection(move, directions[index], pattern.patternStart + 1, true);
                defencePattern.moves.insert(defence);

                defence = getNextMoveInDirection(move, directions[index], pattern.patternStart + 2, true);
                defencePattern.moves.insert(defence);

                store.patterns[AIPatternsStore::DEVELOPMENT_DEFENCE].push_back(defencePattern);
            }
        }

        // 1011 - pattern with blank pattern
        if (pattern.pattern == 11) {
            if (pattern.blankPattern == 33 || pattern.blankPattern == 67 || pattern.blankPattern == 97 || pattern.blankPattern == 195) {
                AITacticalPattern defencePattern;
                defencePattern.type = AITacticalPatternType::VITAL_DEFENCE;

                short defence = getNextMoveInDirection(move, directions[index], pattern.patternStart + 1, true);
                defencePattern.moves.insert(defence);

                short secondDefence = getNextMoveInDirection(move, directions[index], pattern.patternEnd + 1, false);
                defencePattern.moves.insert(secondDefence);

                short thirdDefence = getNextMoveInDirection(move, directions[index], pattern.patternEnd - 1, false);
                defencePattern.moves.insert(thirdDefence);

                store.patterns[AIPatternsStore::VITAL_DEFENCE].push_back(defencePattern);
            }
        }

        // 1101 - pattern with blank pattern
        if (pattern.pattern == 13) {
            if (pattern.blankPattern == 33 || pattern.blankPattern == 67 || pattern.blankPattern == 97 || pattern.blankPattern == 195) {
                AITacticalPattern defencePattern;
                defencePattern.type = AITacticalPatternType::VITAL_DEFENCE;

                short defence = getNextMoveInDirection(move, directions[index], pattern.patternStart + 1, true);
                defencePattern.moves.insert(defence);

                short secondDefence = getNextMoveInDirection(move, directions[index], pattern.patternEnd + 1, false);
                defencePattern.moves.insert(secondDefence);

                short thirdDefence = getNextMoveInDirection(move, directions[index], pattern.patternStart - 1, true);
                defencePattern.moves.insert(thirdDefence);

                store.patterns[AIPatternsStore::VITAL_DEFENCE].push_back(defencePattern);
            }
        }

        // 11 - tactical pattern
        if (pattern.pattern == 3) {
            // blank - 11001 or 110011
            AITacticalPattern defencePattern;
            defencePattern.type = AITacticalPatternType::DEVELOPMENT_DEFENCE;

            if (pattern.blankPattern == 25 || pattern.blankPattern == 51) {
                short defence = getNextMoveInDirection(move, directions[index], pattern.patternEnd + 1, false);
                defencePattern.moves.insert(defence);

                defence = getNextMoveInDirection(move, directions[index], pattern.patternEnd + 2, false);
                defencePattern.moves.insert(defence);
            }

            // blank - 10011 or 110011
            if (pattern.blankPattern == 19 || pattern.blankPattern == 51) {
                short defence = getNextMoveInDirection(move, directions[index], pattern.patternStart + 1, true);
                defencePattern.moves.insert(defence);

                defence = getNextMoveInDirection(move, directions[index], pattern.patternStart + 2, true);
                defencePattern.moves.insert(defence);
            }

            if (!defencePattern.moves.empty()) {
                store.patterns[AIPatternsStore::DEVELOPMENT_DEFENCE].push_back(defencePattern);
            }
        }

        // 101 - tactical pattern
        if (pattern.pattern == 5) {
            AITacticalPattern defencePattern;
            defencePattern.type = AITacticalPatternType::DEVELOPMENT_DEFENCE;

            // blank - 1100011
            if (pattern.blankPattern == 99) {
                short defence = getNextMoveInDirection(move, directions[index], pattern.patternEnd + 1, false);
                defencePattern.moves.insert(defence);

                defence = getNextMoveInDirection(move, directions[index], pattern.patternEnd + 2, false);
                defencePattern.moves.insert(defence);

                defence = getNextMoveInDirection(move, directions[index], pattern.patternStart + 2, true);
                defencePattern.moves.insert(defence);

                defence = getNextMoveInDirection(move, directions[index], pattern.patternStart + 1, true);
                defencePattern.moves.insert(defence);

                defence = getNextMoveInDirection(move, directions[index], pattern.patternStart - 1, true);
                defencePattern.moves.insert(defence);
            }

            // blank - 110001
            if (pattern.blankPattern == 49) {
                short defence = getNextMoveInDirection(move, directions[index], pattern.patternEnd + 1, false);
                defencePattern.moves.insert(defence);

                defence = getNextMoveInDirection(move, directions[index], pattern.patternEnd + 2, false);
                defencePattern.moves.insert(defence);

                defence = getNextMoveInDirection(move, directions[index], pattern.patternStart - 1, true);
                defencePattern.moves.insert(defence);
            }

            // blank - 100011
            if (pattern.blankPattern == 35) {
                short defence = getNextMoveInDirection(move, directions[index], pattern.patternStart + 1, true);
                defencePattern.moves.insert(defence);

                defence = getNextMoveInDirection(move, directions[index], pattern.patternStart + 2, true);
                defencePattern.moves.insert(defence);

                defence = getNextMoveInDirection(move, directions[index], pattern.patternStart - 1, true);
                defencePattern.moves.insert(defence);
            }

            if (!defencePattern.moves.empty()) {
                store.patterns[AIPatternsStore::DEVELOPMENT_DEFENCE].push_back(defencePattern);
            }
        }
    }

    std::unordered_map<short, std::vector<AITacticalPattern>> tacticalMap;
    for (auto& developmentPattern : store.patterns[AIPatternsStore::DEVELOPMENT_DEFENCE]) {
        for (auto& move : developmentPattern.moves) {
            tacticalMap[move].push_back(developmentPattern);
        }
    }

    for (auto& tacticalPoint : tacticalMap) {
        if (tacticalPoint.second.size() >= 2) {
            AITacticalPattern defencePattern;
            defencePattern.type = AITacticalPatternType::VITAL_DEFENCE;

            for (auto& pattern : tacticalPoint.second) {
                for (auto& move : pattern.moves) {
                    defencePattern.moves.insert(move);
                }
            }
            store.patterns[AIPatternsStore::VITAL_DEFENCE].push_back(defencePattern);
        }
    }
}

void AIDomainKnowledge::updateDomainKnowledge(AIDomainKnowledge* domainKnowledge, short move) {
    auto& blackStore = domainKnowledge->getPatternStore(BLACK_PIECE_COLOR);
    for (auto& patternsCategory : blackStore.patterns) {
        for (auto it = patternsCategory.begin(); it != patternsCategory.end();) {
            auto& pattern = *it;
            if (pattern.moves.find(move) != pattern.moves.end()) {
                it = patternsCategory.erase(it);
            } else {
                ++it;
            }
        }
    }

    auto& whiteStore = domainKnowledge->getPatternStore(WHITE_PIECE_COLOR);
    for (auto& patternsCategory : whiteStore.patterns) {
        for (auto it = patternsCategory.begin(); it != patternsCategory.end();) {
            auto& pattern = *it;
            if (pattern.moves.find(move) != pattern.moves.end()) {
                it = patternsCategory.erase(it);
            } else {
                ++it;
            }
        }
    }
}

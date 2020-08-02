#include "aidomainknowledge.h"
#include <QDebug>

AIDomainKnowledge::AIDomainKnowledge()
{
}

bool AIDomainKnowledge::canMoveInDirection(short position, SearchDirection direction, bool moveBackwards) {
    if (direction == SearchDirection::HORIZONTAL) {
        return moveBackwards ? BOARD_DISTANCE_RIGHT[position] > 0 : BOARD_DISTANCE_LEFT[position] > 0;
    }
    if (direction == SearchDirection::VERTICAL) {
        return moveBackwards ? BOARD_DISTANCE_BOTTOM[position] > 0 : BOARD_DISTANCE_TOP[position] > 0;
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
        short offsetX = FieldMove::MIN_Y_VALUE * step;
        short offsetY = FieldMove::MIN_Y_VALUE * step;
        short nextPosition = position + (moveBackwards ? -offsetX - offsetY : offsetX + offsetY);
        return nextPosition;
    }

    if (direction == SearchDirection::DIAGONAL_REVERSE) {
        short offsetX = FieldMove::MIN_Y_VALUE * step;
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
    while (counter <= 3 && canMoveInDirection(getNextMoveInDirection(leftPosition, direction, counter - 1, false), direction, false)) {
        short nextPosition = getNextMoveInDirection(leftPosition, direction, counter, false);
        short nextColor = board[nextPosition];
        if (nextColor == blockColor) {
            break;
        }

        if (nextColor == 0) {
            patternLeftOffset++;
        } else if (nextColor == color) {
            if (additionalPattern == 0) {
                blankOffset += (counter - 1);
            }

            additionalPattern = additionalPattern << 1;
            additionalPattern++;
            patternLeftOffset++;
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
    while (counter <= 3 && canMoveInDirection(getNextMoveInDirection(rightPosition, direction, counter - 1, true), direction, true)) {
        short nextPosition = getNextMoveInDirection(rightPosition, direction, counter, true);
        short nextColor = board[nextPosition];
        if (nextColor == blockColor) {
            break;
        }

        if (nextColor == 0) {
            patternRightOffset++;
        } else if (nextColor == color) {
            if (additionalPattern == 0) {
                blankOffset += (counter - 1);
            }

            additionalPattern = additionalPattern << 1;
            additionalPattern++;
            additionalPatternCounter++;
            patternRightOffset++;
        }

        counter++;
    }

    if (additionalPattern > 0) pattern = (pattern << (blankOffset + additionalPatternCounter)) + additionalPattern;
    if (additionalPattern > 0) rightIndex += patternRightOffset;

    blankOffset = 0;
    counter = 1;
    short leftBlank = 0, rightBlank = 0;
    while (canMoveInDirection(getNextMoveInDirection(move, direction, leftIndex + counter - 1, true), direction, true) && counter <= 2) {
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

std::vector<short> AIDomainKnowledge::generateDefensiveMoves(IField* field, short move, short color) {
    auto horizontalPattern = generatePattern(field, move, color, SearchDirection::HORIZONTAL);
    qDebug() << "Found pattern " << horizontalPattern.patternStart << " " << horizontalPattern.patternEnd << " " << QString::number(horizontalPattern.pattern, 2) << " " << QString::number(horizontalPattern.blankPattern, 2);
    //========================
    // must to defend patterns

    // 11011 - pattern
    if (horizontalPattern.pattern == 27) {
        short defence = getNextMoveInDirection(move, SearchDirection::HORIZONTAL, horizontalPattern.patternEnd - 2, false);
        qDebug() << "Defensive move: " << defence << ", xy = " << FieldMove::getXFromPosition(defence) << " " << FieldMove::getYFromPosition(defence);
    }

    // 10111 - pattern
    if (horizontalPattern.pattern == 23) {
        short defence = getNextMoveInDirection(move, SearchDirection::HORIZONTAL, horizontalPattern.patternStart - 1, false);
        qDebug() << "Defensive move: " << defence << ", xy = " << FieldMove::getXFromPosition(defence) << " " << FieldMove::getYFromPosition(defence);
    }

    return {};
}

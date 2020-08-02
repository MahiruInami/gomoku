#pragma once

#include "IField.h"

struct AIMovePattern {
    short patternStart = -1;
    short patternEnd = -1;
    short pattern = 0;
//    short blankStart = -1;
//    short blankEnd = -1;
    short blankPattern = 0;
};

class AIDomainKnowledge
{
public:
    enum class SearchDirection {
        HORIZONTAL,
        VERTICAL,
        DIAGONAL,
        DIAGONAL_REVERSE
    };

    AIDomainKnowledge();

    static bool canMoveInDirection(short position, SearchDirection direction, bool moveBackwards = false);
    static short getNextMoveInDirection(short position, SearchDirection direction, short step = 1, bool moveBackwards = false);
    static AIMovePattern generatePattern(IField* field, short move, short color, SearchDirection direction);

    static std::vector<short> generateDefensiveMoves(IField* field, short move, short color);
};


#pragma once

#include "IField.h"
#include "list"

struct AIMovePattern {
    short patternStart = -1;
    short patternEnd = -1;
    short pattern = 0;
    short blankPattern = 0;
};

enum class AITacticalPatternType {
    CRITICAL_ATTACK,
    CRITICAL_DEFENCE,
    VITAL_ATTACK,
    VITAL_DEFENCE,
    DEVELOPMENT_ATTACK,
    DEVELOPMENT_DEFENCE
};

class AITacticalPattern {
public:
    AITacticalPatternType type;
    std::unordered_set<short> moves;
};

struct AIPatternsStore {
    static constexpr unsigned CRITICAL_ATTACK = 0;
    static constexpr unsigned CRITICAL_DEFENCE = 1;
    static constexpr unsigned VITAL_ATTACK = 2;
    static constexpr unsigned VITAL_DEFENCE = 3;
    static constexpr unsigned DEVELOPMENT_ATTACK = 4;
    static constexpr unsigned DEVELOPMENT_DEFENCE = 5;

//    static constexpr unsigned MUST_TO_PLAY_CATEGORIES = 4;

//    static constexpr unsigned TACTICAL_CATEGORIES_START = 4;
//    static constexpr unsigned TACTICAL_CATEGORIES_END = 5;

    static constexpr unsigned CATEGORIES_COUNT = 6;

//    std::array<std::unordered_set<short>, CATEGORIES_COUNT> patterns;
//    std::array<std::vector<short>, BOARD_LENGTH> miaiPoints;
//    std::array<std::vector<short>, BOARD_LENGTH> tacticalMiaiPoints;
    std::array<std::list<AITacticalPattern>, CATEGORIES_COUNT> patterns;

    void clear () {
        patterns.fill({});
    }
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
    virtual ~AIDomainKnowledge() {}

    AIDomainKnowledge(const AIDomainKnowledge& AIDomainKnowledge);

    static bool canMoveInDirection(short position, SearchDirection direction, bool moveBackwards = false);
    static short getNextMoveInDirection(short position, SearchDirection direction, short step = 1, bool moveBackwards = false);
    static AIMovePattern generatePattern(IField* field, short move, short color, SearchDirection direction);

    static void generateDefensiveMoves(IField* field, short move, short color, AIPatternsStore& prediction);
    static void generateAttackMoves(IField* field, short move, short color, AIPatternsStore& prediction);

    static void updateDomainKnowledge(AIDomainKnowledge* domainKnowledge, short move);

    AIPatternsStore& getPatternStore(short color) { return _domain[color]; }

    void clear() {
        _domain.fill({});
    }
private:
    std::array<AIPatternsStore, 3> _domain;
};


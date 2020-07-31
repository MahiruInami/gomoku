#pragma once

#include <QtGlobal>

class Debug {
public:
    static Debug& getInstance() {
        static Debug instance;
        return instance;
    }

    void resetTimers() {
        gameUpdate = 0;
        getChildTime = 0;
        updateStatusTime = 0;
        nodeSelectionTime = 0;
        moveGenerationTime = 0;
        simulationBoardTime = 0;
        backpropagateTime = 0;
        fieldMoveTime = 0;
        randomMoveTime = 0;
        selectTime = 0;
        exploreTime = 0;
        simulationTime = 0;
        selectVisits = 0;
        exploreVisits = 0;
        simulationVisits = 0;
    }

public:
    qint64 gameUpdate;
    qint64 getChildTime;
    qint64 updateStatusTime;
    qint64 fieldMoveTime;
    qint64 moveGenerationTime;
    qint64 nodeSelectionTime;
    qint64 randomMoveTime;
    qint64 selectTime;
    qint64 exploreTime;
    qint64 simulationTime;
    qint64 simulationBoardTime;
    qint64 backpropagateTime;

    unsigned selectVisits = 0;
    unsigned exploreVisits = 0;
    unsigned simulationVisits = 0;
};

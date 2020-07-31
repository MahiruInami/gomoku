#include "debug.h"
#include <QDebug>


Debug::Debug() {

}

void Debug::startTrack(DebugTimeTracks track) {
    if (!_timer) {
        _timer = new QElapsedTimer();
        _timer->start();
    }

    _tracksStart[static_cast<int>(track)] = _timer->elapsed();
}

void Debug::stopTrack(DebugTimeTracks track) {
    _tracksEnd[static_cast<int>(track)] = _timer->elapsed();

    _tracksTime[static_cast<int>(track)] += (_tracksEnd[static_cast<int>(track)] - _tracksStart[static_cast<int>(track)]);
}

void Debug::trackCall(DebugCallTracks track) {
    _callTracks[static_cast<int>(track)]++;
}


void Debug::resetStats() {
    for (unsigned i = 0; i < TRACKS_COUNT; ++i) _tracksStart[i] = 0;
    for (unsigned i = 0; i < TRACKS_COUNT; ++i) _tracksEnd[i] = 0;
    for (unsigned i = 0; i < TRACKS_COUNT; ++i) _tracksTime[i] = 0;
    for (unsigned i = 0; i < TRACKS_COUNT; ++i) _callTracks[i] = 0;
    for (unsigned i = 0; i < TRACKS_COUNT; ++i) _trackedTimeTracks[i] = false;
    for (unsigned i = 0; i < TRACKS_COUNT; ++i) _trackedCallTracks[i] = false;

    if (_timer) {
        _timer->restart();
    } else {
        _timer = new QElapsedTimer();
        _timer->start();
    }
}

void Debug::printStats(DebugTrackLevel level) {
    qDebug() << "Time tracks:";
    for (unsigned i = 0; i < static_cast<unsigned>(DebugTimeTracks::TOTAL_TRACKS); ++i) {
        if (!_trackedTimeTracks[i]) {
            continue;
        }
        if (_timeTracksDebugLevel[i] < static_cast<unsigned>(level)) {
            continue;
        }
        qDebug() << _timeTrackNames[i].c_str() << " time: " << _tracksTime[i];
    }
    qDebug() << "Call tracks:";
    for (unsigned i = 0; i < static_cast<unsigned>(DebugCallTracks::TOTAL_TRACKS); ++i) {
        if (!_trackedCallTracks[i]) {
            continue;
        }
        if (_callTracksDebugLevel[i] < static_cast<unsigned>(level)) {
            continue;
        }
        qDebug() << _callTrackNames[i].c_str() << ": " << _callTracks[i];
    }
}

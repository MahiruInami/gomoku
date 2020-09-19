#pragma once

#include <array>
#include <QtGlobal>
#include <QElapsedTimer>


enum class DebugTrackLevel : unsigned {
    DEBUG = 0,
    INFO,
    PRODUCTION
};

enum class DebugTimeTracks : unsigned {
    GAME_UPDATE = 0,
    AI_UPDATE,
    INCREMENTAL_UPDATE,
    MAKE_MOVE,
    UPDATE_PRIORITY,
    NODE_SELECTION,
    TRAVERSE_AND_EXPAND,
    ERASE_AVAILABLE_MOVES,
    CLEAR_TEMPLATES,
    ADD_NEW_MOVES,
    UPDATE_TEMPLATES,
    CREATE_TEMPLATE,

    TOTAL_TRACKS
};

enum class DebugCallTracks : unsigned {
    GAME_UPDATE = 0,
    MAKE_MOVE,
    UPDATE_PRIORITY,
    CREATE_TEMPLATE,
    UPDATE_TEMPLATES,
    UPDATE_TEMPLATES_INLINE,
    TOTAL_TRACKS
};

class Debug {
public:
    static Debug& getInstance() {
        static Debug instance;
        return instance;
    }

    Debug();
    virtual ~Debug() {
        if (_timer) {
            delete _timer;
            _timer = nullptr;
        }
    }

    void registerTimeTrackName(DebugTimeTracks track, std::string trackName) { _timeTrackNames[static_cast<unsigned>(track)] = trackName; }
    void registerCallTrackName(DebugCallTracks track, std::string trackName) { _callTrackNames[static_cast<unsigned>(track)] = trackName; }

    void setTimeTrackDebugLevel(DebugTimeTracks track, DebugTrackLevel level) { _timeTracksDebugLevel[static_cast<unsigned>(track)] = static_cast<unsigned>(level); }
    void setCallTrackDebugLevel(DebugCallTracks track, DebugTrackLevel level) { _callTracksDebugLevel[static_cast<unsigned>(track)] = static_cast<unsigned>(level); }

    void startTrack(DebugTimeTracks track);
    void stopTrack(DebugTimeTracks track);
    void trackCall(DebugCallTracks track);

    void resetStats();
    void printStats(DebugTrackLevel);

    qint64 getTrackStats(DebugTimeTracks track) const { return _tracksTime[static_cast<unsigned>(track)]; }
    qint64 getCallStats(DebugCallTracks track) const { return _callTracks[static_cast<unsigned>(track)]; }


private:
    static constexpr unsigned TRACKS_COUNT = 32;

    std::array<qint64, TRACKS_COUNT> _tracksStart;
    std::array<qint64, TRACKS_COUNT> _tracksEnd;
    std::array<qint64, TRACKS_COUNT> _tracksTime;
    std::array<qint64, TRACKS_COUNT> _callTracks;

    std::array<bool, TRACKS_COUNT> _trackedTimeTracks;
    std::array<bool, TRACKS_COUNT> _trackedCallTracks;

    std::array<std::string, TRACKS_COUNT> _timeTrackNames;
    std::array<std::string, TRACKS_COUNT> _callTrackNames;

    std::array<unsigned, TRACKS_COUNT> _timeTracksDebugLevel;
    std::array<unsigned, TRACKS_COUNT> _callTracksDebugLevel;

    QElapsedTimer* _timer = nullptr;
    bool _isEnabled = false;
};

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "fieldwidget.h"
#include "IField.h"
#include "bitfield.h"
#include "mctstree.h"
#include <QMessageBox>
#include <QTimer>
#include <QDebug>
#include <QElapsedTimer>
#include "debug.h"
#include <bitset>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    _fieldView = new FieldWidget();
    ui->FieldView->layout()->addWidget(_fieldView);

    _bitField = new BitField();

    connect(ui->startGameButton, &QPushButton::clicked, this, &MainWindow::onNewGameStarted);
    connect(ui->showTreeButton, &QPushButton::clicked, this, &MainWindow::checkPattern);
    connect(ui->aiUpdateButton, &QPushButton::clicked, this, &MainWindow::updateAIOnce);
    connect(_fieldView, &FieldWidget::mouseClicked, this, &MainWindow::onFieldClick);

    connect(ui->placeBlack, &QCheckBox::clicked, this, &MainWindow::onlyBlackModeClick);
    connect(ui->placeWhite, &QCheckBox::clicked, this, &MainWindow::onlyWhiteModeClick);
    connect(ui->showPlayouts, &QCheckBox::clicked, this, &MainWindow::showAIPlayouts);

    ui->aiLevel->addItem("Level 1");
    ui->aiLevel->addItem("Level 2");
    ui->aiLevel->addItem("Level 3");
    ui->aiLevel->addItem("Level 4");
    ui->aiLevel->addItem("Level 5");
    ui->aiLevel->addItem("Level 6");
    ui->aiLevel->addItem("Level 7");
    ui->aiLevel->addItem("Level 8");
    ui->aiLevel->addItem("Level 9");
    ui->aiLevel->addItem("Level 10");

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateAI()));
    timer->start(16);

    QTimer *timer2 = new QTimer(this);
    connect(timer2, SIGNAL(timeout()), this, SLOT(updateAIField()));
    timer2->start(2000);

    QTimer *aiMoveTimer = new QTimer(this);
    connect(aiMoveTimer, SIGNAL(timeout()), this, SLOT(checkAIMove()));
    aiMoveTimer->start(5000);


    Debug::getInstance().registerTimeTrackName(DebugTimeTracks::GAME_UPDATE, "Update Game");
    Debug::getInstance().registerTimeTrackName(DebugTimeTracks::INCREMENTAL_UPDATE, "Incremental update");
    Debug::getInstance().registerTimeTrackName(DebugTimeTracks::UPDATE_PRIORITY, "Update priority");
    Debug::getInstance().registerTimeTrackName(DebugTimeTracks::MAKE_MOVE, "Make move");
    Debug::getInstance().registerTimeTrackName(DebugTimeTracks::NODE_SELECTION, "Select node");
    Debug::getInstance().registerTimeTrackName(DebugTimeTracks::TRAVERSE_AND_EXPAND, "Expansion");
    Debug::getInstance().registerTimeTrackName(DebugTimeTracks::ERASE_AVAILABLE_MOVES, "Erasing moves");
    Debug::getInstance().registerTimeTrackName(DebugTimeTracks::CLEAR_TEMPLATES, "Clear templates");
    Debug::getInstance().registerTimeTrackName(DebugTimeTracks::ADD_NEW_MOVES, "Generating moves");
    Debug::getInstance().registerTimeTrackName(DebugTimeTracks::UPDATE_TEMPLATES, "Generating templates");
    Debug::getInstance().registerTimeTrackName(DebugTimeTracks::CREATE_TEMPLATE, "Create templates");
    Debug::getInstance().registerCallTrackName(DebugCallTracks::GAME_UPDATE, "Update Game");
    Debug::getInstance().registerCallTrackName(DebugCallTracks::UPDATE_PRIORITY, "Calculate priority");
    Debug::getInstance().registerCallTrackName(DebugCallTracks::CREATE_TEMPLATE, "Create template");
    Debug::getInstance().registerCallTrackName(DebugCallTracks::UPDATE_TEMPLATES, "Update template");
    Debug::getInstance().registerCallTrackName(DebugCallTracks::UPDATE_TEMPLATES_INLINE, "Update one template");
    Debug::getInstance().registerCallTrackName(DebugCallTracks::MAKE_MOVE, "Make move");

    Debug::getInstance().registerTimeTrackName(DebugTimeTracks::AI_UPDATE, "Update AI");
//    Debug::getInstance().registerCallTrackName(DebugCallTracks::AI_UPDATE, "Update AI");
}

MainWindow::~MainWindow()
{
    delete ui;
    delete _fieldView;
}

void MainWindow::updateAI() {
    if(!_isGameStarted) {
        return;
    }

    if (_currentMode == MODE::VIEW_TREE || _currentMode == MODE::MANUAL_AI_DEBUG) {
        return;
    }

    if (!_mctsTree) {
        return;
    }

    Debug::getInstance().resetStats();
    Debug::getInstance().startTrack(DebugTimeTracks::GAME_UPDATE);

    _aiBudget += 16 * 1000000;
    if (_aiBudget <= 0) {
        if (_aiBudget < 15 * 1000000) {
            _aiBudget = 0;
        }
        return;
    }
    qint64 currentUpdateTime = 0;
    unsigned updatesCount = 0;
    QElapsedTimer updateTimer;
    updateTimer.start();

    do {
        qint64 updateStartTime = updateTimer.nsecsElapsed();

        _mctsTree->update(_bitField);

        Debug::getInstance().trackCall(DebugCallTracks::GAME_UPDATE);

        _totalAiGames += 1;
        _currentAiGames += _mctsTree->getThreadsCount();
        updatesCount++;

        currentUpdateTime = updateTimer.nsecsElapsed() - updateStartTime;
        _aiBudget -= currentUpdateTime;
    } while (_aiBudget > currentUpdateTime);

//    qint64 totalUpdateTime = updateTimer.nsecsElapsed();
//    qDebug() << _aiBudget << currentUpdateTime << totalUpdateTime;

    Debug::getInstance().stopTrack(DebugTimeTracks::GAME_UPDATE);
    Debug::getInstance().printStats(DebugTrackLevel::DEBUG);

    ui->aiTotalGames->setText(QString::number(MCTSNode::MAX_DEPTH));
    ui->aiCurrentGames->setText(QString::number(_currentAiGames));
}

void MainWindow::updateAIOnce() {
    if(!_isGameStarted) {
        return;
    }

    if (_mctsTree) {
        MCTSTree::NODE_EXPLORATIONS_TO_EXPAND = 1;
        _mctsTree->update(_bitField);
    }

    _currentMode = MODE::MANUAL_AI_DEBUG;

    _totalAiGames++;
    _currentAiGames++;

    ui->aiTotalGames->setText(QString::number(_totalAiGames));
    ui->aiCurrentGames->setText(QString::number(_currentAiGames));
}

void MainWindow::updateAIField() {
    updateField();
}

void MainWindow::checkAIMove() {
    if(!_isGameStarted) {
        return;
    }

    if (getCurrentColor() != _aiPlayerColor) {
        return;
    }

    if (_mctsTree && (_mctsTree->getTotalPlayouts() > (_mctsTree->getChildrenCount() * 1000 * _aiLevel * _aiLevel) || _mctsTree->getChildrenCount() == 1)) {
        auto nodesData = _mctsTree->getNodesData();
        float bestScores = -100.f;
        short bestPosition = -1;

        for (auto& node : nodesData) {
            if (node.scores > bestScores) {
                bestScores = node.scores;
                bestPosition = node.position;
            }
        }
        makeMove(FieldMove::getXFromPosition(bestPosition), FieldMove::getYFromPosition(bestPosition));
    }
}

void MainWindow::updateField() {
    _fieldView->clearPieces();
    std::unordered_set<short> cells;
    for (auto& move : _bitField->getGameHistory()) {
        cells.insert(getHashedPosition(std::get<0>(move), std::get<1>(move)));
        _fieldView->addPiece(std::get<0>(move) * FieldWidget::IMAGE_SIZE, std::get<1>(move) * FieldWidget::IMAGE_SIZE, std::get<2>(move));
    }

    if (_mctsTree) {
        if (_showBestPlayout) {
            auto aiMoves = _mctsTree->getBestPlayout(_bestPlayoutX, _bestPlayoutY);
            for (auto& aiMove : aiMoves) {
                aiMove.x = FieldMove::getXFromPosition(aiMove.position) * FieldWidget::IMAGE_SIZE;
                aiMove.y = FieldMove::getYFromPosition(aiMove.position) * FieldWidget::IMAGE_SIZE;

                _fieldView->addAIBestPlayoutMove(aiMove);
                cells.insert(aiMove.position);
            }
        } else if (_showPlayouts) {
            auto aiMoves = _mctsTree->getNodesData();
            for (auto& aiMove : aiMoves) {
                if (cells.find(aiMove.position) != cells.end()) {
                    continue;
                }

                aiMove.x = FieldMove::getXFromPosition(aiMove.position) * FieldWidget::IMAGE_SIZE;
                aiMove.y = FieldMove::getYFromPosition(aiMove.position) * FieldWidget::IMAGE_SIZE;

    //            if (_bitField->getMovePriority(aiMove.position, aiMove.color) > 0) {
    //                aiMove.selectionScore = _bitField->getMovePriority(aiMove.position, aiMove.color);
    //            }


                _fieldView->addAINextMove(aiMove);
                cells.insert(aiMove.position);
            }
        }
    }

    for (short x = 0; x < BOARD_SIZE; ++x) {
        for (short y = 0; y < BOARD_SIZE; ++y) {
            short hashedPosition = FieldMove::getPositionFromPoint(x, y);
            if (cells.find(hashedPosition) != cells.end()) {
                continue;
            }

//            if (_isGameStarted && (_bitField->getMovePriority(hashedPosition, getCurrentColor()) > 0 || _bitField->getMoveDefencePriority(hashedPosition, getCurrentColor()) > 0 || _bitField->getMoveDefencePriority(hashedPosition, getNextPlayerColor(getCurrentColor())) > 0)) {
//                AIMoveData moveData;
//                moveData.position = hashedPosition;
//                moveData.x = FieldMove::getXFromPosition(hashedPosition) * FieldWidget::IMAGE_SIZE;
//                moveData.y = FieldMove::getYFromPosition(hashedPosition) * FieldWidget::IMAGE_SIZE;
//                moveData.color = getCurrentColor();
//                moveData.nodeVisits = _bitField->getMovePriority(hashedPosition, getCurrentColor());
//                moveData.scores = _bitField->getMoveDefencePriority(hashedPosition, getCurrentColor());
//                moveData.selectionScore = _bitField->getMoveDefencePriority(hashedPosition, getNextPlayerColor(getCurrentColor()));

//                _fieldView->addAINextMove(moveData);
//            }

            cells.insert(hashedPosition);
            _fieldView->addPiece(x * FieldWidget::IMAGE_SIZE, y * FieldWidget::IMAGE_SIZE, 0);
        }
    }

    _fieldView->repaint();

    ui->aiTotalGames->setText(QString::number(_totalAiGames));
    ui->aiCurrentGames->setText(QString::number(_currentAiGames));
}

void MainWindow::onFieldClick(short x, short y, Qt::MouseButton button) {
    if (!_isGameStarted) {
        return;
    }

    short cellX = x / FieldWidget::IMAGE_SIZE;
    short cellY = y / FieldWidget::IMAGE_SIZE;

    if (cellX < 0 || cellX >= BOARD_SIZE || cellY < 0 || cellY >= BOARD_SIZE) {
        return;
    }

//    if (_ai && _currentMode == MODE::PLAY && _currentPlayer == _ai->getPlayerColor()) {
//        return;
//    }

    if (_currentMode == MODE::VIEW_TREE) {
//        short position = FieldMove::getPositionFromPoint(cellX, cellY);
//        if (button == Qt::LeftButton) {
//            if (_ai->goTreeForward(position)) {
//                _currentPlayer = FieldMove::getNextColor(_currentPlayer);
//                delete _field;
//                _field = new Field(*_ai->getBoardState());
//            }
//        } else if (button == Qt::RightButton) {
//            if (_ai->goBack()) {
//                _currentPlayer = FieldMove::getNextColor(_currentPlayer);
//                delete _field;
//                _field = new Field(*_ai->getBoardState());
//            }
//        }

        updateField();
        return;
    }

    if (button == Qt::RightButton) {
        if (_showBestPlayout) {
            _showBestPlayout = false;
            _bestPlayoutX = -1;
            _bestPlayoutY = -1;
            updateField();
            return;
        }

        _bestPlayoutX = cellX;
        _bestPlayoutY = cellY;
        _showBestPlayout = true;
        updateField();

        return;
    }

//    qDebug() << "_field->placePiece(" << cellX << ", " << cellY << ", " << _currentPlayer << ");";
//    qDebug() << "_ai->goToNode(" << FieldMove::getPositionFromPoint(cellX, cellY) << ");";

    if (!_testMoves.empty()) {
        auto value = _testMoves.front();
        cellX = std::get<0>(value);
        cellY = std::get<1>(value);

        _testMoves.erase(_testMoves.begin());
    }
    makeMove(cellX, cellY);
}

void MainWindow::makeMove(short x, short y) {
    qDebug() << "{" << x << "," << y << "}";
    if (!_bitField->makeMove(x, y, getCurrentColor())) {
        return;
    }

    if (_mctsTree) {
        _mctsTree->selectChild(x, y);
    }

    _currentAiGames = 0;
    _currentPlayer = FieldMove::getNextColor(_currentPlayer);

    updateField();

    if (_bitField->getGameStatus() != 0) {
        _isGameStarted = false;

        std::string message = "Draw";
        if (_bitField->getGameStatus() == BLACK_PIECE_COLOR) {
            message = "X has won!";
        } else if (_bitField->getGameStatus() == WHITE_PIECE_COLOR) {
            message = "O has won!";
        }

        QMessageBox msgBox;
        msgBox.setText(message.c_str());
        msgBox.exec();

        return;
    }
}

void MainWindow::checkPattern() {
//    _currentMode = MODE::VIEW_TREE;
//    if (true) {
//        auto moves = _bitField->getBestMoves(_currentPlayer);
//        return;
//    }

    constexpr int MOVES_COUNT = 120;
    constexpr std::array<std::pair<short, short>, MOVES_COUNT> moves = {{{ 9 , 9 } ,{ 10 , 8 } ,{ 9 , 8 } ,{ 9 , 7 } ,{ 8 , 6 } ,{ 8 , 7 } ,{ 11 , 7 } ,{ 10 , 7 } ,{ 10 , 9 } ,{ 7 , 7 } ,{ 6 , 7 } ,{ 10 , 6 } ,{ 10 , 5 } ,{ 11 , 5 } ,{ 12 , 4 } ,{ 8 , 8 } ,{ 7 , 9 } ,{ 7 , 8 } ,{ 6 , 8 } ,{ 6 , 9 } ,{ 5 , 10 } ,{ 8 , 9 } ,{ 6 , 6 } ,{ 8 , 10 } ,{ 8 , 11 } ,{ 12 , 6 } ,{ 6 , 5 } ,{ 6 , 4 } ,{ 7 , 6 } ,{ 5 , 6 } ,{ 5 , 8 } ,{ 4 , 9 } ,{ 8 , 5 } ,{ 9 , 4 } ,{ 5 , 9 } ,{ 5 , 11 } ,{ 7 , 5 } ,{ 9 , 5 } ,{ 5 , 5 } ,{ 4 , 5 } ,{ 8 , 4 } ,{ 5 , 7 } ,{ 9 , 6 } ,{ 9 , 3 } ,{ 11 , 6 } ,{ 13 , 7 } ,{ 15 , 6 } ,{ 14 , 5 } ,{ 14 , 4 } ,{ 13 , 3 } ,{ 13 , 4 } ,{ 15 , 4 } ,{ 15 , 7 } ,{ 15 , 8 } ,{ 13 , 9 } ,{ 13 , 10 } ,{ 14 , 10 } ,{ 14 , 9 } ,{ 12 , 10 } ,{ 12 , 11 } ,{ 13 , 11 } ,{ 14 , 12 } ,{ 13 , 13 } ,{ 12 , 13 } ,{ 11 , 14 } ,{ 11 , 13 } ,{ 10 , 12 } ,{ 9 , 12 } ,{ 9 , 13 } ,{ 7 , 13 } ,{ 6 , 13 } ,{ 6 , 14 } ,{ 4 , 13 } ,{ 3 , 12 } ,{ 2 , 11 } ,{ 1 , 9 } ,{ 2 , 7 } ,{ 3 , 10 } ,{ 2 , 8 } ,{ 3 , 8 } ,{ 3 , 6 } ,{ 3 , 5 } ,{ 2 , 5 } ,{ 1 , 6 } ,{ 1 , 2 } ,{ 4 , 3 } ,{ 5 , 2 } ,{ 8 , 2 } ,{ 11 , 1 } ,{ 12 , 1 } ,{ 12 , 2 } ,{ 10 , 2 } ,{ 9 , 1 } ,{ 10 , 1 } ,{ 15 , 1 } ,{ 15 , 2 } ,{ 16 , 4 } ,{ 17 , 5 } ,{ 16 , 7 } ,{ 16 , 9 } ,{ 15 , 10 } ,{ 15 , 11 } ,{ 15 , 12 } ,{ 16 , 13 } ,{ 15 , 14 } ,{ 13 , 14 } ,{ 12 , 15 } ,{ 11 , 16 } ,{ 9 , 17 } ,{ 7 , 17 } ,{ 5 , 17 } ,{ 4 , 16 } ,{ 4 , 15 } ,{ 3 , 15 } ,{ 3 , 16 } ,{ 3 , 17 } ,{ 1 , 15 } ,{ 2 , 15 } ,{ 3 , 14 } ,{ 5 , 15 }}};

    BitField* field = new BitField();

    constexpr bool CHECK_PATTERN = false;
    if (CHECK_PATTERN) {

        field->makeMove(0, 0, BLACK_PIECE_COLOR);
        field->makeMove(1, 0, BLACK_PIECE_COLOR);
//        field->makeMove(4, 0, WHITE_PIECE_COLOR);
        field->makeMove(3, 0, BLACK_PIECE_COLOR);
        delete field;
        return;
    }

    QElapsedTimer timer;
    timer.start();

    for (int i = 0; i < 1000; i++) {
        field->clear();
        short fieldColor = BLACK_PIECE_COLOR;
        for (int j = 0; j < MOVES_COUNT; j++) {
            if (!field->makeMove(moves[j].first, moves[j].second, fieldColor)) {
                return;
            }
            fieldColor = FieldMove::getNextColor(fieldColor);
        }
    }

    delete field;

    qDebug() << "10k plays " << timer.nsecsElapsed() << timer.elapsed();

    return;
}

void MainWindow::onlyBlackModeClick() {
    if (ui->placeWhite->isChecked()) {
        ui->placeWhite->setChecked(false);
    }

    _forcedColor = BLACK_PIECE_COLOR;
}

void MainWindow::onlyWhiteModeClick() {
    if (ui->placeBlack->isChecked()) {
        ui->placeBlack->setChecked(false);
    }

    _forcedColor = WHITE_PIECE_COLOR;
}

void MainWindow::showAIPlayouts() {
    _showPlayouts = ui->showPlayouts->isChecked();

    updateField();
}

void MainWindow::onNewGameStarted() {
    _isGameStarted = true;
    _bitField->clear();
    _currentPlayer = BLACK_PIECE_COLOR;
    _aiPlayerColor = WHITE_PIECE_COLOR;
    _aiLevel = ui->aiLevel->currentIndex() + 1;

    if (_mctsTree) {
        delete _mctsTree;
        _mctsTree = nullptr;
    }
    _mctsTree = new MCTSTree(_aiPlayerColor);

    updateField();

//    _testMoves = {
//        { 7 , 8 },
//        { 7 , 9 },
//        { 8 , 8 },
//        { 8 , 9 },
//        { 10 , 8 },
//        { 11 , 8 },
//        { 6 , 8 },
//        { 5 , 8 },
//        { 6 , 9 },
//        { 9 , 8 },
//        { 8 , 7 },
//        { 9 , 6 },
//        { 6 , 7 },
//        { 6 , 10 },
//        { 5 , 10 },
//        { 4 , 11 },
//        { 6 , 6 }
//    };
}


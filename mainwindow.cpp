#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "fieldwidget.h"
#include "field.h"
#include "ai.h"
#include <QMessageBox>
#include <QTimer>
#include <QDebug>
#include <QElapsedTimer>
#include "debug.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    _fieldView = new FieldWidget();
    ui->FieldView->layout()->addWidget(_fieldView);

    _field = new Field();

    connect(ui->startGameButton, &QPushButton::clicked, this, &MainWindow::onNewGameStarted);
    connect(ui->showTreeButton, &QPushButton::clicked, this, &MainWindow::checkPattern);
    connect(ui->aiUpdateButton, &QPushButton::clicked, this, &MainWindow::updateAIOnce);
    connect(_fieldView, &FieldWidget::mouseClicked, this, &MainWindow::onFieldClick);

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
    Debug::getInstance().registerCallTrackName(DebugCallTracks::GAME_UPDATE, "Update Game");


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

    if (_currentMode == MODE::VIEW_TREE) {
        return;
    }

    if (!_ai) {
        return;
    }

    Debug::getInstance().resetStats();
    Debug::getInstance().startTrack(DebugTimeTracks::GAME_UPDATE);

    _aiBudget += 15 * 1000000;
    if (_aiBudget <= 0) {
        if (_aiBudget < 15 * 10000000) {
            _aiBudget = 0;
        }
        return;
    }
    qint64 currentUpdateTime = 0;
    unsigned updatesCount = 0;
    QElapsedTimer updateTimer;
    updateTimer.start();

    qint64 updateStartTime = updateTimer.nsecsElapsed();
    do {
        _ai->update();

        _totalAiGames++;
        _currentAiGames++;
        updatesCount++;

        currentUpdateTime = updateTimer.nsecsElapsed() - updateStartTime;
        _aiBudget -= currentUpdateTime;
    } while (_aiBudget > currentUpdateTime);

    Debug::getInstance().stopTrack(DebugTimeTracks::GAME_UPDATE);
    Debug::getInstance().printStats(DebugTrackLevel::DEBUG);

    ui->aiTotalGames->setText(QString::number(_totalAiGames));
    ui->aiCurrentGames->setText(QString::number(_currentAiGames));
}

void MainWindow::updateAIOnce() {
    if(!_isGameStarted) {
        return;
    }

    if (!_ai) {
        return;
    }

    _ai->update();

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

    if (!_ai) {
        return;
    }

    if (_currentPlayer != _ai->getPlayerColor()) {
        return;
    }

    if (_ai->getRootPlayoursCount() > 1000 * _aiLevel) {
        auto bestMove = _ai->getBestNodePosition();
        makeMove(FieldMove::getXFromPosition(bestMove.position), FieldMove::getYFromPosition(bestMove.position));
    }
}

void MainWindow::updateField() {
    _fieldView->clearPieces();
    std::unordered_set<short> cells;
    for (auto& piece : _field->getMoves()) {
        cells.insert(piece);
        _fieldView->addPiece(FieldMove::getXFromPosition(piece) * FieldWidget::IMAGE_SIZE, FieldMove::getYFromPosition(piece) * FieldWidget::IMAGE_SIZE, _field->getBoardState()[piece]);
    }

    if (_ai && _currentMode != MODE::PLAY) {
        auto aiMoves = _ai->getPossibleMoves();
        for (auto& aiMove : aiMoves) {
            aiMove.x = FieldMove::getXFromPosition(aiMove.position) * FieldWidget::IMAGE_SIZE;
            aiMove.y = FieldMove::getYFromPosition(aiMove.position) * FieldWidget::IMAGE_SIZE;
            _fieldView->addAINextMove(aiMove);
            cells.insert(aiMove.position);
        }
    }

    for (short x = 0; x < BOARD_SIZE; ++x) {
        for (short y = 0; y < BOARD_SIZE; ++y) {
            short hashedPosition = FieldMove::getPositionFromPoint(x, y);
            if (cells.find(hashedPosition) != cells.end()) {
                continue;
            }

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

    if (_ai && _currentMode == MODE::PLAY && _currentPlayer == _ai->getPlayerColor()) {
        return;
    }

    if (_currentMode == MODE::VIEW_TREE) {
        short position = FieldMove::getPositionFromPoint(cellX, cellY);
        if (button == Qt::LeftButton) {
            if (_ai->goTreeForward(position)) {
                _currentPlayer = FieldMove::getNextColor(_currentPlayer);
                delete _field;
                _field = new Field(*_ai->getBoardState());
            }
        } else if (button == Qt::RightButton) {
            if (_ai->goBack()) {
                _currentPlayer = FieldMove::getNextColor(_currentPlayer);
                delete _field;
                _field = new Field(*_ai->getBoardState());
            }
        }

        updateField();
        return;
    }

    qDebug() << "_field->placePiece(" << cellX << ", " << cellY << ", " << _currentPlayer << ");";
    qDebug() << "_ai->goToNode(" << FieldMove::getPositionFromPoint(cellX, cellY) << ");";
    makeMove(cellX, cellY);
}

void MainWindow::makeMove(short x, short y) {
    if (!_field->placePiece(x, y, _currentPlayer)) {
        return;
    }

    _currentAiGames = 0;
    _ai->goToNode(FieldMove::getPositionFromPoint(x, y));
    _currentPlayer = FieldMove::getNextColor(_currentPlayer);

    updateField();

    if (_field->getFieldStatus() != FieldStatus::IN_PROGRESS) {
        _isGameStarted = false;

        std::string message = "Draw";
        if (_field->getFieldStatus() == FieldStatus::BLACK_WIN) {
            message = "X has won!";
        } else if (_field->getFieldStatus() == FieldStatus::WHITE_WIN) {
            message = "O has won!";
        }

        QMessageBox msgBox;
        msgBox.setText(message.c_str());
        msgBox.exec();

        return;
    }
}

void MainWindow::checkPattern() {
    _currentMode = MODE::VIEW_TREE;
    return;

    _isGameStarted = false;
    if (_ai) {
        delete _ai;
        _ai = nullptr;
    }

    _field->clear();
    _field->placePiece(9, 9, BLACK_PIECE_COLOR);
    _field->placePiece(10, 9, BLACK_PIECE_COLOR);
    _field->placePiece(11, 9, BLACK_PIECE_COLOR);

    qDebug() << AIDomainKnowledge::canMoveInDirection(AIDomainKnowledge::getNextMoveInDirection(29, AIDomainKnowledge::SearchDirection::DIAGONAL, 13 - 1, true), AIDomainKnowledge::SearchDirection::DIAGONAL, true);

    auto pattern = AIDomainKnowledge::generatePattern(_field, FieldMove::getPositionFromPoint(10, 9), BLACK_PIECE_COLOR, AIDomainKnowledge::SearchDirection::HORIZONTAL);
    if (pattern.pattern == 7 && pattern.blankPattern == 99) {
        qDebug() << "First test passed. " << pattern.patternStart << " " << pattern.patternEnd << " " << QString::number(pattern.pattern, 2) << " " << QString::number(pattern.blankPattern, 2);
    } else {
        qDebug() << "First test failed " << pattern.patternStart << " " << pattern.patternEnd << " " << QString::number(pattern.pattern, 2) << " " << QString::number(pattern.blankPattern, 2);
    }

    _field->clear();
    _field->placePiece(9, 9, BLACK_PIECE_COLOR);
    _field->placePiece(10, 9, BLACK_PIECE_COLOR);
    _field->placePiece(11, 9, BLACK_PIECE_COLOR);

    pattern = AIDomainKnowledge::generatePattern(_field, FieldMove::getPositionFromPoint(11, 9), BLACK_PIECE_COLOR, AIDomainKnowledge::SearchDirection::HORIZONTAL);
    if (pattern.pattern == 7 && pattern.blankPattern == 99) {
        qDebug() << "First test passed. " << pattern.patternStart << " " << pattern.patternEnd << " " << QString::number(pattern.pattern, 2) << " " << QString::number(pattern.blankPattern, 2);
    } else {
        qDebug() << "First test failed " << pattern.patternStart << " " << pattern.patternEnd << " " << QString::number(pattern.pattern, 2) << " " << QString::number(pattern.blankPattern, 2);
    }

    _field->clear();
    _field->placePiece(9, 9, BLACK_PIECE_COLOR);
    _field->placePiece(9, 10, BLACK_PIECE_COLOR);
    _field->placePiece(9, 11, BLACK_PIECE_COLOR);

    pattern = AIDomainKnowledge::generatePattern(_field, FieldMove::getPositionFromPoint(9, 11), BLACK_PIECE_COLOR, AIDomainKnowledge::SearchDirection::VERTICAL);
    if (pattern.pattern == 7 && pattern.blankPattern == 99) {
        qDebug() << "First test passed. " << pattern.patternStart << " " << pattern.patternEnd << " " << QString::number(pattern.pattern, 2) << " " << QString::number(pattern.blankPattern, 2);
    } else {
        qDebug() << "First test failed " << pattern.patternStart << " " << pattern.patternEnd << " " << QString::number(pattern.pattern, 2) << " " << QString::number(pattern.blankPattern, 2);
    }

    _field->clear();
    _field->placePiece(9, 9, BLACK_PIECE_COLOR);
    _field->placePiece(10, 9, BLACK_PIECE_COLOR);
    _field->placePiece(12, 9, BLACK_PIECE_COLOR);
    _field->placePiece(13, 9, BLACK_PIECE_COLOR);

    pattern = AIDomainKnowledge::generatePattern(_field, FieldMove::getPositionFromPoint(13, 9), BLACK_PIECE_COLOR, AIDomainKnowledge::SearchDirection::HORIZONTAL);
    if (pattern.pattern == 27 && pattern.blankPattern == 387) {
        qDebug() << "First test passed. " << pattern.patternStart << " " << pattern.patternEnd << " " << QString::number(pattern.pattern, 2) << " " << QString::number(pattern.blankPattern, 2);
    } else {
        qDebug() << "First test failed " << pattern.patternStart << " " << pattern.patternEnd << " " << QString::number(pattern.pattern, 2) << " " << QString::number(pattern.blankPattern, 2);
    }


    _field->clear();
    _field->placePiece(6, 9, BLACK_PIECE_COLOR);
    _field->placePiece(7, 9, BLACK_PIECE_COLOR);
    _field->placePiece(8, 9, BLACK_PIECE_COLOR);
    _field->placePiece(10, 9, BLACK_PIECE_COLOR);
    _field->placePiece(11, 9, BLACK_PIECE_COLOR);
    _field->placePiece(12, 9, BLACK_PIECE_COLOR);
    _field->placePiece(13, 9, BLACK_PIECE_COLOR);
    _field->placePiece(15, 9, BLACK_PIECE_COLOR);
    _field->placePiece(16, 9, BLACK_PIECE_COLOR);
    _field->placePiece(17, 9, BLACK_PIECE_COLOR);

    pattern = AIDomainKnowledge::generatePattern(_field, FieldMove::getPositionFromPoint(13, 9), BLACK_PIECE_COLOR, AIDomainKnowledge::SearchDirection::HORIZONTAL);
    if (pattern.pattern == 15 && pattern.blankPattern == 33) {
        qDebug() << "First test passed. " << pattern.patternStart << " " << pattern.patternEnd << " " << QString::number(pattern.pattern, 2) << " " << QString::number(pattern.blankPattern, 2);
    } else {
        qDebug() << "First test failed " << pattern.patternStart << " " << pattern.patternEnd << " " << QString::number(pattern.pattern, 2) << " " << QString::number(pattern.blankPattern, 2);
    }

    _field->clear();
    _field->placePiece(6, 9, BLACK_PIECE_COLOR);
    _field->placePiece(7, 9, BLACK_PIECE_COLOR);
    _field->placePiece(8, 9, BLACK_PIECE_COLOR);
    _field->placePiece(10, 9, BLACK_PIECE_COLOR);
    _field->placePiece(11, 9, BLACK_PIECE_COLOR);
    _field->placePiece(12, 9, BLACK_PIECE_COLOR);
    _field->placePiece(13, 9, BLACK_PIECE_COLOR);
    _field->placePiece(15, 9, BLACK_PIECE_COLOR);
    _field->placePiece(16, 9, BLACK_PIECE_COLOR);
    _field->placePiece(17, 9, BLACK_PIECE_COLOR);

    pattern = AIDomainKnowledge::generatePattern(_field, FieldMove::getPositionFromPoint(6, 9), BLACK_PIECE_COLOR, AIDomainKnowledge::SearchDirection::HORIZONTAL);
    if (pattern.pattern == 23 && pattern.blankPattern == 3) {
        qDebug() << "First test passed. " << pattern.patternStart << " " << pattern.patternEnd << " " << QString::number(pattern.pattern, 2) << " " << QString::number(pattern.blankPattern, 2);
    } else {
        qDebug() << "First test failed " << pattern.patternStart << " " << pattern.patternEnd << " " << QString::number(pattern.pattern, 2) << " " << QString::number(pattern.blankPattern, 2);
    }

    _field->clear();
    _field->placePiece(7, 9, WHITE_PIECE_COLOR);
    _field->placePiece(8, 9, WHITE_PIECE_COLOR);
    _field->placePiece(9, 9, BLACK_PIECE_COLOR);
    _field->placePiece(10, 9, BLACK_PIECE_COLOR);
    _field->placePiece(11, 9, BLACK_PIECE_COLOR);

    pattern = AIDomainKnowledge::generatePattern(_field, FieldMove::getPositionFromPoint(11, 9), BLACK_PIECE_COLOR, AIDomainKnowledge::SearchDirection::HORIZONTAL);
    qDebug() << "Pattern " << pattern.patternStart << " " << pattern.patternEnd << " " << QString::number(pattern.pattern, 2) << " " << QString::number(pattern.blankPattern, 2) << pattern.pattern << pattern.blankPattern;


    // check defence

//    QElapsedTimer updateTimer;
//    updateTimer.start();

//    _field->clear();
//    _field->placePiece(9, 9, BLACK_PIECE_COLOR);
//    _field->placePiece(10, 9, BLACK_PIECE_COLOR);
//    _field->placePiece(12, 9, BLACK_PIECE_COLOR);
//    _field->placePiece(13, 9, BLACK_PIECE_COLOR);

//    auto defenceMoves = AIDomainKnowledge::generateDefensiveMoves(_field, FieldMove::getPositionFromPoint(13, 9), BLACK_PIECE_COLOR);
//    if (defenceMoves.size() == 1 && defenceMoves[0] == 182) {
//        qDebug() << "Test passed...pattern: 0110110";
//    } else {
//        qDebug() << "Test failed...pattern: 0110110";
//    }

//    _field->clear();
//    _field->placePiece(9, 9, BLACK_PIECE_COLOR);
//    _field->placePiece(10, 9, BLACK_PIECE_COLOR);
//    _field->placePiece(11, 9, BLACK_PIECE_COLOR);
//    _field->placePiece(13, 9, BLACK_PIECE_COLOR);

//    defenceMoves = AIDomainKnowledge::generateDefensiveMoves(_field, FieldMove::getPositionFromPoint(11, 9), BLACK_PIECE_COLOR);
//    if (defenceMoves.size() == 1 && defenceMoves[0] == 183) {
//        qDebug() << "Test passed...pattern: 10111";
//    } else {
//        qDebug() << "Test failed...pattern: 10111";
//    }

//    _field->clear();
//    _field->placePiece(9, 9, BLACK_PIECE_COLOR);
//    _field->placePiece(11, 9, BLACK_PIECE_COLOR);
//    _field->placePiece(12, 9, BLACK_PIECE_COLOR);
//    _field->placePiece(13, 9, BLACK_PIECE_COLOR);

//    defenceMoves = AIDomainKnowledge::generateDefensiveMoves(_field, FieldMove::getPositionFromPoint(9, 9), BLACK_PIECE_COLOR);
//    if (defenceMoves.size() == 1 && defenceMoves[0] == 181) {
//        qDebug() << "Test passed...pattern: 11101";
//    } else {
//        qDebug() << "Test failed...pattern: 11101";
//    }

//    _field->clear();
//    _field->placePiece(9, 9, BLACK_PIECE_COLOR);
//    _field->placePiece(11, 9, BLACK_PIECE_COLOR);
//    _field->placePiece(12, 9, BLACK_PIECE_COLOR);
//    _field->placePiece(13, 9, BLACK_PIECE_COLOR);

//    defenceMoves = AIDomainKnowledge::generateDefensiveMoves(_field, FieldMove::getPositionFromPoint(13, 9), BLACK_PIECE_COLOR);
//    if (defenceMoves.size() == 1 && defenceMoves[0] == 181) {
//        qDebug() << "Test passed...pattern: 11101";
//    } else {
//        qDebug() << "Test failed...pattern: 11101";
//    }

//    _field->clear();
//    _field->placePiece(9, 9, BLACK_PIECE_COLOR);
//    _field->placePiece(9, 11, BLACK_PIECE_COLOR);
//    _field->placePiece(9, 12, BLACK_PIECE_COLOR);
//    _field->placePiece(9, 13, BLACK_PIECE_COLOR);

//    defenceMoves = AIDomainKnowledge::generateDefensiveMoves(_field, FieldMove::getPositionFromPoint(9, 12), BLACK_PIECE_COLOR);
//    if (defenceMoves.size() == 1 && defenceMoves[0] == 199) {
//        qDebug() << "Test passed...pattern: 11101 . vertical";
//    } else {
//        qDebug() << "Test failed...pattern: 11101 . vertical";
//    }

//    _field->clear();
//    _field->placePiece(9, 9, BLACK_PIECE_COLOR);
//    _field->placePiece(10, 9, BLACK_PIECE_COLOR);
//    _field->placePiece(11, 9, BLACK_PIECE_COLOR);
//    _field->placePiece(12, 9, BLACK_PIECE_COLOR);

//    defenceMoves = AIDomainKnowledge::generateDefensiveMoves(_field, FieldMove::getPositionFromPoint(10, 9), BLACK_PIECE_COLOR);
//    if (defenceMoves.size() == 1 && defenceMoves[0] == 179) {
//        qDebug() << "Test passed...pattern: 011110";
//    } else {
//        qDebug() << "Test failed...pattern: 011110";
//    }

//    _field->clear();
//    _field->placePiece(8, 9, WHITE_PIECE_COLOR);
//    _field->placePiece(9, 9, BLACK_PIECE_COLOR);
//    _field->placePiece(10, 9, BLACK_PIECE_COLOR);
//    _field->placePiece(11, 9, BLACK_PIECE_COLOR);
//    _field->placePiece(12, 9, BLACK_PIECE_COLOR);

//    defenceMoves = AIDomainKnowledge::generateDefensiveMoves(_field, FieldMove::getPositionFromPoint(10, 9), BLACK_PIECE_COLOR);
//    if (defenceMoves.size() == 1 && defenceMoves[0] == 184) {
//        qDebug() << "Test passed...pattern: 211110";
//    } else {
//        qDebug() << "Test failed...pattern: 211110";
//    }

//    _field->clear();
//    _field->placePiece(13, 9, WHITE_PIECE_COLOR);
//    _field->placePiece(9, 9, BLACK_PIECE_COLOR);
//    _field->placePiece(10, 9, BLACK_PIECE_COLOR);
//    _field->placePiece(11, 9, BLACK_PIECE_COLOR);
//    _field->placePiece(12, 9, BLACK_PIECE_COLOR);

//    defenceMoves = AIDomainKnowledge::generateDefensiveMoves(_field, FieldMove::getPositionFromPoint(10, 9), BLACK_PIECE_COLOR);
//    if (defenceMoves.size() == 1 && defenceMoves[0] == 179) {
//        qDebug() << "Test passed...pattern: 011112";
//    } else {
//        qDebug() << "Test failed...pattern: 011112";
//    }

//    _field->clear();
//    _field->placePiece(8, 9, BLACK_PIECE_COLOR);
//    _field->placePiece(9, 9, BLACK_PIECE_COLOR);
//    _field->placePiece(10, 9, BLACK_PIECE_COLOR);

//    defenceMoves = AIDomainKnowledge::generateDefensiveMoves(_field, FieldMove::getPositionFromPoint(10, 9), BLACK_PIECE_COLOR);
//    if (defenceMoves.size() == 2 && defenceMoves[0] == 178 && defenceMoves[1] == 182) {
//        qDebug() << "Test passed...pattern: 0011100";
//    } else {
//        qDebug() << "Test failed...pattern: 0011100";
//    }

//    _field->clear();
//    _field->placePiece(6, 9, WHITE_PIECE_COLOR);
//    _field->placePiece(8, 9, BLACK_PIECE_COLOR);
//    _field->placePiece(9, 9, BLACK_PIECE_COLOR);
//    _field->placePiece(10, 9, BLACK_PIECE_COLOR);

//    defenceMoves = AIDomainKnowledge::generateDefensiveMoves(_field, FieldMove::getPositionFromPoint(10, 9), BLACK_PIECE_COLOR);
//    if (defenceMoves.size() == 2 && defenceMoves[0] == 178 && defenceMoves[1] == 182) {
//        qDebug() << "Test passed...pattern: 2011100";
//    } else {
//        qDebug() << "Test failed...pattern: 2011100";
//    }

//    _field->clear();
//    _field->placePiece(12, 9, WHITE_PIECE_COLOR);
//    _field->placePiece(8, 9, BLACK_PIECE_COLOR);
//    _field->placePiece(9, 9, BLACK_PIECE_COLOR);
//    _field->placePiece(10, 9, BLACK_PIECE_COLOR);

//    defenceMoves = AIDomainKnowledge::generateDefensiveMoves(_field, FieldMove::getPositionFromPoint(10, 9), BLACK_PIECE_COLOR);
//    if (defenceMoves.size() == 2 && defenceMoves[0] == 178 && defenceMoves[1] == 182) {
//        qDebug() << "Test passed...pattern: 0011102";
//    } else {
//        qDebug() << "Test failed...pattern: 0011102";
//    }

//    _field->clear();
//    _field->placePiece(7, 9, WHITE_PIECE_COLOR);
//    _field->placePiece(8, 9, BLACK_PIECE_COLOR);
//    _field->placePiece(9, 9, BLACK_PIECE_COLOR);
//    _field->placePiece(10, 9, BLACK_PIECE_COLOR);

//    defenceMoves = AIDomainKnowledge::generateDefensiveMoves(_field, FieldMove::getPositionFromPoint(10, 9), BLACK_PIECE_COLOR);
//    if (defenceMoves.empty()) {
//        qDebug() << "Test passed...pattern: 0211100";
//    } else {
//        qDebug() << "Test failed...pattern: 0211100";
//    }

//    _field->clear();
//    _field->placePiece(11, 9, WHITE_PIECE_COLOR);
//    _field->placePiece(8, 9, BLACK_PIECE_COLOR);
//    _field->placePiece(9, 9, BLACK_PIECE_COLOR);
//    _field->placePiece(10, 9, BLACK_PIECE_COLOR);

//    defenceMoves = AIDomainKnowledge::generateDefensiveMoves(_field, FieldMove::getPositionFromPoint(10, 9), BLACK_PIECE_COLOR);
//    if (defenceMoves.empty()) {
//        qDebug() << "Test passed...pattern: 0011120";
//    } else {
//        qDebug() << "Test failed...pattern: 0011120";
//    }

//    _field->clear();
//    _field->placePiece(16, 2, BLACK_PIECE_COLOR);
//    _field->placePiece(15, 1, BLACK_PIECE_COLOR);
//    _field->placePiece(17, 3, BLACK_PIECE_COLOR);

//    defenceMoves = AIDomainKnowledge::generateDefensiveMoves(_field, FieldMove::getPositionFromPoint(15, 1), BLACK_PIECE_COLOR);
//    if (defenceMoves.empty()) {
//        qDebug() << "Test passed...pattern: 0011120";
//    } else {
//        qDebug() << "Test failed...pattern: 0011120" << defenceMoves;
//    }

//    pattern = AIDomainKnowledge::generatePattern(_field, FieldMove::getPositionFromPoint(15, 1), BLACK_PIECE_COLOR, AIDomainKnowledge::SearchDirection::DIAGONAL);
//    qDebug() << "01110 . diagonal " << pattern.patternStart << " " << pattern.patternEnd << " " << QString::number(pattern.pattern, 2) << " " << QString::number(pattern.blankPattern, 2);

//    _field->clear();
//    _field->placePiece(9, 15, BLACK_PIECE_COLOR);
//    _field->placePiece(8, 16, BLACK_PIECE_COLOR);
//    _field->placePiece(7, 17, BLACK_PIECE_COLOR);
//    _field->placePiece(6, 18, BLACK_PIECE_COLOR);

//    defenceMoves = AIDomainKnowledge::generateDefensiveMoves(_field, FieldMove::getPositionFromPoint(7, 17), BLACK_PIECE_COLOR);
//    if (defenceMoves.empty()) {
//        qDebug() << "Test passed...pattern: 0011120";
//    } else {
//        qDebug() << "Test failed...pattern: 0011120" << defenceMoves;
//    }

//    pattern = AIDomainKnowledge::generatePattern(_field, FieldMove::getPositionFromPoint(7, 17), BLACK_PIECE_COLOR, AIDomainKnowledge::SearchDirection::DIAGONAL_REVERSE);
//    qDebug() << "01111x " << pattern.patternStart << " " << pattern.patternEnd << " " << QString::number(pattern.pattern, 2) << " " << QString::number(pattern.blankPattern, 2);


//    qDebug() << "Performance: " << updateTimer.nsecsElapsed();
}

void MainWindow::onNewGameStarted() {
    _isGameStarted = true;
    _field->clear();
    _currentPlayer = BLACK_PIECE_COLOR;
    _aiPlayerColor = BLACK_PIECE_COLOR;
    _aiLevel = ui->aiLevel->currentIndex() + 1;

    if (_ai) {
        delete _ai;
        _ai = nullptr;
    }
    _ai = new AI(_aiPlayerColor);

    updateField();
}


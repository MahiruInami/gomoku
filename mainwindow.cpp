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
    connect(_fieldView, &FieldWidget::mouseClicked, this, &MainWindow::onFieldClick);

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateAI()));
    timer->start(16);

    QTimer *timer2 = new QTimer(this);
    connect(timer2, SIGNAL(timeout()), this, SLOT(updateAIField()));
    timer2->start(2000);

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

    if (!_ai) {
        return;
    }

    Debug::getInstance().resetStats();
    Debug::getInstance().startTrack(DebugTimeTracks::GAME_UPDATE);

    qint64 timeLeft = 15 * 1000000;
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
        timeLeft -= currentUpdateTime;
    } while (timeLeft > currentUpdateTime);

    Debug::getInstance().stopTrack(DebugTimeTracks::GAME_UPDATE);
    Debug::getInstance().printStats(DebugTrackLevel::DEBUG);

    ui->aiTotalGames->setText(QString::number(_totalAiGames));
    ui->aiCurrentGames->setText(QString::number(_currentAiGames));
}

void MainWindow::updateAIField() {
    updateField();
}

void MainWindow::onFieldClick(short x, short y) {
    if (!_isGameStarted) {
        return;
    }

    short cellX = x / FieldWidget::IMAGE_SIZE;
    short cellY = y / FieldWidget::IMAGE_SIZE;

    if (cellX < 0 || cellX >= BOARD_SIZE || cellY < 0 || cellY >= BOARD_SIZE) {
        return;
    }

    qDebug() << "_field->placePiece(" << cellX << ", " << cellY << ", " << _currentPlayer << ");";
    qDebug() << "_ai->goToNode(" << cellX << ", " << cellY << ", " << _currentPlayer << ");";
    makeMove(cellX, cellY);
}

void MainWindow::updateField() {
    _fieldView->clearPieces();
    std::unordered_set<short> cells;
    for (auto& piece : _field->getMoves()) {
        cells.insert(piece);
        _fieldView->addPiece(FieldMove::getXFromPosition(piece) * FieldWidget::IMAGE_SIZE, FieldMove::getYFromPosition(piece) * FieldWidget::IMAGE_SIZE, _field->getBoardState()[piece]);
    }

    if (_ai) {
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
    _isGameStarted = false;
    if (_ai) {
        delete _ai;
        _ai = nullptr;
    }

    _field->clear();
    _field->placePiece(9, 9, BLACK_PIECE_COLOR);
    _field->placePiece(10, 9, BLACK_PIECE_COLOR);
    _field->placePiece(11, 9, BLACK_PIECE_COLOR);

    auto pattern = AIDomainKnowledge::generatePattern(_field, FieldMove::getPositionFromPoint(10, 9), BLACK_PIECE_COLOR, AIDomainKnowledge::SearchDirection::HORIZONTAL);
    if (pattern.pattern == 7 && pattern.blankPattern == 455) {
        qDebug() << "First test passed. " << pattern.patternStart << " " << pattern.patternEnd << " " << QString::number(pattern.pattern, 2) << " " << QString::number(pattern.blankPattern, 2);
    } else {
        qDebug() << "First test failed " << pattern.patternStart << " " << pattern.patternEnd << " " << QString::number(pattern.pattern, 2) << " " << QString::number(pattern.blankPattern, 2);
    }

    _field->clear();
    _field->placePiece(9, 9, BLACK_PIECE_COLOR);
    _field->placePiece(10, 9, BLACK_PIECE_COLOR);
    _field->placePiece(11, 9, BLACK_PIECE_COLOR);

    pattern = AIDomainKnowledge::generatePattern(_field, FieldMove::getPositionFromPoint(11, 9), BLACK_PIECE_COLOR, AIDomainKnowledge::SearchDirection::HORIZONTAL);
    if (pattern.pattern == 7 && pattern.blankPattern == 455) {
        qDebug() << "First test passed. " << pattern.patternStart << " " << pattern.patternEnd << " " << QString::number(pattern.pattern, 2) << " " << QString::number(pattern.blankPattern, 2);
    } else {
        qDebug() << "First test failed " << pattern.patternStart << " " << pattern.patternEnd << " " << QString::number(pattern.pattern, 2) << " " << QString::number(pattern.blankPattern, 2);
    }

    _field->clear();
    _field->placePiece(9, 9, BLACK_PIECE_COLOR);
    _field->placePiece(9, 10, BLACK_PIECE_COLOR);
    _field->placePiece(9, 11, BLACK_PIECE_COLOR);

    pattern = AIDomainKnowledge::generatePattern(_field, FieldMove::getPositionFromPoint(9, 11), BLACK_PIECE_COLOR, AIDomainKnowledge::SearchDirection::VERTICAL);
    if (pattern.pattern == 7 && pattern.blankPattern == 455) {
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
    if (pattern.pattern == 27 && pattern.blankPattern == 112) {
        qDebug() << "First test passed. " << pattern.patternStart << " " << pattern.patternEnd << " " << QString::number(pattern.pattern, 2) << " " << QString::number(pattern.blankPattern, 2);
    } else {
        qDebug() << "First test failed " << pattern.patternStart << " " << pattern.patternEnd << " " << QString::number(pattern.pattern, 2) << " " << QString::number(pattern.blankPattern, 2);
    }


    // check defence

    _field->clear();
    _field->placePiece(9, 9, BLACK_PIECE_COLOR);
    _field->placePiece(10, 9, BLACK_PIECE_COLOR);
    _field->placePiece(12, 9, BLACK_PIECE_COLOR);
    _field->placePiece(13, 9, BLACK_PIECE_COLOR);

    AIDomainKnowledge::generateDefensiveMoves(_field, FieldMove::getPositionFromPoint(13, 9), BLACK_PIECE_COLOR);

    _field->clear();
    _field->placePiece(9, 9, BLACK_PIECE_COLOR);
    _field->placePiece(10, 9, BLACK_PIECE_COLOR);
    _field->placePiece(11, 9, BLACK_PIECE_COLOR);
    _field->placePiece(13, 9, BLACK_PIECE_COLOR);

    AIDomainKnowledge::generateDefensiveMoves(_field, FieldMove::getPositionFromPoint(11, 9), BLACK_PIECE_COLOR);
}

void MainWindow::onNewGameStarted() {
    _isGameStarted = true;
    _field->clear();
    _currentPlayer = BLACK_PIECE_COLOR;

    if (_ai) {
        delete _ai;
        _ai = nullptr;
    }
    _ai = new AI(WHITE_PIECE_COLOR);

//    _field->placePiece(9, 9, BLACK_PIECE_COLOR);
//    _field->placePiece(10, 8, WHITE_PIECE_COLOR);
//    _field->placePiece(9, 10, BLACK_PIECE_COLOR);
//    _field->placePiece(9, 8, WHITE_PIECE_COLOR);
//    _field->placePiece(8, 8, BLACK_PIECE_COLOR);
//    _field->placePiece(8, 7, WHITE_PIECE_COLOR);
//    _field->placePiece(10, 10, BLACK_PIECE_COLOR);
//    _field->placePiece(7, 7, WHITE_PIECE_COLOR);
//    _field->placePiece(8, 10, BLACK_PIECE_COLOR);
//    _field->placePiece(11, 10, WHITE_PIECE_COLOR);
//    _field->placePiece(7, 10, BLACK_PIECE_COLOR);

//    _field->placePiece(6, 10, WHITE_PIECE_COLOR);
//    _field->placePiece(10, 9, BLACK_PIECE_COLOR);
//    _field->placePiece(9, 7, WHITE_PIECE_COLOR);
//    _field->placePiece(11, 11, BLACK_PIECE_COLOR);
//    _field->placePiece(12, 12, WHITE_PIECE_COLOR);
//    _field->placePiece(10, 7, BLACK_PIECE_COLOR);
//    _field->placePiece(6, 7, WHITE_PIECE_COLOR);
//    _field->placePiece(5, 7, BLACK_PIECE_COLOR);
//    _field->placePiece(11, 9, WHITE_PIECE_COLOR);
//    _field->placePiece(8, 6, BLACK_PIECE_COLOR);
//    _field->placePiece(11, 8, WHITE_PIECE_COLOR);
//    _field->placePiece(7, 9, BLACK_PIECE_COLOR);
//    _field->placePiece(12, 10, WHITE_PIECE_COLOR);
//    _field->placePiece(13, 11, BLACK_PIECE_COLOR);
//    _field->placePiece(6, 8, WHITE_PIECE_COLOR);
//    _field->placePiece(6, 9, BLACK_PIECE_COLOR);

//    _ai->goToNode(FieldMove::getPositionFromPoint(9, 9));
//    _ai->goToNode(FieldMove::getPositionFromPoint(10, 8));
//    _ai->goToNode(FieldMove::getPositionFromPoint(9, 10));
//    _ai->goToNode(FieldMove::getPositionFromPoint(9, 8));
//    _ai->goToNode(FieldMove::getPositionFromPoint(8, 8));
//    _ai->goToNode(FieldMove::getPositionFromPoint(8, 7));
//    _ai->goToNode(FieldMove::getPositionFromPoint(10, 10));
//    _ai->goToNode(FieldMove::getPositionFromPoint(7, 7));
//    _ai->goToNode(FieldMove::getPositionFromPoint(8, 10));
//    _ai->goToNode(FieldMove::getPositionFromPoint(11, 10));
//    _ai->goToNode(FieldMove::getPositionFromPoint(7, 10));

//    _ai->goToNode(FieldMove::getPositionFromPoint(6, 10));
//    _ai->goToNode(FieldMove::getPositionFromPoint(10, 9));
//    _ai->goToNode(FieldMove::getPositionFromPoint(9, 7));
//    _ai->goToNode(FieldMove::getPositionFromPoint(11, 11));
//    _ai->goToNode(FieldMove::getPositionFromPoint(12, 12));
//    _ai->goToNode(FieldMove::getPositionFromPoint(10, 7));
//    _ai->goToNode(FieldMove::getPositionFromPoint(6, 7));
//    _ai->goToNode(FieldMove::getPositionFromPoint(5, 7));
//    _ai->goToNode(FieldMove::getPositionFromPoint(11, 9));
//    _ai->goToNode(FieldMove::getPositionFromPoint(8, 6));
//    _ai->goToNode(FieldMove::getPositionFromPoint(11, 8));
//    _ai->goToNode(FieldMove::getPositionFromPoint(7, 9));
//    _ai->goToNode(FieldMove::getPositionFromPoint(12, 10));
//    _ai->goToNode(FieldMove::getPositionFromPoint(13, 11));
//    _ai->goToNode(FieldMove::getPositionFromPoint(6, 8));
//    _ai->goToNode(FieldMove::getPositionFromPoint(6, 9));

//    _currentPlayer = WHITE_PIECE_COLOR;

    updateField();
}


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
    connect(_fieldView, &FieldWidget::mouseClicked, this, &MainWindow::onFieldClick);

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateAI()));
    timer->start(16);

    QTimer *timer2 = new QTimer(this);
    connect(timer2, SIGNAL(timeout()), this, SLOT(updateAIField()));
    timer2->start(2000);

    Debug::getInstance().registerTimeTrackName(DebugTimeTracks::GAME_UPDATE, "Update AI");
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
    for (int i = 0; i < 16; i++) {
        _ai->update();

        _totalAiGames++;
        _currentAiGames++;
    }
    Debug::getInstance().stopTrack(DebugTimeTracks::GAME_UPDATE);


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

void MainWindow::onNewGameStarted() {
    _isGameStarted = true;
    _field->clear();
    _currentPlayer = BLACK_PIECE_COLOR;

    if (_ai) {
        delete _ai;
        _ai = nullptr;
    }
    _ai = new AI(WHITE_PIECE_COLOR);
    for (int i = 0; i < 100; i++) {
        _ai->update();
    }

    updateField();
}


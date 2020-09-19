#include "fieldwidget.h"
#include "ui_fieldwidget.h"
#include <QPainter>

FieldWidget::FieldWidget(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::FieldWidget)
{
    ui->setupUi(this);
}

FieldWidget::~FieldWidget()
{
    delete ui;
}

void FieldWidget::addPiece(short x, short y, short color) {
    _pieces.push_back(std::make_tuple(x, y, color));
}

void FieldWidget::addAINextMove(AIMoveData moveData) {
    _aiMoves[moveData.position] = moveData;
}

void FieldWidget::addAIBestPlayoutMove(AIMoveData moveData) {
    _aiBestPlayout[moveData.position] = moveData;
}

void FieldWidget::clearPieces() {
    _pieces.clear();
    _aiMoves.clear();
    _aiBestPlayout.clear();
}

void FieldWidget::mousePressEvent(QMouseEvent *event)
{
    mouseClicked(event->x(), event->y(), event->button());
}

void FieldWidget::paintEvent(QPaintEvent* /* event */) {
    QPainter painter(this);

    for (auto& piece : _pieces) {
        std::string image = ":/images/empty_piece.png";
        auto pieceColor = std::get<2>(piece);
        if (pieceColor == 1) {
            image = ":/images/black_piece.png";
        } else if (pieceColor == 2) {
            image = ":/images/white_piece.png";
        }
        painter.drawImage(QRect(std::get<0>(piece), std::get<1>(piece), IMAGE_SIZE, IMAGE_SIZE), QImage(image.c_str()));
    }

    short bestMove = 0;
    float bestScore = -1.f;
    for (auto& aiMove : _aiMoves) {
//        if ((aiMove.second.scores > bestScore && aiMove.second.color == 2) || (aiMove.second.scores < bestScore && aiMove.second.color == 1)) {
        if (aiMove.second.scores > bestScore) {
            bestScore = aiMove.second.scores;
            bestMove = aiMove.first;
        }
    }

    for (auto& aiMove : _aiMoves) {
        std::string image = ":/images/empty_piece.png";
        std::string postfix = aiMove.first == bestMove ? "_best" : "";
        if (aiMove.second.color == 1) {
            image = ":/images/black_piece_ai" + postfix + ".png";
        } else if (aiMove.second.color == 2) {
            image = ":/images/white_piece_ai" + postfix + ".png";
        }
        painter.drawImage(QRect(aiMove.second.x, aiMove.second.y, IMAGE_SIZE, IMAGE_SIZE), QImage(image.c_str()));

        std::string playouts = "";
        if (aiMove.second.nodeVisits < 1000) {
            playouts = std::to_string(aiMove.second.nodeVisits);
        } else if (aiMove.second.nodeVisits < 10000) {
            playouts = std::to_string(static_cast<int>(aiMove.second.nodeVisits / 1000.f)) + std::string(".") + std::to_string(static_cast<int>(aiMove.second.nodeVisits / 100.f)) + std::string("K");
        } else if (aiMove.second.nodeVisits < 100000) {
            playouts = std::to_string(static_cast<int>(aiMove.second.nodeVisits / 10000.f)) + std::string(".") + std::to_string(static_cast<int>(aiMove.second.nodeVisits / 1000.f)) + std::string("T");
        } else if (aiMove.second.nodeVisits < 1000000) {
            playouts = std::to_string(static_cast<int>(aiMove.second.nodeVisits / 100000.f)) + std::string(".") + std::to_string(static_cast<int>(aiMove.second.nodeVisits / 10000.f)) + std::string("M");
        } else if (aiMove.second.nodeVisits < 1000000000) {
            playouts = std::to_string(static_cast<int>(aiMove.second.nodeVisits / 1000000.f)) + std::string(".") + std::to_string(static_cast<int>(aiMove.second.nodeVisits / 100000.f)) + std::string("B");
        }

        painter.drawText(QPoint(aiMove.second.x + 1, aiMove.second.y + IMAGE_SIZE - 4), QString::number(aiMove.second.scores, 'f', 2));
        painter.drawText(QPoint(aiMove.second.x + 1, aiMove.second.y + IMAGE_SIZE - 12), QString::number(aiMove.second.selectionScore, 'f', 2));
        painter.drawText(QPoint(aiMove.second.x + 2, aiMove.second.y + 12), playouts.c_str());
    }

    for (auto& aiMove : _aiBestPlayout) {
        std::string image = ":/images/empty_piece.png";
        if (aiMove.second.color == 1) {
            image = ":/images/black_piece.png";
        } else if (aiMove.second.color == 2) {
            image = ":/images/white_piece.png";
        }
        painter.drawImage(QRect(aiMove.second.x, aiMove.second.y, IMAGE_SIZE, IMAGE_SIZE), QImage(image.c_str()));

        std::string playouts = "";
        if (aiMove.second.nodeVisits < 1000) {
            playouts = std::to_string(aiMove.second.nodeVisits);
        } else if (aiMove.second.nodeVisits < 10000) {
            playouts = std::to_string(static_cast<int>(aiMove.second.nodeVisits / 1000.f)) + std::string(".") + std::to_string(static_cast<int>(aiMove.second.nodeVisits / 100.f)) + std::string("K");
        } else if (aiMove.second.nodeVisits < 100000) {
            playouts = std::to_string(static_cast<int>(aiMove.second.nodeVisits / 10000.f)) + std::string(".") + std::to_string(static_cast<int>(aiMove.second.nodeVisits / 1000.f)) + std::string("T");
        } else if (aiMove.second.nodeVisits < 1000000) {
            playouts = std::to_string(static_cast<int>(aiMove.second.nodeVisits / 100000.f)) + std::string(".") + std::to_string(static_cast<int>(aiMove.second.nodeVisits / 10000.f)) + std::string("M");
        } else if (aiMove.second.nodeVisits < 1000000000) {
            playouts = std::to_string(static_cast<int>(aiMove.second.nodeVisits / 1000000.f)) + std::string(".") + std::to_string(static_cast<int>(aiMove.second.nodeVisits / 100000.f)) + std::string("B");
        }

        painter.drawText(QPoint(aiMove.second.x + 1, aiMove.second.y + IMAGE_SIZE - 4), QString::number(aiMove.second.scores, 'f', 2));
        painter.drawText(QPoint(aiMove.second.x + 2, aiMove.second.y + 12), playouts.c_str());
    }
}



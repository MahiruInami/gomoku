#include "fieldwidget.h"
#include "ui_fieldwidget.h"
#include <QPainter>
#include <QMouseEvent>

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

void FieldWidget::clearPieces() {
    _pieces.clear();
    _aiMoves.clear();
}

void FieldWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        mouseClicked(event->x(), event->y());
    }
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

    for (auto& aiMove : _aiMoves) {
        std::string image = ":/images/empty_piece.png";
        if (aiMove.second.color == 1) {
            image = ":/images/black_piece_ai.png";
        } else if (aiMove.second.color == 2) {
            image = ":/images/white_piece_ai.png";
        }
        painter.drawImage(QRect(aiMove.second.x, aiMove.second.y, IMAGE_SIZE, IMAGE_SIZE), QImage(image.c_str()));

        std::string playouts = "";
        if (aiMove.second.nodeVisits < 1000) {
            playouts = std::to_string(aiMove.second.nodeVisits);
        } else {
            playouts = std::to_string(static_cast<int>(aiMove.second.nodeVisits / 1000.f)) + std::string(".") + std::to_string(static_cast<int>(aiMove.second.nodeVisits / 100.f)) + std::string("K");
        }

        painter.drawText(QPoint(aiMove.second.x + 1, aiMove.second.y + IMAGE_SIZE - 4), QString::number(aiMove.second.scores, 'f', 2));
//        painter.drawText(QPoint(aiMove.second.x + 1, aiMove.second.y + IMAGE_SIZE - 12), QString::number(aiMove.second.minMaxScores, 'f', 2));
        painter.drawText(QPoint(aiMove.second.x + 2, aiMove.second.y + 12), playouts.c_str());
    }
}



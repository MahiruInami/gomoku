#pragma once

#include <QFrame>
#include <QMouseEvent>
#include <unordered_map>
#include "ai.h"

namespace Ui {
class FieldWidget;
}

class FieldWidget : public QFrame
{
    Q_OBJECT

public:
    static constexpr short IMAGE_SIZE = 32;

    explicit FieldWidget(QWidget *parent = nullptr);
    ~FieldWidget();

    void addPiece(short x, short y, short color);
    void addAINextMove(AIMoveData moveData);
    void addAIBestPlayoutMove(AIMoveData moveData);
    void clearPieces();
signals:
    void mouseClicked(short x, short y, Qt::MouseButton button);
protected:
    void mousePressEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent* event) override;
private:
    Ui::FieldWidget *ui;

    std::vector<std::tuple<short, short, short>> _pieces;
    std::unordered_map<short, AIMoveData> _aiMoves;
    std::unordered_map<short, AIMoveData> _aiBestPlayout;
};


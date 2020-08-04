#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class FieldWidget;
class IField;
class AI;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
public slots:
    void onNewGameStarted();
    void checkPattern();
    void updateAI();
    void updateAIField();
    void updateAIOnce();
    void checkAIMove();
private:
    void onFieldClick(short x, short y, Qt::MouseButton button);
    void updateField();

    void makeMove(short x, short y);
private:
    enum class MODE {
        PLAY,
        DEBUG_MODE,
        FULL_DEBUG,
        VIEW_TREE
    } _currentMode = MODE::PLAY;

    Ui::MainWindow *ui;
    FieldWidget* _fieldView = nullptr;
    IField* _field = nullptr;
    AI* _ai = nullptr;

    bool _isGameStarted = false;
    short _currentPlayer = -1;
    short _aiPlayerColor = -1;

    unsigned _aiLevel = 1;
    unsigned _totalAiGames = 0;
    unsigned _currentAiGames = 0;

    qint64 _aiBudget = 0;
};
#endif // MAINWINDOW_H

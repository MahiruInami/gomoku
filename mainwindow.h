#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class FieldWidget;
class BitField;
class MCTSTree;

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

    void onlyBlackModeClick();
    void onlyWhiteModeClick();

    void showAIPlayouts();
private:
    void onFieldClick(short x, short y, Qt::MouseButton button);
    void updateField();

    void makeMove(short x, short y);

    short getCurrentColor() const {
        return _forcedColor > 0 ? _forcedColor : _currentPlayer;
    }
private:
    enum class MODE {
        PLAY,
        MANUAL_AI_DEBUG,
        DEBUG_MODE,
        FULL_DEBUG,
        VIEW_TREE
    } _currentMode = MODE::PLAY;

    bool _showBestPlayout = false;
    bool _showPlayouts = false;
    short _bestPlayoutX = -1, _bestPlayoutY = -1;

    Ui::MainWindow *ui;
    FieldWidget* _fieldView = nullptr;

    BitField* _bitField = nullptr;
    MCTSTree* _mctsTree = nullptr;

    bool _isGameStarted = false;
    short _currentPlayer = -1;
    short _aiPlayerColor = -1;

    short _forcedColor = -1;

    unsigned _aiLevel = 1;
    unsigned _totalAiGames = 0;
    unsigned _currentAiGames = 0;

    qint64 _aiBudget = 0;

    std::vector<std::pair<short, short>> _testMoves;
};
#endif // MAINWINDOW_H

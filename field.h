#pragma once
#include "IField.h"

class Field : public IField
{
public:
    Field();
    virtual ~Field();

    Field(const Field& field);

    virtual const std::unordered_set<short>& getAvailableMoves(short color) const override;
    virtual const std::array<short, BOARD_SIZE * BOARD_SIZE>& getBoardState() const override;
    virtual const std::vector<short> getMoves() const override;
    virtual bool placePiece(short x, short y, short color) override;
    virtual bool placePiece(short position, short color) override;
    virtual bool placePiece(const FieldMove& move) override;
    virtual bool unmakeMove();
    virtual FieldStatus getFieldStatus() const override;
    virtual void printField() const override;
    virtual void clear() override;
protected:
    virtual void updateStatus(const FieldMove& move);
    virtual void updateStatus(short position, short color);
protected:
    FieldStatus _status;
    std::vector<short> _currentMoves;
    std::array<short, BOARD_SIZE * BOARD_SIZE> _board = {0};
    std::unordered_set<short> _availableMoves;

    std::array<std::unordered_set<short>, 5> _winningMoves;
};


#pragma once
#include "IField.h"

class Field : public IField
{
public:
    Field();
    virtual ~Field();

    Field(const Field& field);

    virtual const std::unordered_set<short>& getAvailableMoves() const override;
    virtual const std::array<short, BOARD_SIZE * BOARD_SIZE>& getBoardState() const override;
    virtual const std::vector<short> getMoves() const override;
    virtual bool placePiece(short x, short y, short color) override;
    virtual bool placePiece(short position, short color) override;
    virtual bool placePiece(const FieldMove& move) override;
    virtual FieldStatus getFieldStatus() const override;
    virtual void printField() const override;
    virtual void clear() override;
private:
    void updateStatus(const FieldMove& move);
    void updateStatus(short position, short color);
private:
    FieldStatus _status;
    std::vector<short> _currentMoves;
    std::array<short, BOARD_SIZE * BOARD_SIZE> _board;
    std::unordered_set<short> _availableMoves;
};


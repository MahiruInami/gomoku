#pragma once
#include "IField.h"

class Field : public IField
{
public:
    Field();
    virtual ~Field();

    Field(const Field& field);

    virtual const std::unordered_set<short>& getAvailableMoves() const override;
    virtual const std::unordered_map<short, short>& getCurrentPieces() const override;
    virtual bool placePiece(short x, short y, short color) override;
    virtual bool placePiece(const FieldMove& move) override;
    virtual FieldStatus getFieldStatus() const override;
    virtual void printField() const override;
    virtual void clear() override;
private:
    void updateStatus(const FieldMove& move);
private:
    FieldStatus _status;
    std::unordered_map<short, short> _moves;
    std::unordered_set<short> _availableMoves;
};


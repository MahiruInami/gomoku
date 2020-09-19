#pragma once


class MovePattern
{
public:
    MovePattern();

    unsigned type;
    unsigned start, end;
    unsigned pattern;
    unsigned points;
    unsigned priority;

    MovePattern* next = nullptr;
    MovePattern* prev = nullptr;
};


#ifndef PIECE_H
#define PIECE_H

#include <string>

class Piece
{
private:
    std::string value;

public:
    Piece();
    Piece(const std::string& value);

    std::string toString() const;

    bool isEmpty() const;

    char getColor() const;

    char getType() const;

    bool isWhite() const;

    bool isBlack() const;

    void setValue(const std::string& value);

    bool operator==(const Piece& other) const;

    bool operator!=(const Piece& other) const;
};

#endif
class Position
{
private:
    int row;
    int col;

public:
    Position();
    Position(int row, int col);

    int getRow() const;
    int getCol() const;

    void setRow(int row);
    void setCol(int col);
    void setPosition(int row, int col);
    bool operator==(const Position& other) const;
    bool operator!=(const Position& other) const;
    bool operator<(const Position& other) const;
};
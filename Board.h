class Board
{
private:
    std::vector<std::vector<std::string>> grid;

public:
    int getRows() const;
    int getCols() const;

    bool isValidPosition(int r,int c) const;

    std::string getPiece(int r,int c) const;

    void setPiece(int r,int c,const std::string&);

    void setPiece(int row, int col,const std::string& piece) const;

    void print() const;
};
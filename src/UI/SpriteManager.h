#pragma once
#include <string>
#include <map>
#include "img.h"
#include "../common/DTO/PieceView.h"
#include "../common/enums/PieceState.h"
#include "../common/enums/PieceType.h"
#include "../common/enums/PieceColor.h"

class SpriteManager {
private:
    std::string assetsPath;
    std::string piecesFolder;
    std::map<std::string, std::string> spritePaths;

    // פונקציה עזר פנימית לבניית נתיב
    std::string getPath(PieceType type, PieceColor color, PieceState state, int frame) const;
    std::string typeToString(PieceType type) const;
    std::string colorToString(PieceColor color) const;
    std::string stateToString(PieceState state) const;

public:
    SpriteManager(const std::string& assetsPath, const std::string& piecesFolder = "pieces2");
    Img getPieceSprite(const PieceView& piece, PieceState state, int frame);
};
#include "SpriteManager.h"
#include "../common/enums/PieceStateToString.h"
#include <fstream>

SpriteManager::SpriteManager(const std::string& assetsPath, const std::string& piecesFolder, int spriteSize)
    : assetsPath(assetsPath), piecesFolder(piecesFolder), spriteSize(spriteSize) {}

// פונקציית עזר להמרה לטקסט לצורך בניית נתיב הקובץ
std::string SpriteManager::typeToString(PieceType type) const {
    switch (type) {
        case PieceType::Pawn:   return "P";
        case PieceType::Knight: return "N";
        case PieceType::Bishop: return "B";
        case PieceType::Rook:   return "R";
        case PieceType::Queen:  return "Q";
        case PieceType::King:   return "K";
        default:                return "P";
    }
}

std::string SpriteManager::colorToString(PieceColor color) const {
    return (color == PieceColor::White) ? "W" : "B";
}

std::string SpriteManager::getPath(PieceType type, PieceColor color, PieceState state, int frame) const {
    // מבנה נתיב: assets/pieces1/KW/states/idle/sprites/1.png
    // בהתאם למבנה שהצגת בקוד הקודם שלך
    return assetsPath + "/" + piecesFolder + "/" +
           typeToString(type) + colorToString(color) + "/states/" +
           pieceStateToString(state) + "/sprites/" +
           std::to_string(frame) + ".png";
}

Img SpriteManager::getPieceSprite(const PieceView& piece, PieceState state, int frame) {
    std::string key = typeToString(piece.getType()) + colorToString(piece.getColor()) +
                      pieceStateToString(state) + std::to_string(frame);

    // כל ספרייט נטען ומפוענח מהדיסק פעם אחת בלבד; קריאות חוזרות (למשל כל
    // פריים בלולאת הרינדור) מקבלות עותק זול מהזיכרון (clone) במקום קריאת דיסק.
    auto cached = spriteCache.find(key);
    if (cached != spriteCache.end()) {
        return cached->second;
    }

    Img img;
    img.read(getPath(piece.getType(), piece.getColor(), state, frame), {spriteSize, spriteSize}, true);
    spriteCache[key] = img;
    return img;
}
#include "SpriteManager.h"
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

std::string SpriteManager::stateToString(PieceState state) const {
    switch (state) {
        case PieceState::Idle:     return "idle";
        case PieceState::Moving:   return "moving";
        case PieceState::Captured: return "captured";
        default:                   return "idle";
    }
}

std::string SpriteManager::getPath(PieceType type, PieceColor color, PieceState state, int frame) const {
    // מבנה נתיב: assets/pieces1/KW/states/idle/sprites/1.png
    // בהתאם למבנה שהצגת בקוד הקודם שלך
    return assetsPath + "/" + piecesFolder + "/" + 
           typeToString(type) + colorToString(color) + "/states/" + 
           stateToString(state) + "/sprites/" + 
           std::to_string(frame) + ".png";
}

Img SpriteManager::getPieceSprite(const PieceView& piece, PieceState state, int frame) {
    std::string key = typeToString(piece.getType()) + colorToString(piece.getColor()) + 
                      stateToString(state) + std::to_string(frame);
    // בדיקה אם הספרייט כבר קיים במפה כדי לחסוך טעינה מהדיסק
    if (spritePaths.find(key) == spritePaths.end()) {
        spritePaths[key] = getPath(piece.getType(), piece.getColor(), state, frame);
    }
    Img img;
    // ניסיון קריאה מהנתיב שנבנה, עם שינוי גודל לגודל תא הלוח
    img.read(spritePaths[key], {spriteSize, spriteSize}, true);
    return img;
}
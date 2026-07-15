// #include "UI/img.hpp"
// #include <iostream>

// int main() {
//     try {
//         std::cout << "Testing Img class..." << std::endl;
        
//         Img img;
//         img.read("assets/board_classic.png", {640, 480}, true);
//         //img.put_text("Hello, Img!", 150, 360, 1.0, {0,0,0});
//         img.show();
        
//         std::cout << "Img class test completed successfully!" << std::endl;
//         return 0;
//     } catch (const std::exception& e) {
//         std::cerr << "Error: " << e.what() << std::endl;
//         return 1;
//     }
// } 
#include "UI/BoardCanvas.h"
#include "UI/SpriteManager.h"
#include "common/DTO/PieceView.h"
#include "logic/model/Position.h" // בהתאם ל-include ב-PieceView.h

int main() {
    try {
        BoardCanvas canvas("assets/board_classic.png", 100);
        SpriteManager spriteManager("assets", "pieces1");

        // יצירת מיקום ו-PieceView תקני
        PositionView pos(2, 2); 
        PieceView kingWhite(1, PieceType::King, PieceColor::White, PieceState::Idle, pos);

        Img pieceSprite = spriteManager.getPieceSprite(kingWhite, PieceState::Idle, 1);

        // שימוש במיקום מתוך ה-PieceView
        canvas.drawPiece(pieceSprite, kingWhite.getPosition().getRow(), kingWhite.getPosition().getCol());

        canvas.show();
    } catch (const std::exception& e) {
        return 1;
    }
    return 0;
}
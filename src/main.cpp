
#include "UI/BoardCanvas.h"
#include "UI/SpriteManager.h"
#include "UI/Renderer.h"
#include "common/DTO/BoardView.h"
#include "common/DTO/PieceView.h"
#include "logic/model/Position.h"
#include <vector>
#include <iostream>

int main() {
    try { 
        BoardCanvas canvas("assets/board_classic.png", 100);
        SpriteManager spriteManager("assets", "pieces2");
        Renderer renderer(canvas, spriteManager);

        // 1. יצירת וקטור של 64 משבצות ריקות
        std::vector<PieceView> pieces;
        for (int i = 0; i < 64; ++i) {
            // הנחה: PieceView ריק מאותחל עם Type::Empty וצבע/מצב מתאימים
            pieces.emplace_back(i, PieceType::Empty, PieceColor::White, PieceState::Idle, PositionView(i / 8, i % 8));
        }

        // 2. פונקציית עזר להצבת כלי במיקום הנכון בוקטור
        auto placePiece = [&](PieceType type, PieceColor color, int row, int col) {
            int index = row * 8 + col;
            pieces[index] = PieceView(index, type, color, PieceState::Idle, PositionView(row, col));
        };

        // 3. סידור הכלים הקלאסי
        PieceType rowOrder[] = {PieceType::Rook, PieceType::Knight, PieceType::Bishop, PieceType::Queen, 
                                PieceType::King, PieceType::Bishop, PieceType::Knight, PieceType::Rook};

        for (int i = 0; i < 8; ++i) {
            // שחורים (שורות 0-1)
            placePiece(rowOrder[i], PieceColor::Black, 0, i);
            placePiece(PieceType::Pawn, PieceColor::Black, 1, i);
            
            // לבנים (שורות 6-7)
            placePiece(PieceType::Pawn, PieceColor::White, 6, i);
            placePiece(rowOrder[i], PieceColor::White, 7, i);
        }

        // 4. אתחול ורינדור
        BoardView boardView(8, 8, pieces);
        renderer.render(boardView);

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
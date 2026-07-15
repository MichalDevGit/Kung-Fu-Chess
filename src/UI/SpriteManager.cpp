#include "SpriteManager.h"

#include <filesystem>


SpriteManager::SpriteManager(const std::string& assets_path,
                             const std::string& pieces_folder)
    :
    assets_path(assets_path),
    pieces_folder(pieces_folder)
{
}


std::string SpriteManager::create_key(const std::string& piece,
                                      const std::string& state,
                                      int frame) const
{
    return piece + "_" + state + "_" + std::to_string(frame);
}


std::string SpriteManager::build_sprite_path(const std::string& piece,
                                             const std::string& state,
                                             int frame) const
{
    return assets_path +
           "/" +
           pieces_folder +
           "/" +
           piece +
           "/states/" +
           state +
           "/sprites/" +
           std::to_string(frame) +
           ".png";
}


Img SpriteManager::get_sprite(const std::string& piece,
                              const std::string& state,
                              int frame)
{
    std::string key = create_key(piece, state, frame);


    auto it = sprite_paths.find(key);


    std::string path;


    if (it != sprite_paths.end()) {
        path = it->second;
    }
    else {
        path = build_sprite_path(piece, state, frame);
        sprite_paths[key] = path;
    }


    Img img;
    img.read(path);

    return img;
}


Img SpriteManager::get_piece_sprite(const PieceView& piece,
                                    const std::string& state,
                                    int frame)
{
    return get_sprite(piece.toString(),
                      state,
                      frame);
}
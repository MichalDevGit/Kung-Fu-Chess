#pragma once

#include <string>
#include <map>

#include "img.h"
#include "../common/DTO/PieceView.h"


class SpriteManager {
private:
    std::string assets_path;
    std::string pieces_folder;

    std::map<std::string, std::string> sprite_paths;


    std::string create_key(const std::string& piece,
                           const std::string& state,
                           int frame) const;

    std::string build_sprite_path(const std::string& piece,
                                  const std::string& state,
                                  int frame) const;


public:
    SpriteManager(const std::string& assets_path,
                  const std::string& pieces_folder = "pieces2");


    Img get_sprite(const std::string& piece,
                   const std::string& state,
                   int frame);


    Img get_piece_sprite(const PieceView& piece,
                         const std::string& state,
                         int frame);
};
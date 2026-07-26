#ifndef COMMON_ENUMS_ENUM_JSON_H
#define COMMON_ENUMS_ENUM_JSON_H

#include <nlohmann/json.hpp>

#include "PieceColor.h"
#include "PieceType.h"
#include "PieceState.h"
#include "RestKind.h"

// JSON <-> enum wire mappings for every domain enum that appears in a
// serialized DTO or protocol message. Centralized here (not duplicated per
// call site) so each enum has exactly one JSON string representation,
// wherever it's serialized.
NLOHMANN_JSON_SERIALIZE_ENUM(PieceColor, {
    {PieceColor::White, "white"},
    {PieceColor::Black, "black"},
    {PieceColor::None, "none"},
})

NLOHMANN_JSON_SERIALIZE_ENUM(PieceType, {
    {PieceType::King, "king"},
    {PieceType::Queen, "queen"},
    {PieceType::Rook, "rook"},
    {PieceType::Bishop, "bishop"},
    {PieceType::Knight, "knight"},
    {PieceType::Pawn, "pawn"},
    {PieceType::Empty, "empty"},
})

NLOHMANN_JSON_SERIALIZE_ENUM(PieceState, {
    {PieceState::Idle, "idle"},
    {PieceState::Moving, "moving"},
    {PieceState::Captured, "captured"},
    {PieceState::Jump, "jump"},
    {PieceState::LongRest, "long_rest"},
    {PieceState::ShortRest, "short_rest"},
})

NLOHMANN_JSON_SERIALIZE_ENUM(RestKind, {
    {RestKind::Long, "long"},
    {RestKind::Short, "short"},
})

#endif

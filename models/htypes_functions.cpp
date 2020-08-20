//
// Created by dado on 20/06/2020.
//

#include "htypes.hpp"
#include "htypes_functions.hpp"
#include <core/util.h>

std::string GHTypeToString( uint64_t elem ) {
    std::string ret{};

    if ( checkBitWiseFlag(elem, GHType::None) ) ret+="None";
    if ( checkBitWiseFlag(elem, GHType::Generic) ) ret+="Generic";
    if ( checkBitWiseFlag(elem, GHType::Wall) ) ret+="Wall";
    if ( checkBitWiseFlag(elem, GHType::Floor) ) ret+="Floor";
    if ( checkBitWiseFlag(elem, GHType::Stairs) ) ret+="Stairs";
    if ( checkBitWiseFlag(elem, GHType::Window) ) ret+="Window";
    if ( checkBitWiseFlag(elem, GHType::Door) ) ret+="Door";
    if ( checkBitWiseFlag(elem, GHType::DoorRect) ) ret+="DoorRect";
    if ( checkBitWiseFlag(elem, GHType::DoorFrame) ) ret+="DoorFrame";
    if ( checkBitWiseFlag(elem, GHType::Ceiling) ) ret+="Ceiling";
    if ( checkBitWiseFlag(elem, GHType::Ground) ) ret+="Ground";
    if ( checkBitWiseFlag(elem, GHType::Skirting) ) ret+="Skirting";
    if ( checkBitWiseFlag(elem, GHType::Coving) ) ret+="Coving";
    if ( checkBitWiseFlag(elem, GHType::WallPlaster) ) ret+="WallPlaster";
    if ( checkBitWiseFlag(elem, GHType::WallPlasterUShape) ) ret+="WallPlasterUShape";
    if ( checkBitWiseFlag(elem, GHType::WallPlasterExternal) ) ret+="WallPlasterExternal";
    if ( checkBitWiseFlag(elem, GHType::WallPlasterInternal) ) ret+="WallPlasterInternal";
    if ( checkBitWiseFlag(elem, GHType::WallTilesInternal) ) ret+="WallTilesInternal";
    if ( checkBitWiseFlag(elem, GHType::KitchenWorktop) ) ret+="Worktop";
    if ( checkBitWiseFlag(elem, GHType::KitchenCabinet) ) ret+="Cabinet";
    if ( checkBitWiseFlag(elem, GHType::KitchenBackSplash) ) ret+="BackSplash";
    if ( checkBitWiseFlag(elem, GHType::KitchenSink) ) ret+="KitchenSink";
    if ( checkBitWiseFlag(elem, GHType::KitchenOven) ) ret+="KitchenOven";
    if ( checkBitWiseFlag(elem, GHType::KitchenHob) ) ret+="KitchenHob";
    if ( checkBitWiseFlag(elem, GHType::LightFitting) ) ret+="LightFitting";
    if ( checkBitWiseFlag(elem, GHType::Locator) ) ret+="Locator";
    if ( checkBitWiseFlag(elem, GHType::PowerSocket) ) ret+="PowerSocket";
    if ( checkBitWiseFlag(elem, GHType::LightSwitch) ) ret+="LightSwitch";
    if ( checkBitWiseFlag(elem, GHType::Room) ) ret+="Room";
    if ( checkBitWiseFlag(elem, GHType::KitchenBackSplash) ) ret+="KitchenBackSplash";
    if ( checkBitWiseFlag(elem, GHType::Furniture) ) ret+="Furniture";
    if ( checkBitWiseFlag(elem, GHType::Fitting) ) ret+="Fitting";
    if ( checkBitWiseFlag(elem, GHType::Balcony) ) ret+="Balcony";

    return ret;
}

std::string defaultMaterialAndColorPropertyPresetsForGHType( GHTypeT key ) {
    std::string ret{};

    auto append = [](std::string& source, const std::string& input) {
        if ( source.empty() ) {
            source = input;
        } else {
            source += "+" + input;
        }
    };

    if ( checkBitWiseFlag( key, GHType::KitchenWorktop) )    append( ret, "granite+marble");
    if ( checkBitWiseFlag( key, GHType::KitchenCabinet) )    append( ret, "wood+metal");
    if ( checkBitWiseFlag( key, GHType::KitchenBackSplash) ) append( ret, "wood+metal");
    if ( checkBitWiseFlag( key, GHType::Wall) )              append( ret, "plaster+wood+tiles");
    if ( checkBitWiseFlag( key, GHType::Floor) )             append( ret, "wood+tiles+carpet");
    if ( checkBitWiseFlag( key, GHType::Skirting) )          append( ret, "wood+tiles");
    if ( checkBitWiseFlag( key, GHType::Coving) )            append( ret, "wood+tiles");
    if ( checkBitWiseFlag( key, GHType::Ceiling) )           append( ret, "plaster+wood");

    return ret;
}

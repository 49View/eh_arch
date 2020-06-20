//
// Created by dado on 20/06/2020.
//

#include "htypes.hpp"
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
    if ( checkBitWiseFlag(elem, GHType::KitchenWorktop) ) ret+="KitchenWorktop";
    if ( checkBitWiseFlag(elem, GHType::KitchenCabinet) ) ret+="KitchenCabinet";
    if ( checkBitWiseFlag(elem, GHType::KitchenSink) ) ret+="KitchenSink";
    if ( checkBitWiseFlag(elem, GHType::KitchenOven) ) ret+="KitchenOven";
    if ( checkBitWiseFlag(elem, GHType::KitchenHob) ) ret+="KitchenHob";
    if ( checkBitWiseFlag(elem, GHType::LightFitting) ) ret+="LightFitting";
    if ( checkBitWiseFlag(elem, GHType::Locator) ) ret+="Locator";
    if ( checkBitWiseFlag(elem, GHType::PowerSocket) ) ret+="PowerSocket";
    if ( checkBitWiseFlag(elem, GHType::LightSwitch) ) ret+="LightSwitch";
    if ( checkBitWiseFlag(elem, GHType::Room) ) ret+="Room";

    return ret;
}
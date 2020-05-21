//
// Created by Dado on 22/10/2019.
//

#include "arch_service.hpp"

#include <poly/scene_graph.h>

#include "arch_scene_graph.hpp"
#include "../render/house_render.hpp"
#include "../models/room_service.hpp"

//ArchService::ArchService( ArchSceneGraph& asg, SceneGraph& sg ) : asg( asg ), sg( sg ) {
//    Socket::on( socketMessageHouseElaborate(),
//                std::bind(&ArchService::loadHouseFromSocketJson, this, std::placeholders::_1, std::placeholders::_2 ) );
//}
//
//void ArchService::loadHouseInternal( const HouseBSData& _house ) {
//    const auto key = _house.name;
//    HOD::resolver<HouseBSData>( sg, &_house, [&,key]() {
//        HouseRender::make3dGeometry( sg, asg.get<HouseBSData>(key).get() );
//    } );
//}
//
//void ArchService::loadHouseFromSocketJson( const std::string& _msg, SocketCallbackDataType&& _data ) {
//    loadHouse( HouseBSData{std::move(_data["data"])} );
//}
//
//void ArchService::loadHouse( const HouseBSData& _house ) {
//    asg.loadHouse( _house );
//    loadHouseInternal( _house );
//}
//
//void ArchService::loadHouse( ResourceRef name ) {
//    asg.load<HouseBSData>(name, [&](HttpResouceCBSign key) {
//        auto house = asg.get<HouseBSData>(key).get();
//        loadHouseInternal( *house );
//    });
//
//}

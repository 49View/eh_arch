//
// Created by Dado on 22/10/2019.
//

#include "arch_service.hpp"

#include <poly/scene_graph.h>

#include "arch_scene_graph.hpp"
#include "../render/house_render.hpp"
#include "../models/room_service.hpp"

ArchService::ArchService( ArchSceneGraph& asg, SceneGraph& sg ) : asg( asg ), sg( sg ) {
    Socket::on( socketMessageHouseElaborate(),
                std::bind(&ArchService::loadHouseFromSocketJson, this, std::placeholders::_1, std::placeholders::_2 ) );
}

void ArchService::loadHouseInternal( const HouseBSData& _house ) {
    const auto key = _house.name;
    HOD::resolver<HouseBSData>( sg, &_house, [&,key]() {
        HouseRender::make3dGeometry( sg, asg.get<HouseBSData>(key).get() );
    } );
}

void ArchService::loadHouseFromSocketJson( const std::string& _msg, SocketCallbackDataType&& _data ) {
    loadHouse( HouseBSData{std::move(_data["data"])} );
}

void ArchService::loadHouse( const HouseBSData& _house ) {
    asg.loadHouse( _house );
    loadHouseInternal( _house );
}

void ArchService::loadHouse( ResourceRef name ) {

    asg.load<HouseBSData>(name, [&](HttpResouceCBSign key) {

        auto house = asg.get<HouseBSData>(key).get();

//        FM::writeLocalFile("urca.json", house->serialize() );

//        FurnitureRuleScript ruleScript;
//        V3f cornerSlack{0.2f, 0.0f, 0.0f};
//        ruleScript.addRule( FurniturePlacementRule{ FurnitureRuleIndex(RS::MainWith2Sides),     FurnitureRefs{{ FTH::Bed(), FTH::Bedside(), FTH::Bedside()}, { FTH::Picture()}}, WSLOH::SecondLongest()} );
//        ruleScript.addRule( FurniturePlacementRule{ FurnitureRuleIndex(RS::CornerWithDec),      FurnitureRefs{{ FTH::Bedside()}, { FTH::Picture()}}, WSLOH::Longest(), cornerSlack } );
//        ruleScript.addRule( FurniturePlacementRule{ FurnitureRuleIndex(RS::SetAlignedAtCorner), FurnitureRefs{{ FTH::Shelf(), FTH::Shelf(), FTH::Sofa()}}, WSLOH::Longest(), FurnitureSlacks{ cornerSlack, V3f::ZERO, cornerSlack } });
//        ruleScript.addRule( FurniturePlacementRule{ FurnitureRuleIndex(RS::SetAlignedAtCorner), FurnitureRefs{{ FTH::Wardrobe(), FTH::Wardrobe()}}, WSLOH::LongestOpposite(), WSC_P2 } );
//        ruleScript.addRule( FurniturePlacementRule{ FurnitureRuleIndex(RS::SetAlignedAtCorner),
//                                                    FurnitureRefs{{ FTH::Shelf(), FTH::Wardrobe(), FTH::Armchair()}},
//                                                    WallSegmentIdentifier{ WSLOH::ExactIndex(), 3},
//                                                    FurnitureSlacks{ V3f::ZERO, V3f::ZERO, cornerSlack*0.5f }, WSC_P2 } );
//        ruleScript.addRule( FurniturePlacementRule{ FurnitureRuleIndex(RS::SetAlignedAtCorner),
//                                                    FurnitureRefs{{ FTH::Armchair()}},
//                                                    WallSegmentIdentifier{ WSLOH::ExactIndex(), 4},
//                                                    WSC_P2 } );
//        ruleScript.addRule( FurniturePlacementRule{ FurnitureRuleIndex(RS::SetAlignedAtCorner),
//                                                    FurnitureRefs{{ FTH::Armchair()}},
//                                                    WallSegmentIdentifier{ WSLOH::ExactIndex(), 4},
//                                                    WSC_P1 } );
//        ruleScript.addRule( FurniturePlacementRule{ FurnitureRuleIndex(RS::MiddleOfRoom), FurnitureRefs{{ FTH::Carpet()} }} );
//        bool completed = true;
//        for ( const auto& f : house->mFloors ) {
//            for ( const auto& w : f->rooms ) {
//                completed &= RS::runRuleScript( w.get(), asg.Furns(), ruleScript );
//            }
//        }

        loadHouseInternal( *house );
    });

}

void ArchService::publishHouse( std::shared_ptr<HouseBSData> _house, const std::string& _name ) {
    asg.HM().publish( _house, _name );
}

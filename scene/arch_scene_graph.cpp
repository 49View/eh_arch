//
// Created by Dado on 20/10/2019.
//

#include "arch_scene_graph.hpp"
#include <core/resources/resource_builder.hpp>
#include <poly/scene_graph.h>
#include <utility>

LoadedResouceCallbackContainer ArchSceneGraph::resourceCallbackHouse;

ArchSceneGraph::ArchSceneGraph( HouseManager& _hm, FurnitureMapStorage& _furns ) :  hm(_hm), furns(_furns) {
    hm.connect( [](const ResourceTransfer<HouseBSData>& _val ) {
        LOGRS( "[ASG-Resource] Add " << ArchResourceVersioning<HouseBSData>::Prefix() << ": "  << *_val.names.begin() );
        if ( _val.ccf ) _val.ccf(_val.hash);
    });
}

namespace HOD { // HighOrderDependency

    template <>
    DepRemapsManager resolveDependencies<HouseBSData>( const HouseBSData* data ) {
        DepRemapsManager ret{};

        ret.addDep( ResourceGroup::Image, data->defaultSkybox );
        for ( const auto& floor : data->mFloors ) {
            for ( const auto& wall : floor->walls ) {
                ret.addDep( ResourceGroup::Material, wall->material );
            }
            for ( const auto& room : floor->rooms ) {
                ret.addDep( ResourceGroup::Material, room->ceilingMaterial );
                ret.addDep( ResourceGroup::Material, room->floorMaterial );
                ret.addDep( ResourceGroup::Profile, room->covingProfile );
                ret.addDep( ResourceGroup::Profile, room->skirtingProfile );
                ret.addDep( ResourceGroup::Geom, room->spotlightGeom );

                for ( const auto& furn : room->mFittedFurniture ) {
                    ret.addDep( ResourceGroup::Geom, furn.name );
                    ret.addDep( ResourceGroup::Profile, furn.symbolRef );
                }
            }
            for ( const auto& door : floor->doors ) {
                ret.addDep( ResourceGroup::Profile, door->architraveProfile );
                ret.addDep( ResourceGroup::Geom, "doorhandle,sx" );
                ret.addDep( ResourceGroup::Geom, "doorhandle,dx" );
            }
            for ( const auto& window : floor->windows ) {
                ret.addDep( ResourceGroup::Geom, window->curtainGeom );
                ret.addDep( ResourceGroup::Material, window->curtainMaterial );
            }
        }

        return ret;
    }
}

void ArchSceneGraph::loadHouse( std::string _names, HttpResouceCB _ccf ) {
    B<HB>( _names ).load( std::move(_ccf) );
}

void ArchSceneGraph::loadHouse( const HouseBSData& _res ) {
    B<HB>( _res.name ).addIM( _res );
}

void ArchSceneGraph::loadCallbacks() {
    loadResourceCallback<HouseBSData, HB>(resourceCallbackHouse);
}

void ArchSceneGraph::update() {

    static bool firstFrameEver = true;

    if ( firstFrameEver ) {
        firstFrameEver = false;
        return;
    }

    loadCallbacks();

    HM().update();
}

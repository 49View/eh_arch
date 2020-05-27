//
// Created by Dado on 20/10/2019.
//

#include "arch_scene_graph.hpp"
#include <core/resources/resource_builder.hpp>
#include <core/math/plane3f.h>
#include <core/raw_image.h>
#include <core/TTF.h>
#include <core/camera.h>
#include <core/resources/profile.hpp>
#include <core/resources/material.h>
#include <render_scene_graph/render_orchestrator.h>
#include <core/resources/resource_builder.hpp>
#include <core/math/vector_util.hpp>
#include <core/lightmap_exchange_format.h>
#include <graphics/renderer.h>
#include <graphics/lightmap_manager.hpp>
#include <graphics/render_light_manager.h>
#include <graphics/shader_manager.h>
#include <poly/collision_mesh.hpp>
#include <poly/scene_graph.h>
#include <eh_arch/models/house_bsdata.hpp>
#include <eh_arch/render/house_render.hpp>
#include <eh_arch/models/house_service.hpp>

ArchSceneGraph::ArchSceneGraph( SceneGraph& _sg, RenderOrchestrator& _rsg, FurnitureMapStorage& _furns ) : sg(_sg), rsg(_rsg), furns(_furns) {
}

namespace HOD { // HighOrderDependency

    template <>
    DepRemapsManager resolveDependencies<HouseBSData>( const HouseBSData* data ) {
        DepRemapsManager ret{};

        ret.addDep( ResourceGroup::Image, data->defaultSkybox );
        for ( const auto& floor : data->mFloors ) {
            ret.addDep( ResourceGroup::Material, floor->externalWallsMaterial );
            for ( const auto& room : floor->rooms ) {
                ret.addDep( ResourceGroup::Material, room->wallMaterial );
                ret.addDep( ResourceGroup::Material, room->ceilingMaterial );
                ret.addDep( ResourceGroup::Material, room->floorMaterial );
                ret.addDep( ResourceGroup::Material, room->covingMaterial );
                ret.addDep( ResourceGroup::Material, room->skirtingMaterial );
                ret.addDep( ResourceGroup::Profile, room->covingProfile );
                ret.addDep( ResourceGroup::Profile, room->skirtingProfile );
                ret.addDep( ResourceGroup::Geom, room->spotlightGeom );
                ret.addDep( ResourceGroup::Geom, "powersocket" );
                ret.addDep( ResourceGroup::Geom, "lightswitch" );

                for ( const auto& furn : room->mFittedFurniture ) {
                    ret.addDep( ResourceGroup::Geom, furn.name );
                    ret.addDep( ResourceGroup::Profile, furn.symbolRef );
                }

                // Will load Kitchen and bathroom sets here
                ret.addDep( ResourceGroup::Material, room->kitchenData.worktopMaterial );
                ret.addDep( ResourceGroup::Material, room->kitchenData.unitsMaterial );
                ret.addDep( ResourceGroup::Geom, room->kitchenData.sinkModel );
                ret.addDep( ResourceGroup::Geom, room->kitchenData.ovenPanelModel );
                ret.addDep( ResourceGroup::Geom, room->kitchenData.microwaveModel );
                ret.addDep( ResourceGroup::Geom, room->kitchenData.cooktopModel );
                ret.addDep( ResourceGroup::Geom, room->kitchenData.fridgeModel );
                ret.addDep( ResourceGroup::Geom, room->kitchenData.extractorHoodModel );
                ret.addDep( ResourceGroup::Geom, room->kitchenData.drawersHandleModel );
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

void ArchSceneGraph::calcFloorplanNavigationTransform( std::shared_ptr<HouseBSData> houseJson ) {
    auto m = std::make_shared<Matrix4f>(Matrix4f::IDENTITY);
    float vmax = max(houseJson->bbox.bottomRight().x(), houseJson->bbox.bottomRight().y());
//    float padding = vmax*0.03f;
    float screenFloorplanRatio = (1.0f/1.0f);
    float screenPadding = 0.03f;
    float vmaxScale = vmax / screenFloorplanRatio;
    auto vr = 1.0f/ vmaxScale;
    m->scale( V3f{vr, -vr, -vr});
    m->translate( V3f{getScreenAspectRatio-screenFloorplanRatio-screenPadding, screenFloorplanRatio, screenFloorplanRatio});
    floorplanNavigationMatrix = m;
}

void ArchSceneGraph::showHouse(std::shared_ptr<HouseBSData> _houseJson) {

    houseJson = _houseJson;
    HOD::resolver<HouseBSData>( sg, houseJson.get(), [&]() {
//        calcFloorplanNavigationTransform(houseJson);
//        HouseRender::make2dGeometry(rsg.RR(), sg, houseJson.get(), RDSPreMult(*floorplanNavigationMatrix.get()), FloorPlanRenderMode::Debug2d );
        HouseRender::make2dGeometry(rsg.RR(), sg, houseJson.get(), RDSPreMult(Matrix4f::IDENTITY), FloorPlanRenderMode::Debug3d );

//        sg.loadCollisionMesh( HouseService::createCollisionMesh( houseJson.get() ) );
//        HouseRender::make3dGeometry( sg, houseJson.get() );
//
//        V2f cobr = HouseService::centerOfBiggestRoom( houseJson.get() );
//        V3f lngp = V3f{ cobr.x(), 1.6f, cobr.y() };
//        sg.setLastKnownGoodPosition(lngp);
//        rsg.setRigCameraController(CameraControlType::Walk);
//        rsg.DC()->setQuatAngles(V3f{ 0.08f, -0.70f, 0.0f });
//        rsg.DC()->setPosition(lngp);
    } );

}

void ArchSceneGraph::loadHouse( const std::string& _pid ) {
    Http::get( Url{"/propertybim/" + _pid}, [this]( HttpResponeParams params ) {
        houseJson = std::make_shared<HouseBSData>( params.bufferString );
        callbackStream = std::make_pair(houseJson, true);
    } );
}

void ArchSceneGraph::consumeCallbacks() {
    if ( callbackStream.second ) {
        showHouse(callbackStream.first);
        callbackStream.second = false;
    }
}
void ArchSceneGraph::update() {

    consumeCallbacks();

    if ( floorplanNavigationMatrix && rsg.getRigCameraController() == CameraControlType::Walk ) {
        rsg.drawCameraLocator( *floorplanNavigationMatrix.get() );
    }
}

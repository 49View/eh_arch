//
// Created by Dado on 20/10/2019.
//

#include "arch_orchestrator.hpp"
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

ArchOrchestrator::ArchOrchestrator( SceneGraph& _sg, RenderOrchestrator& _rsg ) : sg(_sg), rsg(_rsg) {
}

namespace HOD { // HighOrderDependency

    template<>
    DepRemapsManager resolveDependencies<HouseBSData>( const HouseBSData *data, SceneGraph& sg ) {
        DepRemapsManager ret{};

        ret.addDep(sg, ResourceGroup::Image, data->defaultSkybox);
        for ( const auto& floor : data->mFloors ) {
            ret.addDep(sg, ResourceGroup::Material, floor->externalWallsMaterial);
            for ( const auto& room : floor->rooms ) {
                ret.addDep(sg, ResourceGroup::Material, room->wallMaterial);
                ret.addDep(sg, ResourceGroup::Material, room->ceilingMaterial);
                ret.addDep(sg, ResourceGroup::Material, room->floorMaterial);
                ret.addDep(sg, ResourceGroup::Material, room->covingMaterial);
                ret.addDep(sg, ResourceGroup::Material, room->skirtingMaterial);
                ret.addDep(sg, ResourceGroup::Profile, room->covingProfile);
                ret.addDep(sg, ResourceGroup::Profile, room->skirtingProfile);
                ret.addDep(sg, ResourceGroup::Geom, room->spotlightGeom);
                ret.addDep(sg, ResourceGroup::Geom, "powersocket");
                ret.addDep(sg, ResourceGroup::Geom, "lightswitch");

                for ( const auto& furn : room->mFittedFurniture ) {
                    ret.addDep(sg, ResourceGroup::Geom, furn.name);
                    ret.addDep(sg, ResourceGroup::Profile, furn.symbolRef);
                }

                // Will load Kitchen and bathroom sets here
                ret.addDep(sg, ResourceGroup::Material, room->kitchenData.worktopMaterial);
                ret.addDep(sg, ResourceGroup::Material, room->kitchenData.unitsMaterial);
                ret.addDep(sg, ResourceGroup::Geom, room->kitchenData.sinkModel);
                ret.addDep(sg, ResourceGroup::Geom, room->kitchenData.ovenPanelModel);
                ret.addDep(sg, ResourceGroup::Geom, room->kitchenData.microwaveModel);
                ret.addDep(sg, ResourceGroup::Geom, room->kitchenData.cooktopModel);
                ret.addDep(sg, ResourceGroup::Geom, room->kitchenData.fridgeModel);
                ret.addDep(sg, ResourceGroup::Geom, room->kitchenData.extractorHoodModel);
                ret.addDep(sg, ResourceGroup::Geom, room->kitchenData.drawersHandleModel);
            }
            for ( const auto& door : floor->doors ) {
                ret.addDep(sg, ResourceGroup::Profile, door->architraveProfile);
                ret.addDep(sg, ResourceGroup::Geom, "doorhandle,sx");
                ret.addDep(sg, ResourceGroup::Geom, "doorhandle,dx");
            }
            for ( const auto& window : floor->windows ) {
                ret.addDep(sg, ResourceGroup::Geom, window->curtainGeom);
                ret.addDep(sg, ResourceGroup::Material, window->curtainMaterial);
            }
        }

        return ret;
    }
}

Matrix4f ArchOrchestrator::calcFloorplanNavigationTransform( std::shared_ptr<HouseBSData> _houseJson, float screenRatio, float screenPadding ) {
    auto m = Matrix4f{Matrix4f::IDENTITY};
    float vmax = max(_houseJson->bbox.bottomRight().x(), _houseJson->bbox.bottomRight().y());
    float screenFloorplanRatio = ( 1.0f / screenRatio );
    float vmaxScale = vmax / screenFloorplanRatio;
    auto vr = 1.0f / vmaxScale;
    m.scale(V3f{ vr, -vr, -vr });
    m.translate(V3f{ getScreenAspectRatio - screenFloorplanRatio - screenPadding, screenFloorplanRatio,
                      screenFloorplanRatio });
    return m;
}

void ArchOrchestrator::centerCameraMiddleOfHouse( HouseBSData* houseJson ) {
    if ( houseJson->bbox.isValid() ) {
        rsg.DC()->setPosition(rsg.DC()->center(houseJson->bbox, 0.0f));
    }
}

void ArchOrchestrator::showIMHouse( HouseBSData* _houseJson, const ArchRenderController& arc  ) {
    HouseRender::IMHouseRender(rsg.RR(), sg, _houseJson, arc);
}

void ArchOrchestrator::show3dHouse( HouseBSData* _houseJson, const PostHouse3dResolvedCallback& ccf ) {
    sg.clearNodes();
    rsg.RR().setLoadingFlag( true );
    HOD::resolver<HouseBSData>(sg, _houseJson, [&, ccf, _houseJson]() {
//        sg.loadCollisionMesh(HouseService::createCollisionMesh(_houseJson));
        HouseRenderContainer hrc = HouseRender::make3dGeometry(sg, _houseJson);
        // Infinite plane
        sg.GB<GT::Shape>(ShapeType::Cube, GT::Tag(SHADOW_MAGIC_TAG), V3f::UP_AXIS_NEG * 0.15f,
                         GT::Scale(500.0f, 0.1f, 500.0f));
//        if ( hrc.ceiling ) {
//            hrc.ceiling->move(V3f{0.0f, -(_houseJson->depth+0.3f), 0.0f} );
//        }
        if ( ccf ) ccf(_houseJson);
        rsg.RR().setLoadingFlag( false );
        rsg.useSkybox(true);
    });
}

void ArchOrchestrator::loadHouse( const std::string& _pid, const PostHouseLoadCallback& ccf ) {
    Http::get(Url{ "/propertybim/" + _pid }, [ccf]( HttpResponeParams params ) {
        ccf( std::make_shared<HouseBSData>(params.bufferString) );
    });
}

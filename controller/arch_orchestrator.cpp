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
    DepRemapsManager resolveDependencies<HouseBSData>( const HouseBSData *data ) {
        DepRemapsManager ret{};

        ret.addDep(ResourceGroup::Image, data->defaultSkybox);
        for ( const auto& floor : data->mFloors ) {
            ret.addDep(ResourceGroup::Material, floor->externalWallsMaterial);
            for ( const auto& room : floor->rooms ) {
                ret.addDep(ResourceGroup::Material, room->wallMaterial);
                ret.addDep(ResourceGroup::Material, room->ceilingMaterial);
                ret.addDep(ResourceGroup::Material, room->floorMaterial);
                ret.addDep(ResourceGroup::Material, room->covingMaterial);
                ret.addDep(ResourceGroup::Material, room->skirtingMaterial);
                ret.addDep(ResourceGroup::Profile, room->covingProfile);
                ret.addDep(ResourceGroup::Profile, room->skirtingProfile);
                ret.addDep(ResourceGroup::Geom, room->spotlightGeom);
                ret.addDep(ResourceGroup::Geom, "powersocket");
                ret.addDep(ResourceGroup::Geom, "lightswitch");

                for ( const auto& furn : room->mFittedFurniture ) {
                    ret.addDep(ResourceGroup::Geom, furn.name);
                    ret.addDep(ResourceGroup::Profile, furn.symbolRef);
                }

                // Will load Kitchen and bathroom sets here
                ret.addDep(ResourceGroup::Material, room->kitchenData.worktopMaterial);
                ret.addDep(ResourceGroup::Material, room->kitchenData.unitsMaterial);
                ret.addDep(ResourceGroup::Geom, room->kitchenData.sinkModel);
                ret.addDep(ResourceGroup::Geom, room->kitchenData.ovenPanelModel);
                ret.addDep(ResourceGroup::Geom, room->kitchenData.microwaveModel);
                ret.addDep(ResourceGroup::Geom, room->kitchenData.cooktopModel);
                ret.addDep(ResourceGroup::Geom, room->kitchenData.fridgeModel);
                ret.addDep(ResourceGroup::Geom, room->kitchenData.extractorHoodModel);
                ret.addDep(ResourceGroup::Geom, room->kitchenData.drawersHandleModel);
            }
            for ( const auto& door : floor->doors ) {
                ret.addDep(ResourceGroup::Profile, door->architraveProfile);
                ret.addDep(ResourceGroup::Geom, "doorhandle,sx");
                ret.addDep(ResourceGroup::Geom, "doorhandle,dx");
            }
            for ( const auto& window : floor->windows ) {
                ret.addDep(ResourceGroup::Geom, window->curtainGeom);
                ret.addDep(ResourceGroup::Material, window->curtainMaterial);
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

void ArchOrchestrator::showIMHouse( std::shared_ptr<HouseBSData> _houseJson, const ArchRenderController& ims  ) {
    HouseRender::IMHouseRender(rsg.RR(), sg, _houseJson.get(), ims);
}

void ArchOrchestrator::show3dHouse( std::shared_ptr<HouseBSData> _houseJson, const PostHouseLoadCallback& ccf ) {
    HOD::resolver<HouseBSData>(sg, _houseJson.get(), [&, ccf, _houseJson]() {
        sg.loadCollisionMesh(HouseService::createCollisionMesh(_houseJson.get()));
        HouseRender::make3dGeometry(sg, _houseJson.get());
        if ( ccf ) ccf(_houseJson);
    });
}

void ArchOrchestrator::loadHouse( const std::string& _pid, const PostHouseLoadCallback& ccf ) {
    Http::get(Url{ "/propertybim/" + _pid }, [this, ccf]( HttpResponeParams params ) {
        auto houseJson = std::make_shared<HouseBSData>(params.bufferString);
        sg.addGenericCallback([&, ccf, houseJson]() {
            show3dHouse(houseJson, ccf);
        } );
    });
}

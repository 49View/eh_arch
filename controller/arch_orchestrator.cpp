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

ArchOrchestrator::ArchOrchestrator( SceneGraph& _sg, RenderOrchestrator& _rsg, ArchRenderController& _arc ) : sg(_sg), rsg(_rsg), arc(_arc) {
}

namespace HOD { // HighOrderDependency

    template<>
    DepRemapsManager resolveDependencies<HouseBSData>( const HouseBSData *data, SceneGraph& sg ) {
        DepRemapsManager ret{};

        sg.clearNodes();
        ret.addDep(sg, ResourceGroup::Image, data->defaultSkybox);
        for ( const auto& floor : data->mFloors ) {
            ret.addDep(sg, ResourceGroup::Material, floor->externalWallsMaterial.materialHash);
            for ( const auto& room : floor->rooms ) {
                ret.addDep(sg, ResourceGroup::Material, room->wallsMaterial.materialHash);
                ret.addDep(sg, ResourceGroup::Material, room->ceilingMaterial.materialHash);
                ret.addDep(sg, ResourceGroup::Material, room->floorMaterial.materialHash);
                ret.addDep(sg, ResourceGroup::Material, room->covingMaterial.materialHash);
                ret.addDep(sg, ResourceGroup::Material, room->skirtingMaterial.materialHash);
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
                ret.addDep(sg, ResourceGroup::Material, room->kitchenData.worktopMaterial.materialHash);
                ret.addDep(sg, ResourceGroup::Material, room->kitchenData.unitsMaterial.materialHash);
                ret.addDep(sg, ResourceGroup::Material, room->kitchenData.backSplashMaterial.materialHash);
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

HouseBSData* ArchOrchestrator::H() {
    return houseJson.get();
}

Matrix4f ArchOrchestrator::calcFloorplanNavigationTransform( float screenRatio, float screenPadding ) {
    auto m = Matrix4f{Matrix4f::IDENTITY};
    float vmax = max(houseJson->bbox.bottomRight().x(), houseJson->bbox.bottomRight().y());
    float screenFloorplanRatio = ( 1.0f / screenRatio );
    float vmaxScale = vmax / screenFloorplanRatio;
    auto vr = 1.0f / vmaxScale;
    m.scale(V3f{ vr, -vr, -vr });
    m.translate(V3f{ getScreenAspectRatio - screenFloorplanRatio - screenPadding, screenFloorplanRatio,
                      screenFloorplanRatio });
    return m;
}

void ArchOrchestrator::centerCameraMiddleOfHouse( float slack ) {
    if ( houseJson->bbox.isValid() ) {
        Timeline::play(rsg.DC()->PosAnim(), 0,
                       KeyFramePair{ 0.9f, rsg.DC()->center(houseJson->bbox, slack) });
    }
}

void ArchOrchestrator::showIMHouse() {
    if ( houseJson ) {
        HouseRender::IMHouseRender(rsg.RR(), sg, houseJson.get(), arc);
    }
}

void ArchOrchestrator::make3dHouse( const PostHouse3dResolvedCallback& ccf ) {
    rsg.RR().setLoadingFlag( true );
    HOD::resolver<HouseBSData>(sg, houseJson.get(), [&, ccf]() {
        sg.loadCollisionMesh(HouseService::createCollisionMesh(houseJson.get()));
        hrc = HouseRender::make3dGeometry(rsg.RR(), sg, houseJson.get());
        if ( ccf ) ccf();
        rsg.RR().setLoadingFlag( false );
        rsg.setProbePosition( HouseService::centerOfBiggestRoom( houseJson.get() ));
        rsg.useSkybox(true);
    });
}

/// This loads a house with a http get
/// \param _pid
/// \param ccf
void ArchOrchestrator::loadHouse( const std::string& _pid, const PostHouseLoadCallback& ccf, const PostHouseLoadCallback& ccfailure ) {
    Http::get(Url{ "/propertybim/" + _pid }, [&,ccf]( HttpResponeParams params ) {
        if ( !params.bufferString.empty() ) {
            houseJson = std::make_shared<HouseBSData>(params.bufferString);
            if ( ccf ) ccf();
        } else {
            if ( ccfailure ) ccfailure();
        }
    });
}

///
void ArchOrchestrator::saveHouse() {
    Http::post(Url{ "/propertybim/" }, H()->serialize(),
               []( HttpResponeParams params ) {
                   LOGRS("Bim updated")
               });
}

/// This loads a house with an already created HouseBSData so basically sets the smart pointer
/// \param _houseJson
void ArchOrchestrator::setHouse( const std::shared_ptr<HouseBSData>& _houseJson ) {
    houseJson = _houseJson;
}

HouseRenderContainer& ArchOrchestrator::HRC() {
    return hrc;
}

void ArchOrchestrator::loadFurnitureMapStorage( const std::string& _name ) {
    Http::get(Url{ "/furnitureset/" + _name }, [&, this]( HttpResponeParams& res ) {
        FurnitureSetContainer fset{ res.bufferString };
        for ( const auto& f : fset.set ) {
            sg.loadProfile(f.symbol);
            furnitureMap.addIndex(f);
        }
    });
}
FurnitureMapStorage& ArchOrchestrator::FurnitureMap() {
    return furnitureMap;
}

void ArchOrchestrator::onEvent(ArchIOEvents event) {
    currIOEvent = event;
}

bool ArchOrchestrator::hasEvent(ArchIOEvents event) const {
    return currIOEvent == event;
}


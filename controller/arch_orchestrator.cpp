//
// Created by Dado on 20/10/2019.
//

#include "arch_orchestrator.hpp"

#include <core/resources/resource_builder.hpp>
#include <core/TTF.h>
#include <core/camera.h>
#include <core/resources/profile.hpp>
#include <core/math/vector_util.hpp>
#include <core/lightmap_exchange_format.h>

#include <graphics/renderer.h>
#include <graphics/mouse_input.hpp>
#include <graphics/render_light_manager.h>
#include <graphics/shader_manager.h>
#include <graphics/vertex_processing_anim.h>

#include <poly/collision_mesh.hpp>
#include <poly/scene_graph.h>
#include <poly/converters/gltf2/gltf2.h>

#include <render_scene_graph/render_orchestrator.h>
#include <render_scene_graph/lightmap_manager.hpp>

#include <eh_arch/models/house_bsdata.hpp>
#include <eh_arch/models/house_service.hpp>
#include <eh_arch/controller/arch_render_controller.hpp>
#include <eh_arch/controller/arch_explorer.hpp>
#include <eh_arch/render/house_render.hpp>
#include <poly/osm/osm_names.hpp>

ArchOrchestrator::ArchOrchestrator( SceneGraph& _sg, RenderOrchestrator& _rsg, ArchRenderController& _arc, ArchExplorer& _ae ) :
    sg(_sg), rsg(_rsg), arc(_arc), archExplorer(_ae) {
}

namespace HOD { // HighOrderDependency

    template<>
    DepRemapsManager resolveDependencies<HouseBSData>( const HouseBSData *data, SceneGraph& sg ) {
        DepRemapsManager ret{};

        sg.clearNodes();
        ret.addDep(sg, ResourceGroup::Image, data->defaultSkybox);
        ret.addDep(sg, ResourceGroup::Image, data->getLightmapID());
//        for ( const auto& elem : OSMGeomEntityList() ) {
//            ret.addDep(sg, ResourceGroup::Geom, elem);
//        }
//        ret.addDep(sg, ResourceGroup::Material, data->defaultPanoramaOSMMaterial.materialHash);
        for ( const auto& floor : data->mFloors ) {
            ret.addDep(sg, ResourceGroup::Material, floor->externalWallsMaterial.materialHash);
            for ( const auto& room : floor->rooms ) {
                ret.addDep(sg, ResourceGroup::Material, room->wallsMaterial.materialHash);
                for ( const auto& as : room->mWallSegmentsSorted ) {
                    ret.addDep(sg, ResourceGroup::Material, as.wallMaterial.materialHash);
                }
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
                    ret.addDep(sg, ResourceGroup::Geom, furn->name);
                    ret.addDep(sg, ResourceGroup::Profile, furn->symbolRef);
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
            for ( const auto& outdoorArea : floor->outdoorAreas ) {
                for ( const auto& ob : outdoorArea->Boundaries() )
                ret.addDep(sg, ResourceGroup::Material, ob.outdoorBoundaryMaterial.materialHash);
            }
        }

        return ret;
    }
}

HouseBSData *ArchOrchestrator::H() {
    return houseJson().get();
}

Matrix4f ArchOrchestrator::calcFloorplanNavigationTransform( float screenRatio, float screenPadding ) {
    auto m = Matrix4f{ Matrix4f::IDENTITY() };
    float vmax = max(houseJson()->BBox().bottomRight().x(), houseJson()->BBox().bottomRight().y());
    float screenFloorplanRatio = ( 1.0f / screenRatio );
    float vmaxScale = vmax / screenFloorplanRatio;
    auto vr = 1.0f / vmaxScale;
    m.scale(V3f{ vr, -vr, -vr });
    m.translate(V3f{ getScreenAspectRatio - screenFloorplanRatio - screenPadding, screenFloorplanRatio,
                     screenFloorplanRatio });
    floorplanNavigationMatrix = m;
    return m;
}

void ArchOrchestrator::centerCameraMiddleOfHouseWithFloorplanInfoOffset( float floorplanOffset, float slack ) {
    if ( houseJson()->BBox3d().isValid() ) {
        Rect2f fpBBox = houseJson()->BBox();
        fpBBox.expand(houseJson()->BBox().centreTop() + V2fc::Y_AXIS * floorplanOffset);
        Timeline::play(rsg.DC()->PosAnim(), 0,
                       KeyFramePair{ 0.9f, rsg.DC()->center(fpBBox, slack) });
    }
}

void ArchOrchestrator::centerCameraMiddleOfHouse( float slack ) {
    if ( houseJson()->BBox().isValid() ) {
        Timeline::play(rsg.DC()->PosAnim(), 0,
                       KeyFramePair{ 0.9f, rsg.DC()->center(houseJson()->BBox(), slack) });
    }
}

void ArchOrchestrator::showIMHouse() {
    HouseRender::IMHouseRender(rsg.RR(), sg, H(), arc);
}

void ArchOrchestrator::make3dHouse( const PostHouse3dResolvedCallback& ccf ) {
    if ( !loadingMutex && H() ) {
        loadingMutex = true;
        rsg.RR().setLoadingFlag(true);
        HOD::resolver<HouseBSData>(sg, H(), [&, ccf]() {
            sg.loadCollisionMesh(HouseService::createCollisionMesh(H()));
            hrc = HouseRender::make3dGeometry(rsg.RR(), sg, H());
            if ( ccf ) ccf();
            LightmapManager::deserialiseLightmap(rsg.SG(), rsg.RR(), H()->getLightmapID(), {GLTF2Tag, ArchType::CurtainT});
            rsg.RR().setLoadingFlag(false);
            rsg.changeTime("14:00", H()->sourceData.northCompassAngle);
            V3f probePos = HouseService::centerOfBiggestRoom(H(), 1.25f);
            rsg.setProbePosition(probePos);
            rsg.setSkyboxCenter(H()->BBox3d().centre());
            loadingMutex = false;
        });
    }
}

/// This loads a house with a http get
/// \param _pid
/// \param ccf
void ArchOrchestrator::loadHouse( const std::string& _pid, const PostHouseLoadCallback& ccf,
                                  const PostHouseLoadCallback& ccfailure ) {
    Renderer::clearColor(C4fc::XTORGBA("8ae9e9"));
    Http::getNoCache(Url{ "/propertybim/" + _pid }, [&, ccf]( HttpResponeParams params ) {
        bool bLoadFailed = true;
        if ( !params.BufferString().empty() ) {
            auto newHouseInstance = std::make_shared<HouseBSData>(params.BufferString());
            if ( newHouseInstance->version >= SHouseJSONVersion ) {
                bLoadFailed = false;
                houseJson.reset(newHouseInstance);
                if ( ccf ) ccf();
            }
        }
        if ( bLoadFailed ) {
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
    houseJson.reset(_houseJson);
}

HouseRenderContainer& ArchOrchestrator::HRC() {
    return hrc;
}

void ArchOrchestrator::loadFurnitureMapStorage( const std::string& _name ) {
    Http::get(Url{ "/furnitureset/" + _name }, [&, this]( HttpResponeParams& res ) {
        FurnitureSetContainer fset{ res.BufferString() };
        for ( const auto& f : fset.set ) {
            sg.loadProfile(f.symbol);
            furnitureMap.addIndex(f);
        }
    });
}

FurnitureMapStorage& ArchOrchestrator::FurnitureMap() {
    return furnitureMap;
}

void ArchOrchestrator::onEvent( ArchIOEvents event ) {
    currIOEvent = event;
}

bool ArchOrchestrator::hasEvent( ArchIOEvents event ) const {
    return currIOEvent == event;
}

void ArchOrchestrator::undoHouseChange() {
    houseJson.undo();
}

void ArchOrchestrator::redoHouseChange() {
    houseJson.redo();
}

void ArchOrchestrator::pushHouseChange() {
    houseJson.push();
}

void ArchOrchestrator::setViewingMode( ArchViewingMode _wm ) {

    switch ( _wm ) {
        case AVM_Hidden:
            break;
        case AVM_Tour:
            setTourView();
            break;
        case AVM_Walk:
            setWalkView();
            break;
        case AVM_FloorPlan:
            setFloorPlanView(lastKnownGoodFloorPlanRenderMode);
            break;
        case AVM_TopDown:
            setTopDownView();
            break;
        case AVM_DollHouse:
            setDollHouseView();
            break;
    }
}

void ArchOrchestrator::setTourView() {
    arc.setViewingMode(ArchViewingMode::AVM_Tour);
    rsg.setRigCameraController(CameraControlType::Walk);
    arc.pm(RDSPreMult(Matrix4f::IDENTITY()));
    rsg.useSkybox(true);

    if ( !H()->tourPaths.empty() ) {
        for ( const auto& tour : H()->tourPaths ) {
            tourPlayback.beginPath(tour.path);
            for ( const auto& path : tour.path ) {
                tourPlayback.addKeyFrame(path);
            }
            tourPlayback.endPath(tour.path);
        }
        tourPlayback.playBack(rsg.DC());
    } else {
        V3f pos = V3fc::ZERO;
        Quaternion quat{ M_PI, V3fc::UP_AXIS };
        HouseService::bestStartingPositionAndAngle(H(), pos, quat);
        rsg.DC()->setPosition(pos);
        rsg.DC()->setQuat(quat);
    }

    Timeline::play(rsg.RR().ssaoBlendFactorAnim(), 0, KeyFramePair{ 0.9f, 0.0f });

    fader(0.9f, 0.0f, rsg.RR().CLI(CommandBufferLimits::UI2dStart));
    fader(0.9f, 0.0f, rsg.RR().CLI(CommandBufferLimits::UnsortedCustom));
    fader(0.9f, 0.0f, rsg.RR().CLI(CommandBufferLimits::CameraLocatorIM));
    fader(0.9f, 0.0f, rsg.RR().CLI(CommandBufferLimits::GridStart));
    fader(0.9f, 1.0f, rsg.RR().CLI(CommandBufferLimits::PBRStart));

    sg.setCollisionEnabled(false);
}

void ArchOrchestrator::setWalkView( float animationSpeed ) {
    arc.setViewingMode(ArchViewingMode::AVM_Walk);
    tourPlayback.stopPlayBack(rsg.DC());
    rsg.setRigCameraController(CameraControlType::Walk);
    rsg.useSkybox(true);
    V3f pos = V3fc::ZERO;
    Quaternion quat;
    HouseService::bestStartingPositionAndAngle(H(), pos, quat);
    if ( animationSpeed > 0.0f ) {
        Timeline::play(rsg.DC()->PosAnim(), 0,
                       KeyFramePair{ 0.9f * animationSpeed, pos });
        Timeline::play(rsg.DC()->QAngleAnim(), 0, KeyFramePair{ 0.9f * animationSpeed, quat });
    } else {
        rsg.DC()->setPosition(pos);
        rsg.DC()->setQuat(quat);
    }

    Timeline::play(rsg.RR().ssaoBlendFactorAnim(), 0, KeyFramePair{ 0.9f, 0.0f });

    fader(0.9f, 0.0f, rsg.RR().CLI(CommandBufferLimits::UI2dStart));
    fader(0.9f, 0.0f, rsg.RR().CLI(CommandBufferLimits::UnsortedCustom));
    fader(0.9f, 0.0f, rsg.RR().CLI(CommandBufferLimits::GridStart));
    fader(0.9f, 1.0f, rsg.RR().CLI(CommandBufferLimits::PBRStart),
          AnimEndCallback{ [&]() {
              fader(0.9f, 1.0f, rsg.RR().CLI(CommandBufferLimits::CameraLocatorIM));
              sg.setCollisionEnabled(true);
              fader(0.9f, 1.0f, rsg.RR().CLI(CommandBufferLimits::UI2dStart));
              arc.pm(RDSPreMult(calcFloorplanNavigationTransform(3.5f, 0.02f)));
              arc.renderMode(FloorPlanRenderMode::Normal2d);
              HouseRender::IMHouseRender(rsg.RR(), sg, H(), arc);
          } });
}

void ArchOrchestrator::setFloorPlanView( FloorPlanRenderMode fprm ) {
//    auto comingFromMode = arc.getViewingMode();
    arc.setViewingMode(ArchViewingMode::AVM_FloorPlan);
    // NDDado: we stop tourPlayback _before_ changing camera controller because camera controller sets a new FoV
    tourPlayback.stopPlayBack(rsg.DC());
    rsg.setRigCameraController(CameraControlType::Edit2d);
    rsg.DC()->LockAtWalkingHeight(false);
    arc.pm(RDSPreMult(Matrix4f::IDENTITY()));
    lastKnownGoodFloorPlanRenderMode = fprm;
    arc.renderMode(fprm);
    HouseRender::IMHouseDrawSourceDataFloorPlan(rsg.RR(), sg, H(), arc);
    HouseRender::IMHouseRender(rsg.RR(), sg, H(), arc);
    auto quatAngles = V3f{ M_PI_2, 0.0f, 0.0f };
    rsg.useSkybox(false);
    if ( H() ) {
        auto quat = quatCompose(quatAngles);
        Timeline::play(rsg.DC()->QAngleAnim(), 0, KeyFramePair{ 0.9f, quat });
        centerCameraMiddleOfHouseWithFloorplanInfoOffset(1.3f, 0.2f);
        showIMHouse();
    }

    Timeline::play(rsg.RR().ssaoBlendFactorAnim(), 0, KeyFramePair{ 0.9f, 1.0f });
    fader(0.9f, 1.0f, rsg.RR().CLI(CommandBufferLimits::UI2dStart));
    fader(0.9f, 1.0f, rsg.RR().CLI(CommandBufferLimits::UnsortedCustom));
    fader(0.9f, 0.0f, rsg.RR().CLI(CommandBufferLimits::CameraLocatorIM));
    fader(0.9f, 1.0f, rsg.RR().CLI(CommandBufferLimits::GridStart));
    fader(0.9f, 0.0f, rsg.RR().CLI(CommandBufferLimits::PBRStart));

    sg.setCollisionEnabled(false);
}

void ArchOrchestrator::setTopDownView() {
//    auto comingFromMode = arc.getViewingMode();
    arc.setViewingMode(ArchViewingMode::AVM_TopDown);
    tourPlayback.stopPlayBack(rsg.DC());
    rsg.setRigCameraController(CameraControlType::Edit2d);
    rsg.DC()->LockAtWalkingHeight(false);
    arc.pm(RDSPreMult(Matrix4f::IDENTITY()));
    arc.renderMode(FloorPlanRenderMode::Normal3d);
    auto quatAngles = V3f{ M_PI_2, 0.0f, 0.0f };
    rsg.useSkybox(true);
//    showIMHouse();
    auto quat = quatCompose(quatAngles);
    Timeline::play(rsg.DC()->QAngleAnim(), 0, KeyFramePair{ 0.9f, quat });
    centerCameraMiddleOfHouse(H()->Depth() + 0.3f);
    Timeline::play(rsg.RR().ssaoBlendFactorAnim(), 0, KeyFramePair{ 0.9f, 0.0f });
    fader(0.9f, 0.0f, rsg.RR().CLI(CommandBufferLimits::UI2dStart));
    fader(0.9f, 0.0f, rsg.RR().CLI(CommandBufferLimits::UnsortedCustom));
    fader(0.9f, 0.0f, rsg.RR().CLI(CommandBufferLimits::CameraLocatorIM));
    fader(0.9f, 0.0f, rsg.RR().CLI(CommandBufferLimits::GridStart));
    fader(0.9f, 0.0f, rsg.RR().CLIIncludingTag(CommandBufferLimits::PBRStart, ArchType::CeilingT));
    fader(0.9f, 1.0f, rsg.RR().CLIExcludingTag(CommandBufferLimits::PBRStart, ArchType::CeilingT));

    sg.setCollisionEnabled(false);
}

void ArchOrchestrator::setDollHouseView() {
//    auto comingFromMode = arc.getViewingMode();
    tourPlayback.stopPlayBack(rsg.DC());
    arc.setViewingMode(ArchViewingMode::AVM_DollHouse);
    rsg.setRigCameraController(CameraControlType::Fly);
    arc.pm(RDSPreMult(Matrix4f::IDENTITY()));
    rsg.useSkybox(true);
    V3f pos = V3fc::ZERO;
    Quaternion quat{};
    HouseService::bestDollyPositionAndAngle(H(), pos, quat);
    Timeline::play(rsg.DC()->PosAnim(), 0, KeyFramePair{ 0.9f, pos });
    Timeline::play(rsg.DC()->QAngleAnim(), 0, KeyFramePair{ 0.9f, quat });
    Timeline::play(rsg.RR().ssaoBlendFactorAnim(), 0, KeyFramePair{ 0.9f, 0.0f });
    fader(0.9f, 0.0f, rsg.RR().CLI(CommandBufferLimits::UI2dStart));
    fader(0.9f, 0.0f, rsg.RR().CLI(CommandBufferLimits::UnsortedCustom));
    fader(0.9f, 0.0f, rsg.RR().CLI(CommandBufferLimits::CameraLocatorIM));
    fader(0.9f, 0.0f, rsg.RR().CLI(CommandBufferLimits::GridStart));
    fader(0.9f, 0.0f, rsg.RR().CLIIncludingTag(CommandBufferLimits::PBRStart, ArchType::CeilingT));
    fader(0.9f, 1.0f, rsg.RR().CLIExcludingTag(CommandBufferLimits::PBRStart, ArchType::CeilingT));

    sg.setCollisionEnabled(false);
}

const Matrix4f& ArchOrchestrator::FloorplanNavigationMatrix() const {
    return floorplanNavigationMatrix;
}

ArchPositionalDot& ArchOrchestrator::PositionalDot() {
    return positionalDot;
}

ArchExplorer& ArchOrchestrator::Explorer() {
    return archExplorer;
}

void ArchOrchestrator::toggleCollisions() {
    sg.setCollisionEnabled(!sg.isCollisionEnabled());
}

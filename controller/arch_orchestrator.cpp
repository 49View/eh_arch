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
#include <eh_arch/models/house_service.hpp>
#include <eh_arch/render/house_render.hpp>

#include <eh_arch/controller/arch_render_controller.hpp>

ArchOrchestrator::ArchOrchestrator( SceneGraph& _sg, RenderOrchestrator& _rsg, ArchRenderController& _arc ) : sg(_sg),
                                                                                                              rsg(_rsg),
                                                                                                              arc(_arc) {
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
        }

        return ret;
    }
}

HouseBSData *ArchOrchestrator::H() {
    return houseJson().get();
}

Matrix4f ArchOrchestrator::calcFloorplanNavigationTransform( float screenRatio, float screenPadding ) {
    auto m = Matrix4f{ Matrix4f::IDENTITY };
    float vmax = max(houseJson()->bbox.bottomRight().x(), houseJson()->bbox.bottomRight().y());
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
    if ( houseJson()->bbox.isValid() ) {
        Rect2f fpBBox = houseJson()->bbox;
        fpBBox.expand(houseJson()->bbox.centreTop() + V2fc::Y_AXIS * floorplanOffset);
        Timeline::play(rsg.DC()->PosAnim(), 0,
                       KeyFramePair{ 0.9f, rsg.DC()->center(fpBBox, slack) });
    }
}

void ArchOrchestrator::centerCameraMiddleOfHouse( float slack ) {
    if ( houseJson()->bbox.isValid() ) {
        Timeline::play(rsg.DC()->PosAnim(), 0,
                       KeyFramePair{ 0.9f, rsg.DC()->center(houseJson()->bbox, slack) });
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
            rsg.RR().setLoadingFlag(false);
            rsg.setProbePosition(HouseService::centerOfBiggestRoom(H()));
            loadingMutex = false;
        });
    }
}

/// This loads a house with a http get
/// \param _pid
/// \param ccf
void ArchOrchestrator::loadHouse( const std::string& _pid, const PostHouseLoadCallback& ccf,
                                  const PostHouseLoadCallback& ccfailure ) {
    Renderer::clearColor(C4f::XTORGBA("8ae9e9"));
    Http::getNoCache(Url{ "/propertybim/" + _pid }, [&, ccf]( HttpResponeParams params ) {
        if ( !params.BufferString().empty() ) {
            houseJson.reset(std::make_shared<HouseBSData>(params.BufferString()));
            setViewingMode(AVM_Hidden);
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

void ArchOrchestrator::setTourView() {
    auto comingFromMode = arc.getViewingMode();
    arc.setViewingMode(ArchViewingMode::AVM_Tour);
    rsg.setRigCameraController(CameraControlType::Walk);
    arc.pm(RDSPreMult(Matrix4f::IDENTITY));
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
        V3f pos = V3f::ZERO;
        V3f quat = V3f::ZERO;
        HouseService::bestStartingPositionAndAngle(H(), pos, quat);
        rsg.DC()->setPosition(pos);
        rsg.DC()->setQuat(quat);
    }

    rsg.RR().showBucket(CommandBufferLimits::PBRStart, arc.getViewingMode() != ArchViewingMode::AVM_FloorPlan);
    if ( comingFromMode == ArchViewingMode::AVM_FloorPlan || comingFromMode == ArchViewingMode::AVM_DollHouse ||
         comingFromMode == ArchViewingMode::AVM_TopDown ) {
        rsg.RR().setVisibilityOnTags(ArchType::CeilingT, false);
        fader(0.001f, 0.0f, rsg.RR().getVPListWithTags(ArchType::CeilingT));
    }
    fader(0.9f, 0.0f, rsg.RR().CLI(CommandBufferLimits::UI2dStart));
    fader(0.9f, 0.0f, rsg.RR().CLI(CommandBufferLimits::CameraLocator));
    fader(0.9f, 0.0f, rsg.RR().CLI(CommandBufferLimits::GridStart));
    fader(0.9f, 1.0f, rsg.RR().CLIExcludingTag(CommandBufferLimits::PBRStart, ArchType::CeilingT),
          AnimEndCallback{ [&]() {
              rsg.RR().setVisibilityOnTags(ArchType::CeilingT, true);
              fader(0.2f, 1.0f, rsg.RR().getVPListWithTags(ArchType::CeilingT));
          } });

    sg.setCollisionEnabled(false);
}

void ArchOrchestrator::setAssistedView() {
}

void ArchOrchestrator::setWalkView( float animationSpeed ) {
    auto comingFromMode = arc.getViewingMode();
    arc.setViewingMode(ArchViewingMode::AVM_Walk);
    tourPlayback.stopPlayBack(rsg.DC());
    rsg.setRigCameraController(CameraControlType::Walk);
    rsg.useSkybox(true);
    V3f pos = V3f::ZERO;
    V3f quatAngles = V3f::ZERO;
    HouseService::bestStartingPositionAndAngle(H(), pos, quatAngles);
    auto quat = quatCompose(quatAngles);
    rsg.DC()->setIncrementQuatAngles(quatAngles);
    if ( animationSpeed > 0.0f ) {
        Timeline::play(rsg.DC()->PosAnim(), 0,
                       KeyFramePair{ 0.9f * animationSpeed, pos });
        Timeline::play(rsg.DC()->QAngleAnim(), 0, KeyFramePair{ 0.9f * animationSpeed, quat });
    } else {
        rsg.DC()->setPosition(pos);
        rsg.DC()->setQuat(quat);
    }

    rsg.RR().showBucket(CommandBufferLimits::PBRStart, arc.getViewingMode() != ArchViewingMode::AVM_FloorPlan);
    if ( comingFromMode == ArchViewingMode::AVM_FloorPlan || comingFromMode == ArchViewingMode::AVM_DollHouse ||
         comingFromMode == ArchViewingMode::AVM_TopDown ) {
        rsg.RR().setVisibilityOnTags(ArchType::CeilingT, false);
        fader(0.001f, 0.0f, rsg.RR().getVPListWithTags(ArchType::CeilingT));
    }
    fader(0.9f, 0.0f, rsg.RR().CLI(CommandBufferLimits::UI2dStart));
    fader(0.9f, 1.0f, rsg.RR().CLI(CommandBufferLimits::CameraLocator));
    fader(0.9f, 0.0f, rsg.RR().CLI(CommandBufferLimits::GridStart));
    fader(0.9f, 1.0f, rsg.RR().CLIExcludingTag(CommandBufferLimits::PBRStart, ArchType::CeilingT),
          AnimEndCallback{ [&]() {
              rsg.RR().setVisibilityOnTags(ArchType::CeilingT, true);
              fader(0.2f, 1.0f, rsg.RR().getVPListWithTags(ArchType::CeilingT));
              sg.setCollisionEnabled(true);
              arc.pm(RDSPreMult(calcFloorplanNavigationTransform(3.5f, 0.02f)));
              arc.renderMode(FloorPlanRenderMode::Normal2d);
              HouseRender::IMHouseRender(rsg.RR(), sg, H(), arc);
          } });
}

void ArchOrchestrator::setFloorPlanView() {
//    auto comingFromMode = arc.getViewingMode();
    arc.setViewingMode(ArchViewingMode::AVM_FloorPlan);
    // NDDado: we stop tourPlayback _before_ changing camera controller because camera controller sets a new FoV
    tourPlayback.stopPlayBack(rsg.DC());
    rsg.setRigCameraController(CameraControlType::Edit2d);
    rsg.DC()->LockAtWalkingHeight(false);
    arc.pm(RDSPreMult(Matrix4f::IDENTITY));
    arc.renderMode(FloorPlanRenderMode::Normal3d);
    HouseRender::IMHouseRender(rsg.RR(), sg, H(), arc);
    auto quatAngles = V3f{ M_PI_2, 0.0f, 0.0f };
    rsg.DC()->setIncrementQuatAngles(quatAngles);
    rsg.useSkybox(false);
    if ( H() ) {
        auto quat = quatCompose(quatAngles);
        Timeline::play(rsg.DC()->QAngleAnim(), 0, KeyFramePair{ 0.9f, quat });
        centerCameraMiddleOfHouseWithFloorplanInfoOffset(0.0f, 0.0f); //1.3f, 0.2f;
        arc.setFloorPlanTransparencyFactor(0.5f);
        showIMHouse();
    }
    fader(0.9f, 1.0f, rsg.RR().CLI(CommandBufferLimits::UI2dStart));
    fader(0.9f, 0.0f, rsg.RR().CLI(CommandBufferLimits::CameraLocator));
    fader(0.9f, 1.0f, rsg.RR().CLI(CommandBufferLimits::GridStart));
    fader(0.9f, 0.0f, AnimEndCallback{ [&]() {
        rsg.RR().showBucket(CommandBufferLimits::PBRStart, false);
    } }, rsg.RR().CLI(CommandBufferLimits::PBRStart));

    sg.setCollisionEnabled(false);
}

void ArchOrchestrator::setTopDownView() {
    auto comingFromMode = arc.getViewingMode();
    arc.setViewingMode(ArchViewingMode::AVM_TopDown);
    tourPlayback.stopPlayBack(rsg.DC());
    rsg.RR().showBucket(CommandBufferLimits::PBRStart, arc.getViewingMode() != ArchViewingMode::AVM_FloorPlan);
    rsg.setRigCameraController(CameraControlType::Edit2d);
    rsg.DC()->LockAtWalkingHeight(false);
    arc.pm(RDSPreMult(Matrix4f::IDENTITY));
    auto quatAngles = V3f{ M_PI_2, 0.0f, 0.0f };
    rsg.DC()->setIncrementQuatAngles(quatAngles);
    rsg.useSkybox(false);
    arc.setFloorPlanTransparencyFactor(0.0f);
    showIMHouse();
    auto quat = quatCompose(quatAngles);
    Timeline::play(rsg.DC()->QAngleAnim(), 0, KeyFramePair{ 0.9f, quat });
    centerCameraMiddleOfHouse(2.45f);
//    rsg.RR().setVisibilityOnTags(ArchType::CeilingT, false);
    fader(0.9f, 1.0f, rsg.RR().CLI(CommandBufferLimits::UI2dStart));
    fader(0.9f, 0.0f, rsg.RR().CLI(CommandBufferLimits::CameraLocator));
    fader(0.9f, 0.0f, rsg.RR().CLI(CommandBufferLimits::GridStart));

    if ( comingFromMode != ArchViewingMode::AVM_FloorPlan && comingFromMode != ArchViewingMode::AVM_DollHouse ) {
        fader(0.1f, 0.0f, rsg.RR().getVPListWithTags(ArchType::CeilingT),
              AnimEndCallback{ [&]() {
                  rsg.RR().setVisibilityOnTags(ArchType::CeilingT, false);
              } });
    } else {
        rsg.RR().setVisibilityOnTags(ArchType::CeilingT, false);
    }
    fader(0.9f, 1.0f, rsg.RR().CLIExcludingTag(CommandBufferLimits::PBRStart, ArchType::CeilingT));

//    fader(0.9f, 1.0f, rsg.RR().CLI(CommandBufferLimits::PBRStart), AnimEndCallback{ [&]() {
//        fader(0.2f, 0.0f, rsg.RR().getVPListWithTags(ArchType::CeilingT));
//    } });
    sg.setCollisionEnabled(false);
}

void ArchOrchestrator::setDollHouseView() {
    auto comingFromMode = arc.getViewingMode();
    arc.setViewingMode(ArchViewingMode::AVM_DollHouse);
    tourPlayback.stopPlayBack(rsg.DC());
    rsg.setRigCameraController(CameraControlType::Fly);
    arc.pm(RDSPreMult(Matrix4f::IDENTITY));
    rsg.useSkybox(true);
    V3f pos = V3f::ZERO;
    V3f quatAngles = V3f::ZERO;
    HouseService::bestDollyPositionAndAngle(H(), pos, quatAngles);
    auto quat = quatCompose(quatAngles);
    rsg.DC()->setIncrementQuatAngles(quatAngles);
    Timeline::play(rsg.DC()->PosAnim(), 0, KeyFramePair{ 0.9f, pos });
    Timeline::play(rsg.DC()->QAngleAnim(), 0, KeyFramePair{ 0.9f, quat });
    fader(0.9f, 0.0f, rsg.RR().CLI(CommandBufferLimits::UI2dStart));
    fader(0.9f, 0.0f, rsg.RR().CLI(CommandBufferLimits::CameraLocator));
    fader(0.9f, 0.0f, rsg.RR().CLI(CommandBufferLimits::GridStart));
    rsg.RR().showBucket(CommandBufferLimits::PBRStart, true);
    if ( comingFromMode != ArchViewingMode::AVM_FloorPlan && comingFromMode != ArchViewingMode::AVM_TopDown ) {
        fader(0.2f, 0.0f, rsg.RR().getVPListWithTags(ArchType::CeilingT),
              AnimEndCallback{ [&]() {
                  rsg.RR().setVisibilityOnTags(ArchType::CeilingT, false);
              } });
    } else {
        rsg.RR().setVisibilityOnTags(ArchType::CeilingT, false);
    }
    fader(0.9f, 1.0f, rsg.RR().CLIExcludingTag(CommandBufferLimits::PBRStart, ArchType::CeilingT));

    sg.setCollisionEnabled(false);
}

void ArchOrchestrator::setViewingMode( ArchViewingMode _wm ) {

    switch ( _wm ) {
        case AVM_Hidden:
            break;
        case AVM_Tour:
            setTourView();
            break;
        case AVM_Assisted:
            setAssistedView();
            break;
        case AVM_Walk:
            setWalkView();
            break;
        case AVM_FloorPlan:
            setFloorPlanView();
            break;
        case AVM_TopDown:
            setTopDownView();
            break;
        case AVM_DollHouse:
            setDollHouseView();
            break;
    }
}

void ArchOrchestrator::updateViewingModes() {
    if ( H() && arc.getViewingMode() == AVM_Walk ) {
        auto camPos = rsg.DC()->getPosition() * V3f::MASK_Y_OUT;
        auto camDir = -rsg.DC()->getDirection() * 0.7f;
        auto sm = DShaderMatrix{ DShaderMatrixValue2dColor };
        rsg.RR().clearBucket(CommandBufferLimits::CameraLocator);
        rsg.RR().draw<DCircleFilled>(CommandBufferLimits::CameraLocator, camPos, V4f::DARK_RED, 0.4f,
                                     RDSPreMult(floorplanNavigationMatrix),
                                     sm, std::string{ "CameraOminoKey" });
        rsg.RR().draw<DArrow>(CommandBufferLimits::CameraLocator, V3fVector{ camPos, camPos + camDir },
                              RDSArrowAngle(0.45f),
                              RDSArrowLength(0.6f), V4f::RED, 0.004f, sm, RDSPreMult(floorplanNavigationMatrix),
                              std::string{ "CameraOminoKeyDirection1" });
    }
}

//
// Created by dado on 04/06/2020.
//

#pragma once

struct ActivateOrbitMode {
    void
    operator()( SceneGraph& sg, ArchOrchestrator& asg, RenderOrchestrator& rsg, ArchRenderController& arc ) noexcept {
        rsg.setRigCameraController(CameraControlType::Orbit);
        rsg.useSkybox(true);
        rsg.RR().createGrid(CommandBufferLimits::GridStart, 1.0f, ( Color4f::PASTEL_GRAYLIGHT ),
                            ( Color4f::DARK_GRAY ), V2f{ 15.0f }, 0.015f);
    }
};

struct InitializeHouseMaker {
    void operator()( SceneGraph& sg, ArchRenderController& arc, RenderOrchestrator& rsg, ArchOrchestrator& asg,
                     const OnActivateEvent& ev ) noexcept {
        rsg.DC()->setQuat(quatCompose(V3f{ M_PI_2, 0.0f, 0.0f }));
        rsg.DC()->setPosition(V3f::UP_AXIS * 15.0f);
        asg.setFloorPlanView(ev.floorPlanRenderMode);
        if ( ev.ccf ) ev.ccf();
    }
};

static inline void show3dViewInternal( ArchOrchestrator& asg, std::function<void()> callback ) {
    if ( !asg.H() ) return;
    if ( asg.HRC().houseId != asg.H()->propertyId ) {
        asg.make3dHouse(callback);
    } else {
        callback();
    }
}

struct ActivateFloorplanView {
    void
    operator()( SceneGraph& sg, ArchRenderController& arc, RenderOrchestrator& rsg, ArchOrchestrator& asg ) noexcept {
        asg.setFloorPlanView(fprm);
    }

    FloorPlanRenderMode fprm = FloorPlanRenderMode::Normal3d;
};

struct ActivateTourView {
    void
    operator()( SceneGraph& sg, ArchOrchestrator& asg, RenderOrchestrator& rsg, ArchRenderController& arc ) noexcept {
        show3dViewInternal(asg, [&]() {
            asg.setTourView();
        });
    }
};

struct ActivateTopDownView {
    void
    operator()( SceneGraph& sg, ArchOrchestrator& asg, RenderOrchestrator& rsg, ArchRenderController& arc ) noexcept {
        show3dViewInternal(asg, [&]() {
            asg.setTopDownView();
        });
    }
};

struct ActivateWalkView {
    void
    operator()( SceneGraph& sg, ArchOrchestrator& asg, RenderOrchestrator& rsg, ArchRenderController& arc ) noexcept {
        LOGRS("Switching to Walk View")
        show3dViewInternal(asg, [&]() {
            asg.setWalkView();
        });
    }
};

struct ActivateDollyHouseView {
    void
    operator()( SceneGraph& sg, ArchOrchestrator& asg, RenderOrchestrator& rsg, ArchRenderController& arc ) noexcept {
        show3dViewInternal(asg, [&]() {
            asg.setDollHouseView();
        });
    }
};

struct WhichRoomAmI {
    void operator()( ArchOrchestrator& asg, RenderOrchestrator& rsg, ArchRenderController& arc ) {
        if ( asg.H() ) {
            V2f pos2d = XZY::C2(rsg.DC()->getPosition());
            if ( auto ret = HouseService::whichRoomAmI(asg.H(), pos2d); ret ) {
                arc.setSelectionList(*ret);
            }
        }
    }
};

struct PushTourPath {
    void operator()( ArchOrchestrator& asg, RenderOrchestrator& rsg, ArchRenderController& arc ) {
        HouseService::pushTourPath(asg.H(), CameraSpatialsKeyFrame{ rsg.DC()->getSpatials(), 0.0f });
    }
};

struct PushKeyFrameTourPath {
    void operator()( OnPushKeyFrameTourPathEvent event, ArchOrchestrator& asg, RenderOrchestrator& rsg ) {
        HouseService::pushKeyFrameTourPath(asg.H(), CameraSpatialsKeyFrame{ rsg.DC()->getSpatials(), event.timestamp });
    }
};

struct PopTourPath {
    void
    operator()( OnPopTourPathEvent event, ArchOrchestrator& asg, RenderOrchestrator& rsg, ArchRenderController& arc ) {
        HouseService::popTourPath(asg.H(), event.popIndex);
    }
};

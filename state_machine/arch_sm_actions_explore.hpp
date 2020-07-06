//
// Created by dado on 04/06/2020.
//

#pragma once

struct WhichRoomAmI {
    void operator()( ArchOrchestrator& asg, RenderOrchestrator& rsg, ArchRenderController& arc ) {
        if ( asg.H() ) {
            V2f pos2d = XZY::C2(rsg.DC()->getPosition());
            if ( auto ret = HouseService::whichRoomAmI(asg.H(), pos2d ); ret ) {
                arc.setSelectionList(*ret);
            }
        }
    }
};

struct PushTourPath {
    void operator()( ArchOrchestrator& asg, RenderOrchestrator& rsg, ArchRenderController& arc ) {
        HouseService::pushTourPath(asg.H(), CameraSpatialsKeyFrame{ rsg.DC()->getSpatials(), 0.0f } );
    }
};

struct PushKeyFrameTourPath {
    void operator()( OnPushKeyFrameTourPathEvent event, ArchOrchestrator& asg, RenderOrchestrator& rsg ) {
        HouseService::pushKeyFrameTourPath(asg.H(), CameraSpatialsKeyFrame{ rsg.DC()->getSpatials(), event.timestamp } );
    }
};

struct PopTourPath {
    void operator()( OnPopTourPathEvent event, ArchOrchestrator& asg, RenderOrchestrator& rsg, ArchRenderController& arc ) {
        HouseService::popTourPath(asg.H(), event.popIndex);
    }
};

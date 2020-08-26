//
// Created by dado on 04/06/2020.
//

#pragma once

struct UndoOutdoorArea {
    void operator()( OutdoorAreaBuilder* rb ) noexcept {
//        rb->undo();
    }
};

struct ExitOutdoorArea {
    void operator()( OutdoorAreaUI& oaUI ) noexcept {
        // On Exit, we might need to save some states, cache or whatever, do it here
        oaUI.activate(false);
    }
};

struct AddPointOutdoorAreaAction {
    void operator()( OutdoorAreaUI& oaUI, const OnSingleTapEvent& mouseEvent ) noexcept {
        oaUI.addPoint(mouseEvent.mousePos);
    }
};

struct FinaliseOutdoorArea {
    void operator()( OutdoorAreaUI& oaUI, ArchOrchestrator& asg ) noexcept {
        if ( asg.H() ) {
            FloorService::addOutdoorAreaFromData( asg.H()->mFloors[0].get(), oaUI.OutdoorAreaData() );
            asg.showIMHouse();
        }
    }
};

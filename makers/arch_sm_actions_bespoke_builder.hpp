//
// Created by dado on 04/06/2020.
//

#pragma once

struct UndoBespoke {
    void operator()( RoomBuilder* rb ) noexcept {
        rb->undo();
    }
};

struct ClearBespoke {
    void operator()( RoomBuilder* rb ) noexcept {
        rb->clear();
    }
};

struct TouchMoveBespoke {
    void operator()( RoomBuilder* rb, const OnTouchMoveEvent& mouseEvent ) noexcept {
        rb->setCurrentPointerPos(mouseEvent.mousePos);
    }
};

struct TouchUpEventBespoke {
    void operator()( RoomBuilder* rb, const OnTouchUpEvent& mouseEvent ) noexcept {
        rb->validateAddPoint(mouseEvent.mousePos);
    }
};


struct KeyToggleBespoke {
    void operator()( RoomBuilder* rb, HouseMakerStateMachine& hm, OnKeyToggleEvent keyEvent ) noexcept {

        if ( keyEvent.keyCode == GMK_Z ) {
            rb->changeSegmentType(ArchType::WallT);
        }
        if ( keyEvent.keyCode == (GMK_X) ) {
            rb->changeSegmentType(ArchType::WindowT);
        }
        if ( keyEvent.keyCode == (GMK_C) ) {
            rb->changeSegmentType(ArchType::DoorT);
        }
    }
};

struct FinaliseBespoke {
    void operator()( HouseMakerStateMachine& hm, RoomBuilder* rb, ArchOrchestrator& asg ) noexcept {
        HouseService::mergePoints( asg.H(), rb->bespokeriseWalls());
        HouseMakerBitmap::makeFromWalls( asg.H() );
        asg.showIMHouse();
    }
};

struct ExitBespoke {
    void operator()( [[maybe_unused]] RoomBuilder* rb ) noexcept {
        // On Exit, we might need to save some states, cache or whatever, do it here
    }
};


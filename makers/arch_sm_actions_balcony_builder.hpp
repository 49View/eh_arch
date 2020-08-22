//
// Created by dado on 04/06/2020.
//

#pragma once

struct UndoBalcony {
    void operator()( BalconyBuilder* rb ) noexcept {
//        rb->undo();
    }
};

struct ExitBalcony {
    void operator()() noexcept {
        // On Exit, we might need to save some states, cache or whatever, do it here
    }
};

struct AddPointBalconyAction {
    void operator()( BalconyBuilder* bb, const OnSingleTapEvent& mouseEvent ) noexcept {
        bb->addPoint(mouseEvent.mousePos);
    }
};

struct FinaliseBalcony {
    void operator()( BalconyBuilder* bb, ArchOrchestrator& asg ) noexcept {
        if ( asg.H() ) {
            bb->finalize();
            FloorService::addBalconyFromData( asg.H()->mFloors[0].get(), bb->BalconyData() );
            asg.showIMHouse();
        }
    }
};

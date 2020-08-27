//
// Created by dado on 19/08/2020.
//

#include "outdoor_area_builder.hpp"

#include <graphics/renderer.h>
#include <poly/scene_graph.h>
#include <render_scene_graph/render_orchestrator.h>
#include <eh_arch/models/house_bsdata.hpp>

void OutdoorAreaBuilder::refresh() {
    Renderer& rr = rsg.RR();
    rr.clearBucket(rrBucket);

    for ( const auto& ob : outdoorAreaData->Boundaries() ) {
        if ( ob.bPoints.size() == 2 ) {
            rr.draw<DLine>(rrBucket, ob.bPoints, "outdoorAreaBase");
        } else if ( ob.bPoints.size() > 2 ) {
            rr.draw<DFlatPoly>(rrBucket, ob.bPoints, "outdoorAreaBase");
        }
        int cc = 0;
        for ( const auto& p : ob.bPoints ) {
            rr.draw<DCircleFilled>(rrBucket, p, 0.02f, C4f::STEEL_BLUE, std::to_string(cc++) + "outdoorAreaBase" );
        }
    }

}

void OutdoorAreaBuilder::addPoint( const V2f& _p, int _bIndex ) {
    outdoorAreaData->Boundary(_bIndex).bPoints.emplace_back( _p );
    refresh();
}

OutdoorAreaBuilder::OutdoorAreaBuilder( SceneGraph& sg, RenderOrchestrator& rsg ) : sg(sg), rsg(rsg) {
    rrBucket = CommandBufferLimits::UnsortedStart + 3;
    clear();
}

const std::shared_ptr<OutdoorAreaBSData>& OutdoorAreaBuilder::OutdoorAreaData() const {
    return outdoorAreaData;
}

void OutdoorAreaBuilder::OutdoorAreaData( std::shared_ptr<OutdoorAreaBSData> _outdoorAreaData ) {
    outdoorAreaData = _outdoorAreaData;
}

void OutdoorAreaBuilder::clear() {
    Renderer& rr = rsg.RR();
    rr.clearBucket(rrBucket);
}


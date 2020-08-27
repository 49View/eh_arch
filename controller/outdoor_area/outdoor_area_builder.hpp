//
// Created by dado on 19/08/2020.
//

#pragma once

#include <memory>
#include <core/math/vector2f.h>
#include <eh_arch/models/house_bsdata.hpp>

class SceneGraph;
class RenderOrchestrator;
class ArchOrchestrator;

class OutdoorAreaBuilder {
public:
    OutdoorAreaBuilder( SceneGraph& sg, RenderOrchestrator& rsg );
    void addPoint( const V2f& _p, int bIndex );
    [[nodiscard]] const std::shared_ptr<OutdoorAreaBSData>& OutdoorAreaData() const;
    void OutdoorAreaData( std::shared_ptr<OutdoorAreaBSData> outdoorAreaData );
    void clear();
private:
    void refresh();

private:
    SceneGraph& sg;
    std::shared_ptr<OutdoorAreaBSData> outdoorAreaData;
    RenderOrchestrator& rsg;
    int rrBucket = 0;
};

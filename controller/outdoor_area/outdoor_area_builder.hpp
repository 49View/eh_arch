//
// Created by dado on 19/08/2020.
//

#pragma once

#include <memory>
#include <core/math/vector3f.h>
#include <eh_arch/models/house_bsdata.hpp>

class SceneGraph;
class RenderOrchestrator;
class ArchOrchestrator;

class OutdoorAreaBuilder {
public:
    [[maybe_unused]] OutdoorAreaBuilder( SceneGraph& sg, RenderOrchestrator& rsg );
    void addPoint( const V2f& _p, int bIndex );
    [[nodiscard]] const std::shared_ptr<OutdoorAreaBSData>& OutdoorAreaData() const;
    void OutdoorAreaData( std::shared_ptr<OutdoorAreaBSData> outdoorAreaData );
    void clear();
    void cloneBoundary( std::size_t _index );
private:
    void refresh();

private:
    SceneGraph& sg;
    std::shared_ptr<OutdoorAreaBSData> outdoorAreaData;
    std::vector<V3f> computedSnapPoints{};
    RenderOrchestrator& rsg;
    int rrBucket = 0;
};

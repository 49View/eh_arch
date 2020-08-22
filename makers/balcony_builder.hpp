//
// Created by dado on 19/08/2020.
//

#pragma once

#include <memory>
#include <core/math/vector2f.h>

struct BalconyBSData;
class SceneGraph;
class RenderOrchestrator;
class ArchOrchestrator;

class BalconyBuilder {
public:
    BalconyBuilder( SceneGraph& sg, RenderOrchestrator& rsg );
    void addPoint( const V2f& _p );
    [[nodiscard]] const std::shared_ptr<BalconyBSData>& BalconyData() const;
    void finalize();
private:
    void refresh();

private:
    SceneGraph& sg;
    std::shared_ptr<BalconyBSData> balconyData;
    RenderOrchestrator& rsg;
    int rrBucket = 0;
};

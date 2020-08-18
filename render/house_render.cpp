//
// Created by Dado on 22/10/2019.
//

#include "house_render.hpp"
#include <core/resources/resource_builder.hpp>
#include <poly/scene_graph.h>
#include <graphics/ghtypes.hpp>
#include <graphics/renderer.h>
#include <graphics/render_light_manager.h>

#include <eh_arch/controller/arch_render_controller.hpp>
#include <core/util.h>
#include <core/font_utils.hpp>
#include "wall_render.hpp"
#include "floor_render.hpp"

namespace HouseRender {

    void IMHouseDrawSourceDataFloorPlan( Renderer& rr, SceneGraph& sg, const HouseBSData *data,
                                         const ArchRenderController& arc ) {
        // If no data clearly early exit with just clear of buckets performed
        if ( !data ) return;

        // We have 3 combinations here:
        // 1) It's a 3d floorPlan with a source image, render the source image as a background
        // 2) it's a 2d floorPlan so no images allowed, render a flat poly
        // 3) it's a 3d floorPlan but it hasn't got a source image, (IE not HouseMakerBitmap), renders a flat poly

        auto sourceImage = sg.get<RawImage>(data->propertyId);
        if ( sourceImage && !isFloorPlanRenderMode2d(arc.renderMode()) ) {
            // 1)
            auto floorPlanBBox = Rect2f{ V2fc::ZERO, V2f{ sourceImage->width, sourceImage->height } *
                                                     data->sourceData.rescaleFactor };
            auto nameKey = data->propertyId;
            rr.draw<DRect>(CommandBufferLimits::UnsortedCustom, floorPlanBBox, RDSImage(data->propertyId), RDSRectAxis::XZ, nameKey);
        }
        // 2), 3) have been moved in IMHouseRender as they are now part of the house IM render
    }

    void IMHouseRender( Renderer& rr, SceneGraph& sg, const HouseBSData *data, const ArchRenderController& arc ) {

        // Clear the bucket always, because if house it's null it should show a clear/empty screen
        rr.clearBucket(CommandBufferLimits::UI2dStart);

        // If no data clearly early exit with just clear of buckets performed
        if ( !data ) return;

        IMHouseDrawSourceDataFloorPlan( rr, sg, data, arc );

        auto sm = arc.floorPlanShader();

        // Render whole house BBox flat poly
//        float padding = 0.02f;
//        auto houseRect = Rect2f{ 0.0f, 0.0f, data->BBox().bottomRight().x() + padding,
//                                 data->BBox().bottomRight().y() + padding };
//        rr.draw<DFlatPoly>(CommandBufferLimits::UI2dStart, sm, houseRect.pointscw(), C4f::WHITE, arc.pm(), "floorplanFlatPoly");

        // Render general house information
        auto font = sg.FM().get(S::DEFAULT_FONT).get();
        auto color = arc.getFillColor(data, C4f::BLACK);

        auto d1 = [&]( const std::string& text, float fontHeight, float yOffset ) {
            auto textPos = FontUtils::fitTextInBox(font, text, data->BBox(), fontHeight);
            textPos.setY(data->BBox().centreTop().y() + yOffset);

            rr.draw<DText>(FDS{ text, font, textPos, fontHeight }, color, arc.pm(),
                           data->hashFeature(text + color.toString()+sm.hash(), 0));
        };

        d1( data->name, 0.5f, 0.7f );
        d1( "Approx Gross Internal Area: " + sqmToSqFtString(data->walkableArea) + " / " + sqmToString(data->walkableArea),
                0.32f, 1.2f);

        for ( const auto& f : data->mFloors ) {
            FloorRender::IMHouseRender(rr, sg, f.get(), arc);
        }
    }

    HouseRenderContainer make3dGeometry( Renderer& rr, SceneGraph& sg, const HouseBSData *data ) {
        // Clear the bucket always, because if house it's null it should show a clear/empty screen
        rr.clearBucket(CommandBufferLimits::PBRStart);
        rr.LM()->removeAllPointLights();

        // If no data clearly early exit with just clear of buckets performed
        if ( !data ) return HouseRenderContainer{};

        HouseRenderContainer ret{ data->propertyId };

        sg.addSkybox(data->defaultSkybox);

        // Infinite plane
        sg.GB<GT::Shape>(ShapeType::Cube, GT::Tag(SHADOW_MAGIC_TAG), V3f::UP_AXIS_NEG * 0.15f,
                         GT::Scale(500.0f, 0.1f, 500.0f));

        for ( const auto& f : data->mFloors ) {
            FloorRender::make3dGeometry(sg, f.get(), ret);
        }

        return ret;
    }
}

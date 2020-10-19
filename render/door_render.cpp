//
//  door_render.cpp
//  sixthmaker
//
//  Created by Dado on 19/03/2017.
//
//

#include "door_render.hpp"

#include <core/math/path_util.h>
#include <core/resources/profile.hpp>
#include <core/v_data.hpp>
#include <core/resources/resource_builder.hpp>
#include <poly/scene_graph.h>
#include <poly/vdata_assembler.h>
#include <poly/poly_services.hpp>
#include <core/math/triangulator.hpp>
#include <graphics/renderer.h>

#include "../models/house_bsdata.hpp"
#include <eh_arch/controller/arch_render_controller.hpp>

namespace DoorRender {

    bool isLeft( int index ) {
        return index >= 2;
    }

    void drawSingleDoor2d( Renderer& rr, const DoorBSData *door, DShaderMatrix sm, const ArchRenderController& arc ) {

        auto lineWidth = arc.floorPlanScaler(0.03f);
        auto color = arc.getFillColor(door, C4fc::BLACK);

        float vWangle = -atan2(-door->dirWidth.y(), door->dirWidth.x());
        float dIndexSign = (isLeft(door->dIndex) ? -1.0f : 1.0f);

        V2f dp = XZY::C2(door->doorPivot);
        dp.rotate(vWangle + M_PI);

        auto p1 = door->Position2d() + dp;

        V2f dn = V2fc::X_AXIS * (door->Width());
        dn.rotate(vWangle + M_PI + door->openingAngleMax * dIndexSign);
        V2f p3 = p1 + dn;

        V2f dn2 = V2fc::X_AXIS * (door->Width());
        dn2.rotate(vWangle + M_PI);
        V2f p2 = p1 + dn2 * dIndexSign;

        float dist = distance(p1, p2);

        std::vector<V2f> vLists;
        vLists.emplace_back(p2);

        static const int numSlopes = 15;
        float deltaInc = 1.0f / static_cast<float>(numSlopes - 1);
        float delta = deltaInc;
        for ( int q = 0; q < numSlopes - 1; q++ ) {
            auto npStraightLerp = JMATH::lerp(delta, p2, p3);
            auto npDir = normalize(npStraightLerp - p1);
            vLists.emplace_back(p1 + npDir * dist);
            delta += deltaInc;
        }

        vLists.emplace_back(p1);
        rr.draw<DLine>(vLists, color, lineWidth, false, sm, arc.pm(), door->hashFeature("singleDoor2d"+color.toString()+sm.hash(), 0));
    }

    void drawDoubleDoor2d();

    void IMHouseRender( Renderer& rr, SceneGraph& sg, const DoorBSData *data, const ArchRenderController& arc ) {
        auto rm = arc.floorPlanShader();
        drawSingleDoor2d(rr, data, rm, arc);
    }

    std::shared_ptr<Profile> makeEnglishDoorProfile( const Vector2f& vv2fs ) {

        std::vector<Vector2f> points;

        points.emplace_back(1.0f, -1.0f);
        points.emplace_back(1.0f, 0.0f);

        points.emplace_back(0.99f, -0.1f);
        points.emplace_back(0.98f, -0.1f);
        points.emplace_back(0.85f, -0.4f);
        points.emplace_back(0.82f, -0.9f);
        points.emplace_back(0.80f, -0.9f);
        points.emplace_back(0.72f, -1.0f);
        points.emplace_back(0.62f, -0.85f);
        points.emplace_back(0.01f, -0.1f);
        points.emplace_back(0.07f, -0.1f);

        points.emplace_back(0.0f, -0.0f);
        points.emplace_back(0.0f, -1.0f);

        for ( auto& p : points ) p *= vv2fs;

        auto doorFrame_profile = std::make_shared<Profile>();
        doorFrame_profile->createArbitrary(points);

        return doorFrame_profile;
    }

    template<size_t Tx = 2, size_t Ty = 3>
    void englishDoor( SceneGraph& sg, GeomSP door,
                      const Vector2f& _doorSize,
                      const V3f& doorPivot,
                      float doorThickness,
                      const std::array<Vector2f, Ty>& _pRatios = { Vector2f{ 0.288f, 0.314f },
                                                                   Vector2f{ 0.288f, 0.314f },
                                                                   Vector2f{ 0.288f, 0.111f } },
                      const std::array<float, Ty>& yGapRatios = { 0.071f, 0.091f, 0.045f } ) {

        std::vector<Vector2fList> holes;
        Rect2f quad{ V2f{ 0.0f, 0.0f }, _doorSize };

        static const size_t numPanels = Tx * Ty;
        Vector2fList hole;
        std::array<Vector2f, Ty> dSizes{};
        for ( size_t i = 0; i < Ty; i++ ) {
            dSizes[i] = { _pRatios[i].x() * _doorSize.x(), _pRatios[i].y() * _doorSize.y() };
        }
        float xGapDelta = ( quad.width() - ( dSizes[0].x() * Tx ) ) / ( Tx + 1 );
        std::vector<float> yGaps;
        yGaps.reserve(yGapRatios.size());
        for ( const auto& yg : yGapRatios ) {
            yGaps.emplace_back(yg * _doorSize.y());
        }

        std::array<Vector2f, numPanels> o{};
        std::array<Rect2f, numPanels> oRect{};
        std::array<Rect2f, numPanels> oRectInner{};
        size_t tc = 0;
        for ( size_t ty = 0; ty < Ty; ty++ ) {
            for ( size_t tx = 0; tx < Tx; tx++ ) {
                float ppy = ty == 0 ? 0.0f : dSizes[ty - 1].y() + yGaps[ty];
                float currY = tc == 0 ? yGaps[0] : o[tc - 1].y() + ( tx == 0 ? ppy : 0.0f );

                o[tc] = { xGapDelta + ( tx * ( dSizes[ty].x() + xGapDelta ) ), currY };

                oRect[tc] = Rect2f{ o[tc], o[tc] + dSizes[ty] };

                holes.emplace_back(oRect[tc].points());
                oRectInner[tc] = oRect[tc];
                oRectInner[tc].shrink(doorThickness);
                ++tc;
            }
        }

        float hf = 0.0f;
        float hf1 = -doorThickness;
        auto silhouette = Triangulator::execute3d(quad.points(), holes, hf, 0.000001f);

        sg.GB<GT::Poly>(silhouette, door, ReverseFlag::True, doorPivot);
        sg.GB<GT::Poly>(Triangulator::setZTriangles(silhouette, hf1), door, doorPivot);

        auto lineProfile = std::make_shared<Profile>(V2fc::Y_AXIS * hf, V2fc::Y_AXIS * hf1);
        sg.GB<GT::Follower>(lineProfile, quad.points(), FollowerFlags::WrapPath, door, doorPivot);

        auto pb = makeEnglishDoorProfile(V2f{ -doorThickness, doorThickness * 0.4f });

        for ( auto i = 0u; i < numPanels; i++ ) {
            sg.GB<GT::Follower>(pb, oRect[i].points3d(), FollowerFlags::WrapPath, V3fc::Z_AXIS * hf + doorPivot, door);
            sg.GB<GT::Poly>(oRectInner[i].points3dcw(), V3fc::Z_AXIS * hf1 + doorPivot, door);

            sg.GB<GT::Follower>(pb, oRect[i].points3d(), FollowerFlags::WrapPath,
                                V3fc::Z_AXIS * hf1 + doorPivot + V3fc::X_AXIS * _doorSize.x(), door,
                                GT::Rotate(Quaternion{ M_PI, V3fc::UP_AXIS }));
            sg.GB<GT::Poly>(oRectInner[i].points3dcw(), ReverseFlag::True, V3fc::Z_AXIS * hf + doorPivot, door);
        }
    }

//
//// Custom profiles
//
    std::shared_ptr<Profile>
    makeInnerDoorFrameProfile( float _depth, float doorGeomThickness, float doorTrim, const V2f& doorInnerBumpSize,
                               int dIndex ) {

        float thickness = _depth;
        float th = thickness * 0.5f;

        std::vector<Vector2f> points;
        // Counterclockwise
        points.emplace_back(0.0f, -th);
        points.emplace_back(-doorTrim, -th);
        points.emplace_back(-doorTrim, th - ( doorGeomThickness + doorInnerBumpSize.y() ));
        points.emplace_back(-( doorTrim + doorInnerBumpSize.x() ), th - ( doorGeomThickness + doorInnerBumpSize.y() ));
        points.emplace_back(-( doorTrim + doorInnerBumpSize.x() ), th - ( doorGeomThickness ));
        points.emplace_back(-doorTrim, th - ( doorGeomThickness ));
        points.emplace_back(-doorTrim, th);
        points.emplace_back(0.0f, th);

        std::string pName = "DoorProfile" + std::to_string(_depth);
        auto doorFrame_profile = std::make_shared<Profile>();
        doorFrame_profile->Name(pName);
        doorFrame_profile->createArbitrary(points);
        if ( isOdd(dIndex) ) {
            doorFrame_profile->flip(V2fc::Y_AXIS);
        }

        return doorFrame_profile;
    }

    void addDoorArchitrave( SceneGraph& sg, const GeomSP& mRootH, const DoorBSData *d, float direction ) {
        // Architraves
        auto fVerts = utilGenerateFlatRect(Vector2f(d->Width(), d->Height()),
                                           WindingOrder::CCW,
                                           PivotPointPosition::BottomCenter);

        if ( auto architrave_ovolo = sg.PL("architrave,ovolo"); architrave_ovolo ) {
            sg.GB<GT::Follower>(architrave_ovolo, fVerts, mRootH,
                                V3fc::Z_AXIS_NEG * ( -d->Depth() * direction * .5f ),
                                GT::Rotate(Quaternion{ M_PI, V3f{ 0.0f, direction < 0.0f ? -1.0f : 0.0f, 0.0f } }));
        }

    }


    void addInnerDoorFrame( SceneGraph& sg, const GeomSP& mRootH, const DoorBSData *d ) {
        auto doorProfile = makeInnerDoorFrameProfile(d->Depth(), d->doorGeomThickness, d->doorTrim, d->doorInnerBumpSize,
                                                     d->dIndex);
        sg.addProfileIM(doorProfile->Name(), *doorProfile);

        // Add inner frame
        auto fVerts2 = utilGenerateFlatRect(Vector2f(d->Width(), d->Height()),
                                            WindingOrder::CCW,
                                            PivotPointPosition::BottomCenter);

        sg.GB<GT::Follower>(sg.PL(doorProfile->Name()), fVerts2, mRootH, V3fc::ZERO);
    }

    void flatUglyDoor( SceneGraph& sg, const GeomSP& mRootH, const DoorBSData *d, const V3f& doorPivot,
                       float _doorThickness = 0.03f ) {

        sg.GB<GT::Extrude>(V2nff{ V2f{ d->Width(), _doorThickness }, V3fc::UP_AXIS, d->Height() }, mRootH,
                           V3f{ 0.0f, doorPivot.yz() });
    }

    std::vector<QuadVector3fNormal> createQuads( const V2f& wss ) {
        std::vector<QuadVector3fNormal> wallQuads;

        V3f v1 = XZY::C(-half(wss.x()), 0.0f, 0.0f);
        V3f v2 = XZY::C(half(wss.x()), 0.0f, 0.0f);
        V3f v3 = v1 + V3fc::UP_AXIS * wss.y();
        V3f v4 = v2 + V3fc::UP_AXIS * wss.y();
        auto quad = QuadVector3f{ { v1, v2, v4, v3 } };
        wallQuads.emplace_back(QuadVector3fNormal{ quad, XZY::C(V2fc::Y_AXIS, 0.0f) });
        return wallQuads;
    }


    auto addDoorGeom( SceneGraph& sg, const GeomSP& mRootH, const DoorBSData *d ) {
        auto child = EF::create<Geom>( "ActualDoor" );
        englishDoor(sg, child, d->doorSize, d->doorGeomPivot, d->doorGeomThickness);

        sg.GB<GT::Asset>("doorhandle,dx", child, d->doorHandlePivotLeft, GT::Rotate(d->doorHandleRot));
        sg.GB<GT::Asset>("doorhandle,sx", child, d->doorHandlePivotRight, GT::Rotate(d->doorHandleRot));

        float openAngle = d->isMainDoor || d->isDoorTypicallyShut ? 0.0f : d->openingAngleMax;
        Quaternion rot(openAngle, V3fc::UP_AXIS);

        return mRootH->addChildren(child, d->doorPivot, rot, V3fc::ONE);

//    flatUglyDoor( sg, mRootH, d, doorPivot );
    }

    void addFloorUnderDoor( SceneGraph& sg, const DoorBSData *d, const GeomSP& root ) {
        sg.GB<GT::Extrude>(V2nff{ V2f{ d->Width(), d->Depth() }, V3fc::UP_AXIS, 0.001f }, root,
                           V3fc::UP_AXIS * -0.001f);
    }

    GeomSP make3dGeometry( SceneGraph& sg, const GeomSP& eRootH, const DoorBSData *data ) {

        auto lRootH = EF::create<Geom>("Door"+ std::to_string(data->hash));

        addDoorArchitrave(sg, lRootH, data, -1.0f);
        addDoorArchitrave(sg, lRootH, data, 1.0f);

        // This is the frame plus the bump bit to stop the door on close
        addInnerDoorFrame(sg, lRootH, data);

        // This is the actual door, if it's a decent size
        if ( data->Width() > 0.45f && data->Width() < 1.3f ) {
            addDoorGeom(sg, lRootH, data);
        }

        // Add a bit of the missing floor between the rooms connecting this door
        addFloorUnderDoor(sg, data, lRootH);

        float vWangle = -atan2(-data->dirWidth.y(), data->dirWidth.x());
        Quaternion rot(vWangle + M_PI, V3fc::UP_AXIS);

        return eRootH->addChildren(lRootH, data->Position(), rot, V3fc::ONE);
    }

}
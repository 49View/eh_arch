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
#include "../models/door_service.hpp"

namespace DoorRender {

    bool isLeft( int index ) {
        return index >= 2;
    }

    void drawSingleDoor2d( Renderer& rr, const V2f& _p1, const V2f& _p2, float _lineWidth, const C4f& _color,
                           const RDSPreMult& _pm ) {
        float windowLineWidth = _lineWidth * 0.2f;
        float halfWindowLineWidth = windowLineWidth * 0.5f;
        float halfLineWidth = _lineWidth * 0.5f;
        float windowLineWidthOffset = halfLineWidth - halfWindowLineWidth;
        float lineWidth = 0.0025f;

        float dist = distance(_p1, _p2) + windowLineWidth;
        V2f vn = normalize(_p1 - _p2);
        auto slope = rotate90(vn);
        auto p1 = _p1 + ( slope * windowLineWidthOffset ) + vn * halfWindowLineWidth;
        auto p2 = _p2 + ( slope * windowLineWidthOffset ) + vn * -halfWindowLineWidth;
        auto p3 = p1 + ( slope * dist );

        std::vector<V2f> vLists;
        vLists.emplace_back(p2);

        static const int NSLOPES = 15;
        float deltaInc = 1.0f / static_cast<float>(NSLOPES - 1);
        float delta = deltaInc;
        for ( int q = 0; q < NSLOPES - 1; q++ ) {
            auto npStraightLerp = JMATH::lerp(delta, p2, p3);
            auto npDir = normalize(npStraightLerp - p1);
            vLists.emplace_back(p1 + npDir * dist);
            delta += deltaInc;
        }

        vLists.emplace_back(p1);
        rr.draw<DLine2d>(vLists, _color, lineWidth, false, _pm);
    }

    void drawDoubleDoor2d();

    void make2dGeometry( Renderer& rr, SceneGraph& sg, const DoorBSData *data, Use2dDebugRendering bDrawDebug,
                         const RDSPreMult& _pm ) {
        auto color = bDrawDebug == Use2dDebugRendering::True ? C4f::PASTEL_GREEN : C4f::BLACK;
        drawSingleDoor2d(rr, data->us1.middle, data->us2.middle, data->us2.width, color, _pm);
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
                      float doorThinkness,
                      const std::array<Vector2f, Ty>& _pratios = { Vector2f{ 0.288f, 0.314f },
                                                                   Vector2f{ 0.288f, 0.314f },
                                                                   Vector2f{ 0.288f, 0.111f } },
                      const std::array<float, Ty>& ygapRatios = { 0.071f, 0.091f, 0.045f } ) {

        std::vector<Vector2fList> holes;
        Rect2f quad{ V2f{ 0.0f, 0.0f }, _doorSize };

        static const size_t numPanels = Tx * Ty;
        Vector2fList hole;
        std::array<Vector2f, Ty> dsizes{};
        for ( size_t i = 0; i < Ty; i++ ) {
            dsizes[i] = { _pratios[i].x() * _doorSize.x(), _pratios[i].y() * _doorSize.y() };
        }
        float xgapDelta = ( quad.width() - ( dsizes[0].x() * Tx ) ) / ( Tx + 1 );
        std::vector<float> ygaps;
        ygaps.reserve(ygapRatios.size());
        for ( const auto& yg : ygapRatios ) {
            ygaps.emplace_back(yg * _doorSize.y());
        }

        std::array<Vector2f, numPanels> o{};
        std::array<Rect2f, numPanels> orect{};
        std::array<Rect2f, numPanels> orectinner{};
        size_t tc = 0;
        for ( size_t ty = 0; ty < Ty; ty++ ) {
            for ( size_t tx = 0; tx < Tx; tx++ ) {
                float ppy = ty == 0 ? 0.0f : dsizes[ty - 1].y() + ygaps[ty];
                float currY = tc == 0 ? ygaps[0] : o[tc - 1].y() + ( tx == 0 ? ppy : 0.0f );

                o[tc] = { xgapDelta + ( tx * ( dsizes[ty].x() + xgapDelta ) ), currY };

                orect[tc] = Rect2f{ o[tc], o[tc] + dsizes[ty] };

                holes.emplace_back(orect[tc].points());
                orectinner[tc] = orect[tc];
                orectinner[tc].shrink(doorThinkness);
                ++tc;
            }
        }

        float hf = 0.0f;
        float hf1 = -doorThinkness;
        auto silhouette = Triangulator::execute3d(quad.points(), holes, hf, 0.000001f);

        sg.GB<GT::Poly>(silhouette, door, ReverseFlag::True, doorPivot);
        sg.GB<GT::Poly>(Triangulator::setZTriangles(silhouette, hf1), door, doorPivot);

        auto lineProfile = std::make_shared<Profile>(Vector2f::Y_AXIS * hf, Vector2f::Y_AXIS * hf1);
        sg.GB<GT::Follower>(lineProfile, quad.points(), FollowerFlags::WrapPath, door, doorPivot);

        auto pb = makeEnglishDoorProfile(V2f{ -doorThinkness, doorThinkness * 0.4f });

        for ( auto i = 0u; i < numPanels; i++ ) {
            sg.GB<GT::Follower>(pb, orect[i].points3d(), FollowerFlags::WrapPath, V3f::Z_AXIS * hf + doorPivot, door);
            sg.GB<GT::Poly>(orectinner[i].points3dcw(), V3f::Z_AXIS * hf1 + doorPivot, door);

            sg.GB<GT::Follower>(pb, orect[i].points3d(), FollowerFlags::WrapPath,
                                V3f::Z_AXIS * hf1 + doorPivot + V3f::X_AXIS * _doorSize.x(), door,
                                GT::Rotate(Quaternion{ M_PI, V3f::UP_AXIS }));
            sg.GB<GT::Poly>(orectinner[i].points3dcw(), ReverseFlag::True, V3f::Z_AXIS * hf + doorPivot, door);
        }
    }

//
//// Custom profiles
//
    std::shared_ptr<Profile> makeInnerDoorFrameProfile( float _depth, float doorGeomThickness, float doorTrim, const V2f& doorInnerBumpSize, int dIndex ) {

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

        std::string pname = "DoorProfile" + std::to_string(_depth);
        auto doorFrame_profile = std::make_shared<Profile>();
        doorFrame_profile->Name(pname);
        doorFrame_profile->createArbitrary(points);
        if ( isOdd(dIndex) ) {
            doorFrame_profile->flip(V2f::Y_AXIS);
        }

        return doorFrame_profile;
    }

    void addDoorArchitrave( SceneGraph& sg, GeomSP mRootH, const DoorBSData *mData, float direction ) {
        // Architraves
        auto fverts = utilGenerateFlatRect(Vector2f(mData->width, mData->height),
                                           WindingOrder::CCW,
                                           PivotPointPosition::BottomCenter);

        if ( auto architrave_ovolo = sg.PL("architrave,ovolo"); architrave_ovolo ) {
            sg.GB<GT::Follower>(architrave_ovolo, fverts, mRootH,
                                Vector3f::Z_AXIS_NEG * ( -mData->depth * direction * .5f ),
                                GT::Rotate(Quaternion{ M_PI, V3f{ 0.0f, direction < 0.0f ? -1.0f : 0.0f, 0.0f } }));
        }

    }


    void addInnerDoorFrame( SceneGraph& sg, GeomSP mRootH, const DoorBSData *mData, float doorTrim, const V2f& doorInnerBumpSize, int dIndex ) {
        auto doorProfile = makeInnerDoorFrameProfile(mData->depth, mData->doorGeomThickness, doorTrim, doorInnerBumpSize, dIndex);
        sg.addProfileIM(doorProfile->Name(), *doorProfile);

        // Add inner frame
        auto fverts2 = utilGenerateFlatRect(Vector2f(mData->width, mData->height),
                                            WindingOrder::CCW,
                                            PivotPointPosition::BottomCenter);

        sg.GB<GT::Follower>(sg.PL(doorProfile->Name()), fverts2, mRootH, V3f::ZERO);
    }

    void flatUglyDoor( SceneGraph& sg, GeomSP mRootH, const DoorBSData *mData, const V3f& doorPivot,
                       float _doorThinkness = 0.03f ) {

        sg.GB<GT::Extrude>(V2nff{ V2f{ mData->width, _doorThinkness }, V3f::UP_AXIS, mData->height }, mRootH,
                           V3f{ 0.0f, doorPivot.yz() });
    }

    auto addDoorGeom( SceneGraph& sg, GeomSP mRootH, const DoorBSData *mData, const V3f& doorPivot, int dIndex,
                      float doorTrim,
                      const V3f& _doorHandlePivotLeft, const V3f& _doorHandlePivotRight,
                      const Quaternion& _doorHandleRot ) {
        auto child = mRootH->addChildren("ActualDoor");
        float maxOpeningAngle = isOdd(dIndex) ? -M_PI_2 : M_PI_2;
        maxOpeningAngle *= isLeft(dIndex) ? -1.0f : 1.0f;
        float depthGap = isOdd(dIndex) ? mData->doorGeomThickness : 0.0f;
        float gapBetweenDoorEdgeAndHinges = 0.003f;
        float totalTrim = ( doorTrim * 2.0f + gapBetweenDoorEdgeAndHinges * 2.0f );
        float xPivot = isLeft(dIndex) ? -mData->width + doorTrim * 2.0f + gapBetweenDoorEdgeAndHinges
                                      : gapBetweenDoorEdgeAndHinges;
        // This 2.5f here is to leave space for the frame trim, the inner frame trim, and the 0.5 is a bit of air between it.
        V2f doorSize{ mData->width - totalTrim, mData->height - doorTrim * 2.5f };
        englishDoor(sg, child, doorSize, { xPivot, 0.0f, depthGap }, mData->doorGeomThickness);

        sg.GB<GT::Asset>("doorhandle,dx", child, _doorHandlePivotLeft, GT::Rotate(_doorHandleRot));
        sg.GB<GT::Asset>("doorhandle,sx", child, _doorHandlePivotRight, GT::Rotate(_doorHandleRot));

        float openAngle = mData->isMainDoor ? 0.0f : maxOpeningAngle;
        Quaternion rot(openAngle, V3f::UP_AXIS);
        child->updateTransform(doorPivot, rot, V3f::ONE);

        return child;
//    flatUglyDoor( sg, mRootH, mData, doorPivot );
    }

    void addFloorUnderDoor( SceneGraph& sg, const DoorBSData* d, GeomSP root ) {
        sg.GB<GT::Extrude>(V2nff{ V2f{ d->width, d->depth }, V3f::UP_AXIS, 0.001f }, root,
                           V3f::UP_AXIS * -0.001f);
    }

    GeomSPContainer make3dGeometry( SceneGraph& sg, const DoorBSData *data ) {

        int dIndex = 3;
        auto rootH = EF::create<Geom>("Door");

        // Internal door data
        Vector2f doorInnerBumpSize = Vector2f(0.020f, 0.03f);
        float doorTrim = 0.01f;
        float realDoorWidth = data->width - doorTrim * 2.0f;
        float doorGeomPivot = 0.0f;
        Vector3f hingesPivot = Vector3f::ZERO;
        Vector3f doorHandlePivotLeft = Vector3f::ZERO;
        Vector3f doorHandlePivotRight = Vector3f::ZERO;
        float doorHandleAngle = 0.0f;
        Vector3f frameHingesPivot = Vector3f::ZERO;
        Vector3f doorHandlePlateDoorSidePivot = Vector3f::ZERO;
        Vector3f doorHandlePlateFrameSidePivot = Vector3f::ZERO;
        V3f doorPivot = V3f::ZERO;
        Quaternion doorHandleRot{};

        DoorService::calculatePivots(data, dIndex, realDoorWidth, doorTrim, hingesPivot, frameHingesPivot,
                                     doorHandlePivotLeft, doorHandlePivotRight, doorHandleRot,
                                     doorHandleAngle, doorGeomPivot, doorPivot, doorHandlePlateDoorSidePivot,
                                     doorHandlePlateFrameSidePivot);

        addDoorArchitrave(sg, rootH, data, -1.0f);
        addDoorArchitrave(sg, rootH, data, 1.0f);

        // This is the frame plus the bump bit to stop the door on close
        addInnerDoorFrame(sg, rootH, data, doorTrim, doorInnerBumpSize, dIndex);

        // This is the actual door
        addDoorGeom(sg, rootH, data, doorPivot, dIndex, doorTrim, doorHandlePivotLeft, doorHandlePivotRight, doorHandleRot);

        // Add a bit of the missing floor between the rooms connecting this door
        addFloorUnderDoor( sg, data, rootH);

        float vwangle = -atan2(-data->dirWidth.y(), data->dirWidth.x());
        Quaternion rot(vwangle + M_PI, V3f::UP_AXIS);
        rootH->updateTransform(XZY::C(data->center, 0.0f), rot, V3f::ONE);

        GeomSPContainer ret;
        ret.emplace_back(rootH);
        sg.addNode(rootH);
        return ret;
    }

}
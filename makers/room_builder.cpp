//
// Created by Dado on 2019-04-25.
//

#include "room_builder.hpp"
#include <core/math/plane3f.h>
#include <core/raw_image.h>
#include <core/TTF.h>
#include <core/camera.h>
#include <core/file_manager.h>
#include <core/resources/profile.hpp>
#include <core/resources/resource_builder.hpp>
#include <core/math/vector_util.hpp>
#include <poly/scene_events.h>
#include <graphics/renderer.h>
#include <graphics/lightmap_manager.hpp>
#include <render_scene_graph/render_orchestrator.h>
#include <render_scene_graph/ui_view.hpp>
#include <poly/converters/gltf2/gltf2.h>
#include <core/math/poly_utils.hpp>

#include "room_builder_segment_points.hpp"
#include <eh_arch/models/floor_service.hpp>


//std::string prebakedSegments{"{\"plist\":[[4.005617141723633,0.0,-3.010443925857544],[4.005617141723633,0.0,-1.1591655015945435],[4.005617141723633,0.0,-1.0091655254364014],[4.005617141723633,0.0,0.8410576581954956],[4.005617141723633,0.0,0.9910577535629273],[4.005617141723633,0.0,3.9899609088897707],[-0.4963325560092926,0.0,3.9899609088897707],[-0.4963325560092926,0.0,-0.5120362043380737],[-1.9985274076461793,0.0,-0.5120362043380737],[-1.9985274076461793,0.0,-1.0091655254364014],[-1.9985274076461793,0.0,-1.1591655015945435],[-1.9985274076461793,0.0,-1.8779958486557007],[-1.9985274076461793,0.0,-2.0279958248138429],[-1.9985274076461793,0.0,-3.010443925857544],[4.005617141723633,0.0,-3.010443925857544]],\"ptypes\":[2,2,256,2,2,2,2,2,2,2,512,2,2,2,2]}"};

void RoomBuilder::activate() {
    onEntry();
}

void RoomBuilder::activateDebug() {
    sg.load<Font>( "amaranth" );
    rsg.DC()->setPosition(V3f{ 0.0f, 1.5f, 5.0f});
    sg.GB<GT::Shape>( ShapeType::Cube, GT::Scale( 5.f, 0.01f, 5.f ) );
    sg.GB<GT::Shape>( ShapeType::Cube, V3f::UP_AXIS );
}

void RoomBuilder::clear( const UICallbackHandle& _ch ) {
    segments.clear();
    cachedSegments.clear();
    finalised = false;
    currentPointValid = false;

    inputPoint = V3f::ZERO;
    hasBeenSnapped = false;
    activeSegmentType = ArchType::WallT;

    setUIStatusAfterChange();
    refresh();
}

void RoomBuilder::undo( const UICallbackHandle& _ch ) {
    if ( !segments.empty() ) {
        segments.pop_back();
        if ( segments.size() == 1 ) {
            segments.pop_back();
        }
    }
    bool wasFinalised = finalised;
    finalised = false;
    currentPointValid = false;
    setUIStatusAfterChange( wasFinalised );
    rsg.RR().clearBucket( furnitureBucket );

    refresh();
}

void RoomBuilder::setSegmentTypeFromIndex( const UICallbackHandle& _ch ) {
    if ( _ch.index == -1 ) return;

    switch (_ch.index ) {
        case 0:
            setSegmentType( ArchType::WallT );
            break;
        case 1:
            setSegmentType( ArchType::WindowT );
            break;
        case 2:
            setSegmentType( ArchType::DoorT );
            break;
        default:
            setSegmentType( ArchType::WallT );
    }
}

void RoomBuilder::loadSegments( const SerializableContainer& _segs ) {
    segments = RoomBuilderSegmentPoints{_segs};
}

void RoomBuilder::saveSegments( const UICallbackHandle& _ch ) {
//    FM::writeRemoteEntity(S::makeImaginary(), "room_layout", cachedSegments );
    Http::post( Url{ HttpFilePrefix::entities + "house/" + S::makeImaginary()}, house->serialize() );
}

void RoomBuilder::saveCachedSegments() {
    cachedSegments = segments.serialize();
    auto shash = std::hash<std::string>{}(segments.serializeString());
    FM::writeLocalFile("./bespoke_segments"+std::to_string(shash), cachedSegments);
}

void RoomBuilder::addPointToRoom() {
    if ( finalised ) return;
    if ( activeSegmentType == ArchType::WindowT || activeSegmentType == ArchType::DoorT ) {
        float twoShapeLength = distance(inputPoint, segments.back());
        float extraWallBitLength = wallWidth;
        auto twoShapeSlope = normalize( inputPoint - segments.back() );
        auto extraPointStart = segments.back() + (twoShapeSlope * extraWallBitLength);
        auto adjustedTwoShapeInputPoint = segments.back() + (twoShapeSlope * (twoShapeLength-extraWallBitLength));
        segments.add( {extraPointStart, activeSegmentType} );
        segments.add( { adjustedTwoShapeInputPoint, ArchType::WallT } );
        segments.add( { inputPoint, ArchType::WallT } );
    } else {
        segments.add( { inputPoint, activeSegmentType } );
    }
//    segments.optimize();
//    setBestFittingSegmentTypeAfterSegmentCompletion();
    segments.area();
    refresh();
    rsg.RR().removeFromCL( "SnapLongLines1" );
    rsg.RR().removeFromCL( "SnapLongLines2" );
    if ( segments.size() == 1 ) setUIStatusAfterChange();
}

bool RoomBuilder::validateAddPoint( const V2f& _p ) {
    if ( finalised || segments.empty() ) return false;
    setInputPoint( _p );
    if ( checkSegmentLongEnough() && !checkPointIntersect( inputPoint ) ) {
        addPointToRoom();
        return true;
    }
    return false;
}

bool RoomBuilder::checkSegmentLongEnough() const {
    if ( segments.empty() ) return true;
    float twoShapeLength = distance( inputPoint, segments.back());
    if ( activeSegmentType == ArchType::WallT ) {
        return !isScalarEqual( twoShapeLength, 0.0f, 0.01f );
    }
    if ( activeSegmentType == ArchType::WindowT || activeSegmentType == ArchType::DoorT ) {
        return twoShapeLength > wallWidth*2.0f;
    }
    return true;
}

bool RoomBuilder::checkPointIntersect( const V3f& _p ) const {
    if ( segments.size() < 2 ) return false;
    V2f vi = V2fc::ZERO;
    // Interesect all but last segment (size-2) to avoid artifacts of colliding with start/end of two connected segments
    for ( size_t t = 0; t < segments.size()-2; t++ ) {
        if ( intersection( _p.xz(), segments.back().xz(), segments[t].xz(), segments[t+1].xz(), vi ) ) {
            // Check if we are effectively colliding because we are closing the path, in this case it's a false positive
            if ( XZY::C(vi) != segments.front() && !( vi == V2fc::ZERO && _p == segments.front()) )
                return true;
        }
    }
    // It doesnt intersect anything up to here but _p can still lie within the last segment, so just check that
    return pointWithinSegment( _p.xz(), segments.back().xz(), segments[segments.size() - 2].xz() );
}

bool RoomBuilder::checkPointWillClosePerimeter() const {
    return ( (segments.size() > 2) && (inputPoint == segments.front()) );
}

void RoomBuilder::setUIStatusAfterChange( bool wasFinalised ) {
    if ( segments.empty() ) {
        rsg.UI().setButtonStatus( "1", UIElementStatus::Enabled );
        rsg.UI().setButtonStatus( "1", UIElementStatus::Selected );
        rsg.UI().setButtonStatus( "2", UIElementStatus::Disabled );
        rsg.UI().setButtonStatus( "3", UIElementStatus::Disabled );
        setSegmentType(ArchType::WallT);
    }
    if ( segments.size() == 1 ) {
        rsg.UI().setButtonStatus( "2", UIElementStatus::Enabled );
        rsg.UI().setButtonStatus( "3", UIElementStatus::Enabled );
    }
    rsg.UI().setButtonStatus( "6", finalised ? UIElementStatus::Enabled : UIElementStatus::Disabled );
    rsg.UI().setButtonStatus( "NewRoom", segments.empty() ? UIElementStatus::Disabled : UIElementStatus::Enabled );
    rsg.UI().setButtonStatus( "Undo", segments.empty() ? UIElementStatus::Disabled : UIElementStatus::Enabled );
    rsg.UI().setButtonStatus( "Save", finalised ? UIElementStatus::Enabled : UIElementStatus::Disabled );
    rsg.UI().setButtonStatus( "4", finalised ? UIElementStatus::Enabled : UIElementStatus::Disabled );

    if ( finalised ) {
        rsg.UI().setButtonStatus( "1", UIElementStatus::Disabled );
        rsg.UI().setButtonStatus( "2", UIElementStatus::Disabled );
        rsg.UI().setButtonStatus( "3", UIElementStatus::Disabled );
    }
    if ( wasFinalised ) {
        rsg.UI().setButtonStatus( "1", UIElementStatus::Enabled );
        rsg.UI().setButtonStatus( "1", UIElementStatus::Selected );
        rsg.UI().setButtonStatus( "2", UIElementStatus::Enabled );
        rsg.UI().setButtonStatus( "3", UIElementStatus::Enabled );
    }
}

V3f RoomBuilder::snapper( const V3f& _p ) {
    auto ret = snapTo( _p, segments.points(), snapThreshold );
    hasBeenSnapped = ret != _p;
    return ret;
}

V3f RoomBuilder::convert2dPosTo3d( const V2f& _p ) {
    auto pd = rsg.rayViewportPickIntersection( _p );
    return snapper(Plane3f{ Vector3f::Y_AXIS, 0.0f }.intersectLineGrace( pd.rayNear, pd.rayFar ));
}

void RoomBuilder::setCurrentPointerPos( const V2f& _p ) {
    if ( finalised ) return;
    currentPoint = convert2dPosTo3d( _p );
    currentPointValid = true;
    if ( segments.empty() ) {
        inputPoint = currentPoint;
        addPointToRoom();
    }
    refresh();
}

void RoomBuilder::setInputPoint( const V2f& _p ) {
    if ( finalised ) return;
    inputPoint = convert2dPosTo3d( _p );
    currentPoint = inputPoint;
    currentPointValid = true;
}

void RoomBuilder::drawWindow( int _bucketList, const V3f& _p1, const V3f& _p2, float _lineWidth, const C4f& _color ) {
    Renderer& rr = rsg.RR();
    float windowLineWidth = _lineWidth*0.2f;
    float halfWindowLineWidth = windowLineWidth * 0.5f;
    float halfLineWidth = _lineWidth*0.5f;
    float windowLineWidthOffset = halfLineWidth - halfWindowLineWidth;
    rr.draw<DLine>( _bucketList, _p1, _p2, _color, windowLineWidth, false );

    V2f vn = normalize( _p1.xz() - _p2.xz() );
    auto slope = XZY::C( rotate90(vn) );
    auto p1 = _p1 + (slope * windowLineWidthOffset);
    auto p2 = _p2 + (slope * windowLineWidthOffset);
    rr.draw<DLine>( _bucketList, p1, p2, _color, windowLineWidth, false );
    auto p3 = _p1 + (slope * -windowLineWidthOffset);
    auto p4 = _p2 + (slope * -windowLineWidthOffset);
    rr.draw<DLine>( _bucketList, p3, p4, _color, windowLineWidth, false );
}

void RoomBuilder::drawSingleDoor2d( int _bucketList, const V3f& _p1, const V3f& _p2, float _lineWidth, const C4f& _color ) {
    Renderer& rr = rsg.RR();
    float windowLineWidth = _lineWidth*0.2f;
    float halfWindowLineWidth = windowLineWidth * 0.5f;
    float halfLineWidth = _lineWidth*0.5f;
    float windowLineWidthOffset = halfLineWidth - halfWindowLineWidth;

    float dist = distance(_p1.xz(), _p2.xz()) + windowLineWidth;
    V2f vn = normalize( _p1.xz() - _p2.xz() );
    auto slope = XZY::C( rotate90(vn) );
    auto p1 = _p1 + (slope * windowLineWidthOffset) + vn*halfWindowLineWidth;
    auto p2 = _p2 + (slope * windowLineWidthOffset) + vn*-halfWindowLineWidth;
    auto p3 =  p1 + (slope * dist);

    std::vector<Vector3f> vLists;
    vLists.emplace_back( p2 );

    static const int NSLOPES = 15;
    float deltaInc = 1.0f / static_cast<float>(NSLOPES-1);
    float delta = deltaInc;
    for ( int q = 0; q < NSLOPES-1; q++ ) {
        auto npStraightLerp = JMATH::lerp( delta, p2, p3 );
        auto npDir = normalize( npStraightLerp - p1 );
        vLists.emplace_back( p1 + npDir * dist );
        delta += deltaInc;
    }

    vLists.emplace_back( p1 );
    rr.draw<DLine>( _bucketList, vLists, _color, windowLineWidth, false );
}

struct FloorplanDrawOuterMeasure {
    V3f p1 = V3f::ZERO;
    V3f p2 = V3f::ZERO;
    V2f normal = V2fc::ZERO;
};

using FloorplanDrawOuterMeasureLayers = std::array<std::vector<std::pair<float, float>>, 4>;

auto clampToFloorplanOuterSizes( int i, const V3f& op1, const V3f& op2, const JMATH::Rect2f& bb,
                                 FloorplanDrawOuterMeasureLayers& tierLayers, float tierGap ) {
    FloorplanDrawOuterMeasure ret{};

    auto tierLayer = tierLayers.back();
    int i2 = i*2;
    auto rp1 = op1;
    auto rp2 = op2;
    int io = i2;
    float tsign = 1.0f;
    if ( ( op1[i2] < bb.centre()[i] && op2[i2] < bb.centre()[i] ) ||
         ( distance(op1[i2], bb[i2]) < distance(op2[i2], bb[i2+1]) ) ) {
        io = i2;
        tsign = -1.0f;
    } else {
        io = i2+1;
        tsign = 1.0f;
    }

    auto& tier = tierLayers[io];
    float tierOff = 1.0f;
    int ii = i2 == 0 ? 2 : 0;
    std::pair<float,float> oRange;
    if ( op1[ii] > op2[ii] ) {
        oRange = { op2[ii], op1[ii] };
    } else {
        oRange = { op1[ii], op2[ii] };
    }

    for ( const auto& level : tier ) {
        if ( isOverlapping( oRange, level ) ) {
            tierOff+=1.0f;
        }
    }

    rp1[i2] = ( bb[io] + ( tierGap * tierOff * tsign) );
    rp2[i2] = ( bb[io] + ( tierGap * tierOff * tsign) );
    tier.emplace_back( oRange.first, oRange.second );

    ret.p1 = rp1;
    ret.p2 = rp2;
    ret.normal = i == 0 ? V2fc::Y_AXIS_NEG : V2fc::X_AXIS;

    return ret;
}

void RoomBuilder::drawFloorplanSegments() {
    Renderer& rr = rsg.RR();
    auto wallColor = C4f::WHITE;

    int segC = 0;
    for ( const auto& pwall : segments.pointsOf(ArchType::WallT) ) {
        auto pext = pwall;
        if ( pext.strip.size() == 1 ) {
//            rr.drawCircle( roomEditBucket, segments.back(), wallWidth*0.5f, currLineColor, 12, "RoomBuilderLastTailCircle" );
        } else {
            rr.draw<DLine>( roomEditBucket, pext.strip, wallColor, wallWidth, false, std::to_string(segC++) );
        }
    }
    for ( const auto& elemList : segments.pointsOf(ArchType::WindowT) ) {
        for ( size_t q = 0; q < elemList.strip.size()-1; q++ ) {
            // We do windows and doors pair by pair
            drawWindow( roomEditBucket, elemList.strip[q], elemList.strip[q+1], wallWidth, wallColor );
        }
    }
    for ( const auto& elemList : segments.pointsOf(ArchType::DoorT) ) {
        for ( size_t q = 0; q < elemList.strip.size()-1; q++ ) {
            // We do windows and doors pair by pair
            drawSingleDoor2d( roomEditBucket, elemList.strip[q], elemList.strip[q+1], wallWidth, wallColor );
        }
    }

    auto font = sg.FM().get(S::DEFAULT_FONT);
    float tierGap = wallWidth * 2.5f;
    auto bb = segments.BBox();

    FloorplanDrawOuterMeasureLayers tierLayers;

    for ( size_t t = 0; t < segments.size()-1; t++ ) {
        auto op1 = segments.points()[t];
        auto op2 = segments.points()[t+1];
        for ( int i = 0; i < 2; i ++ ) {
            auto afx = clampToFloorplanOuterSizes( i, op1, op2, bb, tierLayers, tierGap);
            auto name = afx.p1.toString() + afx.p2.toString();
            rr.drawMeasurementArrow2( roomEditBucket, afx.p1, afx.p2, afx.normal, op1, op2,
                                      C4f::WHITE.A(0.65f), wallWidth*0.1f,
                                      M_PI_4*0.5f, 0.08f, wallWidth*2.0f, font.get(),
                                      wallWidth*1.5f, C4f::WHITE, C4f::BLACK, name );
        }
    }

//    rr.drawRect( roomEditBucket, segments.BBox(), wallColor );
}

void RoomBuilder::refresh() {
    Renderer& rr = rsg.RR();
    rr.clearBucket( roomEditBucket );
    if ( segments.empty() ) return;

    auto font = sg.FM().get(S::DEFAULT_FONT);
    auto wallColor = C4f::WHITE;
    auto textColor = (C4f::WHITE*0.95f).A(1.0f);

    bool intersecting = ((checkPointIntersect( currentPoint ) && segments.back() != currentPoint)) && currentPointValid;
    Color4f currLineColor = intersecting ? Color4f::INDIAN_RED : wallColor.A(1.0f);
    if ( (hasBeenSnapped || currentPoint == segments.front()) && !intersecting ) {
        currLineColor = C4f::DARK_YELLOW;
    }
    if ( (hasBeenSnapped && currentPoint == segments.front()) && !intersecting ) {
        currLineColor = C4f::SPRING_GREEN;
    }

    drawFloorplanSegments();

    float cursorToLastPointDistace = 0.0f;
    if ( !segments.empty() ) cursorToLastPointDistace = JMATH::distance( segments.back(), currentPoint );

    if ( currentPoint != segments.back() && cursorToLastPointDistace > wallWidth && currentPointValid ) {

        switch (activeSegmentType) {
            case ArchType::WallT:
                rr.draw<DLine>( roomEditBucket, currentPoint, segments.back(), currLineColor, wallWidth, "LineSegment"+currLineColor.toString() );
                rr.draw<DCircleFilled>( roomEditBucket, segments.back(), wallWidth*0.5f, currLineColor, "RoomBuilderLastTailCircle"+currLineColor.toString() );
                rr.draw<DCircleFilled>( roomEditBucket, currentPoint, wallWidth*0.5f, currLineColor, "RoomBuilderLastTailCircle"+currLineColor.toString() );
                break;
            case ArchType::WindowT:
                drawWindow( roomEditBucket, segments.back(), currentPoint, wallWidth, currLineColor );
                break;
            case ArchType::DoorT:
                drawSingleDoor2d( roomEditBucket, segments.back(), currentPoint, wallWidth, currLineColor );
                break;
        }

        C4f bgText = V4f{V4f::WHITE-textColor};
        rr.drawMeasurementArrow1( roomEditBucket, currentPoint, segments.back(), textColor, wallWidth*0.1f,
                                  M_PI_4, 0.1f, wallWidth*2.0f, font.get(),
                                  wallWidth*1.5f, C4f::XTORGBA("#00FEFF"), bgText, "currentMeasurementArrow" );
    }

    if ( hasBeenSnapped ) {
        constexpr float SnapLongLines = 15.0f;
        auto xt = V3f{ currentPoint.x(), 0.0f, SnapLongLines};
        auto xb = V3f{ currentPoint.x(), 0.0f, -SnapLongLines};
        auto slopeLineColor = Color4f::BLACK;
        auto snapLineWidth = wallWidth*0.066f;
        rr.draw<DLine>( roomEditBucket, xt, xb, slopeLineColor, snapLineWidth, "SnapLongLines1" );
        xt = V3f{ SnapLongLines, 0.0f, currentPoint.z()  };
        xb = V3f{ -SnapLongLines, 0.0f, currentPoint.z() };
        rr.draw<DLine>( roomEditBucket, xt, xb, slopeLineColor, snapLineWidth, "SnapLongLines2" );
    }
}

ArchHouseBespokeData RoomBuilder::bespokerise() {
    ArchHouseBespokeData bespoker;
    bespoker.wallWidthHint = wallWidth;
    SegmentStripVector2d ewp;
    V2fVectorOfVector pwallLine;
    V3fVectorOfVector pwallLine3d;

    auto optSegments = segments;
    optSegments.optimize();
    optSegments.finalise();

    int numc = 0;
    for ( const auto& pwall : optSegments.pointsOf(ArchType::WallT) ) {
        auto epts = extrudePointsWithWidth<ExtrudeComtour>( pwall.strip, wallWidth, false );
        LOGRS( "Count " << numc );
        for ( const auto np : epts ) LOGRS( np );
        numc++;

        V3fVector v1{ epts.begin(), epts.begin() + (epts.size() / 2 ) };
        V3fVector v2{ epts.begin() + (epts.size() / 2 ), epts.begin() + epts.size()  };

//        pwallLine.emplace_back( XZY::C2(epts) );
        pwallLine.emplace_back( XZY::C2(v1) );
        pwallLine.emplace_back( XZY::C2(v2) );
    }

    const auto& pwall = optSegments.points();
    auto eptstotal = extrudePointsWithWidth<ExtrudeComtour>( pwall, wallWidth, false );
    V3fVector v1{ eptstotal.begin(), eptstotal.begin() + (eptstotal.size() / 2 ) };
    V3fVector v2{ eptstotal.begin() + (eptstotal.size() / 2 ), eptstotal.begin() + eptstotal.size() };
    float a1 = PolyServices::areaOf( XZY::C2(v1) );
    float a2 = PolyServices::areaOf( XZY::C2(v2) );
    LOGRS("Area 1 Purple : " << a1 );
    LOGRS("Area 2 Green  : " << a2 );
//    rsg.RR().draw<DLine>( roomEditBucket, v1, C4f::PURPLE, "urca", false, 0.05f );
//    rsg.RR().draw<DLine>( roomEditBucket, v2, C4f::GREEN, "urca", false, 0.05f );

    bespoker.bsInnerPerimeter  = a1 < a2 ? v1 : v2;
    bespoker.bsOuterPerimeter  = a1 < a2 ? v2 : v1;
    bespoker.bsMiddlePerimeter = eptstotal;
//        auto epts2d = XZY::C2(epts);
//        pwallLine.emplace_back( epts2d );
//        pwallLine3d.emplace_back( pwall );
////        ewp.emplace_back( SegmentStrip2d { epts2d, pwall.type} );

    for ( const auto& dws : optSegments.pairOf<V3f>( ArchType::DoorT ) ) {
        bespoker.twoShapePoints.emplace_back( UshapeBespokeSet{ArchType::DoorT, XZY::C2(dws.first), XZY::C2(dws.second), wallWidth} );
    }
    for ( const auto& dws : optSegments.pairOf<V3f>( ArchType::WindowT ) ) {
        bespoker.twoShapePoints.emplace_back( UshapeBespokeSet{ArchType::WindowT, XZY::C2(dws.first), XZY::C2(dws.second), wallWidth} );
    }
    bespoker.floorPoints.emplace_back(pwallLine);

//    for ( const auto& wl : bespoker.floorPoints ) {
//        for ( const auto ws : wl ) {
//            auto col = C4f::RANDA1();
//            rsg.RR().draw<DLine>( roomEditBucket, XZY::C(ws), col, 0.05f );
//        }
//    }

    return bespoker;
}

V2fVectorOfVector RoomBuilder::bespokeriseWalls( float scaleFactor ) {
    ArchHouseBespokeData bespoker;
    bespoker.wallWidthHint = wallWidth;
    SegmentStripVector2d ewp;
    V2fVectorOfVector pwallLine;

    saveCachedSegments();
    auto optSegments = segments;
    optSegments.optimize();
//    optSegments.finalise();

    int numc = 0;
    for ( const auto& pwall : optSegments.pointsOf(ArchType::WallT) ) {
        auto epts = extrudePointsWithWidth<ExtrudeComtour>( pwall.strip, wallWidth, false );
        LOGRS( "Count " << numc );
        for ( auto& np : epts ) {
            np *= scaleFactor;
            LOGRS( np );
        }
        numc++;

//        V3fVector v1{ epts.begin(), epts.begin() + (epts.size() / 2 ) };
//        V3fVector v2{ epts.begin() + (epts.size() / 2 ), epts.begin() + epts.size()  };

//        V3fVector v1{ epts.begin(), epts.begin() + (epts.size() / 2 ) };
//        V3fVector v2{ epts.begin() + (epts.size() / 2 ), epts.begin() + epts.size()  };

        pwallLine.emplace_back( XZY::C2(epts) );
//        pwallLine.emplace_back( XZY::C2(v1) );
//        pwallLine.emplace_back( XZY::C2(v2) );
    }

    return pwallLine;
}

void RoomBuilder::roomPreBakedFurnitureSetup( FurnitureRuleScript& ruleScript ) {
}

std::shared_ptr<HouseBSData> RoomBuilder::finalise() {
//    rsg.RR().removeFromCL( "RoomBuilderLastTailCircle" );
//    rsg.RR().removeFromCL( "SnapLongLines1" );
//    rsg.RR().removeFromCL( "SnapLongLines2" );
//    finalised = true;
//    currentPointValid = false;
//    saveCachedSegments();
//    setSegmentType(ArchType::WallT);
//
//    HouseMakerBespokeData maker_bespoke;
//    house = maker_bespoke.make(bespokerise() );
//
//    setUIStatusAfterChange();
//
//    return house;
    return nullptr;
}

std::shared_ptr<HouseBSData> RoomBuilder::finaliseWithClose() {
    inputPoint = segments.front();
    currentPoint = inputPoint;
//    addPointToRoom();
    return finalise();
}

bool RoomBuilder::isPerimeterClosed() const {
    return checkPointWillClosePerimeter();
}

void RoomBuilder::toggleSegmentType() {
    switch (activeSegmentType) {
        case ArchType::WallT:
            setSegmentType( ArchType::WindowT );
            break;
        case ArchType::WindowT:
            setSegmentType( ArchType::DoorT );
            break;
        case ArchType::DoorT:
            setSegmentType( ArchType::WallT );
            break;
    }
    refresh();
}

void RoomBuilder::setBestFittingSegmentTypeAfterSegmentCompletion() {
    // IE Goes back to draw walls if you have just put a door in, as you do not want to continue putting another door
    // straight after the one you've just put
//    if ( activeSegmentType == ArchType::WindowT )
//        setSegmentType( ArchType::WallT );
//    if ( activeSegmentType == ArchType::DoorT )
//        setSegmentType( ArchType::WallT );
    setSegmentType( ArchType::WallT );
}

void RoomBuilder::replaceFurniture( const UICallbackHandle& _ch ) {
    FurnitureRuleScript ruleScript;
    ruleScript.addRule( FurniturePlacementRule::randomGeneration() );
    ruleScript.addRule( FurniturePlacementRule::randomGeneration() );
    ruleScript.addRule( FurniturePlacementRule::randomGeneration() );
    ruleScript.addRule( FurniturePlacementRule::randomGeneration() );
    ruleScript.addRule( FurniturePlacementRule::randomGeneration() );
    ruleScript.addRule( FurniturePlacementRule::randomGeneration() );
    ruleScript.addRule( FurniturePlacementRule::randomGeneration() );
    ruleScript.addRule( FurniturePlacementRule::randomGeneration() );

    roomPreBakedFurnitureSetup( ruleScript );
}

void RoomBuilder::setSegmentType( ArchTypeT _st ) {
    activeSegmentType = _st;
}

void RoomBuilder::changeSegmentType( ArchTypeT _st ) {
    setSegmentType( _st );
    refresh();
}

float RoomBuilder::WallWidth() const {
    return wallWidth;
}

V3f RoomBuilder::bestStartingPoint() const {
    return mBestStartingPoint;
}

void RoomBuilder::fade2dScene( float _time, float _value ) {
    fader( _time, _value, gripVPs );
    fader( _time, _value, rsg.RR().CL()[roomEditBucket].mVList );
    fader( _time, _value, rsg.RR().CL()[roomEditBucket].mVListTransparent );
    fader( _time, _value, rsg.RR().CL()[furnitureBucket].mVList );
    fader( _time, _value, rsg.RR().CL()[furnitureBucket].mVListTransparent );
}

void RoomBuilder::onEntry() {
//    rsg.setRigCameraController<CameraControl2d>();
    Renderer::clearColor(C4f::XTORGBA("#238FCBFF"));
    fade2dScene(2.0f, 1.0f);
    rsg.UI()("sidebar")->slideLeftIn( 1.0f );
}

void RoomBuilder::onExit() {
    fade2dScene(2.0f, 0.0f);
    rsg.UI()("sidebar")->slideRightOut( 1.0f );
}

RoomBuilder::RoomBuilder( SceneGraph& sg, RenderOrchestrator& rsg, std::shared_ptr<HouseBSData> _house ) :
        sg( sg ), rsg( rsg ),
        house(_house) {
    roomEditBucket = CommandBufferLimits::UnsortedStart + 2;
}



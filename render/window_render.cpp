//
//  window_render.cpp
//  sixthmaker
//
//  Created by Dado on 19/03/2017.
//
//

#include "window_render.hpp"
#include <core/math/path_util.h>
#include <core/resources/resource_builder.hpp>
#include <poly/scene_graph.h>
#include <graphics/renderer.h>

#include <eh_arch/controller/arch_render_controller.hpp>
#include "house_render.hpp"

namespace WindowRender {

    void drawDefaultWindow( Renderer& rr, const WindowBSData *data, const ArchRenderController& arc ) {
        const V2f& _p1 = data->us2.middle;
        const V2f& _p2 = data->us1.middle;
        float _lineWidth = data->us2.width;
        auto sm = arc.floorPlanShader();
        auto color = arc.getFillColor(data, C4fc::BLACK);
        float windowLineWidth = _lineWidth * 0.2f;
        float halfWindowLineWidth = windowLineWidth * 0.5f;
        float halfLineWidth = _lineWidth * 0.5f;
        float windowLineWidthOffset = halfLineWidth - halfWindowLineWidth;

        auto lineWidth = arc.floorPlanScaler(_lineWidth * 0.05f);

        rr.draw<DLine>(sm, _p1, _p2, color, lineWidth, false, arc.pm(), data->hashFeature("line"+color.toString()+sm.hash(), 0));
        V2f vn = normalize(_p1 - _p2);
        auto slope = rotate90(vn);
        auto p1 = _p1 + ( slope * windowLineWidthOffset );
        auto p2 = _p2 + ( slope * windowLineWidthOffset );
        rr.draw<DLine>(sm, p1, p2, color, lineWidth, false, arc.pm(), data->hashFeature("line"+color.toString()+sm.hash(), 1));
        auto p3 = _p1 + ( slope * -windowLineWidthOffset );
        auto p4 = _p2 + ( slope * -windowLineWidthOffset );
        rr.draw<DLine>(sm, p3, p4, color, lineWidth, false, arc.pm(), data->hashFeature("line"+color.toString()+sm.hash(), 2));
    }

    void IMHouseRender( Renderer& rr, SceneGraph& sg, const WindowBSData *data, const ArchRenderController& arc ) {
        drawDefaultWindow(rr, data, arc);
    }

    // [END] 2D

    std::shared_ptr<Profile> makeBottomWindowFrameProfile( const std::string& _name, float stemTrunk ) {

        V2f size{ 0.018f, stemTrunk * 0.5f };
        float bump = 0.004f;

        std::vector<Vector2f> points;
        points.reserve(4);
        points.emplace_back(0.0f, 0.0f);
        points.emplace_back(0.0f, size.y());
        points.emplace_back(size.x(), size.y() - bump);
        points.emplace_back(size.x(), 0.0f);

        flipAxis(points);

        return Profile::fromPoints(_name, points);
    }

    std::shared_ptr<Profile> makeWindowPanelProfile( const std::string& _name, const V2f& wppSize ) {

        Vector2f size = wppSize; // {7.0f, 4.5f}
        Vector3f sizeh{size * 0.5f};
        float slope = 0.006f; // { 0.4f }
        float glassgap = 0.003f; // { 0.3f }

        std::vector<Vector2f> points;
        points.reserve(5 * 2);
        points.emplace_back(-sizeh.x(), 0.0f);
        points.emplace_back(-sizeh.x(), size.y() - slope);
        points.emplace_back(-sizeh.x() + slope, size.y());
        points.emplace_back(-sizeh.x() + ( slope * 2.f ), size.y());
        points.emplace_back(-sizeh.x() + ( slope * 2.f ), size.y() - glassgap);

        appendMirror(points, V2fc::X_INV);
        flipAxis(points);

        return Profile::fromPoints(_name, points);
    }

    std::shared_ptr<Profile> makeWindowPanelCentralSeparatorProfile( const std::string& _name,
                                                                     float stemTrunk ) {

//    .cf( 0.06 ).cf( 0.004 ).cf( 0.018 ).cf( stemTrunk ).cf( 0.05 )
        float frontMost = 0.06; // 6.0f
        float bump = 0.004; // 0.4f
        float ttop = 0.018; // 1.8f
        float stem = stemTrunk; // 3.0f
        float stemlength = 0.05; // 5.0f

        std::vector<Vector2f> points;
        points.reserve(8);

        points.emplace_back(-frontMost * 0.5f, -ttop);
        points.emplace_back(-frontMost * 0.5f - bump, 0.0f);
        points.emplace_back(-stem * 0.5f, 0.0f);
        points.emplace_back(-stem * 0.5f, stemlength);

        appendMirror(points, V2fc::X_INV);

        return Profile::fromPoints(_name, points);
    }

    std::shared_ptr<Profile> makeValanceProfile( const std::string& _name, const V3f& size ) {
        float bump = size.x() * 0.8f;
        float w1 = size.x() - bump;
        float w2 = size.y() - bump * 2.0f;

        return ProfileMaker{ _name }.sd(10).o().lx(w1).ay(bump).ly(w2).ay(bump).lx(-w1).make();
    }

    void addBlinds( SceneGraph& sg, GeomSP eRootH, WindowBSData *window, const Rect2f& _rect ) {

        if ( !window->hasBlinds ) return;

        // Blind constants
        float bw = _rect.width();
        float bh = _rect.height();
        float bd = 0.04f;
        const V3f blindSize{ _rect.size(), 0.04f };

        Vector2f slatSize{ bw * 0.99f, 0.025f };
        float slatThickness = 0.003f;
        int numslats = static_cast<int>( bh / slatSize.y());
        V2f valanceSize{ 0.005f, bd * 0.75f };
        V3f valancePos{ 0.0f, bh - valanceSize.y(), -( ( blindSize.z() * 0.5f ) + 0.005f ) };
        V3f slat3dSize{ slatSize.x(), slatThickness, slatSize.y() };
        V3f slat3dSizeBig{ slatSize.x(), slatThickness * 2.0f, slatSize.y() };
        V3f wandSize{ 0.01f, bh * 0.6f, 0.01f };
        V3f wandPos{ bw * 0.45f, valancePos.y() - wandSize.y() * 0.5f, valancePos.z() };
        V3f headRailSize{ slat3dSize.x() * 0.98f, valanceSize.y(), valanceSize.y() };
        V3f headRailPos{ 0.0f, valancePos.y() + headRailSize.y() * 0.5f,
                         -( valancePos.z() - headRailSize.z() * 0.5f ) };
        V3f ladderStringSize{ 0.002f, bh, 0.002f };
        float ladderStringGapLength = 0.4f;
        int numLadderStrings = bw < 0.9f ? 2 : static_cast<int>(( bw / ladderStringGapLength ) + 1);
        float ladderOff = ( bw - ( numLadderStrings - 1 ) * ladderStringGapLength ) * 0.5f;

        // new Blind group
//    auto blindH = eRootH->addChildren<V3f>( V3f{0.0f, _rect.bottom(), -window->HalfDepth() + bd*0.5f });
        auto bp = V3f{ 0.0f, _rect.bottom(), -window->HalfDepth() + bd * 0.5f };

        auto valProfile = makeValanceProfile("valence", V3f{valanceSize});
        sg.GB<GT::Follower>(valProfile, lineLR(V3fc::X_AXIS, bw), GT::Direction(V3fc::UP_AXIS), eRootH,
                            valancePos + bp);
//    GeomBuilder{ mPrefs.sg, ProfileBuilder{}.cv2(valanceSize).func(makeValanceProfile),
//                 lineLR(V3fc::X_AXIS, bw), V3fc::UP_AXIS }.inj(eRootH->addChildren()).at(valancePos).build();

        sg.GB<GT::Shape>(ShapeType::Cube, GT::Scale(headRailSize), eRootH, headRailPos + bp);
//    GeomBuilder{ mPrefs.sg, ShapeType::Cube }.s(headRailSize).inj(eRootH->addChildren()).at(headRailPos).build();

        float deltaInc = 1.0f / static_cast<float>(numslats);
        float delta = 0.0f;
        for ( int t = 1; t < numslats; t++ ) {
            sg.GB<GT::Shape>(ShapeType::Pillow, GT::Scale(slat3dSize), eRootH,
                             V3fc::UP_AXIS * ( delta * bh ) + bp);
//        GeomBuilder{ mPrefs.sg, ShapeType::Pillow }.inj(eRootH->addChildren()).s(slat3dSize).at(V3fc::UP_AXIS*(delta*bh)).build();
            delta += deltaInc;
        }

        sg.GB<GT::Shape>(ShapeType::Pillow, GT::Scale(slat3dSizeBig), bp, eRootH);
        sg.GB<GT::Shape>(ShapeType::Cylinder, GT::Scale(wandSize), wandPos, bp, eRootH);
//    GeomBuilder{ mPrefs.sg, ShapeType::Pillow }.inj(eRootH->addChildren()).s(slat3dSizeBig).build();
//    GeomBuilder{ mPrefs.sg, ShapeType::Cylinder }.inj(eRootH->addChildren()).s(wandSize).at(wandPos).build();

        // LadderStrings
        for ( int q = 0; q < numLadderStrings; q++ ) {
            float lz = headRailPos.z() * 0.5f - ladderStringSize.z();
            std::array<V3f, 2> lpos{
                    V3f{ -bw * 0.5f + ( ladderOff + q * ladderStringGapLength ), bh * 0.5f, lz },
                    V3f{ -bw * 0.5f + ( ladderOff + q * ladderStringGapLength ), bh * 0.5f, -lz },
            };
            for ( size_t i = 0; i < lpos.size(); i++ ) {
                sg.GB<GT::Shape>(ShapeType::Cylinder, GT::Scale(ladderStringSize), lpos[i] + bp, eRootH);
//            GeomBuilder{ mPrefs.sg, ShapeType::Cylinder }.inj(eRootH->addChildren()).s(ladderStringSize).at(lpos[i]).build();
            }
        }
    }

    Vector3f depthOffset( WindowBSData *window, float _off ) {
        return V3fc::Z_AXIS * ( ( window->Depth() * 0.5f ) - _off );
    }

    void addPlastersAroundEdges( SceneGraph& sg, GeomSP eRootH, WindowBSData *window, float currBaseOffset ) {

        auto fverts2 = utilGenerateFlatRect(Vector2f(window->Width(), window->Height()),
                                            WindingOrder::CCW,
                                            PivotPointPosition::BottomCenter);
        for ( auto& v : fverts2 ) {
            v += V2fc::Y_AXIS * currBaseOffset;
        }

        for ( int t = 0; t < 2; t++ ) {
            std::array<Vector2f, 2> vline{};
            float plasterWidth = max(window->us1.width, window->us2.width);
            vline[1] = Vector2f(0.0f, plasterWidth * 0.25f);
            vline[0] = Vector2f(0.0f, -plasterWidth * 0.25f);



            auto profile = Profile::fromPoints("WindowPlasterAround", { vline[0], vline[1] });

            float off = window->Depth() * 0.25f * ( ( t == 0 ) ? 1.0f : -1.0f );

            sg.GB<GT::Follower>(profile, fverts2, V3fc::Z_AXIS * off, eRootH);
        }
    }

// These are the wall pieces at floor level and at ceiling level, not top/bottom of the window itself.
// We do this in order to now to have gaps for lighting.
    void addTopBottomWallPieces( SceneGraph& sg, GeomSP& root, WindowBSData *window ) {
        // bottom and top wall pieces

        std::array<Vector2f, 2> vline{};
        vline[1] = Vector2f(0.0f, window->Depth() * 0.5f);
        vline[0] = Vector2f(0.0f, -window->Depth() * 0.5f);

        std::vector<V3f> ptop;
        ptop.emplace_back(-window->Width() * 0.5f, 0.0f, 0.0f);
        ptop.emplace_back(window->Width() * 0.5f, 0.0f, 0.0f);
        sg.GB<GT::Follower>(Profile::fromPoints("WindowPlasterTB", { vline[0], vline[1] }), ptop,
                            GT::ForceNormalAxis(V3fc::Z_AXIS),
                            root);
        sg.GB<GT::Follower>(Profile::fromPoints("WindowPlasterTB", { vline[0], vline[1] }), ptop,
                            V3fc::UP_AXIS * ( window->ceilingHeight ),
                            GT::ForceNormalAxis(V3fc::Z_AXIS_NEG),
                            root);

//        sg.GB<GT::Shape>( ShapeType::Cube, V3fc::UP_AXIS*(0.005f),
//                      GT::Scale{ window->Width(), 0.01f, window->Depth()}, root );
//
//    sg.GB<GT::Shape>( ShapeType::Cube, V3fc::UP_AXIS*(window->ceilingHeight-0.005f),
//                      GT::Scale{ window->Width(), 0.01f, window->Depth()}, root );

    }

    void
    addWindowSill( SceneGraph& sg, GeomSP& root, WindowBSData *window, float windowsSillDepth, float currBaseOffset ) {
        sg.GB<GT::Shape>(ShapeType::Pillow, V3fc::UP_AXIS * ( currBaseOffset + windowsSillDepth * 0.50f ),
                         GT::Scale{ window->Width() * 1.02f, windowsSillDepth, window->Depth() * 1.12f }, root);
    }

    void addWindowMeshes( SceneGraph& sg, GeomSP eRootH, WindowBSData *window, const Rect2f& _windowRect,
                          [[maybe_unused]] float windowsSillDepth ) {

        // steam profile
        float stemTrunk = 0.03f;
        const Vector2f wppSize{ 0.07f, 0.06f };

        auto centralStemProfile = makeWindowPanelCentralSeparatorProfile("centralStemProfile", stemTrunk);

        // window's frame profile
        auto pf = makeBottomWindowFrameProfile("WindowFrameProfile", stemTrunk);

        sg.GB<GT::Follower>(centralStemProfile, _windowRect.pointscw(), depthOffset(window, wppSize.x()),
                            FollowerFlags::WrapPath, GT::Flip(V2fc::X_AXIS), eRootH);

        float panelsWidth = ( window->Width() - stemTrunk );
        int numPanels = static_cast<int>( ceil(panelsWidth / 0.60f));

        // Stems
        for ( int t = 1; t < numPanels; t++ ) {
            float delta = static_cast<float>(t) / numPanels;
            Vector2f p1 = lerp(delta, _windowRect.bottomLeft(), _windowRect.bottomRight());
            Vector2f p2 = lerp(delta, _windowRect.topLeft(), _windowRect.topRight());

            sg.GB<GT::Follower>(centralStemProfile, depthOffset(window, wppSize.x()),
                                GT::Direction(V3fc::Z_AXIS),
                                V3fVector{ V3f{p1}, V3f{p2} }, eRootH);
        }

        // Window panel profile
        auto wpp = makeWindowPanelProfile("wpp", wppSize);
        float stp = ( stemTrunk / _windowRect.width() ) * 0.5f;
//    uint32_t numHandles = numPanels > 1 ? 2 : 1;

        for ( int t = 0; t < numPanels; t++ ) {
//        auto panelH = eRootH->addChildren();
            float delta = ( static_cast<float>(t) / numPanels ) - stp;
            float deltan = ( static_cast<float>(t + 1) / numPanels ) + stp;
            auto panelRect = _windowRect.verticalSlice(delta, deltan);

            sg.GB<GT::Follower>(wpp, panelRect, GT::Direction(V3fc::Z_AXIS),
                                FollowerFlags::WrapPath, depthOffset(window, wppSize.x() * 0.5f), eRootH);

//        GeomBuilder{mPrefs.sg,wpp, panelRect, V3fc::Z_AXIS}.ff(FollowerFlags::WrapPath).inj(panelH).
//                                                      at(depthOffset(wppSize.x()*0.5f)).
//                                                      g(GHType::Window).build();
            // Get the handle on first and last panels
//        This needs to be a children of the panel abouve, not eRootH!
//        bool bFirst = t == 0;
//        bool bLast = (numHandles > 1 && t == numPanels-1);
//        if ( bFirst || bLast ) {
//            auto whoff = bLast ? panelRect.centreRight() : panelRect.centreLeft();
//            whoff += V2fc::X_AXIS * ((bLast ? -wppSize.y()+ wppSlope : wppSize.y() - wppSlope ) );
//            GeomBuilder{mPrefs.sg, GeomBuilderType::file, "window,handle,plastic" }.
//                         at( whoff ).r( V3fc::UP_AXIS*M_PI).
//                         bboff( Vector3f{ bLast ? 0.5f : -0.5f, 0.0f, -0.5f }).
//                         inj(panelH->addChildren()).build();
//        }
        }
    }

    void addCurtains( SceneGraph& sg, GeomSP eRootH, WindowBSData *window, const Rect2f& _windowRect, [[maybe_unused]] float baseOff ) {
        if ( !window->hasCurtains ) return;

        auto s = sg.getGeomNameSize("curtain");
        // Rescale the curtain with a % of slack so the curtain doesn't end exactly at window width
        float totalHeight = window->baseOffset + window->Height() + window->sillThickness;
        V3f curtainScale{ ( window->Width() * 1.25f ) / std::get<1>(s).x(), totalHeight / std::get<1>(s).y(), 1.0f };
        sg.GB<GT::Asset>("curtain", eRootH, V3f{ 0.0f, 0.07f, -window->Depth()+ half(std::get<1>(s).z()) } - 0.07f, GT::Scale(curtainScale),
                         GT::Tag(ArchType::CurtainT),
                         GT::M(window->curtainMaterial));
    }

    GeomSP make3dGeometry( SceneGraph& sg, GeomSP eRootH, WindowBSData *window ) {

        auto lRootH = EF::create<Geom>("Window"+ std::to_string(window->hash));

        float windowsSillDepth = 0.04f;
        float currBaseOffset = window->baseOffset < window->ceilingHeight ? window->baseOffset : window->ceilingHeight;
        Rect2f windowRect(Vector2f(window->Width(), window->Height() - windowsSillDepth));
        windowRect.translate({ window->Width() * -0.5f, currBaseOffset + windowsSillDepth });

        addWindowSill(sg, lRootH, window, windowsSillDepth, currBaseOffset);

        Rect2f windowRectInclusive(Vector2f(window->Width(), window->Height()));
        windowRectInclusive.translate({ window->Width() * -0.5f, currBaseOffset });

        addPlastersAroundEdges(sg, lRootH, window, currBaseOffset);
        addTopBottomWallPieces(sg, lRootH, window);
        addBlinds(sg, lRootH, window, windowRect);
        addWindowMeshes(sg, lRootH, window, windowRect, windowsSillDepth);

        addCurtains(sg, lRootH, window, windowRect, currBaseOffset + windowsSillDepth);

        float vwangle = -atan2(-window->dirWidth.y(), window->dirWidth.x());
        Quaternion rot(vwangle + window->rotOrientation, V3fc::UP_AXIS);

        return eRootH->addChildren(lRootH, window->Position(), rot, V3fc::ONE);
    }

}

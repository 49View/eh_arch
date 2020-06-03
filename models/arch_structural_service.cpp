////
//// Created by dado on 30/05/2020.
////
//
//#include "arch_structural_service.hpp"
//
//bool ArchStructuralService::typeIsiPoint( ArchStructural *a ) {
//    return a->type == ArchType::WallPointT || a->type == ArchType::DoorAnchorT || a->type == ArchType::WindowAnchorT;
//}
//
//bool typeIsDOW( ArchStructural *a ) {
//    return a->type == ArchType::DoorT || a->type == ArchType::WindowT;
//}
//
//bool typeIsWall( ArchStructural *a ) {
//    return a->type == ArchType::WallT;
//}
//
//void rescale( ArchStructural *a, float _scale ) {
//    //height *= _scale;
//    a->width *= _scale;
//    a->depth *= _scale;
//    a->center *= _scale;
//
//    for ( auto& vts : a->mTriangles2d ) {
//        std::get<0>(vts) *= _scale;
//        std::get<1>(vts) *= _scale;
//        std::get<2>(vts) *= _scale;
//    }
//}
//
//bool intersectLine( ArchStructural *a, const Vector3f& linePos, const Vector3f& lineDir,
//                    float& tNear ) {
//    float farV = std::numeric_limits<float>::max();
//    return a->bbox3d.intersectLine(linePos, lineDir, tNear, farV);
//}
//
//Vector3f posForSDV( ArchStructural *a, bool doomMode ) {
//    if ( doomMode ) {
//        Vector3f bc = a->bbox3d.centre();
//        bc.swizzle(2, 1);
//        return Vector3f::ZERO;
//    }
//
//    return a->bbox.centreRight() + Vector2f(a->bbox.width() * 0.05f, 0.0f);
//}
//
//bool isPointInside( ArchStructural *a, const Vector2f& _pos ) {
//    if ( !a->bbox.contains(_pos) ) return false;
//
//    if ( a->mTriangles2d.size() == 0 ) {
//        return true;
//    } else {
//        for ( auto& t : a->mTriangles2d ) {
//            if ( isInsideTriangle(_pos, std::get<0>(t), std::get<1>(t), std::get<2>(t)) ) {
//                return true;
//            }
//        }
//    }
//    return false;
//}

float metersToCentimeters(float valueInMeter) {
    return valueInMeter*0.01f;
}

float centimetersToMeters(float valueInCM) {
    return valueInCM*100.0f;
}
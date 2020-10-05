const { Vector} = require('../geometry/vector');
const {
    getTrianglesFromPolygon,
    getTrianglesFromComplexPolygon,
    extrudePoly,
    createMesh,
    createElementsFromWays,
    createElementsFromRels,
    pointOnLine,
    distanceFromLine,
    setHeight,
    twoLinesIntersectParameter: twoLinesIntersectParameter
} = require('./osmHelper.js');

const HEIGHT_FOR_LEVEL = 3.5;
const DEFAULT_BUILDING_HEIGHT = 10;
const DEFAULT_ROOF_COLOUR = "#cccccc";
const DEFAULT_BUILDING_COLOUR = "#eeeeee";
const USE_TRIANGLES_STRIP = false;

const exportBuildings = (tileBoundary, nodes, ways, rels) => {
    console.log("----------------------------------------------");
    console.log("BUILDINGS");
    console.log("----------------------------------------------");

    const simpleBuildings=createElementsFromWays(ways, tileBoundary, "building"
        , w => w.tags && (w.tags["building"] || w.tags["building:part"])
        , buildingFromWay);
    // const simpleBuildings=[];
    const complexBuildings=createElementsFromRels(rels, tileBoundary, "building"
        , r => r.tags && (r.tags["building"] || r.tags["building:part"])
        , buildingFromRel);

    console.log(`Found ${simpleBuildings.length} simple buildings`);
    console.log(`Found ${complexBuildings.length} complex buildings`);
    console.log("----------------------------------------------");

    return simpleBuildings.concat(complexBuildings);
}

const getBuildingInfo = (tags) => {
    let minHeight,maxHeight,colour;
    let roofOrientation,roofHeight,roofShape,roofColour;

    if (tags["min_height"]) {
        minHeight=Number(tags["min_height"].replace("m",""));
    } else if (tags["building:min_height"]) {
        minHeight=Number(tags["building:min_height"].replace("m",""));
    } else if (tags["building:min_level"]) {
        minHeight=Number(tags["building:min_level"].replace("m",""))*HEIGHT_FOR_LEVEL;
    } else {
        minHeight=0;
    }
    if (tags["height"]) {
        maxHeight=Number(tags["height"].replace("m",""));
    } else if (tags["building:height"]) {
        maxHeight=Number(tags["building:height"].replace("m",""));
    } else if (tags["building:levels"]) {
        if (tags["building:levels"].indexOf(",")!==-1) {
            const levels=tags["building:levels"].split(",");
            maxHeight=levels.length*HEIGHT_FOR_LEVEL;
        } else  {
            maxHeight=Number(tags["building:levels"].replace("m",""))*HEIGHT_FOR_LEVEL;
        }
    } else {
        maxHeight=DEFAULT_BUILDING_HEIGHT;
    }

    if (tags["building:colour"]) {
        colour=tags["building:colour"];
    } else {
        colour=DEFAULT_BUILDING_COLOUR;
    }

    if (!colour.startsWith("#")) {
        colour="#"+colour;
    }

    if (tags["roof:shape"]==="pyramidal" || tags["building:roof:shape"]==="pyramidal") {
        roofShape="pyramidal";
    } else if (tags["roof:shape"]==="gabled" || tags["building:roof:shape"]==="gabled") {
        roofShape="gabled";
    } else if (tags["roof:shape"]==="hipped" || tags["building:roof:shape"]==="hipped") {
        roofShape="hipped";
    } else if (tags["roof:shape"]==="dome" || tags["building:roof:shape"]==="dome") {
        roofShape="dome";
    } else { //FLAT
        roofShape="flat";
    }

    if (tags["roof:height"]) {
        roofHeight=Number(tags["roof:height"].replace("m",""));
    } else if (tags["building:roof:height"]) {
        roofHeight=Number(tags["building:roof:height"].replace("m",""));
    } else if (tags["roof:levels"]) {
        roofHeight=Number(tags["roof:levels"].replace("m",""))*HEIGHT_FOR_LEVEL;
    } else if (tags["building:roof:levels"]) {
        roofHeight=Number(tags["building:roof:levels"].replace("m",""))*HEIGHT_FOR_LEVEL;
    } else {
        if (roofShape==="flat") {
            roofHeight=0;
        } else {
            roofHeight=HEIGHT_FOR_LEVEL;
        }
    }

    if (tags["roof:orientation"]) {
        roofOrientation=tags["roof:orientation"];
    } else {
        roofOrientation="along";
    }

    if (tags["roof:colour"]) {
        roofColour=tags["roof:colour"];
    } else if (tags["building:colour"]) {
        roofColour=tags["building:colour"];
    } else {
        roofColour=DEFAULT_ROOF_COLOUR;
    }
    if (!roofColour.startsWith("#")) {
        roofColour="#"+roofColour;
    }

    if (maxHeight-roofHeight<minHeight) {
        roofHeight=0;
        roofShape="flat";
    } else {
        maxHeight=maxHeight-roofHeight;
    }
    const roofMinHeight=maxHeight;
    const roofMaxHeight=maxHeight+roofHeight;

    return {
        minHeight: minHeight,
        maxHeight: maxHeight,
        colour: colour,
        roof: {
            minHeight: roofMinHeight,
            maxHeight: roofMaxHeight,
            shape: roofShape,
            orientation: roofOrientation,
            colour: roofColour
        }
    }
}

const createBuildingMesh = (id, tags, type, boundingBox, lateralFaces, roofFaces, buildingInfo) => {

    const groups=[];

    groups.push({
        faces: lateralFaces,
        part: "lateral_faces",
        colour: buildingInfo.colour,
        isTriangleStrip: USE_TRIANGLES_STRIP
    });
    groups.push({
        faces: roofFaces,
        part: "roof_faces",
        colour: buildingInfo.roof.colour,
        isTriangleStrip: false
    });
    return createMesh(id, tags, type, boundingBox, groups);
}

const createRoof = (polygon, roofInfo, convexHull, ombb) => {
    if (roofInfo.shape==="pyramidal") {
        return createPyramidalRoof(polygon, roofInfo);
    } else if (roofInfo.shape==="gabled") {
        return createGabledRoof(polygon, roofInfo, ombb, 1);
    } else if (roofInfo.shape==="hipped") {
        return createGabledRoof(polygon, roofInfo, ombb, 0.3);
    } else if (roofInfo.shape==="dome") {
        return createDomeRoof(polygon, roofInfo);
    } else {
        return createFlatRoof(polygon, roofInfo);
    }
}

const createDomeRoof = (polygon, roofInfo) => {
    let faces=[];

    //Compute shorter distance from center to polygon segments
    let inCircleRay = Number.MAX_VALUE;
    for (let i=0;i<polygon.length;i++) {
        const nextI = (i+1)%polygon.length;
        const point=polygon[i];
        const nextPoint=polygon[nextI];

        //Distance from center to polygon segment
        let distanceFromCenter = distanceFromLine({x: 0, y:0}, point, nextPoint, true);
        if (distanceFromCenter<inCircleRay) {
            inCircleRay=distanceFromCenter;
        }
    }

    // let errorCounter = 3;
    // let error=true;
    // let inCircle=[];
    let unionSurface;

    // while (error && errorCounter>0) {
    //     try {
    //         //Reduce ray for avoid error on union surface computation
    //         inCircleRay=inCircleRay*0.98;
    //         inCircle = [];
    //         //Create in circle surface
    //         for (let i=0;i<360;i+=5) {
    //             const a=i*Math.PI/180;

    //             inCircle.push({
    //                 x: inCircleRay*Math.cos(a),
    //                 y: inCircleRay*Math.sin(a)
    //             })
    //         }
    //         //Create union surface, from polygon to in circle
    //         unionSurface=getTrianglesFromComplexPolygon(polygon, [{points: inCircle}]);
    //         error=false;
    //     } catch (ex) {
    //         console.log("Reduce inCircle ray");
    //         errorCounter--;
    //     }
    // }

    // if (error) {
    //     //Cannot compute base
    //     unionSurface=getTrianglesFromPolygon(polygon);
    // }

    unionSurface=getTrianglesFromPolygon(polygon);
    setHeight(unionSurface, roofInfo.minHeight);
    faces = faces.concat(unionSurface);

    for (let alpha=0;alpha<360;alpha+=5) {
        const radAlpha=alpha*Math.PI/180;
        const radNextAlpha=((alpha+5)%360)*Math.PI/180;

        let lastXPoint = inCircleRay*Math.cos(radAlpha);
        let lastYPoint = inCircleRay*Math.sin(radAlpha);
        let lastZPoint = roofInfo.minHeight;
        let lastXNextPoint = inCircleRay*Math.cos(radNextAlpha);
        let lastYNextPoint = inCircleRay*Math.sin(radNextAlpha);
        let lastZNextPoint = roofInfo.minHeight;

        for (let theta=5;theta<90;theta+=5) {
            const radTheta=theta*Math.PI/180;

            let xyPoint = inCircleRay*Math.cos(radTheta);
            let xPoint = xyPoint*Math.cos(radAlpha);
            let yPoint = xyPoint*Math.sin(radAlpha);
            let zPoint = roofInfo.minHeight+inCircleRay*Math.sin(radTheta);

            let xNextPoint = xyPoint*Math.cos(radNextAlpha);
            let yNextPoint = xyPoint*Math.sin(radNextAlpha);
            let zNextPoint = zPoint;

            faces.push({x: lastXPoint, y: lastYPoint, z: lastZPoint});
            faces.push({x: lastXNextPoint, y: lastYNextPoint, z: lastZNextPoint});
            faces.push({x: xNextPoint, y: yNextPoint, z: zNextPoint});

            faces.push({x: lastXPoint, y: lastYPoint, z: lastZPoint});
            faces.push({x: xNextPoint, y: yNextPoint, z: zNextPoint});
            faces.push({x: xPoint, y: yPoint, z: zPoint});

            lastXPoint=xPoint;
            lastYPoint=yPoint;
            lastZPoint=zPoint;
            lastXNextPoint=xNextPoint;
            lastYNextPoint=yNextPoint;
            lastZNextPoint=zNextPoint;
        }

        let xPoint=0;
        let yPoint=0;
        let zPoint=roofInfo.minHeight+inCircleRay;

        faces.push({x: lastXPoint, y: lastYPoint, z: lastZPoint});
        faces.push({x: lastXNextPoint, y: lastYNextPoint, z: lastZNextPoint});
        faces.push({x: xPoint, y: yPoint, z: zPoint});

    }

    return faces;
}

const createPyramidalRoof = (polygon, roofInfo) => {
    let faces=[];

    for (let i=0;i<polygon.length;i++) {
        const nextI = (i+1)%polygon.length;
        const point=polygon[i];
        const nextPoint=polygon[nextI];

        faces.push({x: point.x, y: point.y, z: roofInfo.minHeight});
        faces.push({x: nextPoint.x, y: nextPoint.y, z: roofInfo.minHeight});
        faces.push({x: 0, y: 0, z: roofInfo.maxHeight});
    }

    return faces;
}

const createFlatRoof = (polygon, roofInfo) => {
    let faces=[];
    if (roofInfo.minHeight!==roofInfo.maxHeight) {
        //If required, compute lateral roof face
        faces=faces.concat(extrudePoly(polygon, roofInfo.minHeight, roofInfo.maxHeight, false));
    }
    //
    const topFace = getTrianglesFromPolygon(polygon);
    topFace.forEach(t => t.z=roofInfo.maxHeight);

    return faces.concat(topFace);
}

const createGabledRoof = (polygon, roofInfo, ombb, hippedPerc) => {

    const axes=[];
    let pointA, pointB;
    let startIndex;

    //OMBB Axis
    axes.push(new Vector(ombb[1].x-ombb[0].x, ombb[1].y-ombb[0].y));
    axes.push(new Vector(ombb[2].x-ombb[1].x, ombb[2].y-ombb[1].y));
    axes.push(new Vector(ombb[3].x-ombb[2].x, ombb[3].y-ombb[2].y));
    axes.push(new Vector(ombb[0].x-ombb[3].x, ombb[0].y-ombb[3].y));

    if (roofInfo.orientation==="along") {
        if (axes[0].length()>axes[1].length()) {
            startIndex=1;
        } else {
            startIndex=0;
        }
    } else {
        if (axes[0].length()<axes[1].length()) {
            startIndex=1;
        } else {
            startIndex=0;
        }
    }

    pointA=ombb[startIndex].lerp(ombb[startIndex+1], 0.5);
    pointB=ombb[startIndex+2].lerp(ombb[(startIndex+3)%4], 0.5);

    // const axis = new Vector(pointB.x-pointA.x,pointB.y-pointA.y);

    let faces=[];

    let intersection=[];
    for (let i=0;i<polygon.length;i++) {
        const nextI=(i+1)%polygon.length;
        const p = polygon[i];
        const nextP = polygon[nextI];

        //Check sides intersected by axis
        const u = twoLinesIntersectParameter(pointA, pointB, p, nextP);
        if (u!==Infinity && u>=0 && u<1) {

            intersection.push({
                i1: i,
                i2: nextI,
                p1: p,
                p2: nextP,
                point: (new Vector(p.x, p.y).lerp(new Vector(nextP.x, nextP.y), u))
            })
        }
    }

    //More than 2 intersection degenerate in pyramidal roof
    if (intersection.length!==2) {
        return createPyramidalRoof(polygon, roofInfo);
    }

    if (hippedPerc<1) {
        const newP1 = intersection[0].point.lerp(intersection[1].point, hippedPerc);
        const newP2 = intersection[0].point.lerp(intersection[1].point, 1-hippedPerc);

        intersection[0].point=newP1;
        intersection[1].point=newP2;
    }

    intersection.forEach(i => {
        faces.push({ x: i.p1.x, y: i.p1.y, z:roofInfo.minHeight});
        faces.push({ x: i.p2.x, y: i.p2.y, z:roofInfo.minHeight});
        faces.push({ x: i.point.x, y: i.point.y, z:roofInfo.maxHeight});
    })

    for (let i=0;i<polygon.length;i++) {
        const nextI=(i+1)%polygon.length;
        const p = polygon[i];
        const nextP = polygon[nextI];

        let intersectionSide=false;
        for (let j=0;j<intersection.length;j++) {
            if (intersection[j].i1===i && intersection[j].i2===nextI) {
                intersectionSide=true;
                break;
            }
        }
        if (!intersectionSide) {
            // let prjParamP = projectionParameterPointToSegment(p, intersection[0].point, intersection[1].point, false);
            // let prjParamNextP = projectionParameterPointToSegment(nextP, intersection[0].point, intersection[1].point, false);
            // console.log(prjParamP);
            // console.log(prjParamNextP);
            const prj = pointOnLine(p, intersection[0].point, intersection[1].point, true);
            const nextPrj = pointOnLine(nextP, intersection[0].point, intersection[1].point, true);

            faces.push({x: p.x, y: p.y, z:roofInfo.minHeight});
            faces.push({x: nextP.x, y: nextP.y, z:roofInfo.minHeight});
            faces.push({x: nextPrj.x, y: nextPrj.y, z:roofInfo.maxHeight});

            faces.push({x: p.x, y: p.y, z:roofInfo.minHeight});
            faces.push({x: nextPrj.x, y: nextPrj.y, z:roofInfo.maxHeight});
            faces.push({x: prj.x, y: prj.y, z:roofInfo.maxHeight});
    //console.log("Next");
        }
    }

    // for (let i=0;i<polygon.length;i++) {
    //     const nextI=(i+1)%polygon.length;
    //     const p = polygon[i];
    //     const nextP = polygon[nextI];
    //      const prj = calcPointProjection(pointA, pointB, p);
    //      const nextPrj = calcPointProjection(pointA, pointB, nextP);
    // //    const prj = pointOnLine(p, pointA, pointB, false);
    // //    const nextPrj = pointOnLine(nextP, pointA, pointB, false);
    //     const dir = new Vector(prj.x-p.x,prj.y-p.y).cross(axis);
    //     const nextDir = new Vector(prj.x-nextP.x,prj.y-nextP.y).cross(axis);

    //     if (Math.sign(dir)===Math.sign(nextDir)) {
    //         faces.push({x: p.x, y: p.y, z:roofInfo.minHeight});
    //         faces.push({x: nextP.x, y: nextP.y, z:roofInfo.minHeight});
    //         faces.push({x: nextPrj.x, y: nextPrj.y, z:roofInfo.maxHeight});

    //         faces.push({x: p.x, y: p.y, z:roofInfo.minHeight});
    //         faces.push({x: nextPrj.x, y: nextPrj.y, z:roofInfo.maxHeight});
    //         faces.push({x: prj.x, y: prj.y, z:roofInfo.maxHeight});
    //     } else {
    //         if (prj.x===nextPrj.x && prj.y===nextPrj.y) {
    //             faces.push({x: p.x, y: p.y, z:roofInfo.minHeight});
    //             faces.push({x: nextP.x, y: nextP.y, z:roofInfo.minHeight});
    //             faces.push({x: prj.x, y: prj.y, z:roofInfo.maxHeight});
    //         } else {
    //             faces.push({x: p.x, y: p.y, z:roofInfo.minHeight});
    //             faces.push({x: prj.x, y: prj.y, z:roofInfo.minHeight});
    //             faces.push({x: prj.x, y: prj.y, z:roofInfo.maxHeight});

    //             faces.push({x: prj.x, y: prj.y, z:roofInfo.minHeight});
    //             faces.push({x: nextPrj.x, y: nextPrj.y, z:roofInfo.minHeight});
    //             faces.push({x: nextPrj.x, y: nextPrj.y, z:roofInfo.maxHeight});

    //             faces.push({x: prj.x, y: prj.y, z:roofInfo.minHeight});
    //             faces.push({x: nextPrj.x, y: nextPrj.y, z:roofInfo.maxHeight});
    //             faces.push({x: prj.x, y: prj.y, z:roofInfo.maxHeight});

    //             faces.push({x: nextPrj.x, y: nextPrj.y, z:roofInfo.minHeight});
    //             faces.push({x: nextP.x, y: nextP.y, z:roofInfo.minHeight});
    //             faces.push({x: nextPrj.x, y: nextPrj.y, z:roofInfo.maxHeight});
    //         }
    //     }
    // }


    return faces;
}


const createComplexPolygonRoof = (outerPolygon, innerPolygons, roofInfo) => {

    let faces=[];
    if (roofInfo.minHeight!==roofInfo.maxHeight) {
        //If required, compute lateral roof face
        faces=faces.concat(extrudePoly(outerPolygon.points, roofInfo.minHeight, roofInfo.maxHeight, false));
        innerPolygons.forEach(h => {
            faces = faces.concat(extrudePoly([...h.points].reverse(), roofInfo.minHeight, roofInfo.maxHeight, false));
        })

    }

    const topFace=getTrianglesFromComplexPolygon(outerPolygon.points, innerPolygons);
    topFace.forEach(t => t.z=roofInfo.maxHeight);

    faces=faces.concat(topFace);

    return faces;
}

const buildingFromWay = (way, name) => {

    let isOutline;

    const polygon = way.calc.polygon;
    const localBoundingBox = way.calc.localBoundingBox;
    const convexHull = way.calc.convexHull;
    const orientedMinBoundingBox = way.calc.ombb;

    isOutline=false;
    if (way.tags["building"] && way.tags["building:part"]===undefined && way.children.length>0) {

        way.children.forEach(w => {
            if (w.tags["building:part"]) {
                isOutline=true;
            }
        })
    }

    const buildingInfo = getBuildingInfo(way.tags);
    if  (isOutline && way.tags["height"]) {
        buildingInfo.minHeight=0;
        buildingInfo.maxHeight=0;
        buildingInfo.roof.minHeight=0;
        buildingInfo.roof.maxHeight=0;
        buildingInfo.roof.shape="flat";
    }

    //Compute lateral faces
    const lateralFaces = extrudePoly(polygon, buildingInfo.minHeight, buildingInfo.maxHeight, USE_TRIANGLES_STRIP);
    //Compute roof faces
    const roofFaces = createRoof(polygon, buildingInfo.roof, convexHull, orientedMinBoundingBox);

    //Create building mesh
    return createBuildingMesh("w-" + way.id, way.tags, name, localBoundingBox, lateralFaces, roofFaces, buildingInfo);
}

const buildingFromRel = (rel, name) => {

    const buildingInfo = getBuildingInfo(rel.tags);

    const polygons = rel.calc.polygons;
    const localBoundingBox = rel.calc.localBoundingBox;

    let roofFaces=[];
    let lateralFaces=[];
    polygons.filter(p => p.role==="outer").forEach(o => {
        //Compute lateral faces
        lateralFaces = lateralFaces.concat(extrudePoly(o.points, buildingInfo.minHeight, buildingInfo.maxHeight, USE_TRIANGLES_STRIP));
        o.holes.forEach(h => {
            lateralFaces = lateralFaces.concat(extrudePoly([...h.points].reverse(), buildingInfo.minHeight, buildingInfo.maxHeight, USE_TRIANGLES_STRIP));
        })
        //Compute roof faces
        roofFaces = roofFaces.concat(createComplexPolygonRoof(o, o.holes, buildingInfo.roof));
    })
    return createBuildingMesh("r-" + rel.id, rel.tags, name, localBoundingBox, lateralFaces, roofFaces, buildingInfo);
}

module.exports = { exportBuildings }

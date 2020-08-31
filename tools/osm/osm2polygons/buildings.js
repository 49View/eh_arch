const { Vector} = require('./vector');
const Decimal = require('decimal.js');
Decimal.set({ precision: 20, rounding: 1 });
const {
    getTrianglesFromPolygon,
    getTrianglesFromComplexPolygon,
    extrudePoly,
    calcPointProjection,
    createMesh,
    createElementsFromWays,
    createElementsFromRels

} = require('./osmHelper.js');

const HEIGHT_FOR_LEVEL = 3;
const DEFAULT_BUILDING_HEIGHT = 10;
const DEFAULT_ROOF_COLOUR = "#cccccc";
const DEFAULT_BUILDING_COLOUR = "#eeeeee";
const USE_TRIANGLES_STRIP = false;

const createBuildings = (nodes,ways,rels) => {
    console.log("----------------------------------------------");
    console.log("BUILDINGS");
    console.log("----------------------------------------------");

    const simpleBuildings=createElementsFromWays(ways
        , w => w.tags && (w.tags["building"] || w.tags["building:part"])
        , buildingFromWay);
    // const simpleBuildings=[];
    const complexBuildings=createElementsFromRels(rels
        , r => r.tags && r.tags["building"]
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
    } if (tags["building:min_height"]) {
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
        roofOrientation="across";
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

    if (maxHeight-roofHeight<=minHeight) {
        roofHeight=0;
        roofShape="flat";
    } else {
        maxHeight=maxHeight-roofHeight;
    }
    roofMinHeight=maxHeight;
    roofMaxHeight=maxHeight+roofHeight;

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

const createBuildingMesh = (id, tags, boundingBox, lateralFaces, roofFaces, buildingInfo) => {

    const groups=[];

    groups.push({
        faces: lateralFaces,
        colour: buildingInfo.colour,
        isTriangleStrip: USE_TRIANGLES_STRIP
    });
    groups.push({
        faces: roofFaces,
        colour: buildingInfo.roof.colour,
        isTriangleStrip: false
    });
    return createMesh(id, tags, "building", boundingBox, groups);
}

const createRoof = (polygon, roofInfo, convexHull, ombb) => {
    if (roofInfo.shape==="pyramidal") {
        return createPyramidalRoof(polygon, roofInfo);
    } else if (roofInfo.shape==="gabled") {
        return createGabledRoof(polygon, roofInfo, ombb);
        //return createFlatRoof(polygon, roofInfo);
    } else {
        return createFlatRoof(polygon, roofInfo);
    }
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

const createGabledRoof = (polygon, roofInfo, ombb) => {

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

    const axis = new Vector(pointB.x-pointA.x,pointB.y-pointA.y);

    let faces=[];
    for (let i=0;i<polygon.length;i++) {
        const nextI=(i+1)%polygon.length;
        const p = polygon[i];
        const nextP = polygon[nextI];
        const prj = calcPointProjection(pointA, pointB, p);
        const nextPrj = calcPointProjection(pointA, pointB, nextP);

        const dir = new Vector(prj.x-p.x,prj.y-p.y).cross(axis);
        const nextDir = new Vector(prj.x-nextP.x,prj.y-nextP.y).cross(axis);

        if (Math.sign(dir)===Math.sign(nextDir)) {
            faces.push({x: p.x, y: p.y, z:roofInfo.minHeight});
            faces.push({x: nextP.x, y: nextP.y, z:roofInfo.minHeight});
            faces.push({x: nextPrj.x, y: nextPrj.y, z:roofInfo.maxHeight});

            faces.push({x: p.x, y: p.y, z:roofInfo.minHeight});
            faces.push({x: nextPrj.x, y: nextPrj.y, z:roofInfo.maxHeight});
            faces.push({x: prj.x, y: prj.y, z:roofInfo.maxHeight});
        } else {
            if (prj.x===nextPrj.x && prj.y===nextPrj.y) {
                faces.push({x: p.x, y: p.y, z:roofInfo.minHeight});
                faces.push({x: nextP.x, y: nextP.y, z:roofInfo.minHeight});
                faces.push({x: prj.x, y: prj.y, z:roofInfo.maxHeight});
            } else {
                faces.push({x: p.x, y: p.y, z:roofInfo.minHeight});
                faces.push({x: prj.x, y: prj.y, z:roofInfo.minHeight});
                faces.push({x: prj.x, y: prj.y, z:roofInfo.maxHeight});

                faces.push({x: prj.x, y: prj.y, z:roofInfo.minHeight});
                faces.push({x: nextPrj.x, y: nextPrj.y, z:roofInfo.minHeight});
                faces.push({x: nextPrj.x, y: nextPrj.y, z:roofInfo.maxHeight});

                faces.push({x: prj.x, y: prj.y, z:roofInfo.minHeight});
                faces.push({x: nextPrj.x, y: nextPrj.y, z:roofInfo.maxHeight});
                faces.push({x: prj.x, y: prj.y, z:roofInfo.maxHeight});

                faces.push({x: nextPrj.x, y: nextPrj.y, z:roofInfo.minHeight});
                faces.push({x: nextP.x, y: nextP.y, z:roofInfo.minHeight});
                faces.push({x: nextPrj.x, y: nextPrj.y, z:roofInfo.maxHeight});                
            }
        }

        //console.log("PRJ", prj);
    }

    return faces;
}


const createComplexPolygonRoof = (outerPolygon, innerPolygons, roofInfo) => {

    const topFace=getTrianglesFromComplexPolygon(outerPolygon.points, innerPolygons);
    topFace.forEach(t => t.z=roofInfo.maxHeight);

    return topFace;
}

const buildingFromWay = (way, polygon, localBoundingBox, convexHull, orientedMinBoundingBox) => {

    const buildingInfo = getBuildingInfo(way.tags);

    //Compute lateral faces
    const lateralFaces = extrudePoly(polygon, buildingInfo.minHeight, buildingInfo.maxHeight, USE_TRIANGLES_STRIP);
    //Compute roof faces
    const roofFaces = createRoof(polygon, buildingInfo.roof, convexHull, orientedMinBoundingBox);

    //Create building mesh
    const building = createBuildingMesh("w-"+way.id, way.tags, localBoundingBox, lateralFaces, roofFaces, buildingInfo);
    return building;
}

const buildingFromRel = (rel,polygons,localBoundingBox) => {

    const buildingInfo = getBuildingInfo(rel.tags);

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
    const building = createBuildingMesh("r-"+rel.id, rel.tags, localBoundingBox, lateralFaces, roofFaces, buildingInfo);
    return building;
}

module.exports = { createBuildings }
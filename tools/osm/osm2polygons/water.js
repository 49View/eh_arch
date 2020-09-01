const { Vector} = require('./vector');
const Decimal = require('decimal.js');
Decimal.set({ precision: 20, rounding: 1 });
const {
    getTrianglesFromPolygon,
    getTrianglesFromComplexPolygon,
    getPolygonFromWay,
    getPolygonsFromMultipolygonRelation,
    createMesh,
    createElementsFromWays,
    createElementsFromRels,
    setHeight
} = require('./osmHelper.js');

const createWater = (nodes,ways,rels) => {
    console.log("----------------------------------------------");
    console.log("WATER");
    console.log("----------------------------------------------");

    const simpleWater=createElementsFromWays(ways
        , w => w.tags && w.tags["natural"] && w.tags["natural"]==="water"
        , waterFromWay);
    // const simpleBuildings=[];
    const complexWater=createElementsFromRels(rels
        , r => r.tags && r.tags["natural"] && r.tags["natural"]==="water"
        , waterFromRel);
    
    console.log(`Found ${simpleWater.length} simple water elements`);
    console.log(`Found ${complexWater.length} complex water elements`);
    console.log("----------------------------------------------");

    return simpleWater.concat(complexWater);
}

const createWaterMesh = (id, tags, boundingBox, faces) => {

    const groups=[];

    groups.push({
        faces: faces,
        colour: "#AAD3DF",
        isTriangleStrip: false
    });
    
    return createMesh(id, tags, "water", boundingBox, groups);    
}

const waterFromWay = (way) => {

    const {polygon,localBoundingBox,convexHull,orientedMinBoundingBox}=getPolygonFromWay(way);
    const faces = getTrianglesFromPolygon(polygon);
    setHeight(faces,0.1);

    const water = createWaterMesh("w-"+way.id, way.tags, localBoundingBox, faces);
    return water;
}

const waterFromRel = (rel) => {

    const {polygons,localBoundingBox} = getPolygonsFromMultipolygonRelation(rel);
    let faces=[];
    polygons.filter(p => p.role==="outer").forEach(o => {
        faces = faces.concat(getTrianglesFromComplexPolygon(o.points, o.holes));
        setHeight(faces,0.1);
    });

    const water = createWaterMesh("r-"+rel.id, rel.tags, localBoundingBox, faces);

    return water;
}

module.exports = { createWater }
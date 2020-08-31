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
    createElementsFromRels
} = require('./osmHelper.js');

const createParks = (nodes,ways,rels) => {
    console.log("----------------------------------------------");
    console.log("PARKS");
    console.log("----------------------------------------------");

    const simpleParks=createElementsFromWays(ways
        , w => w.tags && w.tags["leisure"] && (w.tags["leisure"]==="park" || w.tags["leisure"]==="garden")
        , parkFromWay);
    // const simpleBuildings=[];
    const complexParks=createElementsFromRels(rels
        , r => r.tags && r.tags["leisure"] && (r.tags["leisure"]==="park" || r.tags["leisure"]==="garden")
        , parkFromRel);
    
    console.log(`Found ${simpleParks.length} simple parks`);
    console.log(`Found ${complexParks.length} complex parks`);
    console.log("----------------------------------------------");

    return simpleParks.concat(complexParks);
}

const createParkMesh = (id, tags, boundingBox, faces) => {

    const groups=[];

    groups.push({
        faces: faces,
        colour: "#CDF7C9",
        isTriangleStrip: false
    });
    
    return createMesh(id, tags, "park", boundingBox, groups);    
}

const parkFromWay = (way) => {

    const {polygon,localBoundingBox,convexHull,orientedMinBoundingBox}=getPolygonFromWay(way);
    const faces = getTrianglesFromPolygon(polygon);

    const park = createParkMesh("w-"+way.id, way.tags, localBoundingBox, faces);
    return park;
}

const parkFromRel = (rel) => {

    const {polygons,localBoundingBox} = getPolygonsFromMultipolygonRelation(rel);
    let faces=[];
    polygons.filter(p => p.role==="outer").forEach(o => {
        faces = faces.concat(getTrianglesFromComplexPolygon(o.points, o.holes));
    });

    const park = createParkMesh("r-"+rel.id, rel.tags, localBoundingBox, faces);

    return park;
}

module.exports = { createParks }
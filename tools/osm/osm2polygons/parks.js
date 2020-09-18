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

const createParks = (nodes,ways,rels) => {
    console.log("----------------------------------------------");
    console.log("PARKS");
    console.log("----------------------------------------------");

    const simpleParks=createElementsFromWays(ways
        // , w => w.tags && ((w.tags["leisure"] && (w.tags["leisure"]==="park" || w.tags["leisure"]==="garden")) || (w.tags["landuse"] && w.tags["landuse"]==="grass"))
        , w => w.tags && (w.tags["leisure"] || (w.tags["landuse"] && w.tags["landuse"]!=="construction") || (w.tags["area"] && w.tags["man_made"]))
        , parkFromWay);
    // const simpleBuildings=[];
    const complexParks=createElementsFromRels(rels
        //, r => r.tags && ((r.tags["leisure"] && (r.tags["leisure"]==="park" || r.tags["leisure"]==="garden")) || (r.tags["landuse"] && r.tags["landuse"]==="grass"))
        , r => r.tags && (r.tags["leisure"] || (r.tags["landuse"] && r.tags["landuse"]!=="construction") || (r.tags["area"] && r.tags["man_made"]))
        , parkFromRel);
    
    console.log(`Found ${simpleParks.length} simple parks`);
    console.log(`Found ${complexParks.length} complex parks`);
    console.log("----------------------------------------------");

    return simpleParks.concat(complexParks);
}

const createParkMesh = (id, tags, boundingBox, faces, color) => {

    const groups=[];

    groups.push({
        faces: faces,
        colour: color,
        isTriangleStrip: false
    });
    
    return createMesh(id, tags, "park", boundingBox, groups);    
}

const parkFromWay = (way) => {

    const {polygon,localBoundingBox,convexHull,orientedMinBoundingBox}=getPolygonFromWay(way);
    const faces = getTrianglesFromPolygon(polygon);
    setHeight(faces,0);

    let color = "#808080";

    if ((way.tags["landuse"] && way.tags["landuse"]==="grass")
        || (way.tags["leisure"] && (way.tags["leisure"]==="park" || way.tags["leisure"]==="garden"))) {
            color="#CDF7C9";
        }

    const park = createParkMesh("w-"+way.id, way.tags, localBoundingBox, faces, color);
    return park;
}

const parkFromRel = (rel) => {

    const {polygons,localBoundingBox} = getPolygonsFromMultipolygonRelation(rel);
    let faces=[];
    polygons.filter(p => p.role==="outer").forEach(o => {
        faces = faces.concat(getTrianglesFromComplexPolygon(o.points, o.holes));
        setHeight(faces,0);
    });


    let color = "#808080";

    if ((rel.tags["landuse"] && rel.tags["landuse"]==="grass")
        || (rel.tags["leisure"] && (rel.tags["leisure"]==="park" || rel.tags["leisure"]==="garden"))) {
            color="#CDF7C9";
        }

    const park = createParkMesh("r-"+rel.id, rel.tags, localBoundingBox, faces, color);

    return park;
}

module.exports = { createParks }
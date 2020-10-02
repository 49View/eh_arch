const { Vector} = require('./vector');
const Decimal = require('decimal.js');
const {groupFromRel} = require("./osmHelper");
const {groupFromWay} = require("./osmHelper");
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

    const simpleParks=createElementsFromWays(ways, "park"
        // , w => w.tags && ((w.tags["leisure"] && (w.tags["leisure"]==="park" || w.tags["leisure"]==="garden")) || (w.tags["landuse"] && w.tags["landuse"]==="grass"))
        , w => w.tags && (w.tags["leisure"] || (w.tags["landuse"] && w.tags["landuse"]!=="construction") || (w.tags["area"] && w.tags["man_made"] && !w.tags["ferry"]))
        , groupFromWay);

    const complexParks=createElementsFromRels(rels, "park"
        //, r => r.tags && ((r.tags["leisure"] && (r.tags["leisure"]==="park" || r.tags["leisure"]==="garden")) || (r.tags["landuse"] && r.tags["landuse"]==="grass"))
        , r => r.tags && (r.tags["leisure"] || (r.tags["landuse"] && r.tags["landuse"]!=="construction") || (r.tags["area"] && r.tags["man_made"] && !r.tags["ferry"]))
        , groupFromRel);

    console.log(`Found ${simpleParks.length} simple parks`);
    console.log(`Found ${complexParks.length} complex parks`);
    console.log("----------------------------------------------");

    return simpleParks.concat(complexParks);
}

module.exports = { createParks }

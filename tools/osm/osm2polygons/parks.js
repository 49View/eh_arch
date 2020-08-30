const { Vector} = require('./vector');
const Decimal = require('decimal.js');
Decimal.set({ precision: 20, rounding: 1 });
const {
    getTrianglesFromPolygon,
    getTrianglesFromComplexPolygon,
    extrudePoly,
    calcPointProjection,
    getPolygonFromWay,
    getPolygonsFromMultipolygonRelation,
    createMesh
} = require('./osmHelper.js');

const createParks = (nodes,ways,rels) => {
    console.log("----------------------------------------------");
    console.log("PARKS");
    console.log("----------------------------------------------");

    const simpleParks=createSimpleParks(ways.filter(w => w.tags && w.tags["leisure"] && (w.tags["leisure"]==="park" || w.tags["leisure"]==="garden")));
    // const simpleBuildings=[];
    const complexParks=createComplexParks(rels.filter(r => r.tags && r.tags["leisure"] && (r.tags["leisure"]==="park" || r.tags["leisure"]==="garden")));

    console.log(`Found ${simpleParks.length} simple parks`);
    console.log(`Found ${complexParks.length} complex parks`);
    console.log("----------------------------------------------");

    return simpleParks.concat(complexParks);
}

const createSimpleParks = (ways) => {
    return [];
}

const createComplexParks = (rels) => {
    return [];
}

module.exports = { createParks }
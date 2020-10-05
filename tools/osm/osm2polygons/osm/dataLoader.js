const axios = require('axios')
const qs = require('qs');
const fs = require('fs');
const Decimal = require('decimal.js');
const {convertElementFacesToTriangles} = require("./dataTransformer");
Decimal.set({ precision: 20, rounding: 1 })
const OVERPASSAPI_URL="http://overpass-api.de/api/interpreter";

function lon2tile(lon,zoom) { return (Math.floor((lon+180)/360*Math.pow(2,zoom))); }
function lat2tile(lat,zoom)  { return (Math.floor((1-Math.log(Math.tan(lat*Math.PI/180) + 1/Math.cos(lat*Math.PI/180))/Math.PI)/2 *Math.pow(2,zoom))); }

function tile2long(x,z) {
    return (x/Math.pow(2,z)*360-180);
}
function tile2lat(y,z) {
    const n = Math.PI - 2 * Math.PI * y / Math.pow(2, z);
    return (180/Math.PI*Math.atan(0.5*(Math.exp(n)-Math.exp(-n))));
}

RAD2DEG = 180 / Math.PI;
PI_4 = Math.PI / 4;
const TILE_SIZE = 256;

// const lat2y = lat => {
//     return Math.log(Math.tan((lat / 90 + 1) * PI_4 )) * RAD2DEG;
// }

// The mapping between latitude, longitude and pixels is defined by the web
// mercator projection.
function project(lat, lon) {
    let sinY = Math.sin((lat * Math.PI) / 180);

    // Truncating to 0.9999 effectively limits latitude to 89.189. This is
    // about a third of a tile past the edge of the world tile.
    sinY = Math.min(Math.max(sinY, -0.9999), 0.9999);

    return {
        x: TILE_SIZE * (0.5 + lon / 360),
        y: TILE_SIZE * (0.5 - Math.log((1 + sinY) / (1 - sinY)) / (4 * Math.PI))
    }
}

const getBoundingBox = (lat, lon, distance, zoom) => {

    const tileX = lon2tile(lon, zoom);
    const tileY = lat2tile(lat, zoom);

    console.log(`${lat} ${lon}`);
    console.log(`${tileX} ${tileY}`);

    console.log(`TopLeft     ${tile2long(tileX, zoom)} ${tile2lat(tileY, zoom)}`)
    console.log(`TopRight    ${tile2long(tileX+1, zoom)} ${tile2lat(tileY, zoom)}`)
    console.log(`BottomLeft  ${tile2long(tileX, zoom)} ${tile2lat(tileY+1, zoom)}`)
    console.log(`BottomRight ${tile2long(tileX+1, zoom)} ${tile2lat(tileY+1, zoom)}`)
    console.log(`Center      ${tile2long(tileX+0.5, zoom)} ${tile2lat(tileY+0.5, zoom)}`)

    console.log(`Zero        ${tile2long(0,0)} ${tile2lat(0,0)}`)

    const topLat    = tile2lat(tileY+1, zoom);
    const topLon    = tile2long(tileX, zoom);
    const bottomLat = tile2lat(tileY, zoom);
    const bottomLon = tile2long(tileX+1, zoom);

    const centerLat = tile2lat(tileY+0.5, zoom);
    const centerLon = tile2long(tileX+0.5, zoom);

    return {
        bbox: [
            topLat   ,
            topLon   ,
            bottomLat,
            bottomLon
        ],
        center: {
            ...project(centerLat, centerLon),
            lat: centerLat,
            lon: centerLon
        },
        zero: {
            lat: tile2lat(0,0),
            lon: tile2long(0,0)
        }
    };
}

// const getLatLonFromPointDistance = (lat, lon, distance) => {
//
//     const LATITUDEKM = 110.574;
//     const LONGITUDEKM = 111.320;
//
//     const latResult = lat+(distance/LATITUDEKM);
//     const lonResult = lon+(distance/(LONGITUDEKM*Math.cos(lat/180*Math.PI)));
//
//     return [latResult,lonResult];
// }

const getData = async (bbox) => {

    const query=`[out:json][timeout:25];
    (
        rel["building"](${bbox.join(",")});
        rel["building:part"](${bbox.join(",")});
        way["building"](${bbox.join(",")});
        way["building:part"](${bbox.join(",")});
        rel["natural"="water"](${bbox.join(",")});
        way["natural"="water"](${bbox.join(",")});
        way["barrier"](${bbox.join(",")});
        way["highway"](${bbox.join(",")});
        way["railway"="rail"](${bbox.join(",")});
        way["amenity"="parking"](${bbox.join(",")});
        way["landuse"](${bbox.join(",")});
        rel["landuse"](${bbox.join(",")});
        way["leisure"](${bbox.join(",")});
        rel["leisure"](${bbox.join(",")});
        way["area"](${bbox.join(",")});
        rel["area"](${bbox.join(",")});
        node["natural"="tree"](${bbox.join(",")});
    );
    (
        ._;
        >;
    );
    out;
    `;

    console.log(query);

    const options = {
        method: "POST",
        headers: { 'content-type': 'application/x-www-form-urlencoded' },
        data: qs.stringify({data:query})
    };
    //console.log(qs.stringify({data: query}));
    const response=await axios(OVERPASSAPI_URL, options);
    const osmData=response.data;
    fs.writeFileSync("testData.json", JSON.stringify(osmData), { encoding: "utf8"});
    return prepareData(osmData);
}

const getDataLocal = () => {
    const buffer = fs.readFileSync("testData.json");
    const osmData = JSON.parse(buffer.toString());
    return prepareData(osmData);
}

const prepareData = (osmData) => {
    //Group loaded element for type
    const {nodes,ways,rels} = parseData(osmData);

    return {nodes,ways,rels}
}


const parseData = (osmData) => {
    const elements = osmData.elements;
    const nodes = [];
    const ways = [];
    const rels = [];

    elements.forEach((e,i) => {
        if (e.type==="node") nodes.push(e)
        else if (e.type==="way") ways.push(e)
        else if (e.type==="relation") rels.push(e)
        else console.log(`Element ${i} unknown`);
    });

    return {nodes,ways,rels}
}

const exportTile = elements => {

    convertElementFacesToTriangles(elements);

    const jsonOutput = JSON.stringify({elements}, null, 4);
    fs.writeFileSync("../osmdebug/src/elements.json", jsonOutput, {encoding: "utf8"});
    fs.writeFileSync("elements.json", jsonOutput, {encoding: "utf8"});
    fs.writeFileSync("../../../../f9.com/builds/wasm_renderer/debug/elements.json", jsonOutput, {encoding: "utf8"});
}

module.exports = {getBoundingBox,getData,getDataLocal,exportTile}


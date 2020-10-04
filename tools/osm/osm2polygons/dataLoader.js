const axios = require('axios')
const qs = require('qs');
const fs = require('fs');
const Decimal = require('decimal.js');
Decimal.set({ precision: 20, rounding: 1 })
const OVERPASSAPI_URL="http://overpass-api.de/api/interpreter";

const LATITUDEKM = 110.574;
const LONGITUDEKM = 111.320;

const getBoundingBox = (lat, lon, distance) => {

    let result=[];

    result = result.concat(getLatLonFromPointDistance(lat,lon,-distance));
    result = result.concat(getLatLonFromPointDistance(lat,lon,distance));

    return result;
}

const getLatLonFromPointDistance = (lat, lon, distance) => {

    const latResult = lat+(distance/LATITUDEKM);
    const lonResult = lon+(distance/(LONGITUDEKM*Math.cos(lat/180*Math.PI)));

    return [latResult,lonResult];
}

const getData = async (bbox) => {

        // way["landuse"="grass"](${bbox.join(",")});
        // rel["landuse"="grass"](${bbox.join(",")});
        // rel["leisure"="park"](${bbox.join(",")});
        // rel["leisure"="garden"](${bbox.join(",")});
        // way["leisure"="park"](${bbox.join(",")});
        // way["leisure"="garden"](${bbox.join(",")});
        const query=`
    [out:json][timeout:25];
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
    osmData=response.data;
    fs.writeFileSync("testData.json", JSON.stringify(osmData), { encoding: "utf8"});
    return prepareData(osmData);
}

const getDataLocal = (bbox) => {
    const osmData = JSON.parse(fs.readFileSync("testData.json", { encoding: "utf8"}));
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


module.exports = {getBoundingBox,getData,getDataLocal}


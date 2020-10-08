const axios = require('axios')
const qs = require('qs');
const fs = require('fs');
const {convertElementFacesToTriangles} = require("./dataTransformer");
const OVERPASSAPI_URL="http://overpass-api.de/api/interpreter";


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
        rel["highway"](${bbox.join(",")});
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

module.exports = {getData,getDataLocal,exportTile}


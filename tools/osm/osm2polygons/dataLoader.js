const axios = require('axios')
const qs = require('qs');
const fs = require('fs');
const Decimal = require('decimal.js');
Decimal.set({ precision: 20, rounding: 1 })
const OVERPASSAPI_URL="http://overpass-api.de/api/interpreter";

const getData = async (bbox) => {

    const query=`
    [out:json][timeout:25];
    (
        rel["building"](${bbox.join(",")});
        rel["building:part"](${bbox.join(",")});
        way["building"](${bbox.join(",")});
        way["building:part"](${bbox.join(",")});
        rel["leisure"="park"](${bbox.join(",")});
        rel["leisure"="garden"](${bbox.join(",")});
        way["leisure"="park"](${bbox.join(",")});
        way["leisure"="garden"](${bbox.join(",")});
        rel["natural"="water"](${bbox.join(",")});
        way["natural"="water"](${bbox.join(",")});
        way["highway"](${bbox.join(",")});
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


module.exports = {getData,getDataLocal}


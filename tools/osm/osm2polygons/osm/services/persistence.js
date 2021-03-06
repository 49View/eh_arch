import {osmModel} from "../models/osm";

const axios = require('axios')
const qs = require('qs');
const fs = require('fs');
const {graphTypeRel, graphTypeWay, graphTypeNode, tagBarrier} = require("./nameValues");
const OVERPASSAPI_URL="http://overpass-api.de/api/interpreter";

export const getData = async (bbox, useCache) => {

    if (useCache) {
        return getDataLocal();
    }

    const query=`[out:json][timeout:25];
    (
        way["building"](${bbox.join(",")});
        way["building:part"](${bbox.join(",")});
        way["highway"](${bbox.join(",")});
        way["railway"="rail"](${bbox.join(",")});
        way["amenity"="parking"](${bbox.join(",")});
        way["landuse"](${bbox.join(",")});
        way["natural"](${bbox.join(",")});
        way["leisure"](${bbox.join(",")});
        way["area"](${bbox.join(",")});
        way[${tagBarrier}](${bbox.join(",")});
        
        rel["building"](${bbox.join(",")});
        rel["building:part"](${bbox.join(",")});
        rel["highway"](${bbox.join(",")});
        rel["natural"](${bbox.join(",")});        
        rel[${tagBarrier}](${bbox.join(",")});
        rel["landuse"](${bbox.join(",")});
        rel["leisure"](${bbox.join(",")});
        rel["area"](${bbox.join(",")});
        
        node["natural"](${bbox.join(",")});
        node["historic"](${bbox.join(",")});
        node["amenity"](${bbox.join(",")});
        node["highway"](${bbox.join(",")});        
        node[${tagBarrier}](${bbox.join(",")});
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
        if (e.type===graphTypeNode) nodes.push(e)
        else if (e.type===graphTypeWay) ways.push(e)
        else if (e.type===graphTypeRel) rels.push(e)
        else console.log(`Element ${i} unknown`);
    });

    return {nodes,ways,rels}
}

export const exportTile = async (tileBoundary, elements) => {

    const bulkOps = elements.map(update => ({
        updateOne: {
            filter: { id: update.id },
            update: { $set: update },
            upsert: true
        }
    }));
    const results = await osmModel.collection.bulkWrite(bulkOps);

    console.log( "Write result: " + JSON.stringify(results) );
    //console.log( "Updating database: " + (results.BulkWriteResult.result.ok === 1 ? "OK" : results) );

    // for ( const elem of elements ) {
    //     await mapModel.findOneAndUpdate({id: elem.id}, elem, {
    //         upsert: true,
    //         new: true,
    //         setDefaultsOnInsert: true
    //     });
    // }
    // console.log(elements.length + " map entities exported")
}


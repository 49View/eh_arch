const poly2tri = require('poly2tri');
const fs=require('fs');
const {getDataLocal}=require("./dataLoader");
const {createBuildings}=require("./buildings");

const bbox=[51.49634,-0.12999,51.50088,-0.11995];


const main = async () => {
    const {nodes,ways,rels} = await getDataLocal(bbox);
    // extendData(nodes,ways,rels);
    const buildings = createBuildings(nodes,ways,rels);

    fs.writeFileSync("dataExtend.json", JSON.stringify({nodes,ways,rels},null,4), {options:"utf8"});
    fs.writeFileSync("buildings.json", JSON.stringify({buildings},null,4), {options:"utf8"});

    console.log(`Found ${nodes.length} nodes`);
    console.log(`Found ${ways.filter(w => w.inRelation===true).length} ways in relation`);
    console.log(`Found ${ways.filter(w => w.inRelation===false).length} ways not in relation`);
    console.log(`Found ${rels.length} relations`);
}


main().then(r => {
    console.log("----------------------------------------------");
    console.log("Successfully executed")
}).catch(ex => {
    console.log("Error",ex)
})
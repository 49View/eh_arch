const poly2tri = require('poly2tri');
const fs = require('fs');
const {getData} = require("./dataLoader");
const {createBuildings} = require("./buildings");
const {createParks} = require("./parks");

const bbox = [51.4892, -0.1344, 51.5016, -0.1034];


const main = async () => {
  const {nodes, ways, rels} = await getData(bbox);
  // extendData(nodes,ways,rels);
  let elements = [];
  
  elements = elements.concat(createBuildings(nodes, ways, rels));
  elements = elements.concat(createParks(nodes, ways, rels));

  fs.writeFileSync("dataExtend.json", JSON.stringify({nodes,ways,rels},null,4), {options:"utf8"});
  fs.writeFileSync("../osmdebug/src/elements.json", JSON.stringify({elements},null,4), {options:"utf8"});
  fs.writeFileSync("elements.json", JSON.stringify({elements},null,4), {options:"utf8"});

 

}


main().then(r => {
  console.log("----------------------------------------------");
  console.log("Successfully executed")
}).catch(ex => {
  console.log("Error", ex)
})

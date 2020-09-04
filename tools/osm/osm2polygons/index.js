const poly2tri = require('poly2tri');
const fs = require('fs');
const {getData,getDataLocal} = require("./dataLoader");
const {createBuildings} = require("./buildings");
const {createParks} = require("./parks");
const {createWater} = require("./water");
const {elaborateData} = require("./dataTransformer");

//WestMinster
const bbox = [51.4892, -0.1344, 51.5016, -0.1034];
//Battersea Park
//const bbox = [51.4696, -0.1777, 51.4878, -0.1375];
//Royal Albert Hall
//const bbox = [51.50049, -0.17869, 51.50155, -0.17551];
//Instanbul
//const bbox = [28.97342,41.00382,28.98050,41.00739];
//San Pietro
//const bbox = [41.90024, 12.45045, 41.90379, 12.45905];
// Corniche
// const bbox = [51.49045, -0.12262, 51.49139, -0.12080];


const main = async () => {
  //const {nodes, ways, rels} = await getData(bbox);
  const {nodes, ways, rels} = await getDataLocal(bbox);
  elaborateData(nodes, ways, rels);

  // extendData(nodes,ways,rels);
  let elements = [];

  elements = elements.concat(createBuildings(nodes, ways, rels));
  elements = elements.concat(createParks(nodes, ways, rels));
  elements = elements.concat(createWater(nodes, ways, rels));

  elements.forEach( e => {
    e.groups.forEach( g=> {
      g.triangles = g.faces.map( f => [ f.x, f.y, f.z] );
      delete g.faces;
    })
  })

  //fs.writeFileSync("dataExtend.json", JSON.stringify({nodes,ways,rels},null,4), {options:"utf8"});
  fs.writeFileSync("../osmdebug/src/elements.json", JSON.stringify({elements},null,4), {options:"utf8"});
  fs.writeFileSync("elements.json", JSON.stringify({elements},null,4), {options:"utf8"});



}


main().then(r => {
  console.log("----------------------------------------------");
  console.log("Successfully executed")
}).catch(ex => {
  console.log("Error", ex)
})

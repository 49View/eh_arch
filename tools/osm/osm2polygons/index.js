const {exportTile} = require("./osm/dataLoader");
const {createTile} = require("./osm/tileArea");
const {getDataLocal} = require("./osm/dataLoader");
const {getBoundingBox, getData} = require("./osm/dataLoader");
const {elaborateData} = require("./osm/dataTransformer");

//WestMinster
// const bbox = [51.4892, -0.1344, 51.5016, -0.1034];
//Battersea Park
//const bbox = [51.4696, -0.1777, 51.4878, -0.1375];
//Royal Albert Hall
//const bbox = [51.50049, -0.17869, 51.50155, -0.17551];
//Instanbul
//const bbox = [28.97342,41.00382,28.98050,41.00739];
//San Pietro
//const bbox = [41.90024, 12.45045, 41.90379, 12.45905];
// Corniche
//const bbox = [51.49045, -0.12262, 51.49139, -0.12080];

const main = async () => {

  const tileBoundary = getBoundingBox(51.4992784, -0.125376, .3, 15);

  // const {nodes, ways, rels} = await getData(tileBoundary.bbox);
  const {nodes, ways, rels} = await getDataLocal();
  elaborateData(tileBoundary, nodes, ways, rels);

  const elements = createTile(nodes, ways, rels);

  exportTile(elements);
}


main().then(() => {
  console.log("----------------------------------------------");
  console.log("Successfully executed")
}).catch(ex => {
  console.log("Error", ex)
})

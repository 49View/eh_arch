const {elaborateData} = require("./osm/nodeGraph");
const {getBoundingBox} = require("./osm/coordinates");
const {exportTile} = require("./osm/persistence");
const {createTile} = require("./osm/tile");
const {getDataLocal} = require("./osm/persistence");
const {getData} = require("./osm/persistence");

//WestMinster
// const bbox = [51.4992784, -0.125376, 15];
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

const main = async (args) => {

  const useCache = (args.length === 4 && args[3] === "useCache");
  const coords = {lat: args.length===0 ? 51.4992784 : Number(args[0]), lon:  args.length===0 ? -0.125376 : Number(args[1]), zoom: args.length===0 ? 15 : Number(args[2])};

  const tileBoundary = getBoundingBox({...coords});
  const {nodes, ways, rels} = useCache ? await getDataLocal() : await getData(tileBoundary.bbox);
  elaborateData(tileBoundary, nodes, ways, rels);
  const elements = createTile(tileBoundary, nodes, ways, rels);

  exportTile(elements);
}

main(process.argv.slice(2)).then(() => {
  console.log("----------------------------------------------");
  console.log("Successfully executed")
}).catch(ex => {
  console.log("Error", ex)
})

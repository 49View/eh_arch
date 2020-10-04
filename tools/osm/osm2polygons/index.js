const fs = require('fs');
const {getDataLocal} = require("./dataLoader");
const {createTileAreas} = require("./tileArea");
const {getBoundingBox, getData} = require("./dataLoader");
const {createBuildings} = require("./buildings");
const {elaborateData} = require("./dataTransformer");

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

const parkFilter = w => {
  return w.tags && (w.tags["leisure"] || (w.tags["landuse"] && (w.tags["landuse"] !== "construction" && w.tags["landuse"] !== "governmental")) || (w.tags["area"] && w.tags["man_made"] && !w.tags["ferry"]));
}

const parkingFilter = w => {
  return w.tags && (w.tags["amenity"] === "parking");
}

const waterFilter = w => {
  return w.tags && w.tags["natural"] === "water";
}

const fenceFilter = w => {
  return w.tags && w.tags["barrier"];
}

const roadFilter = w => {
  return w.tags && (w.tags["highway"]);
}

const treeFilter = w => {
  return w.tags && (w.tags["natural"] === "tree");
}

const unclassifiedFilter = w => {
  return w.tags === undefined && w.nodes && w.nodes.length > 0;
}

const addTileAreaFilter = (name, areaFilter) => {
  return {
    name,
    areaFilter
  }
}

const main = async () => {

  const bbox = getBoundingBox(51.4992784, -0.125376, .3);

  // const {nodes, ways, rels} = await getData(bbox);
  const {nodes, ways, rels} = await getDataLocal(bbox);
  elaborateData(nodes, ways, rels);

  let elements = [];

  elements = elements.concat(createBuildings(nodes, ways, rels));

  const tileAreas = [
    addTileAreaFilter("unclassified", unclassifiedFilter),
    addTileAreaFilter("park", parkFilter),
    addTileAreaFilter("parking", parkingFilter),
    addTileAreaFilter("water", waterFilter),
    addTileAreaFilter("barrier", fenceFilter),
    addTileAreaFilter("road", roadFilter),
    // addTileAreaFilter("tree", treeFilter),
  ]

  for (const tf of tileAreas) {
    elements = elements.concat(createTileAreas(tf, nodes, ways, rels));
  }

  elements.forEach(e => {
    if ( e.groups ) {
      e.groups.forEach(g => {
        if (g.faces) {
          g.triangles = g.faces.map(f => [f.x, f.y, f.z]);
          delete g.faces;
        }
      })
    }
  })

  const jsonOutput = JSON.stringify({elements}, null, 4);
  fs.writeFileSync("../osmdebug/src/elements.json", jsonOutput, {options: "utf8"});
  fs.writeFileSync("elements.json", jsonOutput, {options: "utf8"});
  fs.writeFileSync("../../../../f9.com/builds/wasm_renderer/debug/elements.json", jsonOutput, {options: "utf8"});
}


main().then(() => {
  console.log("----------------------------------------------");
  console.log("Successfully executed")
}).catch(ex => {
  console.log("Error", ex)
})

import {initDB} from "eh_db";
import {elaborateData} from "./osm/services/nodeGraph";
import {getBoundingBox} from "./osm/services/coordinates";
import {exportTile, getData} from "./osm/services/persistence";
import {createTile} from "./osm/services/tile";

const main = async (args) => {

  initDB().then();

  const useCache = (args.length === 4 && args[3] === "useCache");
  const coords = {lat: args.length===0 ? 51.4992784 : Number(args[0]), lon:  args.length===0 ? -0.125376 : Number(args[1]), zoom: args.length===0 ? 15 : Number(args[2])};

  const tileBoundary = getBoundingBox({...coords});
  console.log(tileBoundary);
  const {nodes, ways, rels} = await getData(tileBoundary.bbox, useCache);
  elaborateData(tileBoundary, nodes, ways, rels);
  const elements = createTile(tileBoundary, nodes, ways, rels);

  await exportTile(tileBoundary, elements);
}

main(process.argv.slice(2)).then(() => {
  console.log("----------------------------------------------");
  console.log("Successfully executed")
  process.exit(0);
}).catch(ex => {
  console.log("Error", ex)
})

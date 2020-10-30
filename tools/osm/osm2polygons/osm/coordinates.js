// RAD2DEG = 180 / Math.PI;
// PI_4 = Math.PI / 4;
//
// const lat2y = lat => {
//     return Math.log(Math.tan((lat / 90 + 1) * PI_4 )) * RAD2DEG;
// }

function lon2tile(lon,zoom) { return (Math.floor((lon+180)/360*Math.pow(2,zoom))); }
function lat2tile(lat,zoom)  { return (Math.floor((1-Math.log(Math.tan(lat*Math.PI/180) + 1/Math.cos(lat*Math.PI/180))/Math.PI)/2 *Math.pow(2,zoom))); }

function tile2long(x,z) {
  return (x/Math.pow(2,z)*360-180);
}
function tile2lat(y,z) {
  const n = Math.PI - 2 * Math.PI * y / Math.pow(2, z);
  return (180/Math.PI*Math.atan(0.5*(Math.exp(n)-Math.exp(-n))));
}

RAD2DEG = 180 / Math.PI;
PI_4 = Math.PI / 4;
const TILE_SIZE = 256;

// const lat2y = lat => {
//     return Math.log(Math.tan((lat / 90 + 1) * PI_4 )) * RAD2DEG;
// }

// The mapping between latitude, longitude and pixels is defined by the web
// mercator projection.
function project(lat, lon) {
  let sinY = Math.sin((lat * Math.PI) / 180);

  // Truncating to 0.9999 effectively limits latitude to 89.189. This is
  // about a third of a tile past the edge of the world tile.
  sinY = Math.min(Math.max(sinY, -0.9999), 0.9999);

  return {
    x: TILE_SIZE * (0.5 + lon / 360),
    y: TILE_SIZE * (0.5 - Math.log((1 + sinY) / (1 - sinY)) / (4 * Math.PI))
  }
}

const calcNodeCoordinates = n => {
  return {
    x: calcDistance(n.lat, 0, n.lat, n.lon),
    y: calcDistance(0, n.lon, n.lat, n.lon)
  }
}

const calcCoordinate = (nodes) => {
  nodes.forEach(n => {
    const nc = calcNodeCoordinates(n);
    n.x = nc.x;
    n.y = nc.y;
  })
}

const calcDistance = (latitude1, longitude1, latitude2, longitude2) => {

  const lat1 = latitude1;
  const lon1 = longitude1;
  const lat2 = latitude2;
  const lon2 = longitude2;
  const R = 6371e3; // metres
  const phi1 = lat1 * Math.PI / 180; // φ, λ in radians
  const phi2 = lat2 * Math.PI / 180;
  const deltaPhi = (lat2 - lat1) * Math.PI / 180;
  const deltaLambda = (lon2 - lon1) * Math.PI / 180;

  const a = Math.sin(deltaPhi / 2) * Math.sin(deltaPhi / 2) +
    Math.cos(phi1) * Math.cos(phi2) *
    Math.sin(deltaLambda / 2) * Math.sin(deltaLambda / 2);
  const c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a));

  // in metres
  return (R * c);
}

const serializeLocation = (lon, lat) => {
  return {
    type: "Point",
    coordinates:[lon, lat]
  }
}

const getBoundingBox = (coords) => {

  const {lat, lon, zoom} = {...coords};
  const tileX = lon2tile(lon, zoom);
  const tileY = lat2tile(lat, zoom);

  const topLat    = tile2lat(tileY+1, zoom);
  const topLon    = tile2long(tileX, zoom);
  const bottomLat = tile2lat(tileY, zoom);
  const bottomLon = tile2long(tileX+1, zoom);

  const centerLat = tile2lat(tileY+0.5, zoom);
  const centerLon = tile2long(tileX+0.5, zoom);

  const sizeX = calcDistance(topLat, topLon, topLat, bottomLon);
  const sizeY = calcDistance(topLat, topLon, bottomLat, topLon);

  const halfW = 0.5;
  return {
    zoom,
    tileX,
    tileY,
    bbox: [
      topLat   ,
      topLon   ,
      bottomLat,
      bottomLon
    ],
    center: {
      ...project(centerLat, centerLon),
      lat: centerLat,
      lon: centerLon,
    },
    size: {
      x: sizeX,
      y: sizeY,
    },
    rect: [
      -sizeX,
      0.0,
      0.0,
      sizeY
    ],
    tileClip: [
      {
        X: -sizeX*halfW,
        Y: sizeY*halfW
      },
      {
        X: sizeX*halfW,
        Y: sizeY*halfW
      },
      {
        X: sizeX*halfW,
        Y: -sizeY*halfW
      },
      {
        X: -sizeX*halfW,
        Y: -sizeY*halfW
      },
    ],
    tilePos: {
      x: calcDistance(topLat, topLon, topLat, 0),
      y: calcDistance(topLat, topLon, 0, topLon),
    },
  };
}

const convertToLocalCoordinate = (points, originX, originY) => {
  points.forEach(p => {
    p.x = p.x - originX;
    p.y = p.y - originY;
  });
}

module.exports = {
  calcDistance,
  calcCoordinate,
  convertToLocalCoordinate,
  calcNodeCoordinates,
  getBoundingBox,
  serializeLocation
}

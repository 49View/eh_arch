const {
  getPolygonsFromMultipolygonRelation,
  getPolygonFromWay,
  checkPolygonInsidePolygon
} = require("./osmHelper.js");

// RAD2DEG = 180 / Math.PI;
// PI_4 = Math.PI / 4;
//
// const lat2y = lat => {
//     return Math.log(Math.tan((lat / 90 + 1) * PI_4 )) * RAD2DEG;
// }

const calcCoordinate = (tileBoundary, nodes) => {
  nodes.forEach(n => {
    const ly = n.lat;
    const lx = n.lon;
    n.x = Math.sign(n.lon) * calcDistance(ly, 0, ly, lx);
    n.y = Math.sign(n.lat) * calcDistance(0, lx, ly, lx);
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

const findElement = (list, id) => {
  return list.find(e => e.id === id);
}

const extendData = (nodes, ways, relations) => {

  nodes.forEach(n => {
    n.inWay = false;
    n.inRelation = false;
  })
  ways.forEach(w => {
    w.inRelation = false;
    const wayNodes = [];
    w.nodes.forEach(n => {
      const node = findElement(nodes, n);
      if (node) {
        node.inWay = true;
        wayNodes.push(node);
      }
    })
    w.nodes = wayNodes;
  });

  relations.forEach(r => {
    const relationMembers = [];
    r.members.forEach(m => {
      const member = {
        type: m.type,
        role: m.role
      }
      if (m.type === "way") {
        const way = findElement(ways, m.ref);
        if (way) {
          way.inRelation = true;
          member.ref = way;
        }
      } else if (m.type === "node") {
        const node = findElement(nodes, m.ref);
        if (node) {
          node.inRelation = true;
          member.ref = node;
        }
      }
      if (member.ref) {
        relationMembers.push(member);
      }
    });
    r.members = relationMembers;
  })
}

const elaborateData = (tileBoundary, nodes, ways, rels) => {
  //Calculate coordinate as distance from bbox center
  calcCoordinate(tileBoundary, nodes);
  //Transform reference in ways and relations to data
  extendData(nodes, ways, rels);
  //Compute ways and rels
  computePolygons(ways, rels);

  createPolygonsHierarchy(ways);
}

const computePolygons = (ways, rels) => {

  ways.forEach(w => {
        try {
          w.calc = {...getPolygonFromWay(w)};
        } catch (ex) {
          console.log(`Error in ${w.id} way`, ex);
        }
      }
    );

  rels.forEach(r => {
      try {
        r.calc = {...getPolygonsFromMultipolygonRelation(r)};
      } catch (ex) {
        console.log(`Error in ${r.id} relation`, ex);
      }
    }
  );

}

const createPolygonsHierarchy = (ways) => {

  ways.filter(w => w.calc !== undefined)
    .forEach(wo => {
        wo.children = [];
        ways
          .filter(w => w.calc !== undefined)
          .filter(w => w.id !== wo.id)
          .filter(w => (w.containers && w.containers.findIndex(c => c.id === "w-" + wo.id)))
          .forEach(wi => {

            if (checkPolygonInsidePolygon(wo.calc.absolutePolygon, wi.calc.absolutePolygon)) {
              if (wi.containers === undefined) {
                wi.containers = [];
              }
              wi.containers.push(wo);
              wo.children.push(wi);
            }
          })
      }
    );
}

module.exports = {
  elaborateData
}

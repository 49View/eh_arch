const {graphTypeWay, graphTypeNode, roleInner, roleOuter, getWidthFromWay} = require("./nameValues");
const {calcConvexHull} = require('../geometry/convexhull');
const {calcOmbb} = require('../geometry/ombb');
const {checkPointsOrder, offsetPolyline, computeBoundingBox, removeCollinearPoints, calcTileDelta, checkPolygonInsidePolygon} = require("../geometry/polygon");
const {calcCoordinate, convertToLocalCoordinate} = require("./coordinates");

const createBasePolygon = (points) => {
  const polygon = [];

  points.forEach(p => {
    polygon.push({
      x: p.x,
      y: p.y
    });
  });

  return polygon;
}

const checkPathClosed = (member) => {
  const firstNodeId = member.nodes[0].id;
  const lastNodeId = member.nodes[member.nodes.length - 1].id;
  return firstNodeId === lastNodeId;
}

const createClosedPath = (member, members) => {

  const connectedMembers = [member];
  const role = member.role;
  const firstId = member.ref.id;
  let currentId = member.ref.id;
  let lastNodeId = member.ref.nodes[member.ref.nodes.length - 1].id;

  let closed = false;
  while (!closed) {
    //Search for next member (lastNode = firstNode)
    let nextMember = members.find(m => m.role === role && m.ref.nodes[0].id === lastNodeId);
    if (nextMember === undefined) {
      //Search for next member (lastNode = lastNode)
      nextMember = members.find(m => m.role === role && m.ref.id !== currentId && m.ref.nodes[m.ref.nodes.length - 1].id === lastNodeId);
      if (nextMember !== undefined) {
        //Reverse nextMember nodes order
        nextMember.ref.nodes.reverse();
      }
    }
    if (nextMember !== undefined) {
      if (nextMember.ref.id === firstId) {
        closed = true;
      } else {
        //Search for next node
        currentId = nextMember.ref.id;
        lastNodeId = nextMember.ref.nodes[nextMember.ref.nodes.length - 1].id;
        connectedMembers.push(nextMember);
      }
    } else {
      //Can't close path, INVALID!
      break;
    }
  }
  //Mark members connected as processed
  connectedMembers.forEach(m => m.processed = true);
  //If not closed,  return nothing
  if (!closed) {
    return [];
  }

  return connectedMembers;
}

const isClosedWay = way => {
  const isClosed = way.nodes[0].id === way.nodes[way.nodes.length - 1].id;

  if (way.tags) {
    if (way.tags["area"] === "yes") {
      return true;
    }
    const highway = way.tags['highway'];
    if (highway && (highway !== "pedestrian" && highway !== "footway")) {
      return false;
    }
  }

  return isClosed;
}

const getPolygonFromWay = (tileBoundary, way) => {

  const isClosed = isClosedWay(way);

  const points = removeCollinearPoints(way.nodes);
  if (points.length < 3 && isClosed) {
    throw new Error("Invalid way " + way.id);
  }
  way.spatial = computeBoundingBox(tileBoundary, points);
  convertToLocalCoordinate(points, way.spatial.center.x, way.spatial.center.y);
  //Using: https://github.com/geidav/ombb-rotating-calipers
  //with some modifications

  const convexHull = calcConvexHull(points);
  if ( isClosed ) {
    checkPointsOrder(points);
  }
  const ombb = calcOmbb(convexHull);

  const {roadWidth, roadLane} = getWidthFromWay(way);
  const polygon = isClosed ? createBasePolygon(points) : offsetPolyline(roadWidth * roadLane, points);

  const absolutePolygon = [];
  polygon.forEach(p => {
    absolutePolygon.push({x: p.x + way.spatial.center.x, y: p.y + way.spatial.center.y});
  })

  const polygons = [];
  polygons.push({
    points: polygon,
    holes: null,
    tags: way.tags
  });
  return {polygons, absolutePolygon, convexHull, ombb}
}

const getPolygonsFromMultipolygonRelation = (tileBoundary, rel) => {

  //Clone members for elaboration
  const polygons = [];
  const members = rel.members;// JSON.parse(JSON.stringify(rel.members));
  //Cosed polygons not require elaboration
  members.forEach(m => {
    m.processed = false;
    if (checkPathClosed(m.ref)) {
      m.processed = true;
      const polygon = {
        role: m.role,
        id: m.ref.id,
        tags: m.ref.tags,
        nodes: []
      }
      //Copy point but not last (same than first)
      for (let i = 0; i < m.ref.nodes.length - 1; i++) {
        polygon.nodes.push({...m.ref.nodes[i]});
      }
      polygons.push(polygon);
    }
  });

  //Search and create closed polygons from segments
  while (members.filter(m => m.processed === true).length < members.length) {
    for (let i = 0; i < members.length; i++) {
      if (!members[i].processed) {
        const connectedMembers = createClosedPath(members[i], members);
        if (connectedMembers.length > 0) {
          const polygon = {
            role: connectedMembers[0].role,
            tags: members[i].ref.tags,
            id: members[i].ref.id,
            nodes: []
          }
          connectedMembers.forEach(m => {
            for (let i = 0; i < m.ref.nodes.length - 1; i++) {
              polygon.nodes.push({...m.ref.nodes[i]});
            }
          });
          polygons.push(polygon);
        }
      }
    }
  }

  //Create global array points
  let relPoints = [];
  polygons.forEach(p => {
    p.points = [];
    p.nodes.forEach(n => p.points.push({x: Number(n.x), y: Number(n.y), lat: n.lat, lon: n.lon}));
    relPoints = relPoints.concat(p.points);
  });
  //Compute local bounding box and correct point direction
  rel.spatial = computeBoundingBox(tileBoundary, relPoints);
  polygons.forEach(p => {
    convertToLocalCoordinate(p.points, rel.spatial.center.x, rel.spatial.center.y);
    checkPointsOrder(p.points);
  });
  //Create reference between inner and outer
  polygons.filter(p => p.role === roleOuter).forEach(o => {
    o.holes = [];
    polygons.filter(p => p.role === roleInner && p.container === undefined).forEach(i => {
      if (checkPolygonInsidePolygon(o.points, i.points)) {
        i.container = o;
        o.holes.push(i);
      }
    });
  })

  return {polygons};
}

const createElements = (elements, ways, name, filter, elementCreator) => {
  ways.filter(filter).forEach(w => {
      try {
        elementCreator(elements, w, name);
      } catch (ex) {
        console.log(`Error in ${w.id} ${name}`, ex);
      }
    }
  );
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
      if (m.type === graphTypeWay) {
        const way = findElement(ways, m.ref);
        if (way) {
          way.inRelation = true;
          member.ref = way;
          way.role = m.role;
        }
      } else if (m.type === graphTypeNode) {
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
  calcCoordinate(nodes);
  //Transform reference in ways and relations to data
  extendData(nodes, ways, rels);
  //Compute ways and rels
  computePolygons(tileBoundary, nodes, ways, rels);

  createPolygonsHierarchy(ways);
}

const computePolygons = (tileBoundary, nodes, ways, rels) => {

  nodes.forEach(node => {
    node.spatial = {
      ...calcTileDelta(node, tileBoundary.tilePos),
      lat: node.lat,
      lon: node.lon
    };
  });

  // We filter the ways to be added following these rules:
  // If not in any relationship => add to list
  // If in a relationship then add to the list all the inner ways, as the outer way of a relationship it's rendered either as a clipped polygon or a closed area.
  const notInRelationshipFilter = w => w.inRelation === false;
  const innerWayInRelationshipFilter = w => (w.inRelation === true && w.role && w.role === roleInner);

  const wayFilter = w => notInRelationshipFilter(w) || innerWayInRelationshipFilter(w);

  ways.filter(wayFilter).forEach(w => {
      try {
        w.calc = {...getPolygonFromWay(tileBoundary, w)};
      } catch (ex) {
        console.log(`Error in ${w.id} way`, ex);
      }
    }
  );

  rels.forEach(r => {
      try {
        r.calc = {...getPolygonsFromMultipolygonRelation(tileBoundary, r)};
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
  elaborateData,
  createElements,
}

const poly2tri = require('poly2tri');
const {calcOmbb} = require("./ombb");
const {calcConvexHull} = require("./convexhull");

const triangulate = (swCtx) => {
  swCtx.triangulate();
  const tri = swCtx.getTriangles();
  const points = [];
  tri.forEach(t => {
    points.push({x: t.points_[0].x / 100000, y: t.points_[0].y / 100000});
    points.push({x: t.points_[1].x / 100000, y: t.points_[1].y / 100000});
    points.push({x: t.points_[2].x / 100000, y: t.points_[2].y / 100000});
  });

  return points;
}

const clipPolyPoints = (tileBoundary, polyPoints) => {

  let subj = [];
  let clip = [];
  let solution = [];
  subj.push(polyPoints.map(p => {
    return {X: p.x, Y: p.y}
  }));
  clip.push(tileBoundary.tileClip.map(tc => {
    return {...tc}
  }));

  let co = new ClipperLib.Clipper();
  const scale = 100000;
  ClipperLib.JS.ScaleUpPaths(subj, scale);
  ClipperLib.JS.ScaleUpPaths(clip, scale);
  co.AddPaths(subj, ClipperLib.PolyType.ptSubject, true);
  co.AddPaths(clip, ClipperLib.PolyType.ptClip, true);
  co.Execute(ClipperLib.ClipType.ctIntersection, solution, ClipperLib.PolyFillType.pftNonZero, ClipperLib.PolyFillType.pftNonZero);
  ClipperLib.JS.ScaleDownPaths(solution, scale);

  let polyPointsClipped = [];
  solution.forEach(s => {
    polyPointsClipped = s.map(p => {
      return {x: p.X, y: p.Y}
    });
  });

  return polyPointsClipped;

}

const setHeight = (faces, height) => {
  faces.forEach(f => {
    f.z = height;
  })
}

const createTriangulationContext = (polyPoints) => {
  const intPoints = [];
  polyPoints.forEach(p => {
    intPoints.push({x: p.x * 100000, y: p.y * 100000})
  });

  return new poly2tri.SweepContext(intPoints);
}

const triangulate3d = (swCtx, height) => {
  const faces = triangulate(swCtx);
  setHeight(faces, height);
  return faces;
}

const addHolesToPoly = ( swCtx, innerPolys ) => {
  if ( innerPolys ) {
    const intInnersPoints = [];
    innerPolys.forEach(i => {
      const intInnerPoint = [];
      i.points.forEach(p => {
        intInnerPoint.push({x: p.x * 100000, y: p.y * 100000})
      });
      intInnersPoints.push(intInnerPoint);
    });
    intInnersPoints.forEach(i => {
      swCtx.addHole(i);
    });
  }
}

const getTrianglesFromPolygon = (outerPolyPoints, innerPolys, height) => {

  const swCtx = createTriangulationContext(outerPolyPoints);

  addHolesToPoly(swCtx, innerPolys);

  // NDDado: we try to triangulate first with the inner holes, if that fails it will roll back (inside the catch)
  // to a simple polygon with outer points only. This means that the eventual inner points that will be maybe
  // rendered somewhere will _overlap_ with the outer points. So we still need to get order dependent rendering
  // on tiles, which is sane anyway.
  // PS: I would have preferred a normal if/then in here, by using the clipper library that's not possible :/ So we
  // had to settle for the try/catch
  try {
    return triangulate3d(swCtx, height);
  } catch (e) {
    return triangulate3d(createTriangulationContext(outerPolyPoints), height);
  }
}

// const calcPointProjection = (pointLineA, pointLineB, point) => {
//   let prj;
//   if (pointLineA.x === pointLineB.x) {
//     //Horizontal aligned
//     prj = {x: pointLineA.y, y: point.y};
//   } else if (pointLineA.y === pointLineB.y) {
//     //Vertical aligned
//     prj = {x: point.y, y: pointLineA.y};
//   } else {
//     const dx = (pointLineB.x - pointLineA.x);
//     const dy = (pointLineB.y - pointLineA.y);
//     const rxy = dx / dy;
//
//     const t = (point.y + point.x * rxy - pointLineA.x * rxy - pointLineA.y) / (dy + rxy * dx);
//
//     prj = {x: pointLineA.x + dx * t, y: pointLineA.y + dy * t};
//   }
//   return prj;
// }

const extrudePoly = (poly, minHeight, maxHeight) => {

  const result = [];

  for (let i = 0; i < poly.length; i++) {
    const nextI = (i + 1) % poly.length;

    const point = poly[i];
    const nextPoint = poly[nextI];
    //First triangle for lateral face
    result.push({x: point.x, y: point.y, z: minHeight});
    result.push({x: nextPoint.x, y: nextPoint.y, z: minHeight});
    result.push({x: nextPoint.x, y: nextPoint.y, z: maxHeight});
    //Second triangle for lateral face
    result.push({x: point.x, y: point.y, z: minHeight});
    result.push({x: nextPoint.x, y: nextPoint.y, z: maxHeight});
    result.push({x: point.x, y: point.y, z: maxHeight});
  }

  return result;
}

const isCollinear = (pa, pb, pc) => {
  // const pax = new Decimal(pa.x);
  // const pay = new Decimal(pa.y);
  // const pbx = new Decimal(pb.x);
  // const pby = new Decimal(pb.y);
  // const pcx = new Decimal(pc.x);
  // const pcy = new Decimal(pc.y);
  const pax = pa.lon;
  const pay = pa.lat;
  const pbx = pb.lon;
  const pby = pb.lat;
  const pcx = pc.lon;
  const pcy = pc.lat;
  //let cp = Math.abs( pa.x * (pb.y - pc.y) + pb.x * (pc.y - pa.y) + pc.x * (pa.y - pb.y) );
  let cp = Math.abs(pax * (pby - pcy) + pbx * (pcy - pay) + pcx * (pay - pby));
  //let cp = Decimal.abs( pax.mul((pby.sub(pcy))).add(pbx.mul((pcy.sub(pay)))).add(pcx.mul((pay.sub(pby)))) );

  //console.log(cp);
  // return cp.lt(new Decimal(0.01));
  return cp < 1e-20;
}

const removeCollinearPoints = (nodes) => {

  let points = [];

  //If last point is equal to first point then remove last
  if (nodes[0].id === nodes[nodes.length - 1].id) {
    nodes.slice(0, nodes.length - 1).forEach(n => {
      points.push({...n});
    })
  }

  if (points.length > 3) {

    let pointToRemove = 0;
    while (pointToRemove > -1 && points.length >= 3) {
      pointToRemove = -1;
      for (let ia = 0; ia < points.length; ia++) {
        let ib = (ia + 1) % points.length;
        let ic = (ia + 2) % points.length;

        let pa = points[ia];
        let pb = points[ib];
        let pc = points[ic];

        if (isCollinear(pa, pb, pc)) {
          pointToRemove = ib;
          break;
        }
      }
      if (pointToRemove > -1) {
        //console.log("Remove point "+points[pointToRemove].id)
        points.splice(pointToRemove, 1);
      }
    }
  }

  if (points.length < 3) {
    points = nodes.map(n => {
      return {...n}
    });
  }

  return points;
}

const calcTileDelta = ( center, tilePos ) => {
  return {
    deltaPosInTile : [
      center.x - tilePos.x,
      center.y - tilePos.y
    ]
  };
}

const computeBoundingBox = (tileBoundary, points) => {
  let minX = Number.MAX_VALUE;
  let minY = Number.MAX_VALUE;
  let maxX = -Number.MAX_VALUE;
  let maxY = -Number.MAX_VALUE;

  let center = {
    x: 0,
    y: 0
  };
  let sizeX;
  let sizeY;
  let lat = 0;
  let lon = 0;

  const convexHull = calcConvexHull(points);
  const ombb = calcOmbb(convexHull);

  ombb.forEach(p => {
    center.x = center.x + p.x;
    center.y = center.y + p.y;
  });
  center.x = center.x / 4;
  center.y = center.y / 4;

  points.forEach(p => {
    lat = lat + p.lat;
    lon = lon + p.lon;
    minX = Math.min(p.x, minX);
    minY = Math.min(p.y, minY);
    maxX = Math.max(p.x, maxX);
    maxY = Math.max(p.y, maxY);
  });

  lat = lat / points.length;
  lon = lon / points.length;
  sizeX = maxX - minX;
  sizeY = maxY - minY;

  return {minX, minY, maxX, maxY, center, sizeX, sizeY, lat, lon, ...calcTileDelta(center, tileBoundary.tilePos)}
}

const checkPointInsidePolygon = (polygon, point) => {

  let intersect = 0;
  for (let i = 0; i < polygon.length; i++) {
    const nextI = (i + 1) % polygon.length;
    const p = polygon[i];
    const nextP = polygon[nextI];

    if ((point.y >= p.y && point.y < nextP.y) || (point.y >= nextP.y && point.y < p.y)) {
      if (point.x > (p.x + (nextP.x - p.x) * (point.y - p.y) / (nextP.y - p.y))) {
        intersect++;
      }
    }
  }
  return intersect % 2 !== 0;
}

const checkPolygonInsidePolygon = (outerPolygon, polygon) => {

  let inside = true;
  for (let i = 0; i < polygon.length; i++) {
    if (!checkPointInsidePolygon(outerPolygon, polygon[i])) {
      inside = false;
      break;
    }
  }

  return inside;
}

module.exports = {
  getTrianglesFromPolygon,
  extrudePoly,
  removeCollinearPoints,
  computeBoundingBox,
  checkPolygonInsidePolygon,
  clipPolyPoints,
  calcTileDelta
}

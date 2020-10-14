const poly2tri = require('poly2tri');
const ClipperLib = require('js-clipper');
const {Vector} = require('../geometry/vector');
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

const offsetPolyline = (offset, points) => {

  // let pointsPartial = points;
  let pointsPartial = [];
  let startIndex = 0;
  for (let i = startIndex; i < points.length; i++) {
    for (let j = i + 1; j < points.length; j++) {
      if (points[i].id === points[j].id) {
        if (startIndex < j) {
          pointsPartial.push(points.slice(startIndex, j));
        }
        startIndex = j;
        i = j;
        break;
      }
    }
  }

  if (startIndex === 0) {
    pointsPartial.push(points);
  } else {
    // pointsPartial.push(points.slice(startIndex - 1, points.length));
  }

  let polygon = [];
  pointsPartial.forEach(pts => {
    let subj = new ClipperLib.Paths();
    let solution = new ClipperLib.Paths();
    subj[0] = pts.map(p => {
      return {X: p.x, Y: p.y}
    });
    const scale = 10000;
    ClipperLib.JS.ScaleUpPaths(subj, scale);
    let co = new ClipperLib.ClipperOffset(2, 0.25);
    co.AddPaths(subj, ClipperLib.JoinType.jtRound, ClipperLib.EndType.etOpenRound);
    co.Execute(solution, offset * scale);
    ClipperLib.JS.ScaleDownPaths(solution, scale);

    solution.forEach(s => {
      const polys = s.map(p => {
        return {x: p.X, y: p.Y}
      });
      polygon = polygon.concat(polys);
    });
  });

  return polygon;
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
  } else {
    // points = [...nodes];
    points = nodes.map(n => {
      return {...n}
    });
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

const projectionParameterPointToSegment = (point, segmentStart, segmentEnd, clamp) => {

  //Vector from segmentStart to segmentEnd
  const segmentVector = new Vector(segmentEnd.x - segmentStart.x, segmentEnd.y - segmentStart.y);
  //Vector from segmentStart to p
  const pointVector = new Vector(point.x - segmentStart.x, point.y - segmentStart.y);
  //Projection of pointVector on segmentVector is dot product of vectors
  //equal to |pointVector|*cos(theta), theta angle between pointVector and segmentVector
  const projection = pointVector.dot(segmentVector);
  //parameter is projection divide by length of segment vector
  //equal to cos(theta)
  let parameter = projection / segmentVector.sqrLength();

  if (clamp) {
    parameter = Math.max(0, Math.min(parameter, 1));
  }

  return parameter;
}

const pointOnLineFromParameter = (parameter, segmentStart, segmentEnd) => {
  return {
    x: segmentStart.x + parameter * (segmentEnd.x - segmentStart.x),
    y: segmentStart.y + parameter * (segmentEnd.y - segmentStart.y)
  };
}

const pointOnLine = (point, segmentStart, segmentEnd, clamp) => {
  const parameter = projectionParameterPointToSegment(point, segmentStart, segmentEnd, clamp);
  return pointOnLineFromParameter(parameter, segmentStart, segmentEnd);
}

const distanceFromLine = (point, segmentStart, segmentEnd, clamp) => {
  const projectedPoint = pointOnLine(point, segmentStart, segmentEnd, clamp);

  const projectedVector = new Vector(projectedPoint.x - point.x, projectedPoint.y - point.y);

  return projectedVector.length();
}

const distanceFromPoint = (pointA, pointB) => {
  const pointsVector = new Vector(pointB.x - pointA.x, pointB.y - pointA.y);

  return pointsVector.length;
}

const twoLinesIntersectParameter = (pointALine1, pointBLine1, pointALine2, pointBLine2) => {

  const line1Vector = new Vector(pointBLine1.x - pointALine1.x, pointBLine1.y - pointALine1.y);
  const line2Vector = new Vector(pointBLine2.x - pointALine2.x, pointBLine2.y - pointALine2.y);

  if (line1Vector.checkParallel(line2Vector)) {
    return Infinity;
  }

  return (((pointALine1.y - pointALine2.y) * line1Vector.x) + ((pointALine2.x - pointALine1.x) * line1Vector.y))
    / ((line2Vector.y * line1Vector.x) - (line2Vector.x * line1Vector.y));
}

const checkPointsOrder = (points) => {

  if (points.length < 3) return points;

  // size_t i1, i2;
  let i2 = 0;
  let area = 0.0;
  for ( let i1 = 0; i1 < points.length; i1++ ) {
    i2 = i1 + 1;
    if ( i2 === points.length ) i2 = 0;
    area += points[i1].x * points[i2].y - points[i1].y * points[i2].x;
  }

  if ( area > 0 ) return points;
  if ( area < 0 ) return points.reverse();
  console.log("[ERROR] cannot get winding order of these points cos area is 0");

}

module.exports = {
  getTrianglesFromPolygon,
  extrudePoly,
  offsetPolyline,
  removeCollinearPoints,
  computeBoundingBox,
  checkPolygonInsidePolygon,
  clipPolyPoints,
  calcTileDelta,
  checkPointsOrder,
  pointOnLine,
  distanceFromLine,
  distanceFromPoint,
  projectionParameterPointToSegment,
  pointOnLineFromParameter,
  twoLinesIntersectParameter
}

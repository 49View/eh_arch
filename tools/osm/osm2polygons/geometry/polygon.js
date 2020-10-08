const poly2tri = require('poly2tri');

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

// const clipPolyPoints = (tileBoundary, polyPoints) => {
//
//   let subj = [];
//   let clip = [];
//   let solution = [];
//   subj.push(polyPoints.map(p => {
//     return {X: p.x, Y: p.y}
//   }));
//   clip.push(tileBoundary.tileClip.map(tc => {
//     return {...tc}
//   }));
//
//   let co = new ClipperLib.Clipper();
//   const scale = 100000;
//   ClipperLib.JS.ScaleUpPaths(subj, scale);
//   ClipperLib.JS.ScaleUpPaths(clip, scale);
//   co.AddPaths(subj, ClipperLib.PolyType.ptSubject, true);
//   co.AddPaths(clip, ClipperLib.PolyType.ptClip, true);
//   co.Execute(ClipperLib.ClipType.ctIntersection, solution, ClipperLib.PolyFillType.pftNonZero, ClipperLib.PolyFillType.pftNonZero);
//   ClipperLib.JS.ScaleDownPaths(solution, scale);
//
//   let polyPointsClipped = [];
//   solution.forEach(s => {
//     polyPointsClipped = s.map(p => {
//       return {x: p.X, y: p.Y}
//     });
//   });
//
//   return polyPointsClipped;
//
// }

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

module.exports = {
  getTrianglesFromPolygon
}

const {getTrianglesFromPolygon} = require("../../geometry/polygon");
const {pointOnLine, distanceFromLine, twoLinesIntersectParameter} = require("../osmHelper");

const {Vector} = require("../../geometry/vector");
const {extrudePoly} = require("../osmHelper");

const createRoof = (polygon, roofInfo, convexHull, ombb) => {
  if (roofInfo.shape==="pyramidal") {
    return createPyramidalRoof(polygon, roofInfo);
  } else if (roofInfo.shape==="gabled") {
    return createGabledRoof(polygon, roofInfo, ombb, 1);
  } else if (roofInfo.shape==="hipped") {
    return createGabledRoof(polygon, roofInfo, ombb, 0.3);
  } else if (roofInfo.shape==="dome") {
    return createDomeRoof(polygon, roofInfo);
  } else {
    return createFlatRoof(polygon, roofInfo);
  }
}

const createDomeRoof = (polygon, roofInfo) => {
  let faces=[];

  //Compute shorter distance from center to polygon segments
  let inCircleRay = Number.MAX_VALUE;
  for (let i=0;i<polygon.length;i++) {
    const nextI = (i+1)%polygon.length;
    const point=polygon[i];
    const nextPoint=polygon[nextI];

    //Distance from center to polygon segment
    let distanceFromCenter = distanceFromLine({x: 0, y:0}, point, nextPoint, true);
    if (distanceFromCenter<inCircleRay) {
      inCircleRay=distanceFromCenter;
    }
  }

  // let errorCounter = 3;
  // let error=true;
  // let inCircle=[];
  let unionSurface;

  unionSurface=getTrianglesFromPolygon(polygon, null, roofInfo.minHeight);
  faces = faces.concat(unionSurface);

  for (let alpha=0;alpha<360;alpha+=5) {
    const radAlpha=alpha*Math.PI/180;
    const radNextAlpha=((alpha+5)%360)*Math.PI/180;

    let lastXPoint = inCircleRay*Math.cos(radAlpha);
    let lastYPoint = inCircleRay*Math.sin(radAlpha);
    let lastZPoint = roofInfo.minHeight;
    let lastXNextPoint = inCircleRay*Math.cos(radNextAlpha);
    let lastYNextPoint = inCircleRay*Math.sin(radNextAlpha);
    let lastZNextPoint = roofInfo.minHeight;

    for (let theta=5;theta<90;theta+=5) {
      const radTheta=theta*Math.PI/180;

      let xyPoint = inCircleRay*Math.cos(radTheta);
      let xPoint = xyPoint*Math.cos(radAlpha);
      let yPoint = xyPoint*Math.sin(radAlpha);
      let zPoint = roofInfo.minHeight+inCircleRay*Math.sin(radTheta);

      let xNextPoint = xyPoint*Math.cos(radNextAlpha);
      let yNextPoint = xyPoint*Math.sin(radNextAlpha);
      let zNextPoint = zPoint;

      faces.push({x: lastXPoint, y: lastYPoint, z: lastZPoint});
      faces.push({x: lastXNextPoint, y: lastYNextPoint, z: lastZNextPoint});
      faces.push({x: xNextPoint, y: yNextPoint, z: zNextPoint});

      faces.push({x: lastXPoint, y: lastYPoint, z: lastZPoint});
      faces.push({x: xNextPoint, y: yNextPoint, z: zNextPoint});
      faces.push({x: xPoint, y: yPoint, z: zPoint});

      lastXPoint=xPoint;
      lastYPoint=yPoint;
      lastZPoint=zPoint;
      lastXNextPoint=xNextPoint;
      lastYNextPoint=yNextPoint;
      lastZNextPoint=zNextPoint;
    }

    let xPoint=0;
    let yPoint=0;
    let zPoint=roofInfo.minHeight+inCircleRay;

    faces.push({x: lastXPoint, y: lastYPoint, z: lastZPoint});
    faces.push({x: lastXNextPoint, y: lastYNextPoint, z: lastZNextPoint});
    faces.push({x: xPoint, y: yPoint, z: zPoint});
  }

  return faces;
}

const createPyramidalRoof = (polygon, roofInfo) => {
  let faces=[];

  for (let i=0;i<polygon.length;i++) {
    const nextI = (i+1)%polygon.length;
    const point=polygon[i];
    const nextPoint=polygon[nextI];

    faces.push({x: point.x, y: point.y, z: roofInfo.minHeight});
    faces.push({x: nextPoint.x, y: nextPoint.y, z: roofInfo.minHeight});
    faces.push({x: 0, y: 0, z: roofInfo.maxHeight});
  }

  return faces;
}

const createFlatRoof = (polygon, roofInfo) => {
  let faces=[];
  if (roofInfo.minHeight!==roofInfo.maxHeight) {
    //If required, compute lateral roof face
    faces=faces.concat(extrudePoly(polygon, roofInfo.minHeight, roofInfo.maxHeight, false));
  }
  //
  const topFace = getTrianglesFromPolygon(polygon, null, roofInfo.maxHeight);

  return faces.concat(topFace);
}

const createGabledRoof = (polygon, roofInfo, ombb, hippedPerc) => {

  const axes=[];
  let pointA, pointB;
  let startIndex;

  //OMBB Axis
  axes.push(new Vector(ombb[1].x-ombb[0].x, ombb[1].y-ombb[0].y));
  axes.push(new Vector(ombb[2].x-ombb[1].x, ombb[2].y-ombb[1].y));
  axes.push(new Vector(ombb[3].x-ombb[2].x, ombb[3].y-ombb[2].y));
  axes.push(new Vector(ombb[0].x-ombb[3].x, ombb[0].y-ombb[3].y));

  if (roofInfo.orientation==="along") {
    if (axes[0].length()>axes[1].length()) {
      startIndex=1;
    } else {
      startIndex=0;
    }
  } else {
    if (axes[0].length()<axes[1].length()) {
      startIndex=1;
    } else {
      startIndex=0;
    }
  }

  pointA=ombb[startIndex].lerp(ombb[startIndex+1], 0.5);
  pointB=ombb[startIndex+2].lerp(ombb[(startIndex+3)%4], 0.5);

  // const axis = new Vector(pointB.x-pointA.x,pointB.y-pointA.y);

  let faces=[];

  let intersection=[];
  for (let i=0;i<polygon.length;i++) {
    const nextI=(i+1)%polygon.length;
    const p = polygon[i];
    const nextP = polygon[nextI];

    //Check sides intersected by axis
    const u = twoLinesIntersectParameter(pointA, pointB, p, nextP);
    if (u!==Infinity && u>=0 && u<1) {

      intersection.push({
        i1: i,
        i2: nextI,
        p1: p,
        p2: nextP,
        point: (new Vector(p.x, p.y).lerp(new Vector(nextP.x, nextP.y), u))
      })
    }
  }

  //More than 2 intersection degenerate in pyramidal roof
  if (intersection.length!==2) {
    return createPyramidalRoof(polygon, roofInfo);
  }

  if (hippedPerc<1) {
    const newP1 = intersection[0].point.lerp(intersection[1].point, hippedPerc);
    const newP2 = intersection[0].point.lerp(intersection[1].point, 1-hippedPerc);

    intersection[0].point=newP1;
    intersection[1].point=newP2;
  }

  intersection.forEach(i => {
    faces.push({ x: i.p1.x, y: i.p1.y, z:roofInfo.minHeight});
    faces.push({ x: i.p2.x, y: i.p2.y, z:roofInfo.minHeight});
    faces.push({ x: i.point.x, y: i.point.y, z:roofInfo.maxHeight});
  })

  for (let i=0;i<polygon.length;i++) {
    const nextI=(i+1)%polygon.length;
    const p = polygon[i];
    const nextP = polygon[nextI];

    let intersectionSide=false;
    for (let j=0;j<intersection.length;j++) {
      if (intersection[j].i1===i && intersection[j].i2===nextI) {
        intersectionSide=true;
        break;
      }
    }
    if (!intersectionSide) {
      // let prjParamP = projectionParameterPointToSegment(p, intersection[0].point, intersection[1].point, false);
      // let prjParamNextP = projectionParameterPointToSegment(nextP, intersection[0].point, intersection[1].point, false);
      // console.log(prjParamP);
      // console.log(prjParamNextP);
      const prj = pointOnLine(p, intersection[0].point, intersection[1].point, true);
      const nextPrj = pointOnLine(nextP, intersection[0].point, intersection[1].point, true);

      faces.push({x: p.x, y: p.y, z:roofInfo.minHeight});
      faces.push({x: nextP.x, y: nextP.y, z:roofInfo.minHeight});
      faces.push({x: nextPrj.x, y: nextPrj.y, z:roofInfo.maxHeight});

      faces.push({x: p.x, y: p.y, z:roofInfo.minHeight});
      faces.push({x: nextPrj.x, y: nextPrj.y, z:roofInfo.maxHeight});
      faces.push({x: prj.x, y: prj.y, z:roofInfo.maxHeight});
      //console.log("Next");
    }
  }

  // for (let i=0;i<polygon.length;i++) {
  //     const nextI=(i+1)%polygon.length;
  //     const p = polygon[i];
  //     const nextP = polygon[nextI];
  //      const prj = calcPointProjection(pointA, pointB, p);
  //      const nextPrj = calcPointProjection(pointA, pointB, nextP);
  // //    const prj = pointOnLine(p, pointA, pointB, false);
  // //    const nextPrj = pointOnLine(nextP, pointA, pointB, false);
  //     const dir = new Vector(prj.x-p.x,prj.y-p.y).cross(axis);
  //     const nextDir = new Vector(prj.x-nextP.x,prj.y-nextP.y).cross(axis);

  //     if (Math.sign(dir)===Math.sign(nextDir)) {
  //         faces.push({x: p.x, y: p.y, z:roofInfo.minHeight});
  //         faces.push({x: nextP.x, y: nextP.y, z:roofInfo.minHeight});
  //         faces.push({x: nextPrj.x, y: nextPrj.y, z:roofInfo.maxHeight});

  //         faces.push({x: p.x, y: p.y, z:roofInfo.minHeight});
  //         faces.push({x: nextPrj.x, y: nextPrj.y, z:roofInfo.maxHeight});
  //         faces.push({x: prj.x, y: prj.y, z:roofInfo.maxHeight});
  //     } else {
  //         if (prj.x===nextPrj.x && prj.y===nextPrj.y) {
  //             faces.push({x: p.x, y: p.y, z:roofInfo.minHeight});
  //             faces.push({x: nextP.x, y: nextP.y, z:roofInfo.minHeight});
  //             faces.push({x: prj.x, y: prj.y, z:roofInfo.maxHeight});
  //         } else {
  //             faces.push({x: p.x, y: p.y, z:roofInfo.minHeight});
  //             faces.push({x: prj.x, y: prj.y, z:roofInfo.minHeight});
  //             faces.push({x: prj.x, y: prj.y, z:roofInfo.maxHeight});

  //             faces.push({x: prj.x, y: prj.y, z:roofInfo.minHeight});
  //             faces.push({x: nextPrj.x, y: nextPrj.y, z:roofInfo.minHeight});
  //             faces.push({x: nextPrj.x, y: nextPrj.y, z:roofInfo.maxHeight});

  //             faces.push({x: prj.x, y: prj.y, z:roofInfo.minHeight});
  //             faces.push({x: nextPrj.x, y: nextPrj.y, z:roofInfo.maxHeight});
  //             faces.push({x: prj.x, y: prj.y, z:roofInfo.maxHeight});

  //             faces.push({x: nextPrj.x, y: nextPrj.y, z:roofInfo.minHeight});
  //             faces.push({x: nextP.x, y: nextP.y, z:roofInfo.minHeight});
  //             faces.push({x: nextPrj.x, y: nextPrj.y, z:roofInfo.maxHeight});
  //         }
  //     }
  // }


  return faces;
}


const createComplexPolygonRoof = (outerPolygon, innerPolygons, roofInfo) => {

  let faces=[];
  if (roofInfo.minHeight!==roofInfo.maxHeight) {
    //If required, compute lateral roof face
    faces=faces.concat(extrudePoly(outerPolygon.points, roofInfo.minHeight, roofInfo.maxHeight, false));
    innerPolygons.forEach(h => {
      faces = faces.concat(extrudePoly([...h.points].reverse(), roofInfo.minHeight, roofInfo.maxHeight, false));
    })

  }

  const topFace=getTrianglesFromPolygon(outerPolygon.points, innerPolygons, roofInfo.maxHeight);

  faces=faces.concat(topFace);

  return faces;
}

module.exports = { createRoof, createComplexPolygonRoof }

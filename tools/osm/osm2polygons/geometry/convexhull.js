"use strict";

const {Vector} = require("./vector");

const ON = 0;
const LEFT = 1;
const RIGHT = 2;
const ALMOST_ZERO = 0.0;

function GetSideOfLine(lineStart, lineEnd, point)
{
  const d = (lineEnd.x - lineStart.x) * (point.y - lineStart.y) - (lineEnd.y - lineStart.y) * (point.x - lineStart.x);
  return (d > ALMOST_ZERO ? LEFT : (d < -ALMOST_ZERO ? RIGHT : ON));
}

// returns convex hull in CW order
// (required by Rotating Calipers implementation)
function calcConvexHull(points)
{
    let endPt;
  const vectors=[];

    points.forEach(p => {
      vectors.push(new Vector(p.x, p.y));
    })

    // bad input?
    if (vectors.length < 3) {
      vectors.forEach(v => {
        const p = points.find(p => p.x===v.x && p.y===v.y);
        if (p) {
          v.id=p.id;
        }
      });
      return vectors;
    }

    // find first hull point
    let hullPt = vectors[0];
    let convexHull = [];

    for (let i=1; i<vectors.length; i++)
    {
        // perform lexicographical compare
        if (vectors[i].x < hullPt.x)
            hullPt = vectors[i];
        else if (Math.abs(vectors[i].x-hullPt.x) < ALMOST_ZERO) // equal
            if (vectors[i].y < hullPt.y)
                hullPt = vectors[i];
    }

    // find remaining hull points
    do
    {
        convexHull.unshift(hullPt.clone());
        endPt = vectors[0];

        for (let j=1; j<vectors.length; j++)
        {
          const side = GetSideOfLine(hullPt, endPt, vectors[j]);

          // in case point lies on line take the one further away.
            // this fixes the co-linearity problem.
            if (endPt.equals(hullPt) || (side === LEFT || (side === ON && hullPt.distance(vectors[j]) > hullPt.distance(endPt))))
                endPt = vectors[j];
        }

        hullPt = endPt;
    }
    while (!endPt.equals(convexHull[convexHull.length-1]));

    convexHull.forEach(v => {
      const p = points.find(p => p.x===v.x && p.y===v.y);
      if (p) {
        v.id=p.id;
      }
    });

    return convexHull;
}

module.exports = {calcConvexHull};

"use strict";

const {Vector} = require("./vector");

var ON = 0;
var LEFT = 1;
var RIGHT = 2;
var ALMOST_ZERO = 0.00001;

function GetSideOfLine(lineStart, lineEnd, point)
{
    var d = (lineEnd.x-lineStart.x)*(point.y-lineStart.y)-(lineEnd.y-lineStart.y)*(point.x-lineStart.x);
    return (d > ALMOST_ZERO ? LEFT : (d < -ALMOST_ZERO ? RIGHT : ON));
}

// returns convex hull in CW order
// (required by Rotating Calipers implementation)
function calcConvexHull(points)
{   
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
    var hullPt = vectors[0];
    var convexHull = [];
    
    for (var i=1; i<vectors.length; i++)
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
        var endPt = vectors[0];
       
        for (var j=1; j<vectors.length; j++)
        {           
            var side = GetSideOfLine(hullPt, endPt, vectors[j]);
            
            // in case point lies on line take the one further away.
            // this fixes the collinearity problem.
            if (endPt.equals(hullPt) || (side == LEFT || (side == ON && hullPt.distance(vectors[j]) > hullPt.distance(endPt))))
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
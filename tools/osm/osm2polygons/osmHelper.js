const Decimal = require('decimal.js');
Decimal.set({ precision: 20, rounding: 1 })
const { Vector} = require('./vector');
const poly2tri = require('poly2tri');
const { calcConvexHull } = require('./convexhull');
const { calcOmbb } = require('./ombb');


const getTrianglesFromPolygon = (polyPoints) => {
    const intPoints = [];

    polyPoints.forEach(p => {
        intPoints.push({x: p.x*100000, y: p.y*100000})
    });

    const swctx = new poly2tri.SweepContext(intPoints);
    swctx.triangulate();
    const tri= swctx.getTriangles()
    const points=[];
    tri.forEach(t => {
        points.push({x: t.points_[0].x/100000, y: t.points_[0].y/100000});
        points.push({x: t.points_[1].x/100000, y: t.points_[1].y/100000});
        points.push({x: t.points_[2].x/100000, y: t.points_[2].y/100000});
    });

    return points;
}

const getTrianglesFromComplexPolygon = (outerPolyPoints, innerPolys) => {
    const intOuterPoints = [];
    const intInnersPoints = [];

    outerPolyPoints.forEach(p => {
        intOuterPoints.push({x: p.x*100000, y: p.y*100000})
    });

    innerPolys.forEach(i => {
        const intInnerPoint = [];
        i.points.forEach(p => {
            intInnerPoint.push({x: p.x*100000, y: p.y*100000})
        });
        intInnersPoints.push(intInnerPoint);
    });

    const swctx = new poly2tri.SweepContext(intOuterPoints);
    intInnersPoints.forEach(i => {
        swctx.addHole(i);
    });
    swctx.triangulate();
    const tri= swctx.getTriangles()
    const points=[];
    tri.forEach(t => {
        points.push({x: t.points_[0].x/100000, y: t.points_[0].y/100000});
        points.push({x: t.points_[1].x/100000, y: t.points_[1].y/100000});
        points.push({x: t.points_[2].x/100000, y: t.points_[2].y/100000});
    });

    return points;
}

const calcPointProjection = (pointLineA, pointLineB, point) => {
    let prj;
    if (pointLineA.x===pointLineB.x) {
        //Horizontal aligned
        prj = { x: pointLineA.y, y: point.y};
    } else if (pointLineA.y===pointLineB.y) {
        //Vertical aligned
        prj = { x: point.y, y: pointLineA.y};
    } else {
        const dx = (pointLineB.x-pointLineA.x);
        const dy = (pointLineB.y-pointLineA.y);
        const rxy = dx/dy; 

        const t = (point.y+point.x*rxy-pointLineA.x*rxy-pointLineA.y)/(dy+rxy*dx);

        prj = { x: pointLineA.x+dx*t, y: pointLineA.y+dy*t};
    }
    return prj;
}


const extrudePoly = (poly, minHeight, maxHeight, useTrianglesStrip) => {
    
    const result = [];

    if (useTrianglesStrip) {
        poly.forEach(p => {
            result.push({x:p.x, y:p.y, z:minHeight});
            result.push({x:p.x, y:p.y, z:maxHeight});
        });
    } else {
        for (let i=0;i<poly.length;i++) {
            const nextI = (i+1)%poly.length;

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
    }

    return result;
}

const isCollinear = (pa,pb,pc) => {
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
    let cp = Math.abs( pax * (pby - pcy) + pbx * (pcy - pay) + pcx * (pay - pby) );
    //let cp = Decimal.abs( pax.mul((pby.sub(pcy))).add(pbx.mul((pcy.sub(pay)))).add(pcx.mul((pay.sub(pby)))) );

    //console.log(cp);
    // return cp.lt(new Decimal(0.01));
    return cp<1e-20;
}

const computeBoundingBox = (points) => {
    let minX=Number.MAX_VALUE;
    let minY=Number.MAX_VALUE;
    let maxX=-Number.MAX_VALUE;
    let maxY=-Number.MAX_VALUE;

    const convexHull = calcConvexHull(points);
    const ombb = calcOmbb(convexHull);

    let centerX = 0;
    let centerY = 0;
    ombb.forEach(p => {
        centerX = centerX+p.x;
        centerY = centerY+p.y;
    });
    centerX = centerX/4;
    centerY = centerY/4;

    let sizeX = 0;
    let sizeY = 0;
    let centerLat = 0;
    let centerLon = 0;

    points.forEach(p => {
        centerLat = centerLat+p.lat;
        centerLon = centerLon+p.lon;
        minX=Math.min(p.x,minX);
        minY=Math.min(p.y,minY);
        maxX=Math.max(p.x,maxX);
        maxY=Math.max(p.y,maxY);
    });

    centerLat = centerLat/points.length;
    centerLon = centerLon/points.length;
    sizeX = maxX-minX;
    sizeY = maxY-minY;

    return {minX,minY,maxX,maxY,centerX,centerY,sizeX,sizeY,centerLat,centerLon}
}

const convertToLocalCoordinate = (points, originX, originY) => {

    points.forEach(p => {
        p.x=p.x-originX;
        p.y=p.y-originY;
    });
}

const removeCollinearPoints = (nodes) => {

    let points = [];

    nodes.slice(0,nodes.length-1).forEach(n => {
        const point = { ...n };
        point.x=n.x; //.toNumber();
        point.y=n.y; //.toNumber();
        points.push(point);
    })

    if (points.length>3) {

        let pointToRemove = 0;
        while (pointToRemove>-1 && points.length>=3) {
            pointToRemove = -1;
            for (let ia=0;ia<points.length;ia++) {
                let ib=(ia+1)%points.length;
                let ic=(ia+2)%points.length;

                let pa=points[ia];
                let pb=points[ib];
                let pc=points[ic];

                if (isCollinear(pa,pb,pc)) {
                    pointToRemove=ib;
                    break;
                }
            }
            if (pointToRemove>-1) {
                //console.log("Remove point "+points[pointToRemove].id)
                points.splice(pointToRemove, 1);
            }
        }
    }  
    
    if (points.length<3) {
        points=[];
        nodes.slice(0,nodes.length-1).forEach(n => {
            const point = { ...n };
            point.x=n.x; //n.x.toNumber();
            point.y=n.y; //n.y.toNumber();
            points.push(point);
        })
    }
//console.log(points);
    return points;
}



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

const projectionParameterPointToSegment = (point, segmentStart, segmentEnd, clamp) => {

    //Vector from segmentStart to segmentEnd
    const segmentVector = new Vector(segmentEnd.x-segmentStart.x, segmentEnd.y-segmentStart.y);
    //Vector from segmentStart to p
    const pointVector = new Vector(point.x-segmentStart.x, point.y-segmentStart.y);
    //Projection of pointVector on segmentVector is dot product of vectors
    //equal to |pointVector|*cos(theta), theta angle between pointVector and segmentVector
    const projection = pointVector.dot(segmentVector);
    //parameter is projection divide by length of segment vector
    //equal to cos(theta)
    let parameter = projection/segmentVector.sqrLength();

    if (clamp) {
        parameter=Math.max(0, Math.min(parameter, 1));
    }

    return parameter;
}

const pointOnLineFromParameter = (parameter, segmentStart, segmentEnd) => {

    const point={
        x: segmentStart.x+parameter*(segmentEnd.x-segmentStart.x),
        y: segmentStart.y+parameter*(segmentEnd.y-segmentStart.y)
    }

    return point;
}

const pointOnLine = (point, segmentStart, segmentEnd, clamp) => {
    const parameter = projectionParameterPointToSegment(point, segmentStart, segmentEnd, clamp);
    const projectedPoint = pointOnLineFromParameter(parameter, segmentStart, segmentEnd);

    return projectedPoint;
}

const distanceFromLine = (point, segmentStart, segmentEnd, clamp) => {
    const projectedPoint = pointOnLine(point, segmentStart, segmentEnd, clamp);

    const projectedVector = new Vector(projectedPoint.x-point.x, projectedPoint.y-point.y);

    return projectedVector.length();
}

const distanceFromPoint = (pointA, pointB) => {
    const pointsVector = new Vector(pointB.x-pointA.x, pointB.y-pointA.y);

    return pointsVector.length;
}

const twoLinesintersectParameter = (pointALine1, pointBLine1, pointALine2, pointBLine2) => {

    const line1Vector = new Vector(pointBLine1.x-pointALine1.x, pointBLine1.y-pointALine1.y);
    const line2Vector = new Vector(pointBLine2.x-pointALine2.x,pointBLine2.y-pointALine2.y);

    if (line1Vector.checkParallel(line2Vector)) {
        return Infinity;
    }
    
    const parameter = (((pointALine1.y-pointALine2.y)*line1Vector.x)+((pointALine2.x-pointALine1.x)*line1Vector.y))
            / ((line2Vector.y*line1Vector.x)-(line2Vector.x*line1Vector.y));
    
    return parameter;                    
}

const checkPointsOrder = (points,convexHull) => {

    let i1,i2,i3;

    //console.log(convexHull.length);

    i1 = points.findIndex(p => p.id==convexHull[0].id);
    i2 = points.findIndex(p => p.id==convexHull[1].id);
    i3 = points.findIndex(p => p.id==convexHull[2].id);

    if (i2<i1) i2=i2+points.length;
    if (i3<i1) i3=i3+points.length;

    if (i1<i2 && i2<i3) {
        //console.log("Point order correct");
        return points;
    } else {
        //console.log("Reverse point list order");
        return points.reverse();
    }
}

const checkPathClosed = (member) => {
    const firstNodeId=member.nodes[0].id;
    const lastNodeId=member.nodes[member.nodes.length-1].id;
    return firstNodeId===lastNodeId;
}

const createClosedPath = (member, members) => {

    const connectedMembers=[member];
    const role=member.role;
    const firstId =member.ref.id;
    let currentId=member.ref.id;
    let lastNodeId=member.ref.nodes[member.ref.nodes.length-1].id;

    let closed=false;
    while (!closed) {
        //Search for next member (lastNode = firstNode)
        let nextMember=members.find(m => m.role===role && m.ref.nodes[0].id===lastNodeId);
        if  (nextMember===undefined) {
            //Search for next member (lastNode = lastNode)
            nextMember=members.find(m => m.role===role && m.ref.id!==currentId && m.ref.nodes[m.ref.nodes.length-1].id===lastNodeId);
            if (nextMember!==undefined) {
                //Reverse nextMember nodes order
                nextMember.ref.nodes.reverse();
            }
        }
        if (nextMember!==undefined) {
            if (nextMember.ref.id===firstId) {
                closed=true;
            } else {
                //Search for next node
                currentId=nextMember.id;
                lastNodeId=nextMember.ref.nodes[nextMember.ref.nodes.length-1].id;
                connectedMembers.push(nextMember);
            }
        } else {
            //Can't close path, INVALID!
            break;
        }
    }
    //Mark members connected as processed
    connectedMembers.forEach(m=>m.processed=true);
    //If not closed,  return nothing
    if (!closed) {
        return [];
    }
    
    return connectedMembers;
}

const checkPointInsidePolygon = (polygon, point) => {

    let intersect = 0;
    for (let i=0;i<polygon.length;i++) {
        const nextI=(i+1)%polygon.length;
        const p=polygon[i];
        const nextP=polygon[nextI];

        if ((point.y>=p.y && point.y<nextP.y) || (point.y>=nextP.y && point.y<p.y)) {
            if (point.x>(p.x+(nextP.x-p.x)*(point.y-p.y)/(nextP.y-p.y))) {
                intersect++;
            }
        }
    }
    return intersect%2!==0;
}

const checkPolygonInsidePolygon = (outerPolygon, polygon) => {

    let inside=true;
    for (let i=0;i<polygon.length;i++) {
        if (!checkPointInsidePolygon(outerPolygon, polygon[i])) {
            inside=false;
            break;
        }
    }

    return inside;
}

const createMesh = (id, tags, type, boundingBox, groups) => {
    return {
        id: id,
        type: type,
        center: {
            x: boundingBox.centerX,
            y: boundingBox.centerY,
            lat: boundingBox.centerLat,
            lon: boundingBox.centerLon
        },
        groups: groups,
        tags: tags
    }
}

const getPolygonFromWay = (way) => {
    const points = removeCollinearPoints(way.nodes); 
    if (points.length<3) {
        throw new Error("Invalid way "+ way.id);
    }
    let localBoundingBox = computeBoundingBox(points);
    convertToLocalCoordinate(points, localBoundingBox.centerX, localBoundingBox.centerY);
    //Using: https://github.com/geidav/ombb-rotating-calipers
    //with some modifications
    
    const convexHull = calcConvexHull(points);
    checkPointsOrder(points,convexHull);
    const orientedMinBoundingBox = calcOmbb(convexHull);

    const polygon = createBasePolygon(points);

    return {polygon,localBoundingBox,convexHull,orientedMinBoundingBox}
}

const getPolygonsFromMultipolygonRelation = (rel) => {

    //Clone members for elaboration
    const polygons = [];
    const members = JSON.parse(JSON.stringify(rel.members));
    //Cosed polygons not require elaboration
    members.forEach(m => {
        if (checkPathClosed(m.ref)) {
            m.processed=true;
            const polygon = {
                role: m.role,
                nodes: []
            }
            //Copy point but not last (same than first)
            for (let i=0;i<m.ref.nodes.length-1;i++) {
                polygon.nodes.push({...m.ref.nodes[i]});
            }
            polygons.push(polygon);
        }
    });

    //Search and create closed polygons from segments
    while (members.filter(m=>m.processed===true).length<members.length) 
    {
        for (i=0;i<members.length;i++) {
            if (!members[i].processed) {
                const connectedMembers = createClosedPath(members[i], members);
                if (connectedMembers.length>0) {
                    const polygon = {
                        role: connectedMembers[0].role,
                        nodes: []
                    }
                    connectedMembers.forEach(m => {
                        for (let i=0;i<m.ref.nodes.length-1;i++) {
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
        p.points=[];
        p.nodes.forEach(n => p.points.push({x: Number(n.x), y: Number(n.y), lat: n.lat, lon:n.lon}));
        relPoints=relPoints.concat(p.points);               
    });
    //Compute local bounding box and correct point direction
    const localBoundingBox=computeBoundingBox(relPoints);
    polygons.forEach(p => {
        convertToLocalCoordinate(p.points, localBoundingBox.centerX, localBoundingBox.centerY);
        const convexHull = calcConvexHull(p.points);
        checkPointsOrder(p.points,convexHull);
    });
    //Create reference between inner and outer
    polygons.filter(p => p.role==="outer").forEach(o => {
        o.holes=[];
        polygons.filter(p => p.role==="inner" && p.container===undefined).forEach(i => {
            if (checkPolygonInsidePolygon(o.points, i.points)) {
                i.container=o;
                o.holes.push(i);
            }
        });
    })

    return {polygons,localBoundingBox};
}

const createElementsFromWays = (ways, filter, elementCreator) => {
    const elements=[];
    ways.filter(w => w.calc!==undefined)
        .filter(filter)        
        .forEach(w => {  // && w.id===364313092
            try {
                //const {polygon,localBoundingBox,convexHull,orientedMinBoundingBox} = getPolygonFromWay(w);

                const element = elementCreator(w, w.calc.polygon, w.calc.lbb, w.calc.convexHull, w.calc.ombb);
                if (element!==null) {
                    elements.push(element);                    
                }
            } catch (ex) {
                console.log(`Error in ${w.id} way`, ex);
            }
        }
    );

    return elements;
}

const createElementsFromRels = (rels, filter, elementCreator) => {
    const elements=[];

    rels.filter(r => r.calc!==undefined)
        .filter(filter)
        .forEach(r => {
            try {
                //const {polygons,localBoundingBox} = getPolygonsFromMultipolygonRelation(r);
                const element = elementCreator(r, r.calc.polygons, r.calc.lbb);
                if (element!==null) {
                    elements.push(element);                    
                }

            } catch (ex) {
                console.log(`Error in ${r.id} relation`, ex);
            }
        }
    );

    return elements;
}

const setHeight = (faces, height) => {
    faces.forEach(f => {
        f.z=height;
    })
}

module.exports = {
    getTrianglesFromPolygon,
    getTrianglesFromComplexPolygon,
    extrudePoly,
    computeBoundingBox,
    convertToLocalCoordinate,
    removeCollinearPoints,
    createBasePolygon,
    checkPointsOrder,
    checkPathClosed,
    createClosedPath,
    checkPolygonInsidePolygon,
    createMesh,
    getPolygonsFromMultipolygonRelation,
    getPolygonFromWay,
    createElementsFromWays,
    createElementsFromRels,
    setHeight,
    pointOnLine,
    distanceFromLine,
    distanceFromPoint,
    calcPointProjection,
    projectionParameterPointToSegment,
    pointOnLineFromParameter,
    twoLinesintersectParameter
}
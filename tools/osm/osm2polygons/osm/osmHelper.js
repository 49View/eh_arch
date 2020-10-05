const { Vector} = require('../geometry/vector');
const poly2tri = require('poly2tri');
const { calcConvexHull } = require('../geometry/convexhull');
const { calcOmbb } = require('../geometry/ombb');
const ClipperLib = require('js-clipper');

const calcNodeCoordinates = n => {
    const ly = n.lat;
    const lx = n.lon;
    return {
        x: Math.sign(n.lon) * calcDistance(ly, 0, ly, lx),
        y: Math.sign(n.lat) * calcDistance(0, lx, ly, lx)
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

const triangulate = (swCtx) => {
    swCtx.triangulate();
    const tri= swCtx.getTriangles()
    const points=[];
    tri.forEach(t => {
        points.push({x: t.points_[0].x/100000, y: t.points_[0].y/100000});
        points.push({x: t.points_[1].x/100000, y: t.points_[1].y/100000});
        points.push({x: t.points_[2].x/100000, y: t.points_[2].y/100000});
    });

    return points;

}

const getTrianglesFromPolygon = (polyPoints) => {
    const intPoints = [];

    polyPoints.forEach(p => {
        intPoints.push({x: p.x*100000, y: p.y*100000})
    });

    const swCtx = new poly2tri.SweepContext(intPoints);
    return triangulate(swCtx);
}

const getTrianglesFromComplexPolygon = (outerPolyPoints, innerPolys) => {
    let intOuterPoints = [];
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

    const swCtx = new poly2tri.SweepContext(intOuterPoints);
    intInnersPoints.forEach(i => {
        swCtx.addHole(i);
    });
    // NDDado: we try to triangulate first with the inner holes, if that fails it will roll back (inside the catch)
    // to a simple polygon with outer points only. This means that the eventual inner points that will be maybe
    // rendered somewhere will _overlap_ with the outer points. So we still need to get order dependent rendering
    // on tiles, which is sane anyway.
    try {
        return triangulate(swCtx);
    } catch (e) {
        return getTrianglesFromPolygon(outerPolyPoints);
    }
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

const computeBoundingBox = (points, isOpen=undefined) => {
    let minX=Number.MAX_VALUE;
    let minY=Number.MAX_VALUE;
    let maxX=-Number.MAX_VALUE;
    let maxY=-Number.MAX_VALUE;

    let centerX = 0;
    let centerY = 0;
    let sizeX;
    let sizeY;
    let centerLat = 0;
    let centerLon = 0;

    if (isOpen===undefined) {
        const convexHull = calcConvexHull(points);
        const ombb = calcOmbb(convexHull);

        ombb.forEach(p => {
            centerX = centerX+p.x;
            centerY = centerY+p.y;
        });
        centerX = centerX/4;
        centerY = centerY/4;
    }

    points.forEach(p => {
        centerLat = centerLat+p.lat;
        centerLon = centerLon+p.lon;
        minX=Math.min(p.x,minX);
        minY=Math.min(p.y,minY);
        maxX=Math.max(p.x,maxX);
        maxY=Math.max(p.y,maxY);
        if (isOpen!==undefined) {
            centerX = centerX+p.x;
            centerY = centerY+p.y;
        }
    });

    centerLat = centerLat/points.length;
    centerLon = centerLon/points.length;
    sizeX = maxX-minX;
    sizeY = maxY-minY;
    if (isOpen!==undefined) {
        centerX = centerX/points.length;
        centerY = centerY/points.length;
    }

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

    //If last point is equal to first point then remove last
    if (nodes[0].id===nodes[nodes.length-1].id) {
        nodes.slice(0,nodes.length-1).forEach(n => {
            points.push({ ...n });
        })
    }

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
        points = nodes.map(n => {return {...n}});
    }

    return points;
}

const offsetPolyline = (way, points) => {
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
        pointsPartial.push(points.slice(startIndex - 1, points.length));
    }

    const {roadWidth, roadLane} = getWidthFromWay(way);
    let polygon = [];
    pointsPartial.forEach(points => {
        let subj = new ClipperLib.Paths();
        let solution = new ClipperLib.Paths();
        subj[0] = points.map(p => {
            return {X: p.x, Y: p.y}
        });
        const scale = 100;
        ClipperLib.JS.ScaleUpPaths(subj, scale);
        let co = new ClipperLib.ClipperOffset(2, 0.25);
        co.AddPaths(subj, ClipperLib.JoinType.jtRound, ClipperLib.EndType.etOpenRound);
        co.Execute(solution, roadWidth * roadLane * scale);
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

    const projectedVector = new Vector(projectedPoint.x-point.x, projectedPoint.y-point.y);

    return projectedVector.length();
}

const distanceFromPoint = (pointA, pointB) => {
    const pointsVector = new Vector(pointB.x-pointA.x, pointB.y-pointA.y);

    return pointsVector.length;
}

const twoLinesIntersectParameter = (pointALine1, pointBLine1, pointALine2, pointBLine2) => {

    const line1Vector = new Vector(pointBLine1.x-pointALine1.x, pointBLine1.y-pointALine1.y);
    const line2Vector = new Vector(pointBLine2.x-pointALine2.x,pointBLine2.y-pointALine2.y);

    if (line1Vector.checkParallel(line2Vector)) {
        return Infinity;
    }

    return (((pointALine1.y - pointALine2.y) * line1Vector.x) + ((pointALine2.x - pointALine1.x) * line1Vector.y))
      / ((line2Vector.y * line1Vector.x) - (line2Vector.x * line1Vector.y));
}

const checkPointsOrder = (points,convexHull) => {

    if ( points.length < 3 ) return points;

    let i1,i2,i3;

    //console.log(convexHull.length);

    i1 = points.findIndex(p => p.id===convexHull[0].id);
    i2 = points.findIndex(p => p.id===convexHull[1].id);
    i3 = points.findIndex(p => p.id===convexHull[2].id);

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
                currentId=nextMember.ref.id;
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

const createGroup = (id, tags, name, boundingBox, faces, color) => {

    const groups=[];

    groups.push({
        faces: faces,
        colour: color,
    });

    return createMesh(id, tags, name, boundingBox, groups);
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
    const isClosed = way.nodes[0].id === way.nodes[way.nodes.length - 1].id;

    const points = removeCollinearPoints(way.nodes);
    if (points.length<3 && isClosed) {
        throw new Error("Invalid way "+ way.id);
    }
    let localBoundingBox = computeBoundingBox(points);
    convertToLocalCoordinate(points, localBoundingBox.centerX, localBoundingBox.centerY);
    //Using: https://github.com/geidav/ombb-rotating-calipers
    //with some modifications

    const convexHull = calcConvexHull(points);
    checkPointsOrder(points,convexHull);
    const ombb = calcOmbb(convexHull);

    const polygon = isClosed ? createBasePolygon(points) : offsetPolyline(way, points);

    const absolutePolygon = [];
    polygon.forEach(p => {
        absolutePolygon.push({x: p.x + localBoundingBox.centerX, y: p.y + localBoundingBox.centerY});
    })

    return {polygon,absolutePolygon,localBoundingBox,convexHull,ombb}
}

const getPolygonsFromMultipolygonRelation = (rel) => {

    //Clone members for elaboration
    const polygons = [];
    const members = rel.members;// JSON.parse(JSON.stringify(rel.members));
    //Cosed polygons not require elaboration
    members.forEach(m => {
        m.processed = false;
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
    while (members.filter(m=> m.processed===true).length<members.length)
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

const createElementsFromWays = (ways, name, filter, elementCreator) => {
    const elements=[];
    ways.filter(w => w.calc!==undefined)
        .filter(filter)
        .forEach(w => {  // && w.id===364313092
            try {
                const element = elementCreator(w, name);
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

const createElementsFromRels = (rels, name, filter, elementCreator) => {
    const elements=[];

    rels.filter(r => r.calc!==undefined)
        .filter(filter)
        .forEach(r => {
            try {
                const element = elementCreator(r, name);
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

const createElementsFromNodes = (nodes, name, filter, elementCreator) => {
    const elements=[];

    nodes.filter(filter)
      .forEach(r => {
            try {
                const element = elementCreator(r, name);
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

const getColorFromTags = (tags) => {
    let color = "#808080";

    if ( tags ) {
        if ((tags["landuse"] && tags["landuse"]==="grass")
          || (tags["leisure"] && (tags["leisure"]==="park" || tags["leisure"]==="garden"))) {
            color="#CDF7C9";
        } else if ( tags["natural"]==="water" ) {
            color = "#AAD3DF";
        } else if ( tags["highway"] ) {
            switch (tags["highway"]) {
                case "motorway":
                    color = "#DF2E6B";
                    break;
                case "motorway_link":
                    color = "#DF2E6B";
                    break;
                case "trunk":
                    color = "#FBB29A";
                    break;
                case "trunk_link":
                    color = "#FBB29A";
                    break;
                case "primary":
                    color = "#FDD7A1";
                    break;
                case "primary_link":
                    color = "#FDD7A1";
                    break;
                case "secondary":
                    color = "#F1EEE8";
                    break;
                case "tertiary":
                    color = "#FFDFFF";
                    break;
                case "unclassified":
                case "residential":
                    color = "#FFDFDF";
                    break;
                case "track":
                case "cycleway":
                case "bridleway":
                case "footway":
                case "path":
                case "steps":
                    color = "#8F8F8F";
                    break;
                default:
                    color = "#CFCFCF";
            }
        } else if (tags["railway"] && tags["railway"]==="rail") {
            color = "#303030";
        } else if ( tags["barrier"] ) {
            color = "#709090";
        }
    } else {
        color = "#606060";
    }

    return color;
}

const getWidthFromWay = (way) => {
    let roadLane = 1;
    let roadWidth = 2;

    if ( way.tags ) {
        switch (way.tags["highway"]) {
            case "motorway":
                roadWidth = 3;
                roadLane = 3;
                break;
            case "motorway_link":
                roadWidth = 2.75;
                roadLane = 2;
                break;
            case "trunk":
                roadWidth = 3;
                roadLane = 2;
                break;
            case "trunk_link":
                roadWidth = 2.75;
                roadLane = 2;
                break;
            case "primary":
                roadWidth = 3;
                roadLane = 2;
                break;
            case "primary_link":
                roadWidth = 2.75;
                roadLane = 2;
                break;
            case "secondary":
                roadWidth = 2.5;
                roadLane = 2;
                break;
            case "tertiary":
                roadWidth = 2.25;
                roadLane = 2;
                break;
            case "unclassified":
            case "residential":
                roadWidth = 2.5;
                roadLane = 1;
                break;
            case "track":
            case "cycleway":
            case "bridleway":
            case "footway":
            case "path":
            case "steps":
                roadWidth = 0.5;
                roadLane = 1;
                break;
            default:
                roadWidth = 1.5;
                roadLane = 1;
        }
        if (way.tags["railway"] && way.tags["railway"]==="rail") {
            roadWidth = 1.5;
            roadLane = 2;
        }
    }

    return { roadWidth, roadLane };
}

const groupFromWay = (way, name) => {

    const faces = getTrianglesFromPolygon(way.calc.polygon);
    setHeight(faces,0);

    return createGroup("w-"+way.id, way.tags, name, way.calc.localBoundingBox, faces, getColorFromTags(way.tags));
}

const groupFromRel = (rel, name) => {

    let faces=[];
    rel.calc.polygons.filter(p => p.role==="outer").forEach(o => {
        faces = faces.concat(getTrianglesFromComplexPolygon(o.points, o.holes));
        setHeight(faces,0);
    });

    return createGroup("r-"+rel.id, rel.tags, name, rel.calc.localBoundingBox, faces, getColorFromTags(rel.tags));
}

const groupFromNode = (node, name) => {
    const nodeCenter = calcNodeCoordinates(node);
    const nodeBBox = {
        centerX: nodeCenter.x,
        centerY: nodeCenter.y,
        centerLat: node.lat,
        centerLon: node.lon
    }
    return createGroup("n-"+node.id, node.tags, name, nodeBBox, [], getColorFromTags(node.tags));
}

const setHeight = (faces, height) => {
    faces.forEach(f => {
        f.z=height;
    })
}

module.exports = {
    calcCoordinate,
    getTrianglesFromPolygon,
    getTrianglesFromComplexPolygon,
    extrudePoly,
    offsetPolyline,
    computeBoundingBox,
    convertToLocalCoordinate,
    removeCollinearPoints,
    createBasePolygon,
    checkPointsOrder,
    checkPathClosed,
    createClosedPath,
    checkPolygonInsidePolygon,
    groupFromWay,
    groupFromRel,
    groupFromNode,
    createGroup,
    createMesh,
    getColorFromTags,
    getWidthFromWay,
    getPolygonsFromMultipolygonRelation,
    getPolygonFromWay,
    createElementsFromNodes,
    createElementsFromWays,
    createElementsFromRels,
    setHeight,
    pointOnLine,
    distanceFromLine,
    distanceFromPoint,
    calcPointProjection,
    projectionParameterPointToSegment,
    pointOnLineFromParameter,
    twoLinesIntersectParameter: twoLinesIntersectParameter
}

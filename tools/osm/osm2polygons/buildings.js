const Decimal = require('decimal.js');
const { calcConvexHull } = require('./convexhull');
const { calcOmbb } = require('./ombb');
const { Vector} = require('./vector');
Decimal.set({ precision: 20, rounding: 1 })
poly2tri = require('poly2tri');


const HEIGHT_FOR_LEVEL = 3;
const DEFAULT_ROOF_COLOUR = "#cccccc";
const DEFAULT_BUILDING_COLOUR = "#eeeeee";
const USE_TRIANGLES_STRIP = false;

const createBuildings = (nodes,ways,rels) => {
    const simpleBuildings=createSimpleBuildings(ways);
    // const simpleBuildings=[];
    const complexBuildings=createComplexBuildings(rels);

    return simpleBuildings.concat(complexBuildings);
}

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

const getBuildingInfo = (tags) => {
    let minHeight,maxHeight,colour;
    let roofOrientation,roofHeight,roofShape,roofColour;

    if (tags["min_height"]) {
        minHeight=Number(tags["min_height"].replace("m",""));
    } else if (tags["building:min_level"]) {
        minHeight=Number(tags["building:min_level"].replace("m",""))*HEIGHT_FOR_LEVEL;
    } else {
        minHeight=0;
    }
    if (tags["height"]) {
        maxHeight=Number(tags["height"].replace("m",""));
    } else if (tags["building:levels"]) {
        maxHeight=Number(tags["building:levels"].replace("m",""))*HEIGHT_FOR_LEVEL;
    } else {
        maxHeight=HEIGHT_FOR_LEVEL;
    }

    if (tags["building:colour"]) {
        colour=tags["building:colour"];
    } else {
        colour=DEFAULT_BUILDING_COLOUR;
    }

    if (tags["roof:shape"]==="pyramidal" || tags["building:roof:shape"]==="pyramidal") {
        roofShape="pyramidal";
    } else if (tags["roof:shape"]==="gabled" || tags["building:roof:shape"]==="gabled") {
        roofShape="gabled";
    } else { //FLAT 
        roofShape="flat";
    }

    if (tags["roof:height"]) {
        roofHeight=Number(tags["roof:height"].replace("m",""));        
    } else if (tags["building:roof:height"]) {
        roofHeight=Number(tags["building:roof:height"].replace("m",""));  
    } else if (tags["roof:levels"]) {
        roofHeight=Number(tags["roof:levels"].replace("m",""))*HEIGHT_FOR_LEVEL;
    } else if (tags["building:roof:levels"]) {
        roofHeight=Number(tags["building:roof:levels"].replace("m",""))*HEIGHT_FOR_LEVEL;
    } else {
        if (roofShape==="flat") {
            roofHeight=0;
        } else {
            roofHeight=HEIGHT_FOR_LEVEL;
        }
    }

    if (tags["roof:orientation"]) {
        roofOrientation=tags["roof:orientation"];
    } else {
        roofOrientation="across";
    }

    if (tags["roof:colour"]) {
        roofColour=tags["roof:colour"];
    } else if (tags["building:colour"]) {
        roofColour=tags["building:colour"];
    } else {        
        roofColour=DEFAULT_ROOF_COLOUR;
    }

    if (maxHeight-roofHeight<=minHeight) {
        roofHeight=0;
        roofShape="flat";
    } else {
        maxHeight=maxHeight-roofHeight;
    }
    roofMinHeight=maxHeight;
    roofMaxHeight=maxHeight+roofHeight;

    return {
        minHeight: minHeight,
        maxHeight: maxHeight,
        colour: colour,
        roof: {
            minHeight: roofMinHeight,
            maxHeight: roofMaxHeight,
            shape: roofShape,
            orientation: roofOrientation,
            colour: roofColour
        }
    }
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

    let centerX = 0;
    let centerY = 0;
    let sizeX = 0;
    let sizeY = 0;
    points.forEach(p => {
        centerX = centerX+p.x;
        centerY = centerY+p.y;
        minX=Math.min(p.x,minX);
        minY=Math.min(p.y,minY);
        maxX=Math.max(p.x,maxX);
        maxY=Math.max(p.y,maxY);
    });

    centerX = centerX/points.length;
    centerY = centerY/points.length;
    sizeX = maxX-minX;
    sizeY = maxY-minY;

    return {minX,minY,maxX,maxY,centerX,centerY,sizeX,sizeY}
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
        point.x=n.x.toNumber();
        point.y=n.y.toNumber();
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
            point.x=n.x.toNumber();
            point.y=n.y.toNumber();
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

const createBuildingMesh = (lateralFaces, roofFaces, boundingBox, buildingInfo, tags, id) => {
    return {
        id: id,
        center: {
            x: boundingBox.centerX,
            y: boundingBox.centerY
        },
        lateralFaces: {
            faces: lateralFaces,
            colour: buildingInfo.colour,
            isTriangleStrip: USE_TRIANGLES_STRIP
        },
        roofFaces: {
            faces: roofFaces,
            colour: buildingInfo.roof.colour
        },
        tags: tags
    }
}

const createRoof = (polygon, roofInfo, convexHull, ombb) => {
    if (roofInfo.shape==="pyramidal") {
        return createPyramidalRoof(polygon, roofInfo);
    } else if (roofInfo.shape==="gabled") {
        return createGabledRoof(polygon, roofInfo, ombb);
        //return createFlatRoof(polygon, roofInfo);
    } else {
        return createFlatRoof(polygon, roofInfo);
    }
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
    const topFace = getTrianglesFromPolygon(polygon);
    topFace.forEach(t => t.z=roofInfo.maxHeight);

    return faces.concat(topFace);
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

const createGabledRoof = (polygon, roofInfo, ombb) => {

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

    const axis = new Vector(pointB.x-pointA.x,pointB.y-pointA.y);

    let faces=[];
    for (let i=0;i<polygon.length;i++) {
        const nextI=(i+1)%polygon.length;
        const p = polygon[i];
        const nextP = polygon[nextI];
        const prj = calcPointProjection(pointA, pointB, p);
        const nextPrj = calcPointProjection(pointA, pointB, nextP);

        const dir = new Vector(prj.x-p.x,prj.y-p.y).cross(axis);
        const nextDir = new Vector(prj.x-nextP.x,prj.y-nextP.y).cross(axis);

        if (Math.sign(dir)===Math.sign(nextDir)) {
            faces.push({x: p.x, y: p.y, z:roofInfo.minHeight});
            faces.push({x: nextP.x, y: nextP.y, z:roofInfo.minHeight});
            faces.push({x: nextPrj.x, y: nextPrj.y, z:roofInfo.maxHeight});

            faces.push({x: p.x, y: p.y, z:roofInfo.minHeight});
            faces.push({x: nextPrj.x, y: nextPrj.y, z:roofInfo.maxHeight});
            faces.push({x: prj.x, y: prj.y, z:roofInfo.maxHeight});
        } else {
            if (prj.x===nextPrj.x && prj.y===nextPrj.y) {
                faces.push({x: p.x, y: p.y, z:roofInfo.minHeight});
                faces.push({x: nextP.x, y: nextP.y, z:roofInfo.minHeight});
                faces.push({x: prj.x, y: prj.y, z:roofInfo.maxHeight});
            } else {
                faces.push({x: p.x, y: p.y, z:roofInfo.minHeight});
                faces.push({x: prj.x, y: prj.y, z:roofInfo.minHeight});
                faces.push({x: prj.x, y: prj.y, z:roofInfo.maxHeight});

                faces.push({x: prj.x, y: prj.y, z:roofInfo.minHeight});
                faces.push({x: nextPrj.x, y: nextPrj.y, z:roofInfo.minHeight});
                faces.push({x: nextPrj.x, y: nextPrj.y, z:roofInfo.maxHeight});

                faces.push({x: prj.x, y: prj.y, z:roofInfo.minHeight});
                faces.push({x: nextPrj.x, y: nextPrj.y, z:roofInfo.maxHeight});
                faces.push({x: prj.x, y: prj.y, z:roofInfo.maxHeight});

                faces.push({x: nextPrj.x, y: nextPrj.y, z:roofInfo.minHeight});
                faces.push({x: nextP.x, y: nextP.y, z:roofInfo.minHeight});
                faces.push({x: nextPrj.x, y: nextPrj.y, z:roofInfo.maxHeight});                
            }
        }

        //console.log("PRJ", prj);
    }

    return faces;
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

const createSimpleBuildings = (ways) => {

    const buildings=[];
    ways.filter(w => !w.inRelation).forEach(w => {  // && w.id===364313092
        //Min 2 points
        if (w.nodes.length>2) {
            //Only closed path (first point id must be equal to last point id)
            if (w.nodes[0].id===w.nodes[w.nodes.length-1].id) {

                try {
                    const points = removeCollinearPoints(w.nodes); 
                    if (points.length<3) {
                        console.log("Invalid way", w.id);
                    }  else {
                        let localBoundingBox = computeBoundingBox(points);
                        convertToLocalCoordinate(points, localBoundingBox.centerX, localBoundingBox.centerY);
                        //Using: https://github.com/geidav/ombb-rotating-calipers
                        //with some modifications
                        
                        const convexHull = calcConvexHull(points);
                        checkPointsOrder(points,convexHull);
                        const orientedMinBoundingBox = calcOmbb(convexHull);

                        const buildingInfo = getBuildingInfo(w.tags);
                        const basePolygon = createBasePolygon(points);

                        //Compute lateral faces
                        const lateralFaces = extrudePoly(basePolygon, buildingInfo.minHeight, buildingInfo.maxHeight, USE_TRIANGLES_STRIP);
                        //Compute roof faces
                        const roofFaces = createRoof(basePolygon, buildingInfo.roof, convexHull, orientedMinBoundingBox);

                        //Create building mesh
                        const building = createBuildingMesh(lateralFaces, roofFaces, localBoundingBox, buildingInfo, w.tags, "w-"+w.id);

                        buildings.push(building);
                    }
                } catch (ex) {
                    console.log(`Error in ${w.id} way`, ex);
                }
            }
        }
    });

    return buildings;
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
    let lastNodeId=member.ref.nodes[member.ref.nodes.length-1].id;

    let closed=false;
    while (!closed) {
        const nextMember=members.find(m => m.role===role && m.ref.nodes[0].id===lastNodeId);
        if (nextMember!==null) {
            if (nextMember.ref.id===firstId) {
                closed=true;
            } else {
                //Search for next node
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

const createComplexPolygonRoof = (outerPolygon, innerPolygons, roofInfo) => {

    const topFace=getTrianglesFromComplexPolygon(outerPolygon.points, innerPolygons);
    topFace.forEach(t => t.z=roofInfo.maxHeight);

    return topFace;
}

const createComplexBuildings = (rels) => {
    const buildings=[];

    rels.forEach(r => {
        try {
            //Clone members for elaboration
            const polygons = [];
            const members = JSON.parse(JSON.stringify(r.members));
            //Create polygons from closed ways
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

            while (members.filter(m=>m.processed===true).length<members.length) 
            {
                for (i=0;i<members.length;i++) {
                    if (!members[i].processed) {
                        const connectedMembers = createClosedPath(members[i], members);
                        if (connectedMembers.length>0) {
                            console.log("Create polygon");
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

            //
            console.log("Found "+polygons.length+" polygons");

            let relPoints = [];
            polygons.forEach(p => {
                p.points=[];
                p.nodes.forEach(n => p.points.push({x: Number(n.x), y: Number(n.y)}));
                relPoints=relPoints.concat(p.points);               
            });
            const localBoundingBox=computeBoundingBox(relPoints);
            polygons.forEach(p => {
                convertToLocalCoordinate(p.points, localBoundingBox.centerX, localBoundingBox.centerY);
                const convexHull = calcConvexHull(p.points);
                checkPointsOrder(p.points,convexHull);
            });

            const buildingInfo = getBuildingInfo(r.tags);

            let roofFaces=[];
            let lateralFaces=[];
            polygons.filter(p => p.role==="outer").forEach(o => {
                o.holes=[];
                polygons.filter(p => p.role==="inner" && p.container===undefined).forEach(i => {
                    if (checkPolygonInsidePolygon(o.points, i.points)) {
                        i.container=o;
                        o.holes.push(i);
                    }
                });

                lateralFaces = lateralFaces.concat(extrudePoly(o.points, buildingInfo.minHeight, buildingInfo.maxHeight, USE_TRIANGLES_STRIP));
                o.holes.forEach(h => {
                    lateralFaces = lateralFaces.concat(extrudePoly(h.points, buildingInfo.minHeight, buildingInfo.maxHeight, USE_TRIANGLES_STRIP));
                })
                //Compute roof faces
                const roofFaces = createComplexPolygonRoof(o, o.holes, buildingInfo.roof);
                const building = createBuildingMesh(lateralFaces, roofFaces, localBoundingBox, buildingInfo, r.tags, "r-"+r.id);

                buildings.push(building);
            })

        } catch (ex) {
            console.log(`Error in ${r.id} relation`, ex);
        }
    });
    return buildings;
}

module.exports = { createBuildings }
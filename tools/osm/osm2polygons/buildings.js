const Decimal = require('decimal.js');
Decimal.set({ precision: 20, rounding: 1 })
poly2tri = require('poly2tri');


const HEIGHT_FOR_LEVEL = 3;
const DEFAULT_ROOF_COLOUR = "#cccccc";
const DEFAULT_BUILDING_COLOUR = "#eeeeee";
const USE_TRIANGLES_STRIP = false;

const createBuildings = (nodes,ways,rels) => {
    const triangles=createSimpleBuildings(ways);

    return triangles;
}

const getTriangles = (polyPoints) => {
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

const getBuildingInfo = (tags) => {
    let minHeight,maxHeight,colour;
    let roofOrientation,roofHeight,roofShape,roofColour;

    if (tags.min_height) {
        minHeight=Number(tags.min_height.replace("m",""));
    } else if (tags.min_levels) {
        minHeight=Number(tags.min_levels.replace("m",""))*HEIGHT_FOR_LEVEL;
    } else {
        minHeight=0;
    }
    if (tags.height) {
        maxHeight=Number(tags.height.replace("m",""));
    } else if (tags.levels) {
        maxHeight=Number(tags.levels.replace("m",""))*HEIGHT_FOR_LEVEL;
    } else {
        maxHeight=HEIGHT_FOR_LEVEL;
    }
    if (tags["building:colour"]) {
        colour=tags["building:colour"];
    } else {
        colour=DEFAULT_BUILDING_COLOUR;
    }

    if (tags["roof:shape"]==="pyramidal") {
        roofShape="pyramidal";
    } else if (tags["roof:shape"]==="gabled") {
        roofShape="gabled";
    } else { //FLAT 
        roofShape="flat";
    }

    if (tags["roof:height"]) {
        roofHeight=Number(tags["roof:height"].replace("m",""));        
    } else if (tags["roof:levels"]) {
        roofHeight=Number(tags["roof:levels"].replace("m",""))*HEIGHT_FOR_LEVEL;
    } else {
        roofHeight=HEIGHT_FOR_LEVEL;
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
    return cp<1e-10;
}

const computeBoundingBox = (points) => {
    let minX=new Decimal(Number.MAX_VALUE);
    let minY=new Decimal(Number.MAX_VALUE);
    let maxX=new Decimal(-Number.MAX_VALUE);
    let maxY=new Decimal(-Number.MAX_VALUE);

    let centerX = new Decimal(0);
    let centerY = new Decimal(0);
    let sizeX = new Decimal(0);
    let sizeY = new Decimal(0);
    points.forEach(p => {
        centerX = centerX.add(p.x);
        centerY = centerY.add(p.y);
        minX=Decimal.min(p.x,minX);
        minY=Decimal.min(p.y,minY);
        maxX=Decimal.max(p.x,maxX);
        maxY=Decimal.max(p.y,maxY);
    });

    centerX = centerX.div(points.length);
    centerY = centerY.div(points.length);
    sizeX = maxX.sub(minX);
    sizeY = maxY.sub(minY);

    return {minX,minY,maxX,maxY,centerX,centerY,sizeX,sizeY}
}

const convertToLocalCoordinate = (points, originX, originY) => {

    points.forEach(p => {
        p.x=p.x.sub(originX);
        p.y=p.y.sub(originY);
    });
}

const removeCollinearPoints = (nodes) => {

    let points = [...nodes.slice(0,nodes.length-1)];

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
//console.log(points);
    return points;
}

const createBasePolygon = (points) => {
    const polygon = [];

    points.forEach(p => {
        polygon.push({
            x: p.x.toNumber(),
            y: p.y.toNumber()
        });
    });

    return polygon;
}

const createBuildingMesh = (lateralFaces, roofFaces, boundingBox, buildingInfo, tags, id) => {
    return {
        id: id,
        center: {
            x: boundingBox.centerX.toNumber(),
            y: boundingBox.centerY.toNumber()
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

const createRoof = (polygon, roofInfo) => {
    if (roofInfo.shape==="pyramidal") {
        return createPyramidalRoof(polygon, roofInfo);
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
    const topFace = getTriangles(polygon);
    topFace.forEach(t => t.z=roofInfo.maxHeight);

    return faces.concat(topFace);
}

const createSimpleBuildings = (ways) => {

    const buildings=[];
    ways.filter(w => !w.inRelation).forEach(w => {
        //Min 2 points
        if (w.nodes.length>2) {
            //Only closed path (first point id must be equal to last point id)
            if (w.nodes[0].id===w.nodes[w.nodes.length-1].id) {

                try {
                    const points = removeCollinearPoints(w.nodes);
                    let localBoundingBox = computeBoundingBox(points);
                    convertToLocalCoordinate(points, localBoundingBox.centerX, localBoundingBox.centerY);

                    const buildingInfo = getBuildingInfo(w.tags);
                    const basePolygon = createBasePolygon(points);

                    //Compute lateral faces
                    const lateralFaces = extrudePoly(basePolygon, buildingInfo.minHeight, buildingInfo.maxHeight, USE_TRIANGLES_STRIP);
                    //Compute roof faces
                    const roofFaces = createRoof(basePolygon, buildingInfo.roof);

                    //Create building mesh
                    const building = createBuildingMesh(lateralFaces, roofFaces, localBoundingBox, buildingInfo, w.tags, w.id);

                    buildings.push(building);
                } catch (ex) {
                    console.log(`Error in ${w.id} way`);
                }
            }
        }
    });

    return buildings;
}

module.exports = { createBuildings }
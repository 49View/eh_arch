const Decimal = require('decimal.js');
Decimal.set({ precision: 20, rounding: 1 })
poly2tri = require('poly2tri');


const HEIGHT_FOR_LEVEL = 3;

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

const getHeightsByTags = (tags) => {
    let minHeight,maxHeight,minRoofHeight,maxRoofHeight;

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

    return {minHeight,maxHeight,minRoofHeight,maxRoofHeight}
}

const extrudePolyStripe = (poly, minHeight, maxHeight) => {
    
    const triStripe = [];

    poly.forEach(p => {
        triStripe.push({x:p.x, y:p.y, z:minHeight});
        triStripe.push({x:p.x, y:p.y, z:maxHeight});
    });

    return triStripe;
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

const createSimpleBuildings = (ways) => {

    const buildings=[];
    ways.filter(w => !w.inRelation && w.id===364313092).forEach(w => {
        //Min 2 points
        if (w.nodes.length>2) {
            //Only closed path (first point id must be equal to last point id)
            if (w.nodes[0].id===w.nodes[w.nodes.length-1].id) {

                const points = removeCollinearPoints(w.nodes);
                let localBoundingBox = computeBoundingBox(points);
                convertToLocalCoordinate(points, localBoundingBox.centerX, localBoundingBox.centerY);

                const groundPoly = [];
                const roofPoly = [];

                const {minHeight,maxHeight,minRoofHeight,maxRoofHeight} = getHeightsByTags(w.tags);
                for (let i=0;i<points.length;i++) {
                    const n=points[i];
                    groundPoly.push({x:n.x.toNumber(),y:n.y.toNumber()});
                    roofPoly.push({x:n.x.toNumber(),y:n.y.toNumber()});
                }

                //const groundFaces = getTriangles(groundPoly);
                try {
                    const roofTriangles = getTriangles(roofPoly);
                    roofTriangles.forEach(t => t.z=maxHeight);
                    const lateralTrianglesStrip = extrudePolyStripe(groundPoly, minHeight, maxHeight);

                    const building={
                        boundingBox: localBoundingBox,
                        triangles: roofTriangles,
                        trianglesStrip: lateralTrianglesStrip
                    }

                    buildings.push(building);
                } catch (ex) {
                    console.log(`Error in ${w.id} way`, ex);
                }
            }
        }
    });

    return buildings;
}

module.exports = { createBuildings }
poly2tri = require('poly2tri');


const HEIGHT_FOR_LEVEL = 3;

const createBuildings = (nodes,ways,rels) => {
    const triangles=createSimpleBuildings(ways);

    return triangles;
}

const getTriangles = (polyPoints) => {
    const swctx = new poly2tri.SweepContext(polyPoints);
    swctx.triangulate();
    const tri= swctx.getTriangles()
    const points=[];
    tri.forEach(t => {
        points.push({x: t.points_[0].x, y: t.points_[0].y, z: t.points_[0].z});
        points.push({x: t.points_[1].x, y: t.points_[1].y, z: t.points_[1].z});
        points.push({x: t.points_[2].x, y: t.points_[2].y, z: t.points_[2].z});
    });

    return points;
}

const getHeightsByTags = (tags) => {
    let minHeight,maxHeight,minRoofHeight,maxRoofHeight;

    if (tags.min_height) {
        minHeight=Number(tags.min_height);
    } else if (tags.min_levels) {
        minHeight=Number(tags.min_levels)*HEIGHT_FOR_LEVEL;
    } else {
        minHeight=0;
    }
    if (tags.height) {
        maxHeight=Number(tags.height);
    } else if (tags.levels) {
        maxHeight=Number(tags.levels)*HEIGHT_FOR_LEVEL;
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

const createSimpleBuildings = (ways) => {

    const buildings=[];
    ways.filter(w => !w.inRelation).forEach(w => {
        //Min 2 points
        if (w.nodes.length>2) {
            //Only closed path (first point id must be equal to last point id)
            if (w.nodes[0].id===w.nodes[w.nodes.length-1].id) {
                const groundPoly = [];
                const roofPoly = [];

                const {minHeight,maxHeight,minRoofHeight,maxRoofHeight} = getHeightsByTags(w.tags);
                for (let i=0;i<w.nodes.length-2;i++) {
                    const n=w.nodes[i];
                    groundPoly.push({x:n.x,y:n.y,z:minHeight});
                    roofPoly.push({x:n.x,y:n.y,z:maxHeight});
                }

                //const groundFaces = getTriangles(groundPoly);
                try {
                    const roofTriangles = getTriangles(roofPoly.reverse());
                    const lateralTrianglesStrip = extrudePolyStripe(groundPoly, minHeight, maxHeight);

                    const building={
                        triangles: roofTriangles,
                        trianglesStrip: lateralTrianglesStrip
                    }

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
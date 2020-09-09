const { Vector} = require('./vector');
const {
    createMesh,
    createElementsFromWays,
    computeBoundingBox,
    removeCollinearPoints,
    getTrianglesFromPolygon,
    setHeight,
    convertToLocalCoordinate
} = require('./osmHelper.js');
const ClipperLib = require('js-clipper');


const createRoads = (nodes,ways,rels) => {
    console.log("----------------------------------------------");
    console.log("ROADS");
    console.log("----------------------------------------------");

    const roads = [];
    
    ways.filter(w => w.tags && w.tags["highway"]).filter(w => w.nodes[0].id!==w.nodes[w.nodes.length-1].id).forEach(w => {
        const road = roadFromWay(w);
        if (road!==null) roads.push(road);
    });
    
    console.log(`Found ${roads.length} roads`);
    console.log("----------------------------------------------");

    return roads;
}

const createRoadMesh = (id, tags, boundingBox, faces) => {

    const groups=[];

    groups.push({
        faces: faces,
        colour: "#000000",
        isTriangleStrip: false
    });
    
    return createMesh(id, tags, "road", boundingBox, groups);    
}

const roadFromWay = (way) => {

    let road=null;
    try {
        points=[];
        way.nodes.forEach(n => {
            const point = { ...n };
            point.x=n.x; //n.x.toNumber();
            point.y=n.y; //n.y.toNumber();
            points.push(point);
        }) ;
        const localBoundingBox = computeBoundingBox(points);
        convertToLocalCoordinate(points, localBoundingBox.centerX, localBoundingBox.centerY);

        var subj = new ClipperLib.Paths();
        var solution = new ClipperLib.Paths();
        subj[0] = points.map(p => { return {X: p.x, Y: p.y}});
        var scale = 100;
        ClipperLib.JS.ScaleUpPaths(subj, scale);
        var co = new ClipperLib.ClipperOffset(2, 0.25);
        co.AddPaths(subj, ClipperLib.JoinType.jtRound, ClipperLib.EndType.etOpenRound);
        co.Execute(solution, 300.0);
        ClipperLib.JS.ScaleDownPaths(solution, scale);

        const polygon = solution[0].map(p => { return { x: p.X, y:p.Y}});
        const faces = getTrianglesFromPolygon(polygon);
        setHeight(faces,3);

        road = createRoadMesh("w-"+way.id, way.tags, localBoundingBox, faces);
    } catch (ex) {
        console.log(`Error creating road from way ${way.id}`);
    }
    return road;
}

module.exports = { createRoads }
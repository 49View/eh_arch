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
    
    ways.filter(w => w.tags && w.tags["highway"]).forEach(w => {
        const road = roadFromWay(w);
        if (road!==null) roads.push(road);
    });
    
    console.log(`Found ${roads.length} roads`);
    console.log("----------------------------------------------");

    return roads;
}

const createRoadMesh = (id, tags, boundingBox, faces, color) => {

    const groups=[];

    groups.push({
        faces: faces,
        colour: color,
        isTriangleStrip: false
    });
    
    return createMesh(id, tags, "road", boundingBox, groups);    
}

const roadFromWay = (way) => {

    let road=null;
    let roadWidth = 2;
    let roadColor = "#FFFFFF";
    let roadLane = 1;
    try {
        switch (way.tags["highway"]) {
            case "motorway":
                roadWidth = 3;
                roadLane = 3;
                roadColor = "#DF2E6B";
                break;
            case "motorway_link":
                roadWidth = 2.75;
                roadLane = 2;
                roadColor = "#DF2E6B";
                break;
            case "thrunk":
                roadWidth = 3;
                roadLane = 2;
                roadColor = "#FBB29A";
                break;
            case "thrunk_link":
                roadWidth = 2.75;
                roadLane = 2;
                roadColor = "#FBB29A";
                break;
            case "primary":
                roadWidth = 3;
                roadLane = 2;
                roadColor = "#FDD7A1";
                break;
            case "primary_link":
                roadWidth = 2.75;
                roadLane = 2;
                roadColor = "#FDD7A1";
                break;
            case "secondary":
                roadWidth = 2.75;
                roadLane = 2;
                roadColor = "#F1EEE8";
                break;
            case "tertiary":
                roadWidth = 2.75;
                roadLane = 2;
                roadColor = "#FFFFFF";
                break;
            case "unclassified":
            case "residential":
                roadWidth = 2.75;
                roadLane = 1;
                roadColor = "#FFFFFF";
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
                roadColor = "#FFFFFF";
        }
        let faces=[]
        let points=[];
        way.nodes.forEach(n => {
            const point = { ...n };
            point.x=n.x; //n.x.toNumber();
            point.y=n.y; //n.y.toNumber();
            points.push(point);
        }) ;

        const localBoundingBox = computeBoundingBox(points);
        convertToLocalCoordinate(points, localBoundingBox.centerX, localBoundingBox.centerY);


        let pointsPartial = [];

        // if (points[0].id===points[points.length-1].id) {
        //     //endType = ClipperLib.EndType.etClosedLine;
        //     //points.splice(points.length-1,1);
        //     pointsPartial.push(points.slice(0, points.length-1));
        //     pointsPartial.push(points.slice(points.length-2, points.length));
        // } else {
        //     pointsPartial.push(points);
        // }

        let startIndex = 0;
        for (let i=startIndex;i<points.length;i++) {
            for (j=i+1;j<points.length;j++) {
                if (points[i].id===points[j].id) {
                    if (startIndex<j) {
                        pointsPartial.push(points.slice(startIndex, j));
                    }
                    startIndex=j;
                    i=j;
                    break;
                }
            }            
        }

        if (startIndex===0) {
            pointsPartial.push(points);
        } else {
            pointsPartial.push(points.slice(startIndex-1, points.length));
        }

        pointsPartial.forEach(points => {
            let subj = new ClipperLib.Paths();
            let solution = new ClipperLib.Paths();
            subj[0] = points.map(p => { return {X: p.x, Y: p.y}});
            var scale = 100;
            ClipperLib.JS.ScaleUpPaths(subj, scale);
            var co = new ClipperLib.ClipperOffset(2, 0.25);
            co.AddPaths(subj, ClipperLib.JoinType.jtRound, ClipperLib.EndType.etOpenRound);
            co.Execute(solution, roadWidth*roadLane*scale);
            ClipperLib.JS.ScaleDownPaths(solution, scale);
    
            solution.forEach(s => {
                const polygon = s.map(p => { return { x: p.X, y:p.Y}});
                faces = faces.concat(getTrianglesFromPolygon(polygon));
            });
        });

        setHeight(faces,1.5);

        road = createRoadMesh("w-"+way.id, way.tags, localBoundingBox, faces, "#000000");
    } catch (ex) {
        console.log(`Error creating road from way ${way.id}`);
    }
    return road;
}

module.exports = { createRoads }
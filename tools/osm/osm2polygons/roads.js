const {
    computeBoundingBox,
    getTrianglesFromPolygon,
    setHeight,
    getColorFromTags,
    getWidthFromWay,
    convertToLocalCoordinate
} = require('./osmHelper.js');
const ClipperLib = require('js-clipper');
const {createGroup} = require("./osmHelper");


const createRoads = (nodes,ways) => {
    console.log("----------------------------------------------");
    console.log("ROADS");
    console.log("----------------------------------------------");

    let roads = [];
    const roadTypeName = "road";

    ways.filter(w => w.tags && !w.tags["area"] && (w.tags["highway"] || (w.tags["railway"] && w.tags["railway"]==="rail")) ).forEach(w => {
        const road = roadFromWay(w, roadTypeName);
        if (road!==null) roads.push(road);
    });

    console.log(`Found way ${roads.length} roads`);
    console.log("----------------------------------------------");

    return roads;//.concat(relRoads).concat(areaRoads);
}

const roadFromWay = (way, name) => {

    let road=null;
    const roadColor = getColorFromTags(way.tags);
    const {roadWidth, roadLane} = getWidthFromWay(way);

    try {
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
            for (let j=i+1;j<points.length;j++) {
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
            const scale = 100;
            ClipperLib.JS.ScaleUpPaths(subj, scale);
            let co = new ClipperLib.ClipperOffset(2, 0.25);
            co.AddPaths(subj, ClipperLib.JoinType.jtRound, ClipperLib.EndType.etOpenRound);
            co.Execute(solution, roadWidth*roadLane*scale);
            ClipperLib.JS.ScaleDownPaths(solution, scale);

            solution.forEach(s => {
                const polygon = s.map(p => { return { x: p.X, y:p.Y}});
                faces = faces.concat(getTrianglesFromPolygon(polygon));
            });
        });

        setHeight(faces,.5);

        road = createGroup("w-"+way.id, way.tags, name, localBoundingBox, faces, roadColor);
    } catch (ex) {
        console.log(`Error creating road from way ${way.id}`);
    }
    return road;
}

module.exports = { createRoads }

include <MCAD/boxes.scad>;

holeSpacingH = 88;
holeSpacingV = 65;
holeToEdge = 4.5;
holeToBottomEdge = 20;
wallThickness = 3;
baseThickness = 3;
innerHeight = 44;
pillarHeight = 5.6;
pillarDiameter = 7;
pillarHoleDiameter = 2.8;
screenLength = 78.5;
screenWidth = 51.5;
m3Diameter = 3.4;
m4Diameter=4;
insertHoleDiameter=4.2;
insertLength=5.4;
overlap = 0.05;
innerLength = holeSpacingH + 2 * holeToEdge;
innerWidth = holeSpacingV + holeToEdge + holeToBottomEdge;
holeVoffset=(holeToBottomEdge-holeToEdge)/2;
encoderYoffsetFromHole=11.7;
piezoYoffsetFromHole=encoderYoffsetFromHole;
encoderXoffsetFromHole=7;
piezoXoffsetFromHole=17.5+encoderXoffsetFromHole;
encoderHoleDiameter=8;
piezoHoleDiameter=3;

lugSeparation = innerWidth - 10;
lugHeight=3;
lugWidth=6;
lugHeightOffset=innerHeight+baseThickness-lugHeight-3;
lidPillarSection=7;
lidPillarHeight=20;
lidPillarSeparation = innerLength - lidPillarSection + 2;
lidPillarYoffset=-innerWidth/2+lidPillarSection/2-1;

handleGripLength = 110;
handleSection = 25;
handleOverlapLength=wallThickness-overlap;
shaftDiameter=10;
handleFixingLength = 8;
handleLength = handleGripLength + handleOverlapLength + shaftDiameter + handleFixingLength;
slotWidth=3;

wireSlotWidth=3;
wireSlotDepth=7;

module pillar(x, y, z) {
    translate([x,y,z]) {
        cylinder(r=pillarDiameter/2,h=pillarHeight+overlap,$fn=20);
    }
}

module m3Hole(x,y,height) {
    translate([x,y,0]) 
        cylinder(r=m3Diameter/2,h=height,$fn=16);
}

module handle() {
    difference() {
        union() {
            translate([0,handleLength/2,handleSection/4])
                roundedBox([handleSection, handleLength, handleSection/2], 1, false);
            translate([0,0,handleSection/2]) rotate([-90,0,0])
                cylinder(r=handleSection/2,h=handleLength,$fn=64);
        }
        translate([0,handleFixingLength+shaftDiameter/2,-overlap]) cylinder(r=shaftDiameter/2,h=handleSection+2*overlap,$fn=64);
        translate([-50,handleFixingLength/2,handleSection/2 - 3]) 
            rotate([0,90,0]) cylinder(r=m4Diameter/2,h=100,$fn=32);
        translate([-slotWidth/2,-overlap,-overlap]) cube([slotWidth,handleFixingLength+shaftDiameter/2,handleSection+2*overlap]);
    }
}

module lidPillar() {
    translate([-lidPillarSection/2,-lidPillarSection/2,0])
        difference() {
            cube([lidPillarSection,lidPillarSection,lidPillarHeight]);
            rotate([0,30,0]) translate([0,-overlap,0]) cube([lidPillarSection+2*overlap,lidPillarSection+2*overlap,lidPillarHeight]);
            rotate([-30,0,0]) translate([-overlap,0,0]) cube([lidPillarSection+2*overlap,lidPillarSection+2*overlap,lidPillarHeight]);
        }
}

module enclosure() {
    difference() {
        union () {
            difference() {
                translate([0,0,(innerHeight+baseThickness)/2])
                    roundedBox([innerLength+2*wallThickness,innerWidth+2*wallThickness,innerHeight+baseThickness], 1, false);
                translate([-innerLength/2,-innerWidth/2,baseThickness])
                    cube([innerLength,innerWidth,innerHeight+baseThickness]);
                translate([-screenLength/2,-screenWidth/2+holeVoffset,-overlap])
                    cube([screenLength, screenWidth, baseThickness+2*overlap]);
               // Shaft hole
               translate([-holeSpacingH/2+encoderXoffsetFromHole,-holeSpacingV/2+holeVoffset-encoderYoffsetFromHole,-overlap])
                    cylinder(r=encoderHoleDiameter/2,h=baseThickness+2*overlap,$fn=32);
               // Piezo hole
               translate([-holeSpacingH/2+piezoXoffsetFromHole,-holeSpacingV/2+holeVoffset-piezoYoffsetFromHole,-overlap])
                    cylinder(r=piezoHoleDiameter/2,h=baseThickness+2*overlap,$fn=16);
           }
           pillar(-holeSpacingH/2,-holeSpacingV/2+holeVoffset,baseThickness-overlap);
           pillar(-holeSpacingH/2,holeSpacingV/2+holeVoffset,baseThickness-overlap);
           pillar(holeSpacingH/2,-holeSpacingV/2+holeVoffset,baseThickness-overlap);
           pillar(holeSpacingH/2,holeSpacingV/2+holeVoffset,baseThickness-overlap);
           
           // Pillars for fixing the lid
           translate([(-lidPillarSeparation)/2,lidPillarSection/2-innerWidth/2-1,baseThickness+innerHeight-lidPillarHeight])
                lidPillar();
           translate([(lidPillarSeparation)/2,lidPillarSection/2-innerWidth/2-1,baseThickness+innerHeight-lidPillarHeight])
                rotate([0,0,90]) lidPillar();
       }
       translate([0,0,-overlap]) m3Hole(-holeSpacingH/2,-holeSpacingV/2+holeVoffset,pillarHeight+baseThickness+2*overlap);
       translate([0,0,-overlap]) m3Hole(-holeSpacingH/2,holeSpacingV/2+holeVoffset,pillarHeight+baseThickness+2*overlap);
       translate([0,0,-overlap]) m3Hole(holeSpacingH/2,-holeSpacingV/2+holeVoffset,pillarHeight+baseThickness+2*overlap);
       translate([0,0,-overlap]) m3Hole(holeSpacingH/2,holeSpacingV/2+holeVoffset,pillarHeight+baseThickness+2*overlap);
       // Holes for lid lugs
       translate([-lugSeparation/2-lugWidth/2,innerWidth/2-overlap,lugHeightOffset]) cube([lugWidth, wallThickness+2*overlap, lugHeight]);
       translate([lugSeparation/2-lugWidth/2,innerWidth/2-overlap,lugHeightOffset]) cube([lugWidth, wallThickness+2*overlap, lugHeight]);
       // Holes for lid screws
       translate([-lidPillarSeparation/2,lidPillarYoffset,baseThickness+innerHeight-insertLength]) cylinder(r=insertHoleDiameter/2,h=50,$fn=32);
       translate([lidPillarSeparation/2,lidPillarYoffset,baseThickness+innerHeight-insertLength]) cylinder(r=insertHoleDiameter/2,h=50,$fn=32);
    }
}

difference() {
    union() {
        enclosure();
        translate([0,-handleLength-innerWidth/2-wallThickness+handleOverlapLength,0]) handle();
    }
    // Slot in handle for wires
    translate([-wireSlotWidth/2,-handleLength-innerWidth/2+handleFixingLength+shaftDiameter+2,handleSection-wireSlotDepth])
        cube([wireSlotWidth,handleLength-handleFixingLength-shaftDiameter-6,wireSlotDepth]);
    // Slot in enclosure for wires
    translate([-wireSlotWidth/2,-innerWidth/2-wallThickness-overlap,baseThickness+innerHeight-wireSlotDepth])
        cube([wireSlotWidth,baseThickness+2*overlap,wireSlotDepth+overlap]);
}

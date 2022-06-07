// Metal detector arm rest

include <MCAD/boxes.scad>;

overlap=0.05;
m4Diameter=4;
armDiameter=60;
wallThickness=3;
length=50;
shaftDiameter=10;
handleGripLength = armDiameter/2+wallThickness+4;
handleSection = 25;
handleOverlapLength=wallThickness-overlap;
handleFixingLength = 8;
handleLength = handleGripLength + handleOverlapLength + shaftDiameter + handleFixingLength;
slotWidth=3;
verlap=0.05;

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

module armRest() {
    difference() {
        union() {
            cylinder(h=length,r=armDiameter/2+wallThickness,$fn=128);
            translate([0,-handleLength,0]) handle();
        }
        translate([0,0,-overlap]) cylinder(h=length+2*overlap,d=armDiameter,$fn=128);
        translate([-armDiameter,15,-overlap]) cube([2*armDiameter,armDiameter,length+2*overlap]);
    }
}

armRest();


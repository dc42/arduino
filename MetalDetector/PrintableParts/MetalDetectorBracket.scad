width=20;
length=40;
shaftDiameter=10;
threadDiameter=8;
screwDiameter=4.5;
height=20;
separation=17;
slotWidth=3;
screwOffset=(length-width/2+separation+shaftDiameter/2)/2;
curvatureRadius=30;
overlap=0.05;

difference() {
    union() {
        translate([-width/2,0,0])
            cube([width,length-width/2,height]);
        cylinder(r=width/2,h=height,$fn=128);
    }
    translate([0,0,-overlap])
        cylinder(r=threadDiameter/2,h=height+2*overlap,$fn=64);
    translate([0,separation,-overlap])
        cylinder(r=shaftDiameter/2,h=height+2*overlap,$fn=64);
    translate([-slotWidth/2,separation,-overlap])
        cube([slotWidth,100,height+2*overlap]);
    translate([-width/2-overlap,screwOffset,height/2])
        rotate([0,90,0])
            cylinder(r=screwDiameter/2,h=width+2*overlap,$fn=32);
    translate([-width/2-overlap,0,height+curvatureRadius-2.5])
        rotate([0,90,0])
            cylinder(r=curvatureRadius,h=width+2*overlap,$fn=256);
}

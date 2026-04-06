$fn=96;
px = 7 * 25.4;
py = .8 * 25.4 +3 +2;
pz = 2;

ix = 6 * 25.4;
iy = 0.4 * 25.4;
iz = 2 + 2;  // adding 2 for offset

radius = 3/2;
offset = (py-iy)/4;

module hole(x,y) 
{
    translate([x,y,-1])
    {
        cylinder(4,radius,radius);
    };
};
module plate() {
    cube([px, py, pz]);
};

module innerCutout() {
    translate([px-7-ix,(py-iy)/2,-1])
    {
        cube([ix,iy,iz]);
    };
}

difference()
{
    plate();
    innerCutout();
    
    hole(offset,offset);
    hole(offset,py-offset);
    hole(px-offset,offset);
    hole(px-offset,py-offset); 
    hole(px/2,py-offset); 
};


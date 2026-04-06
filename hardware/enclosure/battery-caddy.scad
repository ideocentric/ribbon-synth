difference(){
cube([2.4,2.8,1]);
translate([0.1,-0.1,0.1]){
cube([2.2,2.8,2]);
};

translate([0.4, 0.4, -0.1]){
 cylinder(0.4,0.06,0.06, $fn=24);
};
translate([2, 0.4, -0.1]){
 cylinder(0.4,0.06,0.06, $fn=24);
};

translate([0.4, 2.4, -0.1]){
 cylinder(0.4,0.06,0.06, $fn=24);
};
translate([2, 2.4, -0.1]){
 cylinder(0.4,0.06,0.06, $fn=24);
};

};
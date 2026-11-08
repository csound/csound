struct Point x:i,y:i
opcode MakePoint():Point
 p:Point init 11,12
 xout p
endop
opcode MakePoints():Point[]
 points:Point[] init 1
 points[0].x=31
 points[0].y=32
 xout points
endop

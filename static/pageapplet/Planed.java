/**
 * Planed.java - 3D plane matrix operations: flip and project
 *
 * Noel Burton-Krahn <noel@burton-krahn.com>
 */
package com.burtonkrahn.pageapplet.scene;


/*
 * A class for reflecting and translating about a plane.  A plane
 * satisfies the equation Xx + Yy + Zz + W = 0.  x,y,z is the plane
 * normal, and w is the distance from the origin to a point on the
 * plane along the normal.
 */
public class Planed extends Vectord {
    protected Vectord p0 = null;
    
    public Planed(Vectord p0) {
	super(p0);

	this.p0 = p0;
    }

    /**
       p0 lies on the plane and the normal is p1-p0.
    **/
    public Planed(Vectord p0, Vectord p1) {
	super(p0.vec.length + 1);

	this.p0 = p0;
	add(p1);
	sub(p0);
	scale(1.0 / length());
	vec[vec.length - 1] = -dot(p0);
    }
    
    public Vectord normal() {
        return new Vectord(this.vec, 0, vec.length - 1);
    }

    // distance from origin along normal to a point on the plane
    public double distanceFromOrigin() {
        return vec[vec.length - 1];
    }

    // distance from a point p to the plane along the normal
    public double distanceFromPoint(Vectord p) {
        return normal().dot(p) + this.distanceFromOrigin();
    }

    // translate the plane by d along its normal
    public void translate(double d) {
	Vectord n = normal();
	n.scale(-d);
	p0.add(n);
        vec[vec.length - 1] += d;
    }

    // return a matrix that reflects 4D points over the plane
    public Matrix4x4d getReflectMatrix() {
	return getTranslateMatrix(-2);
    }

    // return a matrix that projects 4D points onto the plane
    public Matrix4x4d getProjectMatrix() {
	return getTranslateMatrix(-1);
    }

    // return a matrix that transforms the current axis so the Y axis
    // will point alone the plane normal, and the X and Z axis will
    // lie in the plane, and the plane's p0 will be <0,0,0>.
    public Matrix4x4d getAxisMatrix() {
	// plane normal
	double nx = vec[0];
	double ny = vec[1];
	double nz = vec[2];

	// plane offset
	double D = vec[3];  
	return new Matrix4x4d
	    (ny, nx, nz, p0.vec[0],
	     -nx, ny, nx, p0.vec[1],     
	     nz, nz, ny, p0.vec[2],
	      0,  0,  0,         1      
	     );
    }

    /**
       translate points along the plane normal, proportional to their
       distance from the plane, l.  This is used to make projection
       and reflection matricies:

       if l = -1: project points onto plane,
       if l = -2: reflect points across plane
    
       Derivation of translation matrix for reflecting a point p to a
       point r about plane (l = -2)
    
       r  = p  + -2*((p-p0) dot (p1-p0))*(p1-p0)
       r  = p  + -2*(p dot (p1-p0) - p0 dot (p1-p0))*(p1-p0)
    
       n = (p1-p0)
       D = -p0 dot (p1-p0)
       l = -2
    
       r  = p + l*(p dot n + D)*n
       r  = p + l*(px*nx + py*ny + pz*nz + D) * n
       r  = p + (px*l*nx + py*l*ny + pz*l*nz + l*D) * n
       rx = px + (px*l*nx + py*l*ny + pz*l*nz + l*D) * nx
       rx = px*(1+l*nx*nx) + py*l*ny*nx + pz*l*nz*nx + l*D*nx
    **/
    public Matrix4x4d getTranslateMatrix(double l) {

	// plane normal
	double nx = vec[0];
	double ny = vec[1];
	double nz = vec[2];

	// plane offset
	double D = vec[3];  
	
	return new Matrix4x4d
	    ((1+l*nx*nx), l*ny*nx,     l*nz*nx,     l*D*nx,
	     l*nx*ny,     (1+l*ny*ny), l*nz*ny,     l*D*ny,     
	     l*nx*nz,     l*ny*nz,     (1+l*nz*nz), l*D*nz,
	     0,           0,           0,           1      
	     );
    }
};


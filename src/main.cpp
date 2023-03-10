//Assignment 2 - Rasterization
//Brendan Schneider - bzs14

#include "Helpers.h"

#include <iostream>
#include <fstream>
#include <cmath>
#include <string>
#include <cfloat>
#include <limits>
#include <vector>

#include <GLFW/glfw3.h>
#include <Eigen/Core>
#include <Eigen/Geometry>


using namespace std;
using namespace Eigen;


//VBO and VAO Objects
GLuint VBO;
GLuint VAO;

//2d stuff-----------------------

//Triangle storage (up to 100)
int FullTriangles = 0;
MatrixXf TriangleArray(5, 300);
MatrixXf TriangleData(2, 100);

char mode = ' ';

//For insertion mode
MatrixXf TempPoint(5, 1);
MatrixXf TempLine(5, 2);
MatrixXf TempTriangle(5, 3);
MatrixXf FinalTriangle(5, 3);
int DrawPoints = 0; //Between 0 and 2

//For coloring mode
int VertexColorChoice = -1; //Stores vertex being colored

//For translation mode
int MouseState = 0, TriangleSelected = -1, PreviousTriangle = -1;
float MousePosX, MousePosY, MousePosZ;

//For view transforms
GLint ViewTransform;
float ViewPosX = 0.0f, ViewPosY = 0.0f, ViewScale = 1.0f;

//For interpolation
int Interpolate = 0; //Between 0 and 2
char InterMode = ' ';
MatrixXf InterArray(5, 15);
MatrixXf InterData(2, 5);
int InterTriangle, FrameNum, InterComplete = 0;
float TotalRotation;


//Determines first triangle cursor is over
int MousePressing()
{
	for(int i = FullTriangles - 1; i >= 0; i--)
	{
		Vector3f e1, e2, e3;
		Vector3f c1, c2, c3;
		Vector3f a1, a2, a3, normal;
		
		//Edges
		e1 << TriangleArray(0, 3 * i) - TriangleArray(0, 3 * i + 1), TriangleArray(1, 3 * i) - TriangleArray(1, 3 * i + 1), 0.0;
		e2 << TriangleArray(0, 3 * i + 1) - TriangleArray(0, 3 * i + 2), TriangleArray(1, 3 * i + 1) - TriangleArray(1, 3 * i + 2), 0.0;
		e3 << TriangleArray(0, 3 * i + 2) - TriangleArray(0, 3 * i), TriangleArray(1, 3 * i + 2) - TriangleArray(1, 3 * i), 0.0;
		
		//To mouse
		c1 << TriangleArray(0, 3 * i) - MousePosX, TriangleArray(1, 3 * i) - MousePosY, 0.0;
		c2 << TriangleArray(0, 3 * i + 1) - MousePosX, TriangleArray(1, 3 * i + 1) - MousePosY, 0.0;
		c3 << TriangleArray(0, 3 * i + 2) - MousePosX, TriangleArray(1, 3 * i + 2) - MousePosY, 0.0;
		
		//Normal and crosses
		normal = e1.cross(e2);
		a1 = e1.cross(c1);
		a2 = e2.cross(c2);
		a3 = e3.cross(c3);

		if(normal.dot(a1) < 0 && normal.dot(a2) < 0 && normal.dot(a3) < 0)
			return i;
			
		if(normal.dot(a1) > 0 && normal.dot(a2) > 0 && normal.dot(a3) > 0)
			return i;
	}
	
	return -1;
}


//Push back remainder of arrays
void DeleteShift(int val)
{
	if(val != -1)
	{
		for(int i = val; i < FullTriangles - 1; i++)
		{
			TriangleArray.col(3 * i) << TriangleArray.col(3 * i + 3);
			TriangleArray.col(3 * i + 1) << TriangleArray.col(3 * i + 4);
			TriangleArray.col(3 * i + 2) << TriangleArray.col(3 * i + 5);
			
			TriangleData.col(i) = TriangleData.col(i + 1);
		}
		
		FullTriangles--;
	}
	
	return;
}

//------------------------------------

//3d stuff----------------------------

GLFWwindow* window3d;
GLuint ViewTrans3dx, ViewTrans3dy, ViewTrans3dz, Scale3d, Rotation3dx, Rotation3dy, Rotation3dz;
GLuint Transl3d, SelectColor, Barycenter3d, VertexColor, LightSource, PhongShade, WindowSize;

int NumCubes = 0, NumBunnies = 0, NumBumpyCubes = 0, NumObjects = 0;
int BunnyVertices, BumpyCubeVertices, BunnyFaces, BumpyCubeFaces;
int CurrentObject = -1;

MatrixXf CubeArray(7, 108);
Matrix<float, Dynamic, Dynamic> BunnyArray;
Matrix<float, Dynamic, Dynamic> BumpyCubeArray;

MatrixXf CubeData(11, 3); //Centroid(3), Shading type(1), Scaling(1), Rotation(3), Translate(3)
MatrixXf BunnyData(11, 3);
MatrixXf BumpyCubeData(11, 3);
Vector3f ScreenRotation;

void Program3dRun();


//Determines first triangle cursor is over
int MousePressing3d()
{
	int closestindex = -1;
	float dist, closestdist = numeric_limits<float>::max(), x, y, z;
	Vector3f v1, v2, v3;
	Vector3f e1, e2, e3;
	Vector3f c1, c2, c3;
	Vector3f a1, a2, a3, normal, intlocation, barycenter;
	MatrixXf rotate(3, 3);
	
	//Checks each existing object for collisions
	for(int j = 0; j < 9; j++)
	{
		//Checks with unit cubes
		if((j / 3) == 0 && (j % 3) < NumCubes)
		{
			for(int i = 0; i < 12; i++)
			{
				//Vertices
				v1 << CubeArray(0, 3 * i + (j % 3) * 36), CubeArray(1, 3 * i + (j % 3) * 36),
					CubeArray(2, 3 * i + (j % 3) * 36);
				v2 << CubeArray(0, 3 * i + (j % 3) * 36 + 1), CubeArray(1, 3 * i + (j % 3) * 36 + 1),
					CubeArray(2, 3 * i + (j % 3) * 36 + 1);
				v3 << CubeArray(0, 3 * i + (j % 3) * 36 + 2), CubeArray(1, 3 * i + (j % 3) * 36 + 2),
					CubeArray(2, 3 * i + (j % 3) * 36 + 2);
				
				//Transform vertices
				x = CubeData(5, j % 3);
				y = CubeData(6, j % 3);
				z = CubeData(7, j % 3);
				
				rotate.col(0) << cos(z) * cos(y), sin(z) * cos(y), -sin(y);
				rotate.col(1) << cos(z) * sin(y) * sin(x) - sin(z) * cos(x), sin(z) * sin(y) * sin(x) + cos(z) * cos(x), cos(y) * sin(x);
				rotate.col(2) << cos(z) * sin(y) * cos(x) + sin(z) * sin(x), sin(z) * sin(y) * cos(x) - cos(z) * sin(x), cos(y) * cos(x);
				
				v1 = rotate * (v1 * CubeData(4, j % 3)) + Vector3f(CubeData(8, j % 3), CubeData(9, j % 3), CubeData(10, j % 3));
				v2 = rotate * (v2 * CubeData(4, j % 3)) + Vector3f(CubeData(8, j % 3), CubeData(9, j % 3), CubeData(10, j % 3));
				v3 = rotate * (v3 * CubeData(4, j % 3)) + Vector3f(CubeData(8, j % 3), CubeData(9, j % 3), CubeData(10, j % 3));
				
				
				x = ScreenRotation[0];
				y = ScreenRotation[1];
				z = ScreenRotation[2];
				
				rotate.col(0) << cos(z) * cos(y), sin(z) * cos(y), -sin(y);
				rotate.col(1) << cos(z) * sin(y) * sin(x) - sin(z) * cos(x), sin(z) * sin(y) * sin(x) + cos(z) * cos(x), cos(y) * sin(x);
				rotate.col(2) << cos(z) * sin(y) * cos(x) + sin(z) * sin(x), sin(z) * sin(y) * cos(x) - cos(z) * sin(x), cos(y) * cos(x);
				
				v1 = rotate * v1;
				v2 = rotate * v2;
				v3 = rotate * v3;
				
				RowVector3f Mouse = RowVector3f(MousePosX, MousePosY, MousePosZ);
				RowVector3f ScreenPointing = RowVector3f(0, 0, 1);
				
				//Edges
				e1 << v2 - v1;
				e2 << v3 - v2;
				e3 << v1 - v3;
				
				//Normal
				normal = e1.cross(e2).normalized();
				
				dist = (normal.dot(v1) - normal.dot(Mouse)) / normal.dot(ScreenPointing);
				intlocation = Mouse + ScreenPointing * dist;
				
				if(normal.dot(ScreenPointing) == 0)
					continue;
				
				//To mouse
				c1 << intlocation - v1;
				c2 << intlocation - v2;
				c3 << intlocation - v3;
				
				//Crosses
				a1 = e1.cross(c1);
				a2 = e2.cross(c2);
				a3 = e3.cross(c3);
				
				
				if(normal.dot(a1) > 0 && normal.dot(a2) > 0 && normal.dot(a3) > 0)
				{
					if(dist < closestdist)
					{
						closestdist = dist;
						closestindex = j;
					}
				}
			}
		}
		
		//Checks with bunnies
		if((j / 3) == 1 && (j % 3) < NumBunnies)
		{
			for(int i = 0; i < BunnyFaces; i++)
			{
				//Vertices
				v1 << BunnyArray(0, 3 * i + (j % 3) * 3 * BunnyFaces), BunnyArray(1, 3 * i + (j % 3) * 3 * BunnyFaces),
					BunnyArray(2, 3 * i + (j % 3) * 3 * BunnyFaces);
				v2 << BunnyArray(0, 3 * i + (j % 3) * 3 * BunnyFaces + 1), BunnyArray(1, 3 * i + (j % 3) * 3 * BunnyFaces + 1),
					BunnyArray(2, 3 * i + (j % 3) * 3 * BunnyFaces + 1);
				v3 << BunnyArray(0, 3 * i + (j % 3) * 3 * BunnyFaces + 2), BunnyArray(1, 3 * i + (j % 3) * 3 * BunnyFaces + 2),
					BunnyArray(2, 3 * i + (j % 3) * 3 * BunnyFaces + 2);
				
				//Transform vertices
				x = BunnyData(5, j % 3);
				y = BunnyData(6, j % 3);
				z = BunnyData(7, j % 3);
				barycenter = Vector3f(BunnyData(0, j % 3), BunnyData(1, j % 3), BunnyData(2, j % 3));
				
				rotate.col(0) << cos(z) * cos(y), sin(z) * cos(y), -sin(y);
				rotate.col(1) << cos(z) * sin(y) * sin(x) - sin(z) * cos(x), sin(z) * sin(y) * sin(x) + cos(z) * cos(x), cos(y) * sin(x);
				rotate.col(2) << cos(z) * sin(y) * cos(x) + sin(z) * sin(x), sin(z) * sin(y) * cos(x) - cos(z) * sin(x), cos(y) * cos(x);
				
				v1 = (rotate * ((v1 - barycenter) * BunnyData(4, j % 3)) + barycenter) +
						Vector3f(BunnyData(8, j % 3), BunnyData(9, j % 3), BunnyData(10, j % 3));
				v2 = (rotate * ((v2 - barycenter) * BunnyData(4, j % 3)) + barycenter) +
						Vector3f(BunnyData(8, j % 3), BunnyData(9, j % 3), BunnyData(10, j % 3));
				v3 = (rotate * ((v3 - barycenter) * BunnyData(4, j % 3)) + barycenter) +
						Vector3f(BunnyData(8, j % 3), BunnyData(9, j % 3), BunnyData(10, j % 3));
				
				x = ScreenRotation[0];
				y = ScreenRotation[1];
				z = ScreenRotation[2];
				
				rotate.col(0) << cos(z) * cos(y), sin(z) * cos(y), -sin(y);
				rotate.col(1) << cos(z) * sin(y) * sin(x) - sin(z) * cos(x), sin(z) * sin(y) * sin(x) + cos(z) * cos(x), cos(y) * sin(x);
				rotate.col(2) << cos(z) * sin(y) * cos(x) + sin(z) * sin(x), sin(z) * sin(y) * cos(x) - cos(z) * sin(x), cos(y) * cos(x);
				
				v1 = rotate * v1;
				v2 = rotate * v2;
				v3 = rotate * v3;
				
				RowVector3f Mouse = RowVector3f(MousePosX, MousePosY, MousePosZ);
				RowVector3f ScreenPointing = RowVector3f(0, 0, 1);
				
				//Edges
				e1 << v2 - v1;
				e2 << v3 - v2;
				e3 << v1 - v3;
				
				//Normal
				normal = e1.cross(e2).normalized();
				
				dist = (normal.dot(v1) - normal.dot(Mouse)) / normal.dot(ScreenPointing);
				intlocation = Mouse + ScreenPointing * dist;
				
				if(normal.dot(ScreenPointing) == 0)
					continue;
				
				//To mouse
				c1 << intlocation - v1;
				c2 << intlocation - v2;
				c3 << intlocation - v3;
				
				//Crosses
				a1 = e1.cross(c1);
				a2 = e2.cross(c2);
				a3 = e3.cross(c3);
				
				
				if(normal.dot(a1) > 0 && normal.dot(a2) > 0 && normal.dot(a3) > 0)
				{
					if(dist < closestdist)
					{
						closestdist = dist;
						closestindex = j;
					}
				}
			}
		}
		
		//Checks with bumpy cubes
		if((j / 3) == 2 && (j % 3) < NumBumpyCubes)
		{
			for(int i = 0; i < BumpyCubeFaces; i++)
			{
				//Vertices
				v1 << BumpyCubeArray(0, 3 * i + (j % 3) * 3 * BumpyCubeFaces), BumpyCubeArray(1, 3 * i + (j % 3) * 3 * BumpyCubeFaces),
					BumpyCubeArray(2, 3 * i + (j % 3) * 3 * BumpyCubeFaces);
				v2 << BumpyCubeArray(0, 3 * i + (j % 3) * 3 * BumpyCubeFaces + 1), BumpyCubeArray(1, 3 * i + (j % 3) * 3 * BumpyCubeFaces + 1),
					BumpyCubeArray(2, 3 * i + (j % 3) * 3 * BumpyCubeFaces + 1);
				v3 << BumpyCubeArray(0, 3 * i + (j % 3) * 3 * BumpyCubeFaces + 2), BumpyCubeArray(1, 3 * i + (j % 3) * 3 * BumpyCubeFaces + 2),
					BumpyCubeArray(2, 3 * i + (j % 3) * 3 * BumpyCubeFaces + 2);
				
				//Transform vertices
				x = BumpyCubeData(5, j % 3);
				y = BumpyCubeData(6, j % 3);
				z = BumpyCubeData(7, j % 3);
				
				rotate.col(0) << cos(z) * cos(y), sin(z) * cos(y), -sin(y);
				rotate.col(1) << cos(z) * sin(y) * sin(x) - sin(z) * cos(x), sin(z) * sin(y) * sin(x) + cos(z) * cos(x), cos(y) * sin(x);
				rotate.col(2) << cos(z) * sin(y) * cos(x) + sin(z) * sin(x), sin(z) * sin(y) * cos(x) - cos(z) * sin(x), cos(y) * cos(x);
				
				v1 = rotate * (v1 * BumpyCubeData(4, j % 3)) + Vector3f(BumpyCubeData(8, j % 3), BumpyCubeData(9, j % 3), BumpyCubeData(10, j % 3));
				v2 = rotate * (v2 * BumpyCubeData(4, j % 3)) + Vector3f(BumpyCubeData(8, j % 3), BumpyCubeData(9, j % 3), BumpyCubeData(10, j % 3));
				v3 = rotate * (v3 * BumpyCubeData(4, j % 3)) + Vector3f(BumpyCubeData(8, j % 3), BumpyCubeData(9, j % 3), BumpyCubeData(10, j % 3));
				
				x = ScreenRotation[0];
				y = ScreenRotation[1];
				z = ScreenRotation[2];
				
				rotate.col(0) << cos(z) * cos(y), sin(z) * cos(y), -sin(y);
				rotate.col(1) << cos(z) * sin(y) * sin(x) - sin(z) * cos(x), sin(z) * sin(y) * sin(x) + cos(z) * cos(x), cos(y) * sin(x);
				rotate.col(2) << cos(z) * sin(y) * cos(x) + sin(z) * sin(x), sin(z) * sin(y) * cos(x) - cos(z) * sin(x), cos(y) * cos(x);
				
				v1 = rotate * v1;
				v2 = rotate * v2;
				v3 = rotate * v3;
				
				RowVector3f Mouse = RowVector3f(MousePosX, MousePosY, MousePosZ);
				RowVector3f ScreenPointing = RowVector3f(0, 0, 1);
				
				//Edges
				e1 << v2 - v1;
				e2 << v3 - v2;
				e3 << v1 - v3;
				
				//Normal
				normal = e1.cross(e2).normalized();
				
				dist = (normal.dot(v1) - normal.dot(Mouse)) / normal.dot(ScreenPointing);
				intlocation = Mouse + ScreenPointing * dist;
				
				if(normal.dot(ScreenPointing) == 0)
					continue;
				
				//To mouse
				c1 << intlocation - v1;
				c2 << intlocation - v2;
				c3 << intlocation - v3;
				
				//Crosses
				a1 = e1.cross(c1);
				a2 = e2.cross(c2);
				a3 = e3.cross(c3);
				
				
				if(normal.dot(a1) > 0 && normal.dot(a2) > 0 && normal.dot(a3) > 0)
				{
					if(dist < closestdist)
					{
						closestdist = dist;
						closestindex = j;
					}
				}
			}
		}
	}
	
	return closestindex;
}

void FlatShading(Vector3f v1, Vector3f v2, Vector3f v3, string type, int snum)
{
	Vector3f lightsource = RowVector3f(-2, 2, -2), barycenter;
	float x, y, z;
	MatrixXf rotate(3, 3);
	
	//Transforms
	//Which shape?
	if(type == "cube")
	{
		x = CubeData(5, snum);
		y = CubeData(6, snum);
		z = CubeData(7, snum);
		
		rotate.col(0) << cos(z) * cos(y), sin(z) * cos(y), -sin(y);
		rotate.col(1) << cos(z) * sin(y) * sin(x) - sin(z) * cos(x), sin(z) * sin(y) * sin(x) + cos(z) * cos(x), cos(y) * sin(x);
		rotate.col(2) << cos(z) * sin(y) * cos(x) + sin(z) * sin(x), sin(z) * sin(y) * cos(x) - cos(z) * sin(x), cos(y) * cos(x);
		
		v1 = rotate * (v1 * CubeData(4, snum)) + Vector3f(CubeData(8, snum), CubeData(9, snum), CubeData(10, snum));
		v2 = rotate * (v2 * CubeData(4, snum)) + Vector3f(CubeData(8, snum), CubeData(9, snum), CubeData(10, snum));
		v3 = rotate * (v3 * CubeData(4, snum)) + Vector3f(CubeData(8, snum), CubeData(9, snum), CubeData(10, snum));
	}
	else if(type == "bunny")
	{
		x = BunnyData(5, snum);
		y = BunnyData(6, snum);
		z = BunnyData(7, snum);
		
		barycenter = Vector3f(BunnyData(0, snum), BunnyData(1, snum), BunnyData(2, snum));
		
		rotate.col(0) << cos(z) * cos(y), sin(z) * cos(y), -sin(y);
		rotate.col(1) << cos(z) * sin(y) * sin(x) - sin(z) * cos(x), sin(z) * sin(y) * sin(x) + cos(z) * cos(x), cos(y) * sin(x);
		rotate.col(2) << cos(z) * sin(y) * cos(x) + sin(z) * sin(x), sin(z) * sin(y) * cos(x) - cos(z) * sin(x), cos(y) * cos(x);
		
		v1 = (rotate * ((v1 - barycenter) * BunnyData(4, snum)) + barycenter) +
				Vector3f(BunnyData(8, snum), BunnyData(9, snum), BunnyData(10, snum));
		v2 = (rotate * ((v2 - barycenter) * BunnyData(4, snum)) + barycenter) +
				Vector3f(BunnyData(8, snum), BunnyData(9, snum), BunnyData(10, snum));
		v3 = (rotate * ((v3 - barycenter) * BunnyData(4, snum)) + barycenter) +
				Vector3f(BunnyData(8, snum), BunnyData(9, snum), BunnyData(10, snum));
	}
	if(type == "bumpycube")
	{
		x = BumpyCubeData(5, snum);
		y = BumpyCubeData(6, snum);
		z = BumpyCubeData(7, snum);
		
		rotate.col(0) << cos(z) * cos(y), sin(z) * cos(y), -sin(y);
		rotate.col(1) << cos(z) * sin(y) * sin(x) - sin(z) * cos(x), sin(z) * sin(y) * sin(x) + cos(z) * cos(x), cos(y) * sin(x);
		rotate.col(2) << cos(z) * sin(y) * cos(x) + sin(z) * sin(x), sin(z) * sin(y) * cos(x) - cos(z) * sin(x), cos(y) * cos(x);
		
		v1 = rotate * (v1 * BumpyCubeData(4, snum)) + Vector3f(BumpyCubeData(8, snum), BumpyCubeData(9, snum), BumpyCubeData(10, snum));
		v2 = rotate * (v2 * BumpyCubeData(4, snum)) + Vector3f(BumpyCubeData(8, snum), BumpyCubeData(9, snum), BumpyCubeData(10, snum));
		v3 = rotate * (v3 * BumpyCubeData(4, snum)) + Vector3f(BumpyCubeData(8, snum), BumpyCubeData(9, snum), BumpyCubeData(10, snum));
	}
	
	//Edges
	Vector3f e1, e2, normal, centroid, lightray;
	e1 << v2 - v1;
	e2 << v2 - v3;
	
	//Normal
	normal = e1.cross(e2).normalized();
	
	//Centroid of particular triangle
	centroid << (v1[0] + v2[0] + v3[0]) / 3, (v1[1] + v2[1] + v3[1]) / 3, (v1[2] + v2[2] + v3[2]) / 3;
	lightray = centroid - lightsource;
	float color = max(0.0f, lightray.dot(normal)) * 0.6f;
	
	glUniform3f(VertexColor, color, color, color);
	return;
}

Vector3f CalcNormal(Vector3f v1, Vector3f v2, Vector3f v3)
{
	Vector3f e1, e2, normal;
	e1 << v2 - v1;
	e2 << v2 - v3;
	
	normal = e1.cross(e2).normalized();
	
	return normal;
}

//------------------------------------


//For when the window is resized
void resize_callback(GLFWwindow* window, int width, int height)
{
	if(width >= height)
		glViewport((width - height) / 2, 0, height, height);
	else
		glViewport(0, (height - width) / 2, width, width);
	
	return;
}

//For when the cursor moves
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	//Get the size of the window
	int width, height;
	glfwGetWindowSize(window, &width, &height);
	
	
	//3d actions
	if(window == window3d)
	{
		return;
	}
	
	//Convert screen position to world coordinates
	float xworld = ((float(xpos)/float(width)) * 2) - 1;
	float yworld = (((height - 1 - float(ypos))/float(height)) * 2) - 1; //Y axis is flipped
	xworld = (xworld - ViewPosX)/ViewScale;
	yworld = (yworld - ViewPosY)/ViewScale;
	
	
	//Begin commands based on selected mode
	switch (mode)
	{
		case 'i':
			//Draw based on previous points and current cursor position
			if(DrawPoints == 1)
				TempLine << TempPoint(0, 0), xworld, TempPoint(1, 0), yworld, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0;
			
			else if(DrawPoints == 2)
				TempTriangle << TempLine(0, 0), TempLine(0, 1), xworld, TempLine(1, 0), TempLine(1, 1), yworld,
									0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0;
			break;
		
		case 'o':
			if (MouseState == 1)
			{
				//Change vertex data
				if(TriangleSelected != -1)
				{
					TriangleArray(0, 3 * TriangleSelected) += xworld - MousePosX;
					TriangleArray(0, 3 * TriangleSelected + 1) += xworld - MousePosX;
					TriangleArray(0, 3 * TriangleSelected + 2) += xworld - MousePosX;
					
					TriangleArray(1, 3 * TriangleSelected) += yworld - MousePosY;
					TriangleArray(1, 3 * TriangleSelected + 1) += yworld - MousePosY;
					TriangleArray(1, 3 * TriangleSelected + 2) += yworld - MousePosY;
					
					TriangleData(0, TriangleSelected) += xworld - MousePosX;
					TriangleData(1, TriangleSelected) += yworld - MousePosY;
				}
				
				MousePosX = xworld;
				MousePosY = yworld;
			}
			break;
		
		default:
			break;
	}
	
	return;
}


//For when the mouse is clicked
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	//Get the position of the mouse in the window
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	//Get the size of the window
	int width, height;
	glfwGetWindowSize(window, &width, &height);
	
	if(width >= height)
	{
		xpos -= (width - height) / 2;
		width = height;
	}
	else
	{
		ypos -= (height - width) / 2;
		height = width;
	}
	
	float xworld, yworld;
	
	//3d actions
	if(window == window3d)
	{
		if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
		{
			xworld = ((float(xpos)/float(width)) * 2) - 1;
			yworld = (((height - 1 - float(ypos))/float(height)) * 2) - 1;
			
			MousePosX = xworld;
			MousePosY = yworld;
			MousePosZ = -1;
			
			CurrentObject = MousePressing3d();
			return;
		}
	}
	
	
	//Convert screen position to world coordinates
	xworld = ((float(xpos)/float(width)) * 2) - 1;
	yworld = (((height - 1 - float(ypos))/float(height)) * 2) - 1; //Y axis is flipped
	xworld = (xworld - ViewPosX)/ViewScale;
	yworld = (yworld - ViewPosY)/ViewScale;
	
	float closest = numeric_limits<float>::max(), distance, baryx, baryy;
	
	//Begin commands based on selected mode
	switch (mode)
	{
		case 'i':
			//Update the position of the first vertex if the left button is pressed
			if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
			{
				if(DrawPoints == 0)
				{
					TempPoint << xworld, yworld, 0.0, 0.0, 0.0;
					TempLine << xworld, xworld, yworld, yworld, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0;
					DrawPoints++;
				}
				else if(DrawPoints == 1)
				{
					TempLine << TempPoint(0, 0), xworld, TempPoint(1, 0), yworld, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0;
					TempTriangle << TempPoint(0, 0), xworld, xworld, TempPoint(1, 0), yworld, yworld,
										0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0;
					DrawPoints++;
				}
				else
				{
					FinalTriangle << TempLine(0, 0), TempLine(0, 1), xworld, TempLine(1, 0), TempLine(1, 1), yworld,
									0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0;
					DrawPoints = 0;
					
					//Drawn coordinates
					TriangleArray.col(FullTriangles * 3) << FinalTriangle.col(0);
					TriangleArray.col(FullTriangles * 3 + 1) << FinalTriangle.col(1);
					TriangleArray.col(FullTriangles * 3 + 2) << FinalTriangle.col(2);
					
					//Barycenter and rotation (rotation initialized to 0)
					baryx = (FinalTriangle(0, 0) + FinalTriangle(0, 1) + FinalTriangle(0, 2)) / 3.0f;
					baryy = (FinalTriangle(1, 0) + FinalTriangle(1, 1) + FinalTriangle(1, 2)) / 3.0f;
					TriangleData.col(FullTriangles) << baryx, baryy;
					
					FullTriangles++;
				}
			}
			break;
		
		case 'o':
			//First frame of click
			if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && MouseState == 0)
			{
				if(Interpolate == 2 && InterMode != 'o')
					break;
				
				MouseState = 1;
				
				if(Interpolate == 1)
				{
					Interpolate = 2;
					InterMode = 'o';
				}
				
				MousePosX = xworld;
				MousePosY = yworld;
				
				//Find triangle it is over
				TriangleSelected = MousePressing();
				
				if(TriangleSelected != PreviousTriangle && PreviousTriangle != -1)
				{
					//Reset color to old values
					for(int i = 2; i < 5; i++)
					{
						TriangleArray(i, 3 * PreviousTriangle) = TempTriangle(i, 0);
						TriangleArray(i, 3 * PreviousTriangle + 1) = TempTriangle(i, 1);
						TriangleArray(i, 3 * PreviousTriangle + 2) = TempTriangle(i, 2);
					}
				}
				
				if(TriangleSelected != PreviousTriangle && TriangleSelected != -1)
				{
					//Save triangle data
					TempTriangle.col(0) << TriangleArray.col(3 * TriangleSelected);
					TempTriangle.col(1) << TriangleArray.col(3 * TriangleSelected + 1);
					TempTriangle.col(2) << TriangleArray.col(3 * TriangleSelected + 2);
					
					//Change triangle color
					for(int i = 2; i < 5; i++)
					{
						TriangleArray(i, 3 * TriangleSelected) = 0.7f;
						TriangleArray(i, 3 * TriangleSelected + 1) = 0.7f;
						TriangleArray(i, 3 * TriangleSelected + 2) = 0.7f;
					}
				}
				
				PreviousTriangle = -1;
			}
			
			//Once released
			if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE && MouseState == 1)
			{
				MouseState = 0;
				PreviousTriangle = TriangleSelected;
				TriangleSelected = -1;
			}
			break;
		
		case 'p':
			if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
			{
				MousePosX = xworld;
				MousePosY = yworld;
				DeleteShift(MousePressing());
			}
			break;
		
		case 'c':
			for(int i = 0; i < FullTriangles; i++)
			{
				for(int j = 0; j < 3; j++)
				{
					distance = sqrt(pow(TriangleArray(0, i * 3 + j) - xworld, 2) + pow(TriangleArray(1, i * 3 + j) - yworld, 2));
					if(distance < closest)
					{
						closest = distance;
						VertexColorChoice = i * 3 + j;
					}
				}
			}
			break;
		
		default:
			break;
	}
	
	return;
}


//For when a key is pressed
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if(action == GLFW_RELEASE)
		return;
	
	
	//3d actions
	if(window == window3d)
	{
		//Add and delete objects
		switch (key)
		{
			case GLFW_KEY_1:
				if(NumCubes < 3)
					NumCubes++;
				break;
				
			case GLFW_KEY_2:
				if(NumBunnies < 3)
					NumBunnies++;
				break;
				
			case GLFW_KEY_3:
				if(NumBumpyCubes < 3)
					NumBumpyCubes++;
				break;
			
			
			case GLFW_KEY_7:
				if(NumCubes > 0)
				{
					NumCubes--;
					CubeData.col(NumCubes) << 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f;
					if(CurrentObject / 3 == 0 && CurrentObject % 3 == NumCubes)
						CurrentObject = -1;
				}
				break;
				
			case GLFW_KEY_8:
				if(NumBunnies > 0)
				{
					NumBunnies--;
					BunnyData.col(NumBunnies) << BunnyData(0, NumBunnies), BunnyData(1, NumBunnies), BunnyData(2, NumBunnies),
												0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f;
					if(CurrentObject / 3 == 1 && CurrentObject % 3 == NumBunnies)
						CurrentObject = -1;
				}
				break;
				
			case GLFW_KEY_9:
				if(NumBumpyCubes > 0)
				{
					NumBumpyCubes--;
					BumpyCubeData.col(NumBumpyCubes) << 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f;
					if(CurrentObject / 3 == 2 && CurrentObject % 3 == NumBumpyCubes)
						CurrentObject = -1;
				}
				break;
				
			default:
				break;
		}
		
		//Translate using w, a, s, d, q, and e
		if(CurrentObject != -1)
		{
			switch (key)
			{
				case GLFW_KEY_W:
					if(CurrentObject / 3 == 0)
						CubeData(9, CurrentObject % 3) += 0.1f;
					
					if(CurrentObject / 3 == 1)
						BunnyData(9, CurrentObject % 3) += 0.1f;
					
					if(CurrentObject / 3 == 2)
						BumpyCubeData(9, CurrentObject % 3) += 0.1f;
					
					break;
					
				case GLFW_KEY_S:
					if(CurrentObject / 3 == 0)
						CubeData(9, CurrentObject % 3) -= 0.1f;
					
					if(CurrentObject / 3 == 1)
						BunnyData(9, CurrentObject % 3) -= 0.1f;
					
					if(CurrentObject / 3 == 2)
						BumpyCubeData(9, CurrentObject % 3) -= 0.1f;
					
					break;
				
				case GLFW_KEY_A:
					if(CurrentObject / 3 == 0)
						CubeData(8, CurrentObject % 3) -= 0.1f;
					
					if(CurrentObject / 3 == 1)
						BunnyData(8, CurrentObject % 3) -= 0.1f;
					
					if(CurrentObject / 3 == 2)
						BumpyCubeData(8, CurrentObject % 3) -= 0.1f;
					
					break;
					
				case GLFW_KEY_D:
					if(CurrentObject / 3 == 0)
						CubeData(8, CurrentObject % 3) += 0.1f;
					
					if(CurrentObject / 3 == 1)
						BunnyData(8, CurrentObject % 3) += 0.1f;
					
					if(CurrentObject / 3 == 2)
						BumpyCubeData(8, CurrentObject % 3) += 0.1f;
					
					break;
					
				case GLFW_KEY_Q:
					if(CurrentObject / 3 == 0)
						CubeData(10, CurrentObject % 3) -= 0.1f;
					
					if(CurrentObject / 3 == 1)
						BunnyData(10, CurrentObject % 3) -= 0.1f;
					
					if(CurrentObject / 3 == 2)
						BumpyCubeData(10, CurrentObject % 3) -= 0.1f;
					
					break;
					
				case GLFW_KEY_E:
					if(CurrentObject / 3 == 0)
						CubeData(10, CurrentObject % 3) += 0.1f;
					
					if(CurrentObject / 3 == 1)
						BunnyData(10, CurrentObject % 3) += 0.1f;
					
					if(CurrentObject / 3 == 2)
						BumpyCubeData(10, CurrentObject % 3) += 0.1f;
					
					break;
				
				default:
					break;
			}
		}
		
		//Scaling using n and m
		if(CurrentObject != -1)
		{
			switch (key)
			{
				case GLFW_KEY_N:
					if(CurrentObject / 3 == 0)
						CubeData(4, CurrentObject % 3) = CubeData(4, CurrentObject % 3) * 0.8f;
					
					if(CurrentObject / 3 == 1)
						BunnyData(4, CurrentObject % 3) = BunnyData(4, CurrentObject % 3) * 0.8f;
					
					if(CurrentObject / 3 == 2)
						BumpyCubeData(4, CurrentObject % 3) = BumpyCubeData(4, CurrentObject % 3) * 0.8f;
					break;
					
				case GLFW_KEY_M:
					if(CurrentObject / 3 == 0)
						CubeData(4, CurrentObject % 3) = CubeData(4, CurrentObject % 3) * 1.2f;
					
					if(CurrentObject / 3 == 1)
						BunnyData(4, CurrentObject % 3) = BunnyData(4, CurrentObject % 3) * 1.2f;
					
					if(CurrentObject / 3 == 2)
						BumpyCubeData(4, CurrentObject % 3) = BumpyCubeData(4, CurrentObject % 3) * 1.2f;
					break;
				
				default:
					break;
			}
		}
		
		//Rotate using i, j, k, l, u, and object
		if(CurrentObject != -1)
		{
			switch (key)
			{
				case GLFW_KEY_I:
					if(CurrentObject / 3 == 0)
						CubeData(5, CurrentObject % 3) += 31.415926f / 180.0f;
					
					if(CurrentObject / 3 == 1)
						BunnyData(5, CurrentObject % 3) += 31.415926f / 180.0f;
					
					if(CurrentObject / 3 == 2)
						BumpyCubeData(5, CurrentObject % 3) += 31.415926f / 180.0f;
					
					break;
					
				case GLFW_KEY_K:
					if(CurrentObject / 3 == 0)
						CubeData(5, CurrentObject % 3) -= 31.415926f / 180.0f;
					
					if(CurrentObject / 3 == 1)
						BunnyData(5, CurrentObject % 3) -= 31.415926f / 180.0f;
					
					if(CurrentObject / 3 == 2)
						BumpyCubeData(5, CurrentObject % 3) -= 31.415926f / 180.0f;
					
					break;
				
				case GLFW_KEY_J:
					if(CurrentObject / 3 == 0)
						CubeData(6, CurrentObject % 3) -= 31.415926f / 180.0f;
					
					if(CurrentObject / 3 == 1)
						BunnyData(6, CurrentObject % 3) -= 31.415926f / 180.0f;
					
					if(CurrentObject / 3 == 2)
						BumpyCubeData(6, CurrentObject % 3) -= 31.415926f / 180.0f;
					
					break;
					
				case GLFW_KEY_L:
					if(CurrentObject / 3 == 0)
						CubeData(6, CurrentObject % 3) += 31.415926f / 180.0f;
					
					if(CurrentObject / 3 == 1)
						BunnyData(6, CurrentObject % 3) += 31.415926f / 180.0f;
					
					if(CurrentObject / 3 == 2)
						BumpyCubeData(6, CurrentObject % 3) += 31.415926f / 180.0f;
					
					break;
					
				case GLFW_KEY_U:
					if(CurrentObject / 3 == 0)
						CubeData(7, CurrentObject % 3) -= 31.415926f / 180.0f;
					
					if(CurrentObject / 3 == 1)
						BunnyData(7, CurrentObject % 3) -= 31.415926f / 180.0f;
					
					if(CurrentObject / 3 == 2)
						BumpyCubeData(7, CurrentObject % 3) -= 31.415926f / 180.0f;
					
					break;
					
				case GLFW_KEY_O:
					if(CurrentObject / 3 == 0)
						CubeData(7, CurrentObject % 3) += 31.415926f / 180.0f;
					
					if(CurrentObject / 3 == 1)
						BunnyData(7, CurrentObject % 3) += 31.415926f / 180.0f;
					
					if(CurrentObject / 3 == 2)
						BumpyCubeData(7, CurrentObject % 3) += 31.415926f / 180.0f;
					
					break;
				
				default:
					break;
			}
		}
		
		//Rotate screen around y-axis with up, down, left, and right keys
		switch (key)
		{
			case GLFW_KEY_UP:
				ScreenRotation[0] -= 31.415926f / 180.0f;
				glUniform2f(ViewTrans3dx, cos(ScreenRotation[0]), sin(ScreenRotation[0]));
				break;
				
			case GLFW_KEY_DOWN:
				ScreenRotation[0] += 31.415926f / 180.0f;
				glUniform2f(ViewTrans3dx, cos(ScreenRotation[0]), sin(ScreenRotation[0]));
				break;
				
			case GLFW_KEY_LEFT:
				ScreenRotation[1] -= 31.415926f / 180.0f;
				glUniform2f(ViewTrans3dy, cos(ScreenRotation[1]), sin(ScreenRotation[1]));
				break;
				
			case GLFW_KEY_RIGHT:
				ScreenRotation[1] += 31.415926f / 180.0f;
				glUniform2f(ViewTrans3dy, cos(ScreenRotation[1]), sin(ScreenRotation[1]));
				break;
			
			default:
				break;
		}
		
		//Set shading mode
		if(CurrentObject != -1)
		{
			switch(key)
			{
				case GLFW_KEY_Z:
					if(CurrentObject / 3 == 0)
						CubeData(3, CurrentObject % 3) = 0.0f;
					
					if(CurrentObject / 3 == 1)
						BunnyData(3, CurrentObject % 3) = 0.0f;
					
					if(CurrentObject / 3 == 2)
						BumpyCubeData(3, CurrentObject % 3) = 0.0f;
					break;
				
				case GLFW_KEY_X:
					if(CurrentObject / 3 == 0)
						CubeData(3, CurrentObject % 3) = 1.0f;
					
					if(CurrentObject / 3 == 1)
						BunnyData(3, CurrentObject % 3) = 1.0f;
					
					if(CurrentObject / 3 == 2)
						BumpyCubeData(3, CurrentObject % 3) = 1.0f;
					break;
					
				case GLFW_KEY_C:
					if(CurrentObject / 3 == 0)
						CubeData(3, CurrentObject % 3) = 2.0f;
					
					if(CurrentObject / 3 == 1)
						BunnyData(3, CurrentObject % 3) = 2.0f;
					
					if(CurrentObject / 3 == 2)
						BumpyCubeData(3, CurrentObject % 3) = 2.0f;
					break;
			}
		}
		
		return;
	}
	
	
	char modeprior = mode;
	
	//Mode selector or color/position change
	switch (key)
	{
		case GLFW_KEY_I:
			mode = 'i';
			break;
		
		case GLFW_KEY_O:
			mode = 'o';
			break;
		
		case GLFW_KEY_P:
			mode = 'p';
			break;
		
		case GLFW_KEY_C:
			mode = 'c';
			break;
		
		default:
			break;
	}
	
	
	//Cleanup if mode is changed
	if(modeprior == 'i' && mode != 'i')
		DrawPoints = 0;
	if(modeprior == 'c' && mode != 'c')
		VertexColorChoice = -1;
	if(modeprior == 'o' && mode != 'o')
	{
		MouseState = 0;
		if(TriangleSelected != -1)
		{
			//Reset color to old values
			for(int i = 2; i < 5; i++)
			{
				TriangleArray(i, 3 * TriangleSelected) = TempTriangle(i, 0);
				TriangleArray(i, 3 * TriangleSelected + 1) = TempTriangle(i, 1);
				TriangleArray(i, 3 * TriangleSelected + 2) = TempTriangle(i, 2);
			}
		}
		
		if(PreviousTriangle != -1)
		{
			//Reset color to old values
			for(int i = 2; i < 5; i++)
			{
				TriangleArray(i, 3 * PreviousTriangle) = TempTriangle(i, 0);
				TriangleArray(i, 3 * PreviousTriangle + 1) = TempTriangle(i, 1);
				TriangleArray(i, 3 * PreviousTriangle + 2) = TempTriangle(i, 2);
			}
		}
		
		Interpolate = 0;
		InterComplete = 0;
		PreviousTriangle = -1;
		TriangleSelected = -1;
	}
	
	
	//For coloring
	MatrixXf colors(1, 3);
	bool invalid = false;
	if(mode == 'c')
	{
		switch (key)
		{
			case GLFW_KEY_1:
				colors << 0.0, 0.0, 0.0;
				break;
			case GLFW_KEY_2:
				colors << 1.0, 1.0, 1.0;
				break;
			case GLFW_KEY_3:
				colors << 1.0, 0.0, 0.0;
				break;
			case GLFW_KEY_4:
				colors << 0.0, 1.0, 0.0;
				break;
			case GLFW_KEY_5:
				colors << 0.0, 0.0, 1.0;
				break;
			case GLFW_KEY_6:
				colors << 1.0, 1.0, 0.0;
				break;
			case GLFW_KEY_7:
				colors << 1.0, 0.0, 1.0;
				break;
			case GLFW_KEY_8:
				colors << 0.0, 1.0, 1.0;
				break;
			case GLFW_KEY_9:
				colors << 0.5, 0.5, 0.5;
				break;
			
			default:
				invalid = true;
				break;
		}
		
		//Actual coloring
		if(!invalid && VertexColorChoice != -1)
		{
			TriangleArray(2, VertexColorChoice) = colors(0, 0);
			TriangleArray(3, VertexColorChoice) = colors(0, 1);
			TriangleArray(4, VertexColorChoice) = colors(0, 2);
		}
	}
	

	//Rotation and scaling
	int target = -1;
	float distx, disty, rotation = float(31.4159/180.0);
	
	if(PreviousTriangle != -1)
		target = PreviousTriangle;
	if(TriangleSelected != -1)
		target = TriangleSelected;
	
	if(mode == 'o' && target != -1 && (Interpolate != 2 || InterMode == 'h'))
	{
		if(Interpolate == 1 && (key == GLFW_KEY_H || key == GLFW_KEY_J))
		{
			Interpolate = 2;
			InterMode = 'h';
			TotalRotation = 0;
		}
		
		switch(key)
		{
			case GLFW_KEY_H:
				for(int i = 0; i < 3; i++)
				{
					distx = TriangleArray(0, 3 * target + i) - TriangleData(0, target);
					disty = TriangleArray(1, 3 * target + i) - TriangleData(1, target);
					
					TriangleArray(0, 3 * target + i) = TriangleData(0, target) + distx * cos(-rotation) - disty * sin(-rotation);
					TriangleArray(1, 3 * target + i) = TriangleData(1, target) + distx * sin(-rotation) + disty * cos(-rotation);
				}
				TotalRotation -= rotation;
				break;
				
			case GLFW_KEY_J:
				for(int i = 0; i < 3; i++)
				{
					distx = TriangleArray(0, 3 * target + i) - TriangleData(0, target);
					disty = TriangleArray(1, 3 * target + i) - TriangleData(1, target);
					
					TriangleArray(0, 3 * target + i) = TriangleData(0, target) + distx * cos(rotation) - disty * sin(rotation);
					TriangleArray(1, 3 * target + i) = TriangleData(1, target) + distx * sin(rotation) + disty * cos(rotation);
				}
				TotalRotation += rotation;
				break;
				
			default:
				break;
		}
	}
	
	if(mode == 'o' && target != -1 && (Interpolate != 2 || InterMode == 'k'))
	{
		if(Interpolate == 1 && (key == GLFW_KEY_K || key == GLFW_KEY_L))
		{
			Interpolate = 2;
			InterMode = 'k';
		}
		
		switch(key)
		{
			case GLFW_KEY_K:
				for(int i = 0; i < 3; i++)
				{
					TriangleArray(0, 3 * target + i) += (TriangleArray(0, 3 * target + i) - TriangleData(0, target)) * 0.25f;
					TriangleArray(1, 3 * target + i) += (TriangleArray(1, 3 * target + i) - TriangleData(1, target)) * 0.25f;
				}
				break;
				
			case GLFW_KEY_L:
				for(int i = 0; i < 3; i++)
				{
					TriangleArray(0, 3 * target + i) -= (TriangleArray(0, 3 * target + i) - TriangleData(0, target)) * 0.25f;
					TriangleArray(1, 3 * target + i) -= (TriangleArray(1, 3 * target + i) - TriangleData(1, target)) * 0.25f;
				}
				break;
				
			default:
				break;
		}
	}
	
	//View Scaling
	switch(key)
	{
		case GLFW_KEY_W:
			ViewPosY -= 0.2f * ViewScale;
			glUniform3f(ViewTransform, ViewPosX, ViewPosY, ViewScale);
			break;
		
		case GLFW_KEY_S:
			ViewPosY += 0.2f * ViewScale;
			glUniform3f(ViewTransform, ViewPosX, ViewPosY, ViewScale);
			break;
			
		case GLFW_KEY_A:
			ViewPosX += 0.2f * ViewScale;
			glUniform3f(ViewTransform, ViewPosX, ViewPosY, ViewScale);
			break;
			
		case GLFW_KEY_D:
			ViewPosX -= 0.2f * ViewScale;
			glUniform3f(ViewTransform, ViewPosX, ViewPosY, ViewScale);
			break;
			
		case GLFW_KEY_MINUS:
			ViewScale = 0.8f * ViewScale;
			glUniform3f(ViewTransform, ViewPosX, ViewPosY, ViewScale);
			break;
			
		case GLFW_KEY_EQUAL:
			ViewScale = 1.2f * ViewScale;
			glUniform3f(ViewTransform, ViewPosX, ViewPosY, ViewScale);
			break;
			
		default:
			break;
	}
	
	//Interpolation
	float transformx, transformy;
	
	switch(key)
	{
		case GLFW_KEY_Z:
			//Start interpolation
			if(Interpolate == 0 && PreviousTriangle != -1)
			{
				Interpolate = 1;
				InterComplete = 0;
				InterTriangle = PreviousTriangle;
				
				for(int i = 0; i < 3; i++)
					InterArray.col(i) = TriangleArray.col(InterTriangle * 3 + i);
				InterData.col(0) = TriangleData.col(InterTriangle);
			}
			
			//Cancel early
			else if(Interpolate == 1)
				Interpolate = 0;
			
			//View interpolation
			else if(Interpolate == 2)
			{
				Interpolate = 0;
				InterComplete = 1;
				FrameNum = 4;
				
				//Rotation
				if(InterMode == 'h')
				{
					for(int i = 1; i < 5; i++)
					{
						InterData.col(i) = InterData.col(0);
						
						for(int j = 0; j < 3; j++)
						{
							distx = InterArray(0, j) - TriangleData(0, InterTriangle);
							disty = InterArray(1, j) - TriangleData(1, InterTriangle);
							
							transformx = distx * cos(TotalRotation * i / 4.0f) - disty * sin(TotalRotation * i / 4.0f);
							transformy = distx * sin(TotalRotation * i / 4.0f) + disty * cos(TotalRotation * i / 4.0f);
							
							InterArray(0, 3 * i + j) = InterData(0, i) + transformx;
							InterArray(1, 3 * i + j) = InterData(1, i) + transformy;
						}
					}
				}
				
				//Scaling
				else if(InterMode == 'k')
				{	
					for(int i = 1; i < 5; i++)
					{
						InterData.col(i) = InterData.col(0);
						
						for(int j = 0; j < 3; j++)
						{
							distx = (TriangleArray(0, InterTriangle * 3 + j) - InterArray(0, j)) / 4.0f;
							InterArray(0, 3 * i + j) = InterArray(0, j) + distx * i;
							
							disty = (TriangleArray(1, InterTriangle * 3 + j) - InterArray(1, j)) / 4.0f;
							InterArray(1, 3 * i + j) = InterArray(1, j) + disty * i;
						}
					}
				}
				
				//Translation
				else if(InterMode == 'o')
				{
					distx = (TriangleArray(0, InterTriangle * 3) - InterArray(0, 0)) / 4.0f;
					disty = (TriangleArray(1, InterTriangle * 3) - InterArray(1, 0)) / 4.0f;
					
					for(int i = 1; i < 5; i++)
					{
						for(int j = 0; j < 3; j++)
						{
							InterArray(0, 3 * i + j) = InterArray(0, j) + distx * i;
							InterArray(1, 3 * i + j) = InterArray(1, j) + disty * i;
						}
						
						InterData(0, i) = InterData(0, 0) + distx * i;
						InterData(1, i) = InterData(1, 0) + disty * i;
					}
				}
			}
			break;
			
		case GLFW_KEY_N:
			if(FrameNum > 0 && InterComplete == 1)
			{
				FrameNum--;
				for(int i = 0; i < 3; i++)
				{
					TriangleArray(0, InterTriangle * 3 + i) = InterArray(0, 3 * FrameNum + i);
					TriangleArray(1, InterTriangle * 3 + i) = InterArray(1, 3 * FrameNum + i);
				}
				TriangleData.col(InterTriangle) = InterData.col(FrameNum);
			}
			break;
		
		case GLFW_KEY_M:
			if(FrameNum < 4 && InterComplete == 1)
			{
				FrameNum++;
				for(int i = 0; i < 3; i++)
				{
					TriangleArray(0, InterTriangle * 3 + i) = InterArray(0, 3 * FrameNum + i);
					TriangleArray(1, InterTriangle * 3 + i) = InterArray(1, 3 * FrameNum + i);
				}
				TriangleData.col(InterTriangle) = InterData.col(FrameNum);
			}
			break;
	}
	
	return;
}


//MAIN FUNCTIONS
int main(void)
{
	//Lame required stuff
	GLFWwindow* window;

	//Initialize the library
	if (!glfwInit())
		return -1;

	//Activate supersampling
	glfwWindowHint(GLFW_SAMPLES, 8);

	//Ensure that we get at least a 3.2 context
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

	//On apple we have to load a core profile with forward compatibility
	#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	#endif

	//Create a windowed mode window and its OpenGL context
	window = glfwCreateWindow(640, 640, "Rasterization 2D", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	//Make the window's context current
	glfwMakeContextCurrent(window);

	#ifndef __APPLE__
	glewExperimental = true;
	GLenum err = glewInit();
	if(GLEW_OK != err)
	{
		/*Problem: glewInit failed, something is seriously wrong.*/
		fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
	}
	glGetError(); //pull and savely ignonre unhandled errors like GL_INVALID_ENUM
	fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
	#endif

	int major, minor, rev;
	major = glfwGetWindowAttrib(window, GLFW_CONTEXT_VERSION_MAJOR);
	minor = glfwGetWindowAttrib(window, GLFW_CONTEXT_VERSION_MINOR);
	rev = glfwGetWindowAttrib(window, GLFW_CONTEXT_REVISION);
	printf("OpenGL version recieved: %d.%d.%d\n", major, minor, rev);
	printf("Supported OpenGL is %s\n", (const char*)glGetString(GL_VERSION));
	printf("Supported GLSL is %s\n", (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));

	//End Required Stuff
	//------------------


	//Initialize the VAO
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	//Initialize the VBO with the vertices data
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	
	const GLchar* vertex_shader =
			"#version 150 core\n"
				"in vec2 position;"
				"in vec3 pointcolor;"
				
				"uniform vec3 transform;"
				
				"out vec3 PointColor;"
				"void main()"
				"{"
				"	vec2 newposition = transform[2] * position + transform.xy;"
				"	PointColor = pointcolor;"
				"	gl_Position = vec4(newposition, 0.0, 1.0);"
				"}";
	const GLchar* fragment_shader =
			"#version 150 core\n"
				"in vec3 PointColor;"
				"out vec4 outColor;"
				
				"void main()"
				"{"
				"    outColor = vec4(PointColor, 1.0);"
				"}";
	
	//Vertex Shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertex_shader, NULL);
	glCompileShader(vertexShader);
	
	//Fragment Shader
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragment_shader, NULL);
	glCompileShader(fragmentShader);
	
	//Program Creation
	GLuint Program2d = glCreateProgram();
	glAttachShader(Program2d, vertexShader);
	glAttachShader(Program2d, fragmentShader);
	glBindFragDataLocation(Program2d, 0, "outColor");
	glLinkProgram(Program2d);
	
	//Linking Inputs
	GLint posAttrib = glGetAttribLocation(Program2d, "position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);
	
	GLint colorAttrib = glGetAttribLocation(Program2d, "pointcolor");
	glEnableVertexAttribArray(colorAttrib);
	glVertexAttribPointer(colorAttrib, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
	
	glUseProgram(Program2d);
	
	ViewTransform = glGetUniformLocation(Program2d, "transform");
	glUniform3f(ViewTransform, ViewPosX, ViewPosY, ViewScale);

	//Register the keyboard callback
	glfwSetKeyCallback(window, key_callback);

	//Register the mouse callbacks
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	

	//MAIN LOOP
	while (!glfwWindowShouldClose(window))
	{
		//Bind program
		glUseProgram(Program2d);

		//Clear the framebuffer
		glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		
		//Buffers and Drawing
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 15 * FullTriangles, TriangleArray.data(), GL_DYNAMIC_DRAW);
		glDrawArrays(GL_TRIANGLES, 0, 3 * FullTriangles);
		
		if(DrawPoints == 1)
		{
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * TempLine.size(), TempLine.data(), GL_DYNAMIC_DRAW);
			glDrawArrays(GL_LINES, 0, 2);
		}
		else if(DrawPoints == 2)
		{
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * TempTriangle.size(), TempTriangle.data(), GL_DYNAMIC_DRAW);
			glDrawArrays(GL_TRIANGLES, 0, 3);
		}
		
		
		//Swap front and back buffers
		glfwSwapBuffers(window);
		
		//Poll for and process events
		glfwPollEvents();
	}


	//Ending Stuff
	//------------

	// Deallocate opengl memory
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	glDeleteProgram(Program2d);
	
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);

	// Deallocate glfw internals
	glfwTerminate();
	
	Program3dRun();
	return 0;
}

//The 3d version of the above program
void Program3dRun()
{
	//Lame required stuff
	GLFWwindow* window2;

	//Initialize the library
	if (!glfwInit())
		return;

	//Activate supersampling
	glfwWindowHint(GLFW_SAMPLES, 8);

	//Ensure that we get at least a 3.2 context
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

	//On apple we have to load a core profile with forward compatibility
	#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	#endif

	//Create a windowed mode window and its OpenGL context
	window2 = glfwCreateWindow(640, 640, "Rasterization 3D", NULL, NULL);
	window3d = window2;
	if (!window2)
	{
		glfwTerminate();
		return;
	}

	//Make the window's context current
	glfwMakeContextCurrent(window2);

	#ifndef __APPLE__
	glewExperimental = true;
	GLenum err = glewInit();
	if(GLEW_OK != err)
	{
		/*Problem: glewInit failed, something is seriously wrong.*/
		fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
	}
	glGetError(); //pull and savely ignonre unhandled errors like GL_INVALID_ENUM
	fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
	#endif

	//End Required Stuff
	//------------------
	
	
	//Initialize the VAO
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	//Initialize the VBO with the vertices data
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	
	
	const GLchar* vertex_shader =
			"#version 150 core\n"
				"in vec3 position;"
				"in vec3 normal;"
				
				"uniform vec2 viewtrx;"
				"uniform vec2 viewtry;"
				"uniform vec2 viewtrz;"
				"uniform vec3 objectscale = vec3(1.0f, 1.0f, 1.0f);"
				"uniform vec2 objrotx;"
				"uniform vec2 objroty;"
				"uniform vec2 objrotz;"
				"uniform vec3 objecttranslation = vec3(0.0f, 0.0f, 0.0f);"
				"uniform vec3 barycenter = vec3(0.0f, 0.0f, 0.0f);"
				"uniform vec3 lightsource;"
				"uniform vec2 windowdim;"
				
				"out vec4 phongResult;"
				
				"void main()"
				"{"
				"	mat4 rotate;"
				"	mat4 viewrotate;"
				"	vec4 hposition = vec4(position, 1.0f);"
				"	vec4 hnormal = vec4(normal, 0.0f);"
				"	vec3 barydist = hposition.xyz - barycenter;"
				"	float color;"
				
				//Rotation matrix
				"	vec4 matrix1 = vec4(objrotz[0] * objroty[0], objrotz[1] * objroty[0], -objroty[1], 0);"
				"	vec4 matrix2 = vec4(objrotz[0] * objroty[1] * objrotx[1] - objrotz[1] * objrotx[0],"
				"						objrotz[1] * objroty[1] * objrotx[1] + objrotz[0] * objrotx[0], objroty[0] * objrotx[1], 0);"
				"	vec4 matrix3 = vec4(objrotz[0] * objroty[1] * objrotx[0] + objrotz[1] * objrotx[1],"
				"						objrotz[1] * objroty[1] * objrotx[0] - objrotz[0] * objrotx[1], objroty[0] * objrotx[0], 0);"
				"	vec4 matrix4 = vec4(0, 0, 0, 1);"
				"	rotate[0] =  matrix1;"
				"	rotate[1] =  matrix2;"
				"	rotate[2] =  matrix3;"
				"	rotate[3] =  matrix4;"
				
				"	vec3 baryscale = barydist * objectscale + barycenter;"
				"	hposition = vec4(baryscale, hposition[3]);" //Scales vertices
				"	barydist = hposition.xyz - barycenter;"
				"	hposition = rotate * vec4(barydist, 1.0f) + vec4(barycenter, 0.0f);" //Rotates vertices
				
				"	hposition = vec4(objecttranslation + hposition.xyz, hposition[3]);" //Translates vertices
				
				//Screen Rotation and resizing
				"	vec4 rotmat1 = vec4(viewtrz[0] * viewtry[0], viewtrz[1] * viewtry[0], -viewtry[1], 0);"
				"	vec4 rotmat2 = vec4(viewtrz[0] * viewtry[1] * viewtrx[1] - viewtrz[1] * viewtrx[0],"
				"					viewtrz[1] * viewtry[1] * viewtrx[1] + viewtrz[0] * viewtrx[0], viewtry[0] * viewtrx[1], 0);"
				"	vec4 rotmat3 = vec4(viewtrz[0] * viewtry[1] * viewtrx[0] + viewtrz[1] * viewtrx[1],"
				"					viewtrz[1] * viewtry[1] * viewtrx[0] - viewtrz[0] * viewtrx[1], viewtry[0] * viewtrx[0], 0);"
				"	vec4 rotmat4 = vec4(0, 0, 0, 1);"
				"	viewrotate[0] =  rotmat1;"
				"	viewrotate[1] =  rotmat2;"
				"	viewrotate[2] =  rotmat3;"
				"	viewrotate[3] =  rotmat4;"
				
				"	hposition = viewrotate * hposition;"
				"	hposition = vec4(hposition.xy * windowdim, hposition[2], hposition[3]);"
				
				//Lighting calculations
				"	vec4 lightray = vec4((position - lightsource), 0.0f);"
				"	hnormal = rotate * hnormal;"
				"	color = dot(hnormal, lightray);"
				"	phongResult = vec4(color, color, color, 0.0f) * 0.3;"
				
				"	gl_Position = vec4(hposition);"
				"}";
	const GLchar* fragment_shader =
			"#version 150 core\n"
				"in vec4 phongResult;"
				"out vec4 outColor;"
				
				"uniform vec4 selectcolor;" //Changes color of selected object
				"uniform vec3 vcolor;"
				"uniform float phongshade;"
				
				"void main()"
				"{"
				"	vec4 finalcolor = vec4(0.2f, 0.2f, 0.2f, 1.0f);"
				"	finalcolor = vec4(vcolor, 1.0f) + phongshade * phongResult;"
				"	finalcolor = finalcolor + selectcolor;"
				"	outColor = vec4(finalcolor);"
				"}";
	
	//Vertex Shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertex_shader, NULL);
	glCompileShader(vertexShader);
	
	//Fragment Shader
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragment_shader, NULL);
	glCompileShader(fragmentShader);
	
	//Program Creation
	GLuint Program3d = glCreateProgram();
	glAttachShader(Program3d, vertexShader);
	glAttachShader(Program3d, fragmentShader);
	glBindFragDataLocation(Program3d, 0, "outColor");
	glLinkProgram(Program3d);
	
	//Linking Inputs
	GLint posAttrib = glGetAttribLocation(Program3d, "position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), 0);
	
	GLint normalAttrib = glGetAttribLocation(Program3d, "normal");
	glEnableVertexAttribArray(normalAttrib);
	glVertexAttribPointer(normalAttrib, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
	
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	
	glUseProgram(Program3d);
	
	//Uniforms
	ViewTrans3dx = glGetUniformLocation(Program3d, "viewtrx");
	ViewTrans3dy = glGetUniformLocation(Program3d, "viewtry");
	ViewTrans3dz = glGetUniformLocation(Program3d, "viewtrz");
	Scale3d = glGetUniformLocation(Program3d, "objectscale");
	Rotation3dx = glGetUniformLocation(Program3d, "objrotx");
	Rotation3dy = glGetUniformLocation(Program3d, "objroty");
	Rotation3dz = glGetUniformLocation(Program3d, "objrotz");
	Transl3d = glGetUniformLocation(Program3d, "objecttranslation");
	SelectColor = glGetUniformLocation(Program3d, "selectcolor");
	Barycenter3d = glGetUniformLocation(Program3d, "barycenter");
	VertexColor = glGetUniformLocation(Program3d, "vcolor");
	PhongShade = glGetUniformLocation(Program3d, "phongshade");
	LightSource = glGetUniformLocation(Program3d, "lightsource");
	WindowSize = glGetUniformLocation(Program3d, "windowdim");
	
	//Register the keyboard callback
	glfwSetKeyCallback(window2, key_callback);

	//Register the mouse callbacks
	glfwSetMouseButtonCallback(window2, mouse_button_callback);
	glfwSetCursorPosCallback(window2, cursor_position_callback);
	
	//Register screen resize callback
	glfwSetWindowSizeCallback(window2, resize_callback);

	//MAIN FUNCTIONS
	
	//Populate CubeArray, BunnyArray, and BumpyCubeArray
	string trashstring;
	int trashint;
	int temp1, temp2, temp3, temp4;
	ifstream read;
	
	//Bumpy Cube is scaled down to 1/8
	read.open("../data/bumpy_cube.off");
	
	read >> trashstring;
	read >> BumpyCubeVertices >> BumpyCubeFaces >> trashint;
	Matrix<float, Dynamic, Dynamic> bumpycubevertexarray = Matrix<float, Dynamic, Dynamic>::Zero(BumpyCubeVertices, 3);
	BumpyCubeArray = Matrix<float, Dynamic, Dynamic>::Zero(7, BumpyCubeFaces * 3 * 3);
	
	for(int i = 0; i < BumpyCubeVertices; i++)
		read >> bumpycubevertexarray(i, 0) >> bumpycubevertexarray(i, 1) >> bumpycubevertexarray(i, 2);
	
	for(int i = 0; i < BumpyCubeFaces; i++)
	{
		read >> trashint >> temp1 >> temp2 >> temp3;
		for(int j = 0; j < 3; j++)
		{
			for(int k = 0; k < 3; k++)
			{
				BumpyCubeArray(j, 3 * i + BumpyCubeFaces * 3 * k) = bumpycubevertexarray(temp1, j) / 8.0f;
				BumpyCubeArray(j, 3 * i + BumpyCubeFaces * 3 * k + 1) = bumpycubevertexarray(temp2, j) / 8.0f;
				BumpyCubeArray(j, 3 * i + BumpyCubeFaces * 3 * k + 2) = bumpycubevertexarray(temp3, j) / 8.0f;
			}
			
			BumpyCubeArray(6, 3 * i + BumpyCubeFaces * 3 * j) = float(temp1);
			BumpyCubeArray(6, 3 * i + BumpyCubeFaces * 3 * j + 1) = float(temp2);
			BumpyCubeArray(6, 3 * i + BumpyCubeFaces * 3 * j + 2) = float(temp3);
		}
	}
	
	read.close();
	
	
	//Bunny is scaled by 5 times
	read.open("../data/bunny.off");
	
	read >> trashstring;
	read >> BunnyVertices >> BunnyFaces >> trashint;
	Matrix<float, Dynamic, Dynamic> bunnyvertexarray = Matrix<float, Dynamic, Dynamic>::Zero(BunnyVertices, 3);
	BunnyArray = Matrix<float, Dynamic, Dynamic>::Zero(7, BunnyFaces * 3 * 3);
	
	float totalx = 0, totaly = 0, totalz = 0;
	for(int i = 0; i < BunnyVertices; i++)
	{
		read >> bunnyvertexarray(i, 0) >> bunnyvertexarray(i, 1) >> bunnyvertexarray(i, 2);
		bunnyvertexarray(i, 0) = bunnyvertexarray(i, 0) * 5.0f;
		bunnyvertexarray(i, 1) = bunnyvertexarray(i, 1) * 5.0f;
		bunnyvertexarray(i, 2) = bunnyvertexarray(i, 2) * 5.0f;
		
		totalx += bunnyvertexarray(i, 0);
		totaly += bunnyvertexarray(i, 1);
		totalz += bunnyvertexarray(i, 2);
	}
	
	for(int i = 0; i < BunnyFaces; i++)
	{
		read >> trashint >> temp1 >> temp2 >> temp3;
		for(int j = 0; j < 3; j++)
		{
			for(int k = 0; k < 3; k++)
			{
				BunnyArray(j, 3 * i + BunnyFaces * 3 * k) = bunnyvertexarray(temp1, j);
				BunnyArray(j, 3 * i + BunnyFaces * 3 * k + 1) = bunnyvertexarray(temp2, j);
				BunnyArray(j, 3 * i + BunnyFaces * 3 * k + 2) = bunnyvertexarray(temp3, j);
			}
			
			BunnyArray(6, 3 * i + BunnyFaces * 3 * j) = float(temp1);
			BunnyArray(6, 3 * i + BunnyFaces * 3 * j + 1) = float(temp2);
			BunnyArray(6, 3 * i + BunnyFaces * 3 * j + 2) = float(temp3);
		}
	}
	
	read.close();
	
	
	read.open("../data/cube.off");
	
	read >> trashstring;
	read >> trashint >> trashint >> trashint;
	MatrixXf tempvertexarray(8, 3);
	
	for(int i = 0; i < 8; i++)
		read >> tempvertexarray(i, 0) >> tempvertexarray(i, 1) >> tempvertexarray(i, 2);
	
	for(int i = 0; i < 6; i++)
	{
		read >> trashint >> temp1 >> temp2 >> temp3 >> temp4;
		for(int j = 0; j < 3; j++)
		{
			for(int k = 0; k < 3; k++)
			{
				CubeArray(j, 6 * i + 36 * k) = tempvertexarray(temp1, j);
				CubeArray(j, 6 * i + 36 * k + 1) = tempvertexarray(temp2, j);
				CubeArray(j, 6 * i + 36 * k + 2) = tempvertexarray(temp3, j);
				
				CubeArray(j, 6 * i + 36 * k + 3) = tempvertexarray(temp3, j);
				CubeArray(j, 6 * i + 36 * k + 4) = tempvertexarray(temp4, j);
				CubeArray(j, 6 * i + 36 * k + 5) = tempvertexarray(temp1, j);
				
				CubeArray(j + 3, 6 * i + 36 * k) = 0;
				CubeArray(j + 3, 6 * i + 36 * k + 1) = 0;
				CubeArray(j + 3, 6 * i + 36 * k + 2) = 0;
				CubeArray(j + 3, 6 * i + 36 * k + 3) = 0;
				CubeArray(j + 3, 6 * i + 36 * k + 4) = 0;
				CubeArray(j + 3, 6 * i + 36 * k + 5) = 0;
			}
			
			CubeArray(6, 6 * i + 36 * j) = float(temp1);
			CubeArray(6, 6 * i + 36 * j + 1) = float(temp2);
			CubeArray(6, 6 * i + 36 * j + 2) = float(temp3);
			CubeArray(6, 6 * i + 36 * j + 3) = float(temp3);
			CubeArray(6, 6 * i + 36 * j + 4) = float(temp4);
			CubeArray(6, 6 * i + 36 * j + 5) = float(temp1);
		}
	}
	
	read.close();
	
	
	//Calculate vertex normals
	Vector3f v1, v2, v3, normal, total;
	for(int i = 0; i < 12; i++)
	{
		v1 << CubeArray(0, i * 3), CubeArray(1, i * 3), CubeArray(2, i * 3);
		v2 << CubeArray(0, i * 3 + 1), CubeArray(1, i * 3 + 1), CubeArray(2, i * 3 + 1);
		v3 << CubeArray(0, i * 3 + 2), CubeArray(1, i * 3 + 2), CubeArray(2, i * 3 + 2);
		normal = CalcNormal(v1, v2, v3);
		
		for(int j = 0; j < 36 * 3; j++)
		{
			if(CubeArray(6, i * 3) == CubeArray(6, j))
			{
				CubeArray(3, j) += normal[0];
				CubeArray(4, j) += normal[1];
				CubeArray(5, j) += normal[2];
			}
			
			if(CubeArray(6, i * 3 + 1) == CubeArray(6, j))
			{
				CubeArray(3, j) += normal[0];
				CubeArray(4, j) += normal[1];
				CubeArray(5, j) += normal[2];
			}
				
			if(CubeArray(6, i * 3 + 2) == CubeArray(6, j))
			{
				CubeArray(3, j) += normal[0];
				CubeArray(4, j) += normal[1];
				CubeArray(5, j) += normal[2];
			}
		}
	}
	for(int i = 0; i < 36 * 3; i++)
	{
		total = RowVector3f(CubeArray(3, i), CubeArray(4, i), CubeArray(5, i));
		total.normalize();
		CubeArray(3, i) = total[0];
		CubeArray(4, i) = total[1];
		CubeArray(5, i) = total[2];
	}

	for(int i = 0; i < BunnyFaces * 3; i++)
	{
		v1 << BunnyArray(0, i * 3), BunnyArray(1, i * 3), BunnyArray(2, i * 3);
		v2 << BunnyArray(0, i * 3 + 1), BunnyArray(1, i * 3 + 1), BunnyArray(2, i * 3 + 1);
		v3 << BunnyArray(0, i * 3 + 2), BunnyArray(1, i * 3 + 2), BunnyArray(2, i * 3 + 2);
		normal = CalcNormal(v1, v2, v3);
		
		for(int j = 0; j < BunnyFaces * 3 * 3; j++)
		{
			if(BunnyArray(6, i * 3) == BunnyArray(6, j))
			{
				BunnyArray(3, j) += normal[0];
				BunnyArray(4, j) += normal[1];
				BunnyArray(5, j) += normal[2];
			}
			
			if(BunnyArray(6, i * 3 + 1) == BunnyArray(6, j))
			{
				BunnyArray(3, j) += normal[0];
				BunnyArray(4, j) += normal[1];
				BunnyArray(5, j) += normal[2];
			}
				
			if(BunnyArray(6, i * 3 + 2) == BunnyArray(6, j))
			{
				BunnyArray(3, j) += normal[0];
				BunnyArray(4, j) += normal[1];
				BunnyArray(5, j) += normal[2];
			}
		}
	}
	for(int i = 0; i < BunnyFaces * 3 * 3; i++)
	{
		total = RowVector3f(BunnyArray(3, i), BunnyArray(4, i), BunnyArray(5, i));
		total.normalize();
		BunnyArray(3, i) = total[0];
		BunnyArray(4, i) = total[1];
		BunnyArray(5, i) = total[2];
	}
	
	for(int i = 0; i < BumpyCubeFaces * 3; i++)
	{
		v1 << BumpyCubeArray(0, i * 3), BumpyCubeArray(1, i * 3), BumpyCubeArray(2, i * 3);
		v2 << BumpyCubeArray(0, i * 3 + 1), BumpyCubeArray(1, i * 3 + 1), BumpyCubeArray(2, i * 3 + 1);
		v3 << BumpyCubeArray(0, i * 3 + 2), BumpyCubeArray(1, i * 3 + 2), BumpyCubeArray(2, i * 3 + 2);
		normal = CalcNormal(v1, v2, v3);
		
		for(int j = 0; j < BumpyCubeFaces * 3 * 3; j++)
		{
			if(BumpyCubeArray(6, i * 3) == BumpyCubeArray(6, j))
			{
				BumpyCubeArray(3, j) += normal[0];
				BumpyCubeArray(4, j) += normal[1];
				BumpyCubeArray(5, j) += normal[2];
			}
			
			if(BumpyCubeArray(6, i * 3 + 1) == BumpyCubeArray(6, j))
			{
				BumpyCubeArray(3, j) += normal[0];
				BumpyCubeArray(4, j) += normal[1];
				BumpyCubeArray(5, j) += normal[2];
			}
				
			if(BumpyCubeArray(6, i * 3 + 2) == BumpyCubeArray(6, j))
			{
				BumpyCubeArray(3, j) += normal[0];
				BumpyCubeArray(4, j) += normal[1];
				BumpyCubeArray(5, j) += normal[2];
			}
		}
	}
	for(int i = 0; i < BumpyCubeFaces * 3 * 3; i++)
	{
		total = RowVector3f(BumpyCubeArray(3, i), BumpyCubeArray(4, i), BumpyCubeArray(5, i));
		total.normalize();
		BumpyCubeArray(3, i) = total[0];
		BumpyCubeArray(4, i) = total[1];
		BumpyCubeArray(5, i) = total[2];
	}
	
	//Populate data fields
	totalx = totalx / BunnyVertices;
	totaly = totaly / BunnyVertices;
	totalz = totalz / BunnyVertices;
	for(int i = 0; i < 3; i++)
	{
		CubeData.col(i) << 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f;
		BunnyData.col(i) <<  totalx, totaly, totalz, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f;
		BumpyCubeData.col(i) << 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f;
	}
	ScreenRotation << 0.0f, 0.0f, 0.0f;
	glUniform2f(ViewTrans3dx, cos(ScreenRotation[0]), sin(ScreenRotation[0]));
	glUniform2f(ViewTrans3dy, cos(ScreenRotation[1]), sin(ScreenRotation[1]));
	glUniform2f(ViewTrans3dz, cos(ScreenRotation[2]), sin(ScreenRotation[2]));
	glUniform1f(PhongShade, 0.0f);
	glUniform3f(LightSource, -2.0f, 2.0f, -2.0f);
	glUniform2f(WindowSize, 1.0f, 1.0f);
	
	//MAIN LOOP
	while (!glfwWindowShouldClose(window2))
	{
		//Bind program
		glUseProgram(Program3d);

		//Clear the framebuffer
		glClearColor(0.95f, 0.95f, 0.95f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		
		//Draw Cubes
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 36 * 7 * NumCubes, CubeArray.data(), GL_DYNAMIC_DRAW);
		Vector3f v1, v2, v3;
		
		for(int i = 0; i < NumCubes; i++)
		{
			if(CurrentObject == i)
				glUniform4f(SelectColor, 0.4f, 0.4f, 0.4f, 0.0f);
			else
				glUniform4f(SelectColor, 0.0f, 0.0f, 0.0f, 0.0f);
			
			//Barycenter [0-2]
			glUniform3f(Barycenter3d, CubeData(0, i), CubeData(1, i), CubeData(2, i));
			//Scaling [4]
			glUniform3f(Scale3d, CubeData(4, i), CubeData(4, i), CubeData(4, i));
			//Rotation [5-7]
			glUniform2f(Rotation3dx, cos(CubeData(5, i)), sin(CubeData(5, i)));
			glUniform2f(Rotation3dy, cos(CubeData(6, i)), sin(CubeData(6, i)));
			glUniform2f(Rotation3dz, cos(CubeData(7, i)), sin(CubeData(7, i)));
			//Translation [8-10]
			glUniform3f(Transl3d, CubeData(8, i), CubeData(9, i), CubeData(10 ,i));
			
			//Shading type [3]
			//Wire frame
			if(CubeData(3, i) == 0.0f)
			{
				glUniform3f(VertexColor, 0.0f, 0.0f, 0.0f);
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				glDrawArrays(GL_TRIANGLES, i * 6 * 6, 6 * 6);
			}
			//Flat shading
			else if(CubeData(3, i) == 1.0f)
			{
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				
				for(int j = 0; j < 12; j++)
				{
					v1 << CubeArray(0, j * 3 + i * 36), CubeArray(1, j * 3 + i * 36), CubeArray(2, j * 3 + i * 36);
					v2 << CubeArray(0, j * 3 + i * 36 + 1), CubeArray(1, j * 3 + i * 36 + 1), CubeArray(2, j * 3 + i * 36 + 1);
					v3 << CubeArray(0, j * 3 + i * 36 + 2), CubeArray(1, j * 3 + i * 36 + 2), CubeArray(2, j * 3 + i * 36 + 2);
					
					FlatShading(v1, v2, v3, "cube", i); //Will update uniform for this triangle
					glDrawArrays(GL_TRIANGLES, 6 * 6 * i + 3 * j, 3);
				}
				
				glUniform3f(VertexColor, 0.0f, 0.0f, 0.0f);
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				glDrawArrays(GL_TRIANGLES, i * 6 * 6, 6 * 6);
			}
			//Phong shading
			else if(CubeData(3, i) == 2.0f)
			{
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				glUniform3f(VertexColor, 0.0f, 0.0f, 0.0f);
				glUniform1f(PhongShade, 1.0f);
				glDrawArrays(GL_TRIANGLES, i * 6 * 6, 6 * 6);
				glUniform1f(PhongShade, 0.0f);
			}
		}
		
		
		//Draw Bunnies
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * BunnyFaces * 3 * 7 * NumBunnies, BunnyArray.data(), GL_DYNAMIC_DRAW);
		
		for(int i = 0; i < NumBunnies; i++)
		{
			if(CurrentObject == i + 3)
				glUniform4f(SelectColor, 0.4f, 0.4f, 0.4f, 0.0f);
			else
				glUniform4f(SelectColor, 0.0f, 0.0f, 0.0f, 0.0f);
			
			//Barycenter [0-2]
			glUniform3f(Barycenter3d, BunnyData(0, i), BunnyData(1, i), BunnyData(2, i));
			//Scaling [4]
			glUniform3f(Scale3d, BunnyData(4, i), BunnyData(4, i), BunnyData(4, i));
			//Rotation [5-7]
			glUniform2f(Rotation3dx, cos(BunnyData(5, i)), sin(BunnyData(5, i)));
			glUniform2f(Rotation3dy, cos(BunnyData(6, i)), sin(BunnyData(6, i)));
			glUniform2f(Rotation3dz, cos(BunnyData(7, i)), sin(BunnyData(7, i)));
			//Translation [8-10]
			glUniform3f(Transl3d, BunnyData(8, i), BunnyData(9, i), BunnyData(10, i));
			
			//Shading type [3]
			//Wire frame
			if(BunnyData(3, i) == 0.0f)
			{
				glUniform3f(VertexColor, 0.0f, 0.0f, 0.0f);
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				glDrawArrays(GL_TRIANGLES, i * BunnyFaces * 3, BunnyFaces * 3);
			}
			//Flat shading
			else if(BunnyData(3, i) == 1.0f)
			{
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				
				for(int j = 0; j < BunnyFaces; j++)
				{
					v1 << BunnyArray(0, j * 3 + i * BunnyFaces * 3), BunnyArray(1, j * 3 + i * BunnyFaces * 3),
							BunnyArray(2, j * 3 + i * BunnyFaces * 3);
					v2 << BunnyArray(0, j * 3 + i * BunnyFaces * 3 + 1), BunnyArray(1, j * 3 + i * BunnyFaces * 3 + 1),
							BunnyArray(2, j * 3 + i * BunnyFaces * 3 + 1);
					v3 << BunnyArray(0, j * 3 + i * BunnyFaces * 3 + 2), BunnyArray(1, j * 3 + i * BunnyFaces * 3 + 2),
							BunnyArray(2, j * 3 + i * BunnyFaces * 3 + 2);
					
					FlatShading(v1, v2, v3, "bunny", i); //Will update uniform for this triangle
					glDrawArrays(GL_TRIANGLES, BunnyFaces * 3 * i + 3 * j, 3);
				}
				
				glUniform3f(VertexColor, 0.0f, 0.0f, 0.0f);
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				glDrawArrays(GL_TRIANGLES, i * BunnyFaces * 3, BunnyFaces * 3);
			}
			//Phong shading
			else if(BunnyData(3, i) == 2.0f)
			{
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				glUniform3f(VertexColor, 0.0f, 0.0f, 0.0f);
				glUniform1f(PhongShade, 1.0f);
				glDrawArrays(GL_TRIANGLES, i * BunnyFaces * 3, BunnyFaces * 3);
				glUniform1f(PhongShade, 0.0f);
			}
		}
		
		
		//Draw Bumpy Cubes
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * BumpyCubeFaces * 3 * 7 * NumBumpyCubes, BumpyCubeArray.data(), GL_DYNAMIC_DRAW);
		
		for(int i = 0; i < NumBumpyCubes; i++)
		{
			if(CurrentObject == i + 6)
				glUniform4f(SelectColor, 0.4f, 0.4f, 0.4f, 0.0f);
			else
				glUniform4f(SelectColor, 0.0f, 0.0f, 0.0f, 0.0f);
			
			//Barycenter [0-2]
			glUniform3f(Barycenter3d, BumpyCubeData(0, i), BumpyCubeData(1, i), BumpyCubeData(2, i));
			//Scaling [4]
			glUniform3f(Scale3d, BumpyCubeData(4, i), BumpyCubeData(4, i), BumpyCubeData(4, i));
			//Rotation [5-7]
			glUniform2f(Rotation3dx, cos(BumpyCubeData(5, i)), sin(BumpyCubeData(5, i)));
			glUniform2f(Rotation3dy, cos(BumpyCubeData(6, i)), sin(BumpyCubeData(6, i)));
			glUniform2f(Rotation3dz, cos(BumpyCubeData(7, i)), sin(BumpyCubeData(7, i)));
			//Translation [8-10]
			glUniform3f(Transl3d, BumpyCubeData(8, i), BumpyCubeData(9, i), BumpyCubeData(10, i));
			
			//Shading type [3]
			//Wire frame
			if(BumpyCubeData(3, i) == 0.0f)
			{
				glUniform3f(VertexColor, 0.0f, 0.0f, 0.0f);
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				glDrawArrays(GL_TRIANGLES, i * BumpyCubeFaces * 3, BumpyCubeFaces * 3);
			}
			//Flat shading
			else if(BumpyCubeData(3, i) == 1.0f)
			{
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				
				for(int j = 0; j < BumpyCubeFaces; j++)
				{
					v1 << BumpyCubeArray(0, j * 3 + i * BumpyCubeFaces * 3), BumpyCubeArray(1, j * 3 + i * BumpyCubeFaces * 3),
							BumpyCubeArray(2, j * 3 + i * BumpyCubeFaces * 3);
					v2 << BumpyCubeArray(0, j * 3 + i * BumpyCubeFaces * 3 + 1), BumpyCubeArray(1, j * 3 + i * BumpyCubeFaces * 3 + 1),
							BumpyCubeArray(2, j * 3 + i * BumpyCubeFaces * 3 + 1);
					v3 << BumpyCubeArray(0, j * 3 + i * BumpyCubeFaces * 3 + 2), BumpyCubeArray(1, j * 3 + i * BumpyCubeFaces * 3 + 2),
							BumpyCubeArray(2, j * 3 + i * BumpyCubeFaces * 3 + 2);
					
					FlatShading(v1, v2, v3, "bumpycube", i); //Will update uniform for this triangle
					glDrawArrays(GL_TRIANGLES, BumpyCubeFaces * 3 * i + 3 * j, 3);
				}
				
				glUniform3f(VertexColor, 0.0f, 0.0f, 0.0f);
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				glDrawArrays(GL_TRIANGLES, i * BumpyCubeFaces * 3, BumpyCubeFaces * 3);
			}
			//Phong shading
			else if(BumpyCubeData(3, i) == 2.0f)
			{
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				glUniform3f(VertexColor, 0.0f, 0.0f, 0.0f);
				glUniform1f(PhongShade, 1.0f);
				glDrawArrays(GL_TRIANGLES, i * BumpyCubeFaces * 3, BumpyCubeFaces * 3);
				glUniform1f(PhongShade, 0.0f);
			}
		}
		
		check_gl_error();
		//Swap front and back buffers
		glfwSwapBuffers(window2);
		
		//Poll for and process events
		glfwPollEvents();
	}
	
	//Ending Stuff
	//------------

	// Deallocate opengl memory
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	glDeleteProgram(Program3d);
	
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glfwTerminate();
	
	return;
}
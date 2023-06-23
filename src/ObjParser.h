#pragma once
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
/*** freeglut***/
#include <freeglut.h>
// gltools
//#include "gltools.h"
//#include "math3d.h"
struct Vec3f
{
	float x;
	float y;
	float z;
};
struct Vec3d
{
	int a;
	int b;
	int c;
};
class ObjParser
{
private:
	// min vertex
	float _minX, _minY, _minZ;
	// max vertex
	float _maxX, _maxY, _maxZ;
	// properties
	Vec3f _origin;
	Vec3f _offset;
	Vec3f _boundingBox;
	float _maxBoundingBoxSide;
	float _pointSize, _lineWidth;
	std::vector<Vec3f> _vertices;
	std::vector<Vec3d> _faces;
	void DrawPoints(bool isTex);
	void DrawLines(bool isTex);
	void DrawFaces(bool isTex);
public:
	ObjParser(std::string filename);
	ObjParser(std::string filename, float pointSize, float lineWidth);
	void LoadFile(std::string filename);
	void Draw(GLenum renderMode, bool isTex);
	Vec3f GetOrigin();
	Vec3f GetOffset();
	float GetMaxBoundingBoxSide();
};
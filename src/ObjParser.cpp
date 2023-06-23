#include "ObjParser.h"

ObjParser::ObjParser(std::string filename)
{
	LoadFile(filename);
	_pointSize = 4;
	_lineWidth = 2;
}
ObjParser::ObjParser(std::string filename, float pointSize, float lineWidth)
{
	LoadFile(filename);
	if (pointSize > 0) { _pointSize = pointSize; }
	if (lineWidth > 0) { _lineWidth = lineWidth; }
}
void ObjParser::LoadFile(std::string filename)
{
	_vertices.clear();
	_faces.clear();
	_minX = _minY = _minZ = _maxX = _maxY = _maxZ = 0;

	Vec3f points;
	Vec3d indexes;
	int u, v, w;
	float x, y, z;
	std::ifstream file(filename);
	std::string line;
	while (getline(file, line))
	{
		if (line.substr(0, 1) == "v")
		{
			std::istringstream values(line.substr(2));
			values >> x; values >> y; values >> z;
			// update min vertex
			_minX = x < _minX ? x : _minX;
			_minY = y < _minY ? y : _minY;
			_minZ = z < _minZ ? z : _minZ;
			// update max vertex
			_maxX = x > _maxX ? x : _maxX;
			_maxY = y > _maxY ? y : _maxY;
			_maxZ = z > _maxZ ? z : _maxZ;
			points.x = x;
			points.y = y;
			points.z = z;
			_vertices.push_back(points);
		}
		else if (line.substr(0, 1) == "f")
		{
			std::istringstream values(line.substr(2));
			values >> u; values >> v; values >> w;
			indexes.a = u - 1;
			indexes.b = v - 1;
			indexes.c = w - 1;
			_faces.push_back(indexes);
		}
	}
	file.close();
	// calculate origin
	_origin.x = _minX + _maxX;
	_origin.y = _minY + _maxY;
	_origin.z = _minZ + _maxZ;
	// calculate centroid
	_offset.x = -(_origin.x) / 2.f;
	_offset.y = -(_origin.y) / 2.f;
	_offset.z = -(_origin.z) / 2.f;
	// calculate bounding box and max size of axis
	_boundingBox.x = (std::abs(_minX) + std::abs(_maxX)) / 2.f;
	_boundingBox.y = (std::abs(_minY) + std::abs(_maxY)) / 2.f;
	_boundingBox.z = (std::abs(_minZ) + std::abs(_maxZ)) / 2.f;
	_maxBoundingBoxSide = _boundingBox.x > _boundingBox.y ? _boundingBox.x : _boundingBox.y;
	_maxBoundingBoxSide = _boundingBox.z > _maxBoundingBoxSide ? _boundingBox.z : _maxBoundingBoxSide;
	// print status
	std::cout << "origin: " << _origin.x << " " << _origin.y << " " << _origin.z << std::endl;
	std::cout << "offset: " << _offset.x << " " << _offset.y << " " << _offset.z << std::endl;
}
void ObjParser::Draw(GLenum renderMode, bool isTex)
{
	switch (renderMode)
	{
	case GL_POINTS:
		DrawPoints(isTex);
		break;
	case GL_LINES:
		DrawLines(isTex);
		break;
	case GL_TRIANGLES:
		DrawFaces(isTex);
		break;
	default:
		break;
	}
}
void ObjParser::DrawPoints(bool isTex)
{
	glPointSize(_pointSize);
	glBegin(GL_POINTS);
	for (const Vec3f& vertex : _vertices) 
	{
		glVertex3f(vertex.x, vertex.y, vertex.z);
	}
	glEnd();
	glPointSize(1); // reset default value
}
void ObjParser::DrawLines(bool isTex)
{
	Vec3f a, b, c;
	GLint vertexIndexA, vertexIndexB, vertexIndexC;
	glLineWidth(_lineWidth);
	glBegin(GL_LINES);
	for (const Vec3d& face : _faces) 
	{
		vertexIndexA = face.a;
		vertexIndexB = face.b;
		vertexIndexC = face.c;

		a.x = _vertices[vertexIndexA].x;
		a.y = _vertices[vertexIndexA].y;
		a.z = _vertices[vertexIndexA].z;

		b.x = _vertices[vertexIndexB].x;
		b.y = _vertices[vertexIndexB].y;
		b.z = _vertices[vertexIndexB].z;

		c.x = _vertices[vertexIndexC].x;
		c.y = _vertices[vertexIndexC].y;
		c.z = _vertices[vertexIndexC].z;

		glVertex3f(a.x, a.y, a.z);
		glVertex3f(b.x, b.y, b.z);
		glVertex3f(b.x, b.y, b.z);
		glVertex3f(c.x, c.y, c.z);
		glVertex3f(c.x, c.y, c.z);
		glVertex3f(a.x, a.y, a.z);
	}
	glEnd();
	glLineWidth(1); // reset default value
}
void ObjParser::DrawFaces(bool isTex)
{
	Vec3f a, b, c;
	GLint vertexIndexA, vertexIndexB, vertexIndexC;
	glBegin(GL_TRIANGLES);
	for (const Vec3d& face : _faces) 
	{
		vertexIndexA = face.a;
		vertexIndexB = face.b;
		vertexIndexC = face.c;

		a.x = _vertices[vertexIndexA].x;
		a.y = _vertices[vertexIndexA].y;
		a.z = _vertices[vertexIndexA].z;

		b.x = _vertices[vertexIndexB].x;
		b.y = _vertices[vertexIndexB].y;
		b.z = _vertices[vertexIndexB].z;

		c.x = _vertices[vertexIndexC].x;
		c.y = _vertices[vertexIndexC].y;
		c.z = _vertices[vertexIndexC].z;

		if (isTex)
		{
			glTexCoord2f(0, 0); glVertex3f(a.x, a.y, a.z);
			glTexCoord2f(1, 0); glVertex3f(b.x, b.y, b.z);
			glTexCoord2f(0, 1); glVertex3f(c.x, c.y, c.z);
		}
		else
		{
			glVertex3f(a.x, a.y, a.z);
			glVertex3f(b.x, b.y, b.z);
			glVertex3f(c.x, c.y, c.z);
		}
	}
	glEnd();
}
Vec3f ObjParser::GetOrigin()
{
	return _origin;
}
Vec3f ObjParser::GetOffset()
{
	return _offset;
}
float ObjParser::GetMaxBoundingBoxSide()
{
	return _maxBoundingBoxSide;
}
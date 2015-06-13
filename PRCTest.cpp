
#include "libPRC\oPRCFile.h"

struct Vector3
{
	float X, Y, Z;
	Vector3()
	{}
	Vector3(float x, float y, float z) :
		X(x), Y(y), Z(z)
	{}
};

Vector3 operator+(const Vector3& a, const Vector3& b)
{
	return Vector3(a.X + b.X, a.Y + b.Y, a.Z + b.Z);
}

Vector3 operator-(const Vector3& a, const Vector3& b)
{
	return Vector3(a.X - b.X, a.Y - b.Y, a.Z - b.Z);
}

struct IMesh
{
	virtual int GetVertexCount() const = 0;
	virtual Vector3 GetVertex(int i) const = 0;
	virtual Vector3 GetNormal(int i)  const = 0;

	virtual int GetFaceColor(int i)  const = 0;

	virtual const std::vector<int>& GetFaceIndices() const = 0;
};

struct Box : IMesh
{
	std::vector < std::pair<Vector3, Vector3>> _vertices;
	std::vector <int> _indices;

	int vbAdd(const Vector3& v, const Vector3& n)
	{
		_vertices.push_back(std::make_pair(v, n));
		return _vertices.size() - 1;
	}

	void ibAdd(int index)
	{
		_indices.push_back(index);
	}

	Box()
	{
		// 1-----2
		// |\    |\       Y
		// | 5-----6      |
		// 0-|---3 |      +---- X
		//  \|    \|       \
		//   4-----7        Z

		Vector3 center(0, 0, 0);
		Vector3 size(1, 1, 1);

		Vector3 x(size.X / 2, 0, 0);
		Vector3 y(0, size.Y / 2, 0);
		Vector3 z(0, 0, size.Z / 2);

		std::vector<Vector3> vertices;
		vertices.resize(8);

		vertices[0] = center - x - y - z;
		vertices[1] = center - x + y - z;
		vertices[2] = center + x + y - z;
		vertices[3] = center + x - y - z;
		vertices[4] = center - x - y + z;
		vertices[5] = center - x + y + z;
		vertices[6] = center + x + y + z;
		vertices[7] = center + x - y + z;

		ibAdd(vbAdd(vertices[5], Vector3(-1, 0, 0)));
		ibAdd(vbAdd(vertices[1], Vector3(-1, 0, 0)));
		ibAdd(vbAdd(vertices[0], Vector3(-1, 0, 0)));
		ibAdd(vbAdd(vertices[0], Vector3(-1, 0, 0)));
		ibAdd(vbAdd(vertices[4], Vector3(-1, 0, 0)));
		ibAdd(vbAdd(vertices[5], Vector3(-1, 0, 0)));

		ibAdd(vbAdd(vertices[7], Vector3(+1, 0, 0)));
		ibAdd(vbAdd(vertices[3], Vector3(+1, 0, 0)));
		ibAdd(vbAdd(vertices[2], Vector3(+1, 0, 0)));
		ibAdd(vbAdd(vertices[2], Vector3(+1, 0, 0)));
		ibAdd(vbAdd(vertices[6], Vector3(+1, 0, 0)));
		ibAdd(vbAdd(vertices[7], Vector3(+1, 0, 0)));

		ibAdd(vbAdd(vertices[0], Vector3(0, -1, 0)));
		ibAdd(vbAdd(vertices[3], Vector3(0, -1, 0)));
		ibAdd(vbAdd(vertices[4], Vector3(0, -1, 0)));
		ibAdd(vbAdd(vertices[3], Vector3(0, -1, 0)));
		ibAdd(vbAdd(vertices[7], Vector3(0, -1, 0)));
		ibAdd(vbAdd(vertices[4], Vector3(0, -1, 0)));

		ibAdd(vbAdd(vertices[5], Vector3(0, +1, 0)));
		ibAdd(vbAdd(vertices[2], Vector3(0, +1, 0)));
		ibAdd(vbAdd(vertices[1], Vector3(0, +1, 0)));
		ibAdd(vbAdd(vertices[5], Vector3(0, +1, 0)));
		ibAdd(vbAdd(vertices[6], Vector3(0, +1, 0)));
		ibAdd(vbAdd(vertices[2], Vector3(0, +1, 0)));

		ibAdd(vbAdd(vertices[0], Vector3(0, 0, -1)));
		ibAdd(vbAdd(vertices[1], Vector3(0, 0, -1)));
		ibAdd(vbAdd(vertices[2], Vector3(0, 0, -1)));
		ibAdd(vbAdd(vertices[2], Vector3(0, 0, -1)));
		ibAdd(vbAdd(vertices[3], Vector3(0, 0, -1)));
		ibAdd(vbAdd(vertices[0], Vector3(0, 0, -1)));

		ibAdd(vbAdd(vertices[5], Vector3(0, 0, +1)));
		ibAdd(vbAdd(vertices[4], Vector3(0, 0, +1)));
		ibAdd(vbAdd(vertices[7], Vector3(0, 0, +1)));
		ibAdd(vbAdd(vertices[6], Vector3(0, 0, +1)));
		ibAdd(vbAdd(vertices[5], Vector3(0, 0, +1)));
		ibAdd(vbAdd(vertices[7], Vector3(0, 0, +1)));

	}
	virtual int GetVertexCount() const
	{
		return _vertices.size();
	}
	virtual Vector3 GetVertex(int i) const
	{
		return _vertices[i].first;
	}

	virtual Vector3 GetNormal(int i)  const
	{
		return _vertices[i].second;
	}

	virtual int GetFaceColor(int i) const
	{
		return 0xff000000;
	}

	virtual const std::vector<int>& GetFaceIndices() const
	{
		return _indices;
	}

};

class PRCWriter
{
	oPRCFile _prcFile;

	uint32_t addMaterial(int rgba)
	{
		RGBAColour ambient(1, 0, 0);
		RGBAColour diffuse(1, 0, 0);
		RGBAColour emission(1, 0, 0);
		RGBAColour specular(1, 0, 0);
		double alpha = 1;
		double shiness = 1;

		PRCmaterial m(ambient, diffuse, emission, specular, alpha, shiness);

		const uint32_t style = _prcFile.addMaterial(m);
		return style;
	}

public:
	void addMesh(const IMesh& mesh)
	{
		PRC3DTess *tess = new PRC3DTess();

		tess->coordinates.reserve((size_t)mesh.GetVertexCount());
		tess->normal_coordinate.reserve((size_t)mesh.GetVertexCount());
		for (int i = 0; i < mesh.GetVertexCount(); i++)
		{
			auto v = mesh.GetVertex(i);
			tess->coordinates.push_back(v.X);
			tess->coordinates.push_back(v.Y);
			tess->coordinates.push_back(v.Z);

			auto n = mesh.GetNormal(i);

			tess->normal_coordinate.push_back(n.X);
			tess->normal_coordinate.push_back(n.Y);
			tess->normal_coordinate.push_back(n.Z);
		}

		auto faceIndices = mesh.GetFaceIndices();

		PRCTessFace *tessFace = new PRCTessFace();
		tessFace->number_of_texture_coordinate_indexes = 0;
		tessFace->used_entities_flag |= PRC_FACETESSDATA_Triangle;
		tessFace->start_triangulated = 0;
		tessFace->sizes_triangulated.push_back(faceIndices.size());
		tessFace->is_rgba = false;


		int color = mesh.GetFaceColor(0);
		for (auto it = faceIndices.cbegin(); it != faceIndices.cend(); ++it)
		{
			tess->triangulated_index.push_back(*it);
		}

		tess->addTessFace(tessFace);
		tess->has_faces = true;

		const uint32_t tess_index = _prcFile.add3DTess(tess);

		auto styleIndex = addMaterial(color);

		_prcFile.begingroup("mesh");

		_prcFile.useMesh(tess_index, styleIndex);

		_prcFile.endgroup();
	}

public:
	PRCWriter(std::ostream& stream) :
		_prcFile(stream)
	{

	}
};

void main(void)
{
	std::ofstream ostream("D:\\dev\\libs\\libPRC\\Debug\\test.prc");
	PRCWriter prcWriter(ostream);

	Box box;
	prcWriter.addMesh(box);
}


#include "libPRC\oPRCFile.h"

//http://fossies.org/dox/mathgl-2.3.3/oPRCFile_8cc_source.html

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

#include <array>

class PRCWriter
{
	oPRCFile _prcFile;

	uint32_t addMaterial(int rgba)
	{
		RGBAColour ambient(0.5, 0.0, 0.0);
		RGBAColour diffuse(1, 0, 0);
		RGBAColour emission(0.5, 0.0, 0.0);
		RGBAColour specular(0.5, 0.0, 0.0);
		double alpha = 1;
		double shiness = 1;

		PRCmaterial m(ambient, diffuse, emission, specular, alpha, shiness);

		const uint32_t style = _prcFile.addMaterial(m);
		return style;
	}

public:
	void addMesh(const IMesh& mesh)
	{
		_prcFile.begingroup("mesh");

		uint32_t nP = mesh.GetVertexCount();
		uint32_t nN = mesh.GetVertexCount();

		auto P = new double[nP][3];
		auto N = new double[nP][3];

		for (int i = 0; i < mesh.GetVertexCount(); i++)
		{
			auto v = mesh.GetVertex(i);
			P[i][0] = v.X;
			P[i][1] = v.Y;
			P[i][2] = v.Z;

			auto n = mesh.GetNormal(i);

			N[i][0] = n.X;
			N[i][1] = n.Y;
			N[i][2] = n.Z;
		}

		auto indices = mesh.GetFaceIndices();
		uint32_t nI = indices.size() / 3;
		auto PI = new uint32_t[nI][3];

		auto& NI = PI;

		int i = 0;
		for (auto it = indices.cbegin(); it != indices.cend();)
		{
			PI[i][0] = (uint32_t)*it++;
			PI[i][1] = (uint32_t)*it++;
			PI[i++][2] = (uint32_t)*it++;
		}

		auto color = mesh.GetFaceColor(0);
		auto styleIndex = addMaterial(color);

		const uint32_t tess_index = _prcFile.createTriangleMesh(
			nP, 
			P,
			nI,
			PI, 
			styleIndex, 
			nN, 
			N,
			NI, 0, NULL, NULL, 0, NULL, NULL, 0, NULL, NULL, 0);

		_prcFile.useMesh(tess_index, styleIndex);

		_prcFile.endgroup();
	}

public:
	PRCWriter(std::ostream& stream) :
		_prcFile(stream)
	{

	}

	~PRCWriter()
	{
		_prcFile.finish();
	}
};


#include <hpdf.h>
#include <hpdf_conf.h>
#include <hpdf_u3d.h>
#include <hpdf_annotation.h>


bool Create3DPdf(const char *filepdf, const char *fileprc)
{
	typedef struct
	{
		double x, y, z;
	} XYZ;

	float _rcleft(0),
		_rctop(0),
		_rcwidth(600),
		_rcheight(600),
		_bgr(0.2f), // OSG default clear color: (.2, .2, .4)
		_bgg(0.2f),
		_bgb(0.4f);

	HPDF_Rect rect = { _rcleft, _rctop, _rcwidth, _rcheight };

	HPDF_Doc pdf = HPDF_New(NULL, NULL);
	if (!pdf)
	{
		printf("error: cannot create PdfDoc object\n");
		return false;
	}

	HPDF_Page page = HPDF_AddPage(pdf);

	HPDF_Page_SetWidth(page, _rcwidth);
	HPDF_Page_SetHeight(page, _rcheight);

	HPDF_U3D u3d = HPDF_LoadU3DFromFile(pdf, fileprc);

#define NS2VIEWS 7
	HPDF_Dict views[NS2VIEWS + 1];
	const char *view_names[] = {
		"Front perspective ('1','H')",
		"Back perspective ('2')",
		"Right perspective ('3')",
		"Left perspective ('4')",
		"Bottom perspective ('5')",
		"Top perspective ('6')",
		"Oblique perspective ('7')" };

	const float view_c2c[][3] = { { 0., 0., 1. },
	{ 0., 0., -1. },
	{ -1., 0., 0. },
	{ 1., 0., 0. },
	{ 0., 1., 0. },
	{ 0., -1., 0. },
	{ -1., 1., -1. } };

	const float view_roll[] = { 0., 180., 90., -90., 0., 0., 60. };

	// query camera point to rotate about - sometimes not the same as focus point
	XYZ pr;
	pr.x = 0;
	pr.y = 0;
	pr.z = 0;

	// camera focus point
	XYZ focus;
	focus.x = 0;
	focus.y = 0;
	focus.z = 0;

	float camrot = 5.0;

	// create views
	for (int iv = 0; iv < NS2VIEWS; iv++)
	{
		views[iv] = HPDF_Create3DView(u3d->mmgr, view_names[iv]);
		HPDF_3DView_SetCamera(views[iv], 0., 0., 0.,
			view_c2c[iv][0], view_c2c[iv][1], view_c2c[iv][2],
			camrot, view_roll[iv]);
		HPDF_3DView_SetPerspectiveProjection(views[iv], 45.0);
		HPDF_3DView_SetBackgroundColor(views[iv], _bgr, _bgg, _bgb);
		HPDF_3DView_SetLighting(views[iv], "White");

		HPDF_U3D_Add3DView(u3d, views[iv]);
	}

	// add a psuedo-orthographic for slicing (actually perspective with point at infinity)
	views[NS2VIEWS] = HPDF_Create3DView(u3d->mmgr, "Orthgraphic slicing view");
	HPDF_3DView_SetCamera(views[NS2VIEWS], 0., 0., 0.,
		view_c2c[0][0], view_c2c[0][1], view_c2c[0][2],
		camrot*82.70f, view_roll[0]);
	HPDF_3DView_SetPerspectiveProjection(views[NS2VIEWS], 0.3333f);
	//HPDF_3DView_SetOrthogonalProjection(views[NS2VIEWS], 45.0/1000.0);
	HPDF_3DView_SetBackgroundColor(views[NS2VIEWS], _bgr, _bgg, _bgb);
	HPDF_3DView_SetLighting(views[NS2VIEWS], "White");
	HPDF_U3D_Add3DView(u3d, views[NS2VIEWS]);


	HPDF_U3D_SetDefault3DView(u3d, "Front perspective");

	//  Create annotation
	//annot = HPDF_Page_Create3DAnnot(page, rect, HPDF_TRUE, HPDF_FALSE, u3d, NULL);
	HPDF_Annotation annot = HPDF_Page_Create3DAnnot(page, rect, u3d);
	// make the toolbar appear by default

	// HPDF_Dict action = (HPDF_Dict)HPDF_Dict_GetItem (annot, "3DA", HPDF_OCLASS_DICT);
	// HPDF_Dict_AddBoolean (action, "TB", HPDF_TRUE);

	// save the document to a file 
	HPDF_SaveToFile(pdf, filepdf);

	// clean up
	HPDF_Free(pdf);

	return true;
}

void main(void)
{
	std::ofstream ostream("D:\\dev\\libs\\libPRC\\Debug\\test.prc", std::ios_base::out | std::ios_base::binary);

	{
		PRCWriter prcWriter(ostream);
		Box box;
		prcWriter.addMesh(box);
	}

	ostream.close();

	Create3DPdf("D:\\dev\\libs\\libPRC\\Debug\\test.pdf", "D:\\dev\\libs\\libPRC\\Debug\\test.prc");
}

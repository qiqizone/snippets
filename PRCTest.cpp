
#include "libPRC\oPRCFile.h"

//http://fossies.org/dox/mathgl-2.3.3/oPRCFile_8cc_source.html


#include <TopoDS.hxx>
#include <TopoDS_Solid.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>

#include <TopExp_Explorer.hxx>
#include <Poly_Polygon3D.hxx>
#include <BRepTools.hxx>

#include <STEPControl_Reader.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
#include <BRep_Builder.hxx>
#include <Poly_Triangulation.hxx>
#include <Precision.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepGProp_Face.hxx>
#include <BOPTools_AlgoTools3D.hxx>

#include <TColgp_Array1OfPnt2d.hxx>
#include <TColStd_HArray1OfReal.hxx>

#include <Poly_PolygonOnTriangulation.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <GCPnts_TangentialDeflection.hxx>
#include <TColStd_HSequenceOfTransient.hxx>
#include <Interface_InterfaceModel.hxx>

#include <TDF_Label.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <TCollection_ExtendedString.hxx>
#include <TDataStd_Name.hxx>
#include <TDF_ChildIterator.hxx>
#include <Quantity_Color.hxx>
#include <TDocStd_Document.hxx>
#include <STEPCAFControl_Reader.hxx>
#include <TDF_Tool.hxx>
#include <XCAFDoc_Location.hxx>

#include <functional>
#include < map >

class IBRep
{
public:
	virtual int AddVertex(const gp_Pnt& pnt) = 0;
	virtual int AddNormal(const gp_Dir& dir) = 0;
	virtual void AddFace(const std::vector<int>& indices) = 0;
	virtual void SetFaceColor(int color) = 0;
	virtual void AddEdge(const std::vector<int>& indices) = 0;
	virtual void AddInstance(const gp_Trsf& transform) = 0;

	virtual void Flush() = 0;
};

class PRCShape : public IBRep
{
	oPRCFile& _prcFile;

	uint32_t addMaterial(int rgba)
	{
		Quantity_Color color;
		Quantity_Color::Argb2color(rgba, color);

		double r = color.Red();
		double g = color.Green();
		double b = color.Blue();

		RGBAColour ambient(r / 3, g / 3, b / 3);
		RGBAColour diffuse(r, g, b);
		RGBAColour emission(r / 3, g / 3, b / 3);
		RGBAColour specular(0, 0, 0, 0);
		double alpha = 1;
		double shiness = 1;

		PRCmaterial m(ambient, diffuse, emission, specular, alpha, shiness);

		const uint32_t style = _prcFile.addMaterial(m);
		return style;
	}

	std::vector<gp_Pnt> _points;
	std::vector<gp_Dir> _normals;

	std::vector<int>* _indices; //vi, ni, vi, ni, vi, ni
	std::map<int, std::vector<int>> _faces;
	std::vector<gp_Trsf> _instances;
	std::vector<std::vector<int>> _edges;
public:

	virtual int AddVertex(const gp_Pnt& pnt)
	{
		_points.push_back(pnt);
		return _points.size() - 1;
	}

	virtual int AddNormal(const gp_Dir& dir)
	{
		_normals.push_back(dir);
		return _normals.size() - 1;
	}

	virtual void AddFace(const std::vector<int>& indices)
	{
		_indices->insert(_indices->end(), indices.cbegin(), indices.cend());
	}

	virtual void SetFaceColor(int color)
	{
		_indices = &_faces[color];
	}

	virtual void AddEdge(const std::vector<int>& indices)
	{
		_edges.push_back(indices);
	}

	virtual void AddInstance(const gp_Trsf& transform)
	{
		_instances.push_back(transform);
	}

	virtual void Flush()
	{
		uint32_t nP = _points.size();
		auto P = new double[nP][3];

		for (uint32_t i = 0; i < nP; i++)
		{
			auto v = _points[i];
			P[i][0] = v.X();
			P[i][1] = v.Y();
			P[i][2] = v.Z();
		}

		uint32_t nN = _normals.size();
		auto N = new double[nN][3];

		for (uint32_t i = 0; i < nN; i++)
		{
			auto n = _normals[i];

			N[i][0] = n.X();
			N[i][1] = n.Y();
			N[i][2] = n.Z();
		}

		for (auto it = _faces.cbegin(); it != _faces.cend(); it++)
		{
			const auto& indices = it->second;

			uint32_t nI = indices.size() / 6;
			auto PI = new uint32_t[nI][3];
			auto NI = new uint32_t[nI][3];

			for (uint32_t i = 0; i < nI; i++)
			{
				int ii = i * 6;
				PI[i][0] = indices[ii + 0];
				NI[i][0] = indices[ii + 1];

				PI[i][1] = indices[ii + 2];
				NI[i][1] = indices[ii + 3];

				PI[i][2] = indices[ii + 4];
				NI[i][2] = indices[ii + 5];
			}

			auto styleIndex = addMaterial(it->first);

			const uint32_t tess_index = _prcFile.createTriangleMesh(
				nP,
				P,
				nI,
				PI,
				styleIndex,
				nN,
				N,
				NI,
				0, NULL, NULL, 0, NULL, NULL, 0, NULL, NULL, 25.8419 /* arccos(0.9)*/);

			for (auto transform : _instances)
			{
				double m[16];
				m[0] = transform.Value(1, 1); m[4] = transform.Value(1, 2); m[8] = transform.Value(1, 3);  m[12] = transform.Value(1, 4);
				m[1] = transform.Value(2, 1); m[5] = transform.Value(2, 2); m[9] = transform.Value(2, 3);  m[13] = transform.Value(2, 4);
				m[2] = transform.Value(3, 1); m[6] = transform.Value(3, 2); m[10] = transform.Value(3, 3); m[14] = transform.Value(3, 4);
				m[3] = 0;                     m[7] = 0;                     m[11] = 0;                     m[15] = 1;

				_prcFile.begingroup("face", NULL, m);
				_prcFile.useMesh(tess_index, styleIndex);
				_prcFile.endgroup();
			}
		}

		for (auto transform : _instances)
		{
			double m[16];
			m[0] = transform.Value(1, 1); m[4] = transform.Value(1, 2); m[8] = transform.Value(1, 3);  m[12] = transform.Value(1, 4);
			m[1] = transform.Value(2, 1); m[5] = transform.Value(2, 2); m[9] = transform.Value(2, 3);  m[13] = transform.Value(2, 4);
			m[2] = transform.Value(3, 1); m[6] = transform.Value(3, 2); m[10] = transform.Value(3, 3); m[14] = transform.Value(3, 4);
			m[3] = 0;                     m[7] = 0;                     m[11] = 0;                     m[15] = 1;

			_prcFile.begingroup("edges", NULL, m);

			for (auto edges : _edges)
			{
				uint32_t nP = edges.size() - 1;
				auto P = new double[nP][3];

				for (uint32_t i = 0; i < nP; i++)
				{
					P[i][0] = _points[edges[i]].X();
					P[i][1] = _points[edges[i]].Y();
					P[i][2] = _points[edges[i]].Z();
				}

				_prcFile.addLine(nP, P, RGBAColour(0, 0, 0));
			}

			_prcFile.endgroup();
		}
	}

public:
	PRCShape(oPRCFile& prcFile) :
		_prcFile(prcFile),
		_indices(NULL)
	{

	}
};


#include <hpdf.h>
#include <hpdf_conf.h>
#include <hpdf_u3d.h>
#include <hpdf_annotation.h>


bool Create3DPdf(const char *filepdf, const char *fileprc)
{
	float _rcleft(0), _rctop(0), _rcwidth(600), _rcheight(600), _bgr(1), _bgg(1), _bgb(1);

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


	HPDF_U3D_SetDefault3DView(u3d, "Default");

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

/////////////////////


class Step2Prc
{
public:
	static double s_fDeflection;
	static double s_fDeflAngle;

	static std::vector<IBRep*> ImportFromStepFile(const std::wstring fileName, std::function<IBRep*()> createBrep);
	static std::vector<IBRep*> ImportFromLabel(const TDF_Label& shapeLabel, std::function<IBRep*()> createBrep);

private:

	static void importRecursive(
		Handle_XCAFDoc_ShapeTool shapeTool,
		Handle_XCAFDoc_ColorTool colorTool,
		const TDF_Label& label,
		const TopLoc_Location& location,
		int color,
		bool isRef,
		std::set<int>& refShapes,
		std::vector<std::tuple<TDF_Label, TopLoc_Location, int>>& shapes);

	static bool GetColor(Handle_XCAFDoc_ColorTool colorTool, const TopoDS_Shape& shape, int& color);
	static void iterateTopo(IBRep& brep, Handle_XCAFDoc_ColorTool colorTool, const TopoDS_Shape& shape, int color);

	static bool addFace(IBRep& brep, const TopoDS_Face& aFace);
	static bool addEdge(IBRep& brep, const TopoDS_Edge& aEdge);
};

double Step2Prc::s_fDeflection = 0.1;
double Step2Prc::s_fDeflAngle = 0.5;

std::vector<IBRep*> Step2Prc::ImportFromStepFile(const std::wstring fileName, std::function<IBRep*()> createBrep)
{
	Handle_TDocStd_Document doc = new TDocStd_Document("MDTV-CAF");

	STEPCAFControl_Reader reader;
	reader.SetColorMode(true);
	reader.SetNameMode(true);

	std::vector<IBRep*> result;

	if (reader.ReadFile(std::string(fileName.begin(), fileName.end()).c_str()) != IFSelect_RetDone)
		return result; //Empty

	if (reader.Transfer(doc) == Standard_False)
		return result; //Empty

	result = Step2Prc::ImportFromLabel(doc->Main(), createBrep);

	doc->Main().ForgetAllAttributes();	//http://www.opencascade.org/org/forum/thread_13874/?forum=3
	doc.Nullify();

	return result;
}

std::vector<IBRep*> Step2Prc::ImportFromLabel(const TDF_Label& shapeLabel, std::function<IBRep*()> createBrep)
{
	Handle_XCAFDoc_ShapeTool shapeTool = XCAFDoc_DocumentTool::ShapeTool(shapeLabel);
	Handle_XCAFDoc_ColorTool colorTool = XCAFDoc_DocumentTool::ColorTool(shapeLabel);

	auto breps = std::vector<IBRep*>();

	std::vector<std::tuple<TDF_Label, TopLoc_Location, int>> shapes;

	Step2Prc::importRecursive(
		shapeTool,
		colorTool,
		shapeLabel,
		TopLoc_Location(),
		-1,
		false,
		std::set<int>(),
		shapes);

	cout << "\n-----------------------------------------------------------------\n ";

	std::map<int, int> brep_map;

	for (size_t i = 0; i < shapes.size(); i++)
	{
		const TDF_Label& L = std::get<0>(shapes[i]);

		int color = std::get<2>(shapes[i]);

		const TopoDS_Shape& shape = shapeTool->GetShape(L);

		if (shape.IsNull())
			continue;

		int hash = shape.HashCode(INT_MAX);

		IBRep* pBrep = nullptr;

		if (brep_map.find(hash) != brep_map.end())
			pBrep = breps[brep_map[hash]];

		if (pBrep == NULL)
		{
			pBrep = createBrep();

			//const TopoDS_Shape& shape = shapeTool->GetShape(L);
			GetColor(colorTool, shape, color);

			iterateTopo(*pBrep, colorTool, shape, color);

			brep_map.insert(std::make_pair(hash, breps.size()));
			breps.push_back(pBrep);
		}

		TCollection_AsciiString entry;
		TDF_Tool::Entry(L, entry);
		cout << "\n\t\t" << entry << " -> Instance";

		gp_Trsf trs = std::get<1>(shapes[i]).Transformation();

		pBrep->AddInstance(trs);
	}

	return breps;
}

void Step2Prc::importRecursive(
	Handle_XCAFDoc_ShapeTool shapeTool,
	Handle_XCAFDoc_ColorTool colorTool,
	const TDF_Label& label,
	const TopLoc_Location& location,
	int color,
	bool isRef,
	std::set<int>& refShapes,
	std::vector<std::tuple<TDF_Label, TopLoc_Location, int>>& shapes)
{
	TopoDS_Shape aShape;
	shapeTool->GetShape(label, aShape);

	TopLoc_Location part_loc = location;
	Handle_XCAFDoc_Location hLoc;
	if (label.FindAttribute(XCAFDoc_Location::GetID(), hLoc))
	{
		if (isRef)
			part_loc = part_loc * hLoc->Get();
		else
			part_loc = hLoc->Get();
	}

	GetColor(colorTool, aShape, color);

	TDF_Label ref;
	if (shapeTool->IsReference(label) && shapeTool->GetReferredShape(label, ref))
	{
		importRecursive(shapeTool, colorTool, ref, part_loc, color, true, refShapes, shapes);
	}

	int hash = aShape.HashCode(INT_MAX);
	if (isRef || refShapes.find(hash) == refShapes.end())
	{
		if (shapeTool->IsSimpleShape(label) && (isRef || shapeTool->IsFree(label)))
		{
			if (isRef)
				shapes.push_back(std::make_tuple(label, location, color));
			else
				shapes.push_back(std::make_tuple(label, part_loc, color));
		}
		else
		{
			for (TDF_ChildIterator it(label); it.More(); it.Next())
			{
				importRecursive(shapeTool, colorTool, it.Value(), part_loc, color, isRef, refShapes, shapes);
			}
		}
	}
}

bool Step2Prc::GetColor(Handle_XCAFDoc_ColorTool colorTool, const TopoDS_Shape& shape, int& color)
{
	Quantity_Color aColor;
	if (colorTool->GetColor(shape, XCAFDoc_ColorSurf, aColor) ||
		colorTool->GetColor(shape, XCAFDoc_ColorCurv, aColor) ||
		colorTool->GetColor(shape, XCAFDoc_ColorGen, aColor))
	{
		Quantity_Color::Color2argb(aColor, color);
		return true;
	}
	else
	{
		return false;
	}
}

void Step2Prc::iterateTopo(IBRep& brep, Handle_XCAFDoc_ColorTool colorTool, const TopoDS_Shape& shape, int color)
{
	GetColor(colorTool, shape, color);

	for (TopoDS_Iterator theIterator(shape); theIterator.More(); theIterator.Next())
	{
		const TopoDS_Shape& child = theIterator.Value();

		if (child.IsNull())
			continue;

		if (child.ShapeType() == TopAbs_FACE)
		{
			const TopoDS_Face& face = TopoDS::Face(child);
			GetColor(colorTool, face, color);

			brep.SetFaceColor(color);

			if (face.IsNull() == Standard_False)
				addFace(brep, face);

			for (TopExp_Explorer edgeExp(child, TopAbs_EDGE); edgeExp.More(); edgeExp.Next())
			{
				const TopoDS_Edge& edge = TopoDS::Edge(edgeExp.Current());

				if (edge.IsNull() == Standard_False)
					addEdge(brep, edge);
			}
		}
		else
			iterateTopo(brep, colorTool, child, color);
	}
}

bool Step2Prc::addFace(IBRep& brep, const TopoDS_Face& aFace)
{
	TopLoc_Location aLoc;
	Handle(Poly_Triangulation) aTri = BRep_Tool::Triangulation(aFace, aLoc);

	if (aTri.IsNull())
	{
		//Triangulate the face by the standard OCC mesher
		BRepMesh_IncrementalMesh IM(aFace, s_fDeflection, Standard_False, s_fDeflAngle);
		IM.Perform();
		TopoDS_Face meshFace = TopoDS::Face(IM.Shape());
		aTri = BRep_Tool::Triangulation(meshFace, aLoc);
	}

	if (aTri.IsNull() == Standard_False)
	{
		// const Standard_Integer nNodes(aTri->NbNodes());
		const TColgp_Array1OfPnt&    arrPolyNodes = aTri->Nodes();
		const Poly_Array1OfTriangle& arrTriangles = aTri->Triangles();
		const TColgp_Array1OfPnt2d&  arrUvnodes = aTri->UVNodes();

		Handle(Geom_Surface) aSurface = BRep_Tool::Surface(aFace, aLoc);

		TopAbs_Orientation orientation = aFace.Orientation();

		auto indexes = std::vector<int>();

		for (Standard_Integer i = arrTriangles.Lower(); i <= arrTriangles.Upper(); i++)
		{
			int ia = 0, ib = 0, ic = 0;
			const Poly_Triangle& tri = arrTriangles(i);

			if (orientation == TopAbs_REVERSED)
				tri.Get(ia, ic, ib);
			else
				tri.Get(ia, ib, ic);

			gp_Pnt va, vb, vc;
			va = arrPolyNodes(ia);
			va.Transform(aLoc.Transformation());
			vb = arrPolyNodes(ib);
			vb.Transform(aLoc.Transformation());
			vc = arrPolyNodes(ic);
			vc.Transform(aLoc.Transformation());

			gp_Dir na, nb, nc;

			if (BOPTools_AlgoTools3D::GetNormalToSurface(aSurface, arrUvnodes(ia).X(), arrUvnodes(ia).Y(), na) &&
				BOPTools_AlgoTools3D::GetNormalToSurface(aSurface, arrUvnodes(ib).X(), arrUvnodes(ib).Y(), nb) &&
				BOPTools_AlgoTools3D::GetNormalToSurface(aSurface, arrUvnodes(ic).X(), arrUvnodes(ic).Y(), nc))
			{
				na.Transform(aLoc.Transformation());
				nb.Transform(aLoc.Transformation());
				nc.Transform(aLoc.Transformation());

				//if(orientation == TopAbs_REVERSED)
				//{
				//	na = -na;
				//	nb = -nb;
				//	nc = -nc;
				//}


				indexes.push_back(brep.AddVertex(va));
				indexes.push_back(brep.AddNormal(na));

				indexes.push_back(brep.AddVertex(vb));
				indexes.push_back(brep.AddNormal(nb));

				indexes.push_back(brep.AddVertex(vc));
				indexes.push_back(brep.AddNormal(nc));
			}
			else
			{
				;//System::Console::WriteLine("BOPTools_Tools3D::GetNormalToSurface returns false!!");
			}
		}

		brep.AddFace(indexes);

		return true;
	}

	return false;
}

bool Step2Prc::addEdge(IBRep& brep, const TopoDS_Edge& aEdge)
{
	TopLoc_Location aLoc;
	Handle_Poly_Polygon3D aPol = BRep_Tool::Polygon3D(aEdge, aLoc);
	Standard_Boolean isTessellate(Standard_False);

	if (aPol.IsNull())
		isTessellate = Standard_True;
	// Check the existing deflection
	else if (aPol->Deflection() > s_fDeflection + Precision::Confusion() && BRep_Tool::IsGeometric(aEdge))
		isTessellate = Standard_True;

	if (isTessellate && BRep_Tool::IsGeometric(aEdge))
	{
		//try to find PolygonOnTriangulation
		Handle(Poly_PolygonOnTriangulation) aPT;
		Handle(Poly_Triangulation) aT;
		TopLoc_Location aL;

		Standard_Boolean found = Standard_False;
		for (int i = 1; Standard_True; i++)
		{
			BRep_Tool::PolygonOnTriangulation(aEdge, aPT, aT, aL, i);

			if (aPT.IsNull() || aT.IsNull())
				break;

			if (aPT->Deflection() <= s_fDeflection + Precision::Confusion() && aPT->HasParameters())
			{
				found = Standard_True;
				break;
			}
		}

		if (found)
		{
			BRepAdaptor_Curve aCurve(aEdge);
			Handle(TColStd_HArray1OfReal) aPrs = aPT->Parameters();
			Standard_Integer nbNodes = aPT->NbNodes();
			TColgp_Array1OfPnt arrNodes(1, nbNodes);
			TColStd_Array1OfReal arrUVNodes(1, nbNodes);

			for (int i = 1; i <= nbNodes; i++)
			{
				arrUVNodes(i) = aPrs->Value(aPrs->Lower() + i - 1);
				arrNodes(i) = aCurve.Value(arrUVNodes(i));
			}
			aPol = new Poly_Polygon3D(arrNodes, arrUVNodes);
			aPol->Deflection(aPT->Deflection());
		}
		else
		{

			BRepAdaptor_Curve aCurve(aEdge);
			const Standard_Real aFirst = aCurve.FirstParameter();
			const Standard_Real aLast = aCurve.LastParameter();

			GCPnts_TangentialDeflection TD(aCurve, aFirst, aLast, s_fDeflAngle, s_fDeflection, 2);
			const Standard_Integer nbNodes = TD.NbPoints();

			TColgp_Array1OfPnt arrNodes(1, nbNodes);
			TColStd_Array1OfReal arrUVNodes(1, nbNodes);
			for (int i = 1; i <= nbNodes; i++)
			{
				arrNodes(i) = TD.Value(i);
				arrUVNodes(i) = TD.Parameter(i);
			}
			aPol = new Poly_Polygon3D(arrNodes, arrUVNodes);
			aPol->Deflection(s_fDeflection);
		}

		BRep_Builder aBld;
		aBld.UpdateEdge(aEdge, aPol);
	}

	const Standard_Integer nNodes(aPol->NbNodes());
	const TColgp_Array1OfPnt& arrPolyNodes = aPol->Nodes();

	auto indexes = std::vector<int>();

	for (int i = 1; i <= nNodes; i++)
	{
		gp_Pnt p = arrPolyNodes(i);
		p.Transform(aLoc.Transformation());
		indexes.push_back(brep.AddVertex(p));
	}

	indexes.push_back(-1);
	brep.AddEdge(indexes);

	return true;
}


void main(void)
{
	std::ofstream ostream("D:\\dev\\libs\\libPRC\\Debug\\test.prc", std::ios_base::out | std::ios_base::binary);

	oPRCFile prcFile(ostream);

	auto step = L"D:\\dev\\libs\\libPRC\\Debug\\test.stp";

	{
		auto result = Step2Prc::ImportFromStepFile(step, [&prcFile] { return new PRCShape(prcFile); });

		for (auto it : result)
			it->Flush();
	}

	prcFile.finish();
	ostream.close();

	Create3DPdf("D:\\dev\\libs\\libPRC\\Debug\\test.pdf", "D:\\dev\\libs\\libPRC\\Debug\\test.prc");
}


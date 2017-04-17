using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using UnityEngine;

public class DynamicMesh : MonoBehaviour
{
    // Use this for initialization

    public GameObject EmptyNode;
    void Start()
    {
        BRep[] breps;

        //C:\Users\Public\Documents\Unity Projects\Test
        using (var stream = System.IO.File.OpenRead("8920980000.stp.brep"))
            //using (var decompressed = new System.IO.Compression.GZipStream(stream, System.IO.Compression.CompressionMode.Decompress))
            breps = BRep.Deserialize(stream);

        var colors = new List<Color>();
        var vertices = new List<Vector3>();
        var normals = new List<Vector3>();
        var vertexMap = new Dictionary<KeyValuePair<Vector3, Vector3>, int>();
        var edges = new List<int>();

        foreach (var brep in breps)
        {
            var indices = new Dictionary<int, List<int>>();

            for (int i = 0; i < brep.FaceIndexes.Count; i += 2)
            {
                var fi = brep.FaceIndexes[i];
                var ni = brep.FaceIndexes[i + 1];

                Vector3 v, n;
                brep.GetVertex(fi, out v.x, out v.y, out v.z);
                brep.GetNormal(ni, out n.x, out n.y, out n.z);

                var key = new KeyValuePair<Vector3, Vector3>(v, n);

                int ii = -1;

                if (vertexMap.TryGetValue(key, out ii) == false)
                {
                    vertices.Add(v);
                    normals.Add(n);
                    ii = vertices.Count - 1;

                    vertexMap.Add(key, ii);

                    var key2 = new KeyValuePair<Vector3, Vector3>(v, Vector3.zero);

                    if (vertexMap.ContainsKey(key2) == false)
                        vertexMap.Add(key2, ii);
                }

                var material = brep.GetFaceColor(i);

                List<int> faces = null;
                if (indices.TryGetValue(material, out faces) == false)
                {
                    faces = new List<int>();
                    indices.Add(material, faces);
                }

                faces.Add(ii);
            }

            if (true)
            {
                edges.Clear();

                var edgeIndexes = brep.EdgeIndexes;
                for (int j = 0; j < edgeIndexes.Count - 1; j++)
                {
                    if (edgeIndexes[j] != -1 && edgeIndexes[j + 1] != -1)
                    {
                        var a = new Vector3();
                        var b = new Vector3();
                        brep.GetVertex(edgeIndexes[j], out a.x, out a.y, out a.z);
                        brep.GetVertex(edgeIndexes[j + 1], out b.x, out b.y, out b.z);

                        int ii;
                        var key = new KeyValuePair<Vector3, Vector3>(a, Vector3.zero);
                        vertexMap.TryGetValue(key, out ii);

                        edges.Add(ii);

                        key = new KeyValuePair<Vector3, Vector3>(b, Vector3.zero);
                        vertexMap.TryGetValue(key, out ii);

                        edges.Add(ii);
                    }
                }

                if (edges.Any())
                {
                    var mesh = new Mesh();

                    mesh.SetVertices(vertices);
                    mesh.SetNormals(normals);

                    var obj = Instantiate<GameObject>(EmptyNode);
                    obj.transform.parent = this.transform;
                    var meshFilter = obj.AddComponent<MeshFilter>();
                    var renderer = obj.AddComponent<MeshRenderer>();
                    meshFilter.mesh = mesh;
                    renderer.material.color = new Color(0, 0, 0, 0);

                    var m = brep.GetInstance(0);

                    obj.transform.position = ExtractTranslationFromMatrix(m);
                    obj.transform.rotation = ExtractRotationFromMatrix(m);

                    mesh.SetIndices(edges.ToArray(), MeshTopology.Lines, 0);
                }
            }

            //subMeshes.Add(new KeyValuePair<MeshTopology, List<int>>(MeshTopology.Triangles, edge));

            foreach (var face in indices)
            {
                var mesh = new Mesh();

                mesh.SetVertices(vertices);
                mesh.SetNormals(normals);

                var obj = Instantiate<GameObject>(EmptyNode);
                obj.transform.parent = this.transform;
                var meshFilter = obj.AddComponent<MeshFilter>();
                var renderer = obj.AddComponent<MeshRenderer>();
                meshFilter.mesh = mesh;

                var m = brep.GetInstance(0);

                obj.transform.position = ExtractTranslationFromMatrix(m);
                obj.transform.rotation = ExtractRotationFromMatrix(m);

                var bytes = System.BitConverter.GetBytes(face.Key);
                renderer.material.color = new Color(bytes[2] / 255.0f, bytes[1] / 255.0f, bytes[0] / 255.0f, 0);

                mesh.SetIndices(face.Value.ToArray(), MeshTopology.Triangles, 0);
            }
        }
    }

    void Update()
    {
        this.transform.Rotate(new Vector3(0, 1, 0), Mathf.Deg2Rad * 90);
    }

    public static Vector3 ExtractTranslationFromMatrix(float[] m)
    {
        var matrix = new Matrix4x4()
        {
            m00 = m[0],
            m01 = m[4],
            m02 = m[8],
            m03 = m[12],
            m10 = m[1],
            m11 = m[5],
            m12 = m[9],
            m13 = m[13],
            m20 = m[2],
            m21 = m[6],
            m22 = m[10],
            m23 = m[14],
            m30 = m[3],
            m31 = m[7],
            m32 = m[11],
            m33 = m[15]
        };

        Vector3 translate;
        translate.x = matrix.m03;
        translate.y = matrix.m13;
        translate.z = matrix.m23;
        return translate;
    }

    /// <summary>
    /// Extract rotation quaternion from transform matrix.
    /// </summary>
    /// <param name="matrix">Transform matrix. This parameter is passed by reference
    /// to improve performance; no changes will be made to it.</param>
    /// <returns>
    /// Quaternion representation of rotation transform.
    /// </returns>
    public static Quaternion ExtractRotationFromMatrix(float[] m)
    {
        //var matrix = new Matrix4x4()
        //{
        //    m00 = m[0], m01 = m[1], m02 = m[2], m03 = m[3],
        //    m10 = m[4], m11 = m[5], m12 = m[6], m13 = m[7],
        //    m20 = m[8], m21 = m[9], m22 = m[10], m23 = m[11],
        //    m30 = m[12], m31 = m[13], m32 = m[14], m33 = m[15]

        //};
        var matrix = new Matrix4x4()
        {
            m00 = m[0],
            m01 = m[4],
            m02 = m[8],
            m03 = m[12],
            m10 = m[1],
            m11 = m[5],
            m12 = m[9],
            m13 = m[13],
            m20 = m[2],
            m21 = m[6],
            m22 = m[10],
            m23 = m[14],
            m30 = m[3],
            m31 = m[7],
            m32 = m[11],
            m33 = m[15]
        };

        Vector3 forward;
        forward.x = matrix.m02;
        forward.y = matrix.m12;
        forward.z = matrix.m22;

        Vector3 upwards;
        upwards.x = matrix.m01;
        upwards.y = matrix.m11;
        upwards.z = matrix.m21;

        return Quaternion.LookRotation(forward, upwards);
    }
}

public sealed class BRep
{
    private const int CURRENT_VERSION = 4;

    private List<float> _instances = new List<float>();     //16 flaots => Matrix
    private List<float> _vertices = new List<float>();      //x,y,z => Vector3
    private List<float> _normals = new List<float>();       //x,y,z => Vector3
    private List<int> _edges = new List<int>();           //vi,vi,vi,-1... => Line
    private List<int> _faces = new List<int>();           //vi,ni,vi,ni,vi,ni... => Triangle
    private List<int> _faceColors = new List<int>();      //startFaceIndex, Color....
    private List<string> _names = new List<string>();       //name of instance

    public void AddInstance(float[] instance, string name = "")
    {
        if (instance == null || instance.Length != 16)
            throw new ArgumentException(System.Reflection.MethodBase.GetCurrentMethod().Name, "instance");

        this._instances.AddRange(instance);
        this._names.Add(name);
    }

    private class Tuple<T1, T2, T3>
    {
        public Tuple(T1 t1, T2 t2, T3 t3)
        {
            Item1 = t1;
            Item2 = t2;
            Item3 = t3;
        }

        T1 Item1;
        T2 Item2;
        T3 Item3;
    }

    private readonly Dictionary<Tuple<int, int, int>, int> _hashesV = new Dictionary<Tuple<int, int, int>, int>();

    public int AddVertex(float positionX, float positionY, float positionZ)
    {
        var tuple = new Tuple<int, int, int>((int)(positionX * 100), (int)(positionY * 100), (int)(positionZ * 100));
        int index = -1;
        if (_hashesV.TryGetValue(tuple, out index))
            return index;

        this._vertices.Add(positionX);
        this._vertices.Add(positionY);
        this._vertices.Add(positionZ);

        _hashesV.Add(tuple, this.VertexCount - 1);

        return this.VertexCount - 1;
    }

    private readonly Dictionary<Tuple<float, float, float>, int> _hashesN = new Dictionary<Tuple<float, float, float>, int>();

    public int AddNormal(float normalX, float normalY, float normalZ)
    {
        var tuple = new Tuple<float, float, float>(normalX, normalY, normalZ * 100);
        int index = -1;
        if (_hashesN.TryGetValue(tuple, out index))
            return index;

        this._normals.Add(normalX);
        this._normals.Add(normalY);
        this._normals.Add(normalZ);

        _hashesN.Add(tuple, this.NormalCount - 1);

        return this.NormalCount - 1;
    }

    public void AddFace(IList<int> indexes)
    {
        if (indexes == null)
            throw new ArgumentNullException("indexes");

        if (indexes.Count % 6 != 0)   //müssen immer 6 Indizes sein!!!
            throw new ArgumentException("indexes not modulo 6");

        for (int i = 0; i < indexes.Count - 1; i += 2)  //Verifizierung ob Indices auch im Bereich der Vertices und Normalen
        {
            if (indexes[i] < 0 || indexes[i] >= this.VertexCount || indexes[i + 1] >= this.NormalCount)
                throw new ArgumentOutOfRangeException("indexes");
        }

        this._faces.AddRange(indexes);
    }

    public void AddEdge(IList<int> indexes)
    {
        if (indexes == null)
            throw new ArgumentNullException("indexes");

        foreach (int t in indexes)
        {
            if (t != -1 && t >= this.VertexCount)
                throw new ArgumentOutOfRangeException("indexes");
        }

        this._edges.AddRange(indexes);

    }

    public void GetVertex(int index, out float positionX, out float positionY, out float positionZ)
    {
        if (index >= this.VertexCount)
            throw new ArgumentOutOfRangeException("index");

        positionX = this._vertices[index * 3];
        positionY = this._vertices[(index * 3) + 1];
        positionZ = this._vertices[(index * 3) + 2];
    }

    public void GetNormal(int index, out float normalX, out float normalY, out float normalZ)
    {
        if (index >= this.NormalCount)
            throw new ArgumentOutOfRangeException("index");

        normalX = this._normals[index * 3];
        normalY = this._normals[(index * 3) + 1];
        normalZ = this._normals[(index * 3) + 2];
    }

    public float[] GetInstance(int index)
    {
        if (index >= this.InstanceCount)
            throw new ArgumentOutOfRangeException("index");

        return this._instances.GetRange(index * 16, 16).ToArray();
    }

    public string GetInstanceName(int index)
    {
        if (index >= this.InstanceCount)
            throw new ArgumentOutOfRangeException("index");

        return this._names[index];
    }

    public void SetFaceColor(int color)
    {
        if (_faceColors.Count == 0 && this._faces.Count > 0)
            throw new OperationCanceledException(); //Color must be set, before Faces added

        if (_faceColors.Any() == false || _faceColors.Last() != color)
        {
            _faceColors.Add(this._faces.Count);
            _faceColors.Add(color);
        }
    }

    public int GetFaceColor(int index)
    {
        if (index > this._faces.Count)
            throw new ArgumentOutOfRangeException("index");

        for (int i = 0; i < _faceColors.Count; i += 2)
        {
            var startIndex = _faceColors[i];
            if (index < startIndex)
                return _faceColors[i - 1];
        }

        if (_faceColors.Any())
            return _faceColors.Last();

        return -1;
    }

    public ReadOnlyCollection<int> EdgeIndexes
    {
        get { return new ReadOnlyCollection<int>(this._edges); }
    }

    public ReadOnlyCollection<int> FaceIndexes
    {
        get { return new ReadOnlyCollection<int>(this._faces); }
    }

    public int InstanceCount
    {
        get { return this._instances.Count / 16; }
    }

    public int VertexCount
    {
        get { return this._vertices.Count / 3; }
    }

    public int NormalCount
    {
        get { return this._normals.Count / 3; }
    }

    private static float[] Transform(float[] matrix, float[] position)
    {
        var vector = new float[3];
        vector[0] = (((position[0] * matrix[0]) + (position[1] * matrix[4])) + (position[2] * matrix[8])) + matrix[12];
        vector[1] = (((position[0] * matrix[1]) + (position[1] * matrix[5])) + (position[2] * matrix[9])) + matrix[13];
        vector[2] = (((position[0] * matrix[2]) + (position[1] * matrix[6])) + (position[2] * matrix[10])) + matrix[14];
        return vector;
    }

    private static void Min(ref float[] result, float[] b)
    {
        if (result == null)
        {
            result = new float[] { b[0], b[1], b[2] };
        }
        else
        {
            result[0] = Math.Min(result[0], b[0]);
            result[1] = Math.Min(result[1], b[1]);
            result[2] = Math.Min(result[2], b[2]);
        }
    }

    private static void Max(ref float[] result, float[] b)
    {
        if (result == null)
            result = new float[] { b[0], b[1], b[2] };
        else
        {
            result[0] = Math.Max(result[0], b[0]);
            result[1] = Math.Max(result[1], b[1]);
            result[2] = Math.Max(result[2], b[2]);
        }
    }

    public static void Serialize(Stream compressedStream, params BRep[] boundaryRepresentations)
    {
        if (boundaryRepresentations == null || boundaryRepresentations.Count((brep) => brep == null) > 0)
            throw new ArgumentException(System.Reflection.MethodBase.GetCurrentMethod().Name, "boundaryRepresentations");

        using (var writer = new BinaryWriter(compressedStream))
        {
            writer.Write(CURRENT_VERSION);  //Version

            float[] boundingMin = { float.MaxValue, float.MaxValue, float.MaxValue };
            float[] boundingMax = { float.MinValue, float.MinValue, float.MinValue };

            foreach (var brep in boundaryRepresentations)
            {
                float[] min = { float.MaxValue, float.MaxValue, float.MaxValue };
                float[] max = { float.MinValue, float.MinValue, float.MinValue };
                var v = new float[3];

                for (int i = 0; i < brep.VertexCount; i++)
                {
                    brep.GetVertex(i, out v[0], out v[1], out v[2]);
                    Min(ref min, v);
                    Max(ref max, v);
                }

                for (int i = 0; i < brep.InstanceCount; i++)
                {
                    var m = brep.GetInstance(i);
                    var a = Transform(m, min);
                    var b = Transform(m, max);

                    Min(ref boundingMin, a);
                    Min(ref boundingMin, b);
                    Max(ref boundingMax, a);
                    Max(ref boundingMax, b);
                }
            }

            writer.WriteFloatList(boundingMin.ToList());                    //BoundingMin
            writer.WriteFloatList(boundingMax.ToList());                    //BoundingMax

            writer.Write(boundaryRepresentations.Length);                   //Anzahl der Breps
            foreach (var brep in boundaryRepresentations)
            {
                writer.WriteFloatList(brep._instances);
                writer.WriteStringList(brep._names);
                writer.WriteFloatList(brep._vertices);
                writer.WriteFloatList(brep._normals);
                writer.WriteInt32List(brep._edges);
                writer.WriteInt32List(brep._faces);
                writer.WriteInt32List(brep._faceColors);
            }
        }
    }

    public static bool GetBounding(Stream decompressedStream, out float[] boundingMin, out float[] boundingMax)
    {
        using (var reader = new BinaryReader(decompressedStream))
        {
            int version = reader.ReadInt32();
            if (version == CURRENT_VERSION)
            {
                List<float> lboundingMin;
                List<float> lboundingMax;

                reader.ReadFloatList(out lboundingMin);
                reader.ReadFloatList(out lboundingMax);

                boundingMin = lboundingMin.ToArray();
                boundingMax = lboundingMax.ToArray();

                return true;
            }
            else
            {
                throw new NotImplementedException();
            }
        }
    }

    public static BRep[] Deserialize(Stream decompressedStream)
    {
        using (var reader = new BinaryReader(decompressedStream))
        {
            BRep[] breps = null;

            int version = reader.ReadInt32();
            if (version == CURRENT_VERSION)
            {
                List<float> boundingMin;
                List<float> boundingMax;

                reader.ReadFloatList(out boundingMin);
                reader.ReadFloatList(out boundingMax);

                breps = new BRep[reader.ReadInt32()];

                for (int i = 0; i < breps.Length; i++)
                {
                    BRep brep = breps[i] = new BRep();

                    reader.ReadFloatList(out brep._instances);

                    reader.ReadStringList(out brep._names);
                    reader.ReadFloatList(out brep._vertices);
                    reader.ReadFloatList(out brep._normals);
                    reader.ReadInt32List(out brep._edges);
                    reader.ReadInt32List(out brep._faces);
                    reader.ReadInt32List(out brep._faceColors);
                }
            }

            return breps;
        }
    }

    public static string ComputeHash(BRep[] boundaryRepresentations, bool ignoreColor)
    {
        if (boundaryRepresentations == null)
            throw new ArgumentNullException("boundaryRepresentations");

        if (ignoreColor)
        {
            foreach (var brep in boundaryRepresentations)
            {
                brep._faceColors = new int[] { 0, 0 }.ToList();
            }
        }

        using (var stream = new MemoryStream())
        {
            Serialize(stream, boundaryRepresentations);
            return GetMD5HashFromBuffer(stream.GetBuffer());
        }
    }

    private static string GetMD5HashFromBuffer(byte[] buffer)
    {
        using (var md5 = new System.Security.Cryptography.MD5CryptoServiceProvider())
        {
            byte[] retVal = md5.ComputeHash(buffer);

            var sb = new System.Text.StringBuilder();
            foreach (byte t in retVal)
            {
                sb.Append(t.ToString("x2", System.Globalization.CultureInfo.InvariantCulture));
            }
            return sb.ToString();
        }
    }
}

internal static class BinaryReaderWriterExt
{
    public static void WriteFloatList(this BinaryWriter writer, List<float> list)
    {
        writer.Write(list.Count);
        foreach (var f in list)
            writer.Write(f);
    }

    public static void WriteInt32List(this BinaryWriter writer, List<int> list)
    {
        writer.Write(list.Count);
        foreach (var i in list)
            writer.Write(i);
    }

    public static void WriteStringList(this BinaryWriter writer, List<string> list)
    {
        writer.Write(list.Count);
        foreach (var i in list)
            writer.Write(i);
    }

    public static void ReadFloatList(this BinaryReader reader, out List<float> list)
    {
        list = new List<float>(reader.ReadInt32());
        for (int i = 0; i < list.Capacity; i++)
            list.Add(reader.ReadSingle());
    }

    public static void ReadInt32List(this BinaryReader reader, out List<int> list)
    {
        list = new List<int>(reader.ReadInt32());
        for (int i = 0; i < list.Capacity; i++)
            list.Add(reader.ReadInt32());
    }

    public static void ReadStringList(this BinaryReader reader, out List<string> list)
    {
        list = new List<string>(reader.ReadInt32());
        for (int i = 0; i < list.Capacity; i++)
            list.Add(reader.ReadString());
    }
}

using System;
using System.Collections.Generic;
using System.Linq;

namespace eh
{
    public static class BrepToDAE
    {
        public static DAE.geometry CreateMeshes(BRep brep)
        {
            var mesh_items = new List<object>();
            var mesh_source = new List<DAE.source>();

            var coords = new List<double>();

            for (var i = 0; i < brep.VertexCount; i++)
            {
                float x, y, z;
                brep.GetVertex(i, out x, out y, out z);
                coords.Add(x);
                coords.Add(y);
                coords.Add(z);
            }

            mesh_source.Add(new DAE.source
            {
                id = "positions0",
                Item = new DAE.float_array() { id = "float-array-positions0", count = (ulong)brep.VertexCount * 3, _Text_ = string.Join(" ", coords) },
                technique_common = new DAE.sourceTechnique_common()
                {
                    accessor = new DAE.accessor()
                    {
                        count = (ulong)brep.VertexCount,
                        stride = 3,
                        source = "#float-array-positions0",
                        param = new[]
                        {
                                        new DAE.param() { name = "X", type = "float" },
                                        new DAE.param() { name = "Y", type = "float" },
                                        new DAE.param() { name = "Z", type = "float" }
                                    }
                    }
                },
            });

            var normals = new List<double>();

            for (var i = 0; i < brep.NormalCount; i++)
            {
                float x, y, z;
                brep.GetNormal(i, out x, out y, out z);
                normals.Add(x);
                normals.Add(y);
                normals.Add(z);
            }

            mesh_source.Add(new DAE.source
            {
                id = "normals0",
                Item = new DAE.float_array() { id = "float-array-normals0", count = (ulong)brep.NormalCount * 3, _Text_ = string.Join(" ", normals) },
                technique_common = new DAE.sourceTechnique_common()
                {
                    accessor = new DAE.accessor()
                    {
                        count = (ulong)brep.NormalCount,
                        stride = 3,
                        source = "#float-array-normals0",
                        param = new[]
                        {
                                new DAE.param() { name = "X", type = "float" },
                                new DAE.param() { name = "Y", type = "float" },
                                new DAE.param() { name = "Z", type = "float" }
                            }
                    }
                }
            });

            {
                var triangleIndices = new List<int>();

                var color = brep.GetFaceColor(0);
                var faceIndexes = brep.FaceIndexes;
                for (int j = 0; j < faceIndexes.Count; j += 2)
                {
                    if (j > 0 && (j % 6 == 0))
                    {
                        var faceColor = brep.GetFaceColor(j);
                        if (faceColor != color)
                        {

                            mesh_items.Add(new DAE.triangles()
                            {
                                count = (ulong)triangleIndices.Count / 2 / 3,
                                material = "materialRef" + color,
                                input = new[]
                                {
                                new DAE.InputLocalOffset() { offset = 1, semantic = "NORMAL", source = "#normals0" },
                                new DAE.InputLocalOffset() { offset = 0, semantic = "VERTEX", source = "#vertices0" }
                            },
                                p = string.Join(" ", triangleIndices)
                            });

                            triangleIndices = new List<int>();
                            color = faceColor;
                        }
                    }

                    triangleIndices.Add(faceIndexes[j]);
                    triangleIndices.Add(faceIndexes[j + 1]);
                }
            }

            {
                var lines = new List<int>();

                var edgeIndexes = brep.EdgeIndexes;
                for (int j = 0; j < edgeIndexes.Count - 1; j++)
                {
                    if (edgeIndexes[j] != -1 && edgeIndexes[j + 1] != -1)
                    {
                        lines.Add(edgeIndexes[j]);
                        lines.Add(edgeIndexes[j + 1]);
                    }
                }

                mesh_items.Add(new DAE.lines()
                {
                    count = (ulong)lines.Count / 2,
                    material = "materialRef0",
                    input = new[]
                    {
                        new DAE.InputLocalOffset() { offset = 0, semantic = "VERTEX", source = "#vertices0" }
                    },
                    p = string.Join(" ", lines)
                });
            }

            {
                mesh_items.Add(new DAE.linestrips()
                {
                    material = "materialRef0",
                    input = new[]
                    {
                        new DAE.InputLocalOffset() { offset = 0, semantic = "VERTEX", source = "#vertices0" }
                    },
                    p = new[] { string.Join(" ", brep.EdgeIndexes) },
                    count = 1,
                });
            }

            return new DAE.geometry()
            {
                id = "geometry" + Guid.NewGuid(),
                name = "geometry" + Guid.NewGuid(),
                Item = new DAE.mesh()
                {
                    source = mesh_source.ToArray(),
                    vertices = new DAE.vertices()
                    {
                        id = "vertices0",
                        input = new[]
                        {
                                new DAE.InputLocal() { semantic = "POSITION", source = "#positions0" }
                        }
                    },
                    Items = mesh_items.ToArray()
                }
            };
        }

        public static void SaveAsDAE(this BRep[] breps, System.IO.Stream daeStream)
        {
            var geometries = new DAE.library_geometries();

            var effectList = new List<DAE.effect>();
            var materials = new List<DAE.material>();

            for (int j = 0; j < breps.Length; j++)
            {
                var brep = breps[j];

                foreach (var color in ForInt(brep.FaceIndexes.Count).Select(brep.GetFaceColor).Concat(new[] { 0 }).Distinct())
                {
                    if (effectList.Any(e => e.id == "effect" + color.ToString()))
                        continue;

                    var bytes = BitConverter.GetBytes(color);
                    var rgba = new[] { bytes[2] / 255.0, bytes[1] / 255.0, bytes[0] / 255.0, 0 };

                    effectList.Add(new DAE.effect()
                    {
                        id = "effect" + color.ToString(),
                        name = "effect" + color.ToString(),
                        Items = new[]
                        {
                                new DAE.effectFx_profile_abstractProfile_COMMON()
                                {
                                    technique = new DAE.effectFx_profile_abstractProfile_COMMONTechnique()
                                    {
                                         sid = "common",
                                         Item = new DAE.effectFx_profile_abstractProfile_COMMONTechniquePhong()
                                         {
                                              diffuse = new DAE.common_color_or_texture_type()
                                              {
                                                  Item = new DAE.common_color_or_texture_typeColor()
                                                  {
                                                      Values = rgba
                                                  }
                                              }
                                         }
                                    }
                                }
                           }
                    });

                    materials.Add(new DAE.material()
                    {
                        id = "material" + color.ToString(),
                        name = "material" + color.ToString(),
                        instance_effect = new DAE.instance_effect() { url = "#effect" + color.ToString(), }
                    });
                }
            }

            geometries.geometry = breps.Where(b => b.VertexCount > 0).Select(b => CreateMeshes(b)).ToArray();

            var scenes = new DAE.library_visual_scenes()
            {
                visual_scene = new[]
                {
                    new DAE.visual_scene()
                    {
                        id = "myscene",
                        node = new []
                        {
                            new DAE.node()
                            {
                                id="node0",
                                name="node0",
                                instance_geometry = geometries.geometry.Select(g =>

                                    new DAE.instance_geometry()
                                    {
                                        url ="#" + g.id,
                                        bind_material = new DAE.bind_material()
                                        {
                                            technique_common = ((DAE.mesh)g.Item).Items.OfType<DAE.triangles>().Select(t =>
                                            {
                                                return new DAE.instance_material() { symbol = t.material, target="#" + t.material.Replace("materialRef", "material") };
                                            }).ToArray()
                                        }
                                    }
                                ).ToArray()
                            }
                        }
                    }
               }
            };

            var dae = new DAE.COLLADA()
            {
                Items = new object[]
                {
                    new DAE.library_effects() { effect = effectList.ToArray() },
                    geometries,
                    new DAE.library_materials() { material = materials.ToArray() },
                    scenes
                },
                scene = new DAE.COLLADAScene()
                {
                    instance_visual_scene = new DAE.InstanceWithExtra() { url = "#myscene" }
                }
            };

            dae.Save(daeStream);
        }
        public static IEnumerable<int> ForInt(int max)
        {
            for (int i = 0; i < max; i++)
                yield return i;
        }
    }
}

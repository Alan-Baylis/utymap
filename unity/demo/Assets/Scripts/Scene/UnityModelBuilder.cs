﻿using System.Collections.Generic;
using Assets.Scripts.Scene.Builders;
using UnityEngine;
using UtyDepend;
using UtyMap.Unity;
using Mesh = UtyMap.Unity.Mesh;

namespace Assets.Scripts.Scene
{
    /// <summary> Responsible for building Unity game objects from meshes and elements. </summary>
    internal class UnityModelBuilder
    {
        private readonly MaterialProvider _materialProvider;

        private Dictionary<string, IElementBuilder> _elementBuilders = new Dictionary<string, IElementBuilder>();

        [Dependency]
        public UnityModelBuilder(MaterialProvider materialProvider)
        {
            _materialProvider = materialProvider;

            // register custom builders here.
            _elementBuilders.Add("info", new PlaceElementBuilder(_materialProvider));
            _elementBuilders.Add("label", new LabelElementBuilder());
        }

        /// <inheritdoc />
        public void BuildElement(Tile tile, Element element)
        {
            foreach (var pair in _elementBuilders)
            {
                if (element.Styles["builders"].Contains(pair.Key))
                    pair.Value.Build(tile, element);
            }
        }

        /// <inheritdoc />
        public void BuildMesh(Tile tile, Mesh mesh)
        {
            var gameObject = new GameObject(mesh.Name);

            var uMesh = new UnityEngine.Mesh();
            uMesh.vertices = mesh.Vertices;
            uMesh.triangles = mesh.Triangles;
            uMesh.colors = mesh.Colors;
            uMesh.uv = mesh.Uvs;
            uMesh.uv2 = mesh.Uvs2;
            uMesh.uv3 = mesh.Uvs3;

            uMesh.RecalculateNormals();

            gameObject.isStatic = true;
            gameObject.AddComponent<MeshFilter>().mesh = uMesh;
            gameObject.AddComponent<MeshRenderer>().sharedMaterial =
                _materialProvider.GetSharedMaterial(@"Materials/Default");
            gameObject.AddComponent<MeshCollider>();
            gameObject.transform.parent = tile.GameObject.transform;
        }
    }
}
﻿using System;
using System.Collections.Generic;
using System.Linq;
using UnityEngine;
using UtyMap.Unity;
using UtyMap.Unity.Data;
using UtyMap.Unity.Infrastructure.Primitives;
using UtyMap.Unity.Utils;

namespace Assets.Scenes.Default.Scripts.Tiling
{
    /// <summary>  </summary>
    internal sealed class SurfaceTileController : TileController
    {
        private readonly float _minHeight;
        private readonly float _maxHeight;
        private readonly float _scale;

        private readonly Vector3 _origin = Vector3.zero;

        private float _zoom;
        private Vector3 _position;
        private GeoCoordinate _geoOrigin;
        private Dictionary<QuadKey, Tile> _loadedQuadKeys = new Dictionary<QuadKey, Tile>();

        public SurfaceTileController(IMapDataStore dataStore, Stylesheet stylesheet,
            ElevationDataType elevationType, Range<int> lodRange, GeoCoordinate origin,
            float cameraAspect, float scale, float maxDistance) :
            base(dataStore, stylesheet, elevationType, lodRange)
        {
            _scale = scale;
            _geoOrigin = origin;

            LodTree = GetLodTree(cameraAspect, maxDistance);
            Projection = CreateProjection();

            _maxHeight = LodTree.Max;
            _minHeight = LodTree.Min;
        }

        /// <inheritdoc />
        public override float FieldOfView { get; protected set; }

        /// <inheritdoc />
        public override IProjection Projection { get; protected set; }

        /// <inheritdoc />
        public override float ZoomLevel { get { return _zoom; } }

        /// <inheritdoc />
        public override GeoCoordinate Coordinate { get { return GeoUtils.ToGeoCoordinate(_geoOrigin, _position); } }

        /// <inheritdoc />
        public override bool IsAboveMax { get { return _maxHeight < GetDistanceToOrigin(); } }

        /// <inheritdoc />
        public override bool IsBelowMin { get { return _minHeight > GetDistanceToOrigin(); } }

        /// <inheritdoc />
        public override void MoveOrigin(Vector3 position)
        {
            _geoOrigin = GeoUtils.ToGeoCoordinate(_geoOrigin, new Vector2(position.x, position.z) / _scale);
            Projection = CreateProjection();
        }

        /// <inheritdoc />
        public override void OnUpdate(Transform planet, Vector3 position, Vector3 rotation)
        {
            _position = position;
            Build(planet);
        }

        /// <inheritdoc />
        public override void Dispose()
        {
            foreach (var tile in _loadedQuadKeys.Values)
                tile.Dispose();

            Resources.UnloadUnusedAssets();
        }

        #region Tile processing

        /// <summary> Gets distance to origin. </summary>
        private float GetDistanceToOrigin()
        {
            return Vector3.Distance(_position, _origin);
        }

        /// <summary> Builds quadkeys if necessary. Decision is based on current position and lod level. </summary>
        private void Build(Transform parent)
        {
            var oldLod = (int) _zoom;
            var currentLod = LodTree[_position.y].First().Value;

            var currentQuadKey = GetQuadKey(currentLod);

            // zoom in/out
            if (oldLod != currentLod)
            {
                foreach (var tile in _loadedQuadKeys.Values)
                    tile.Dispose();

                Resources.UnloadUnusedAssets();
                _loadedQuadKeys.Clear();

                foreach (var quadKey in GetNeighbours(currentQuadKey))
                    BuildQuadKey(parent, quadKey);
            }
            // pan
            else
            {
                var quadKeys = new HashSet<QuadKey>(GetNeighbours(currentQuadKey));
                var newlyLoadedQuadKeys = new Dictionary<QuadKey, Tile>();

                foreach (var quadKey in quadKeys)
                    newlyLoadedQuadKeys.Add(quadKey, _loadedQuadKeys.ContainsKey(quadKey)
                        ? _loadedQuadKeys[quadKey]
                        : BuildQuadKey(parent, quadKey));

                foreach (var quadKeyPair in _loadedQuadKeys)
                    if (!quadKeys.Contains(quadKeyPair.Key))
                        quadKeyPair.Value.Dispose();

                Resources.UnloadUnusedAssets();
                _loadedQuadKeys = newlyLoadedQuadKeys;
            }
        }

        /// <summary> Get tiles surrounding given. </summary>
        private IEnumerable<QuadKey> GetNeighbours(QuadKey quadKey)
        {
            yield return new QuadKey(quadKey.TileX, quadKey.TileY, quadKey.LevelOfDetail);

            yield return new QuadKey(quadKey.TileX - 1, quadKey.TileY, quadKey.LevelOfDetail);
            yield return new QuadKey(quadKey.TileX - 1, quadKey.TileY + 1, quadKey.LevelOfDetail);
            yield return new QuadKey(quadKey.TileX, quadKey.TileY + 1, quadKey.LevelOfDetail);
            yield return new QuadKey(quadKey.TileX + 1, quadKey.TileY + 1, quadKey.LevelOfDetail);
            yield return new QuadKey(quadKey.TileX + 1, quadKey.TileY, quadKey.LevelOfDetail);
            yield return new QuadKey(quadKey.TileX + 1, quadKey.TileY - 1, quadKey.LevelOfDetail);
            yield return new QuadKey(quadKey.TileX, quadKey.TileY - 1, quadKey.LevelOfDetail);
            yield return new QuadKey(quadKey.TileX - 1, quadKey.TileY - 1, quadKey.LevelOfDetail);
        }

        /// <summary> Build specific quadkey. </summary>
        private Tile BuildQuadKey(Transform parent, QuadKey quadKey)
        {
            var tileGameObject = new GameObject(quadKey.ToString());
            tileGameObject.transform.parent = parent.transform;
            var tile = CreateTile(quadKey, tileGameObject);
            _loadedQuadKeys.Add(quadKey, tile);
            LoadTile(tile);
            return tile;
        }

        /// <summary> Gets quadkey for position. </summary>
        private QuadKey GetQuadKey(int lod)
        {
            var currentPosition = GeoUtils.ToGeoCoordinate(_geoOrigin, new Vector2(_position.x, _position.z) / _scale);
            return GeoUtils.CreateQuadKey(currentPosition, lod);
        }

        #endregion

        #region Lod calculations

        /// <summary> Gets range (interval) tree with LODs </summary>
        private RangeTree<float, int> GetLodTree(float cameraAspect, float maxDistance)
        {
            const float sizeRatio = 0.75f;
            var tree = new RangeTree<float, int>();

            var aspectRatio = sizeRatio * (Screen.height < Screen.width ? 1 / cameraAspect : 1);

            var fov = GetFieldOfView(GeoUtils.CreateQuadKey(_geoOrigin, LodRange.Minimum), maxDistance, aspectRatio);

            tree.Add(maxDistance, float.MaxValue, LodRange.Minimum);
            for (int lod = LodRange.Minimum + 1; lod <= LodRange.Maximum; ++lod)
            {
                var frustumHeight = GetFrustumHeight(GeoUtils.CreateQuadKey(_geoOrigin, lod), aspectRatio);
                var distance = frustumHeight * 0.5f / Mathf.Tan(fov * 0.5f * Mathf.Deg2Rad);
                tree.Add(distance, maxDistance, lod - 1);
                maxDistance = distance;
            }
            tree.Add(float.MinValue, maxDistance, LodRange.Maximum);

            FieldOfView = fov;

            return tree;
        }

        /// <summary> Gets height of camera's frustum. </summary>
        private float GetFrustumHeight(QuadKey quadKey, float aspectRatio)
        {
            return GetGridSize(quadKey) * aspectRatio;
        }

        /// <summary> Gets field of view for given quadkey and distance. </summary>
        private float GetFieldOfView(QuadKey quadKey, float distance, float aspectRatio)
        {
            return 2.0f * Mathf.Rad2Deg * Mathf.Atan(GetFrustumHeight(quadKey, aspectRatio) * 0.5f / distance);
        }

        /// <summary> Get side size of grid consists of 9 quadkeys. </summary>
        private float GetGridSize(QuadKey quadKey)
        {
            var bbox = GeoUtils.QuadKeyToBoundingBox(quadKey);
            var bboxWidth = bbox.MaxPoint.Longitude - bbox.MinPoint.Longitude;
            var minPoint = new GeoCoordinate(bbox.MinPoint.Latitude, bbox.MinPoint.Longitude - bboxWidth);
            var maxPoint = new GeoCoordinate(bbox.MinPoint.Latitude, bbox.MaxPoint.Longitude + bboxWidth);
            return (float)GeoUtils.Distance(minPoint, maxPoint) * _scale;
        }

        #endregion

        /// <summary> Gets projection for current georigin and scale. </summary>
        private IProjection CreateProjection()
        {
            IProjection projection = new CartesianProjection(_geoOrigin);
            return _scale > 0
                ? new ScaledProjection(projection, _scale)
                : projection;
        }

    }
}

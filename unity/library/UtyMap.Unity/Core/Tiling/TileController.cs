﻿using System;
using System.Collections.Generic;
using System.Linq;
using UnityEngine;
using UtyMap.Unity.Core.Models;
using UtyMap.Unity.Core.Utils;
using UtyMap.Unity.Infrastructure;
using UtyMap.Unity.Maps.Data;
using UtyDepend;
using UtyDepend.Config;
using UtyRx;

namespace UtyMap.Unity.Core.Tiling
{
    /// <summary> Controls flow of loading/unloading tiles. </summary>
    /// <summary> Tested for cartesian projection only. </summary>
    public interface ITileController
    {
        /// <summary> Current position on map in world coordinates. </summary>
        Vector3 CurrentMapPoint { get; }

        /// <summary> Current position on map in geo coordinates. </summary>
        GeoCoordinate CurrentPosition { get; }

        /// <summary> Gets current tile. </summary>
        Tile CurrentTile { get; }

        /// <summary> Stylesheet. </summary>
        Stylesheet Stylesheet { get; set; }

        /// <summary> Projection. </summary>
        IProjection Projection { get; set; }

        /// <summary> Called when position is changed. </summary>
        void OnPosition(Vector3 position, int levelOfDetails);

        /// <summary> Called when position is changed. </summary>
        void OnPosition(GeoCoordinate coordinate, int levelOfDetails);
    }

    #region Default implementation

    /// <summary> Default implementation of tile controller. </summary>
    /// <remarks> Not thread safe. </remarks>
    internal class TileController : ITileController, IConfigurable, IDisposable
    {
        private readonly IModelBuilder _modelBuilder;
        private readonly object _lockObj = new object();

        private readonly IMapDataLoader _tileLoader;
        private readonly IMessageBus _messageBus;

        private double _offsetRatio;
        private double _moveSensitivity;
        private int _maxTileDistance;

        private Vector3 _lastUpdatePosition = new Vector3(float.MinValue, float.MinValue, float.MinValue);

        private QuadKey _currentQuadKey;
        private readonly Dictionary<QuadKey, Tile> _loadedTiles = new Dictionary<QuadKey, Tile>();

        #region Public members

        /// <summary> Creates instance of <see cref="TileController"/>. </summary>
        [Dependency]
        public TileController(IModelBuilder modelBuilder, IMapDataLoader tileLoader, IMessageBus messageBus)
        {
            _modelBuilder = modelBuilder;
            _tileLoader = tileLoader;
            _messageBus = messageBus;
        }

        /// <inheritdoc />
        public Vector3 CurrentMapPoint { get; private set; }

        /// <inheritdoc />
        public GeoCoordinate CurrentPosition { get; private set; }

        /// <inheritdoc />
        public Tile CurrentTile { get { return _loadedTiles[_currentQuadKey]; } }

        /// <inheritdoc />
        [Dependency]
        public Stylesheet Stylesheet { get; set; }

        /// <inheritdoc />
        [Dependency]
        public IProjection Projection { get; set; }

        /// <inheritdoc />
        public void OnPosition(Vector3 position, int levelOfDetails)
        {
            OnPosition(Projection.Project(position), position, levelOfDetails);
        }

        /// <inheritdoc />
        public void OnPosition(GeoCoordinate coordinate, int levelOfDetails)
        {
            OnPosition(coordinate, Projection.Project(coordinate, 0), levelOfDetails);
        }

        private void OnPosition(GeoCoordinate geoPosition, Vector3 position, int levelOfDetails)
        {
            if (!IsValidLevelOfDetails(levelOfDetails))
                throw new ArgumentException(String.Format("Invalid level of details: {0}", levelOfDetails), "levelOfDetails");

            CurrentMapPoint = position;
            CurrentPosition = geoPosition;

            // call update logic only if threshold is reached
            if (Vector3.Distance(position, _lastUpdatePosition) > _moveSensitivity)
            {
                lock (_lockObj)
                {
                    _lastUpdatePosition = position;

                    _currentQuadKey = GeoUtils.CreateQuadKey(geoPosition, levelOfDetails);

                    UnloadFarTiles(_currentQuadKey);

                    if (_loadedTiles.ContainsKey(_currentQuadKey))
                    {
                        var tile = _loadedTiles[_currentQuadKey];
                        if (ShouldPreload(tile, position))
                            PreloadNextTile(tile, position);
                        return;
                    }

                    Load(_currentQuadKey);
                }
            }
        }

        /// <inheritdoc />
        public void Configure(IConfigSection configSection)
        {
            _moveSensitivity = configSection.GetFloat("sensitivity", 30);
            _offsetRatio = configSection.GetFloat("offset", 10); // percentage of tile size
            _maxTileDistance = configSection.GetInt("max_tile_distance", 2);
        }

        #endregion

        #region Loading

        /// <summary> Loads tile for given quadKey. </summary>
        private void Load(QuadKey quadKey)
        {
            Tile tile = new Tile(quadKey, Stylesheet, Projection);
            _loadedTiles.Add(quadKey, tile); // TODO remove tile from hashmap if exception is raised
            _messageBus.Send(new TileLoadStartMessage(tile));
            _tileLoader
                .Load(tile)
                .SubscribeOn(Scheduler.ThreadPool)
                .ObserveOn(Scheduler.MainThread)
                .Subscribe(
                    u => u.Match(e => _modelBuilder.BuildElement(tile, e), m => _modelBuilder.BuildMesh(tile, m)),
                    () => _messageBus.Send(new TileLoadFinishMessage(tile)));
        }

        #endregion

        #region Preloading

        private void PreloadNextTile(Tile tile, Vector3 position)
        {
            var quadKey = GetNextQuadKey(tile, position);
            if (!_loadedTiles.ContainsKey(quadKey))
                Load(quadKey);
        }

        private bool ShouldPreload(Tile tile, Vector3 position)
        {
            return !tile.Contains(position, tile.Rectangle.Width * _offsetRatio);
        }

        /// <summary> Gets next quadkey. </summary>
        private QuadKey GetNextQuadKey(Tile tile, Vector3 position)
        {
            var quadKey = tile.QuadKey;
            var position2D = new Vector2(position.x, position.z);

            // NOTE left-right and top-bottom orientation
            Vector2 topLeft = new Vector2(tile.Rectangle.Left, tile.Rectangle.Top);
            Vector2 topRight = new Vector2(tile.Rectangle.Right, tile.Rectangle.Top);

            // top
            if (IsPointInTriangle(position2D, tile.Rectangle.Center, topLeft, topRight))
                return new QuadKey(quadKey.TileX, quadKey.TileY - 1, quadKey.LevelOfDetail);

            Vector2 bottomLeft = new Vector2(tile.Rectangle.Left, tile.Rectangle.Bottom);

            // left
            if (IsPointInTriangle(position2D, tile.Rectangle.Center, topLeft, bottomLeft))
                return new QuadKey(quadKey.TileX - 1, quadKey.TileY, quadKey.LevelOfDetail);

            Vector2 bottomRight = new Vector2(tile.Rectangle.Right, tile.Rectangle.Bottom);

            // right
            if (IsPointInTriangle(position2D, tile.Rectangle.Center, topRight, bottomRight))
                return new QuadKey(quadKey.TileX + 1, quadKey.TileY, quadKey.LevelOfDetail);

            // bottom
            return new QuadKey(quadKey.TileX, quadKey.TileY + 1, quadKey.LevelOfDetail);
        }

        #endregion

        #region Unloading tiles

        /// <summary> Removes far tiles from list of loaded and sends corresponding message. </summary>
        private void UnloadFarTiles(QuadKey currentQuadKey)
        {
            var tiles = _loadedTiles.ToArray();

            foreach (var loadedTile in tiles)
            {
                var quadKey = loadedTile.Key;
                if ((Math.Abs(quadKey.TileX - currentQuadKey.TileX) + 
                     Math.Abs(quadKey.TileY - currentQuadKey.TileY)) <= _maxTileDistance)
                    continue;
                
                loadedTile.Value.Dispose();
                _loadedTiles.Remove(quadKey);
                _messageBus
                    .Send(new TileDestroyMessage(loadedTile.Value));
            }
        }

        #endregion

        /// <summary>
        ///     Checks that passed level of details is greater than zero and it equals last used quadkey's one. 
        /// </summary>
        /// <remarks>
        ///     This class is not designed to support dynamic level of details changes.
        /// </remarks>
        private bool IsValidLevelOfDetails(int levelOfDetails)
        {
            var currentLevelOfDetails = _currentQuadKey.LevelOfDetail == 0
                ? levelOfDetails
                : _currentQuadKey.LevelOfDetail;

            return levelOfDetails > 0 && currentLevelOfDetails == levelOfDetails;
        }

        /// <summary>
        ///     Checks whether point is located in triangle.
        ///     http://stackoverflow.com/questions/13300904/determine-whether-point-lies-inside-triangle
        /// </summary>
        private static bool IsPointInTriangle(Vector2 p, Vector2 p1, Vector2 p2, Vector2 p3)
        {
            var alpha = ((p2.y - p3.y) * (p.x - p3.x) + (p3.x - p2.x) * (p.y - p3.y)) /
                          ((p2.y - p3.y) * (p1.x - p3.x) + (p3.x - p2.x) * (p1.y - p3.y));
            var beta = ((p3.y - p1.y) * (p.x - p3.x) + (p1.x - p3.x) * (p.y - p3.y)) /
                         ((p2.y - p3.y) * (p1.x - p3.x) + (p3.x - p2.x) * (p1.y - p3.y));
            var gamma = 1.0f - alpha - beta;

            return alpha > 0 && beta > 0 && gamma > 0;
        }

        /// <inheritdoc />
        public void Dispose()
        {
            foreach (var loadedTile in _loadedTiles)
                loadedTile.Value.Dispose();
        }
    }

    #endregion
}

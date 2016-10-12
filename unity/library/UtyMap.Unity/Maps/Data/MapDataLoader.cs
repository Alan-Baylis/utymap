﻿using System;
using System.IO;
using UtyMap.Unity.Core;
using UtyMap.Unity.Core.Models;
using UtyMap.Unity.Core.Tiling;
using UtyMap.Unity.Infrastructure.Diagnostic;
using UtyMap.Unity.Infrastructure.IO;
using UtyMap.Unity.Infrastructure.Primitives;
using UtyDepend;
using UtyDepend.Config;
using UtyRx;
using Mesh = UtyMap.Unity.Core.Models.Mesh;

namespace UtyMap.Unity.Maps.Data
{
    /// <summary> Defines behavior of class responsible to mapdata processing. </summary>
    public interface IMapDataLoader
    {
        /// <summary> Adds mapdata to the specific storage. </summary>
        /// <param name="storage"> Storage type. </param>
        /// <param name="dataPath"> Path to mapdata. </param>
        /// <param name="stylesheet"> Stylesheet which to use during import. </param>
        /// <param name="levelOfDetails"> Which level of details to use. </param>
        void AddToStore(MapStorageType storage, string dataPath, Stylesheet stylesheet, Range<int> levelOfDetails);

        /// <summary> Loads given tile. This method triggers real loading and processing osm data. </summary>
        /// <param name="tile">Tile to load.</param>
        IObservable<Union<Element, Mesh>> Load(Tile tile);
    }

    /// <summary> Default implementation of tile loader. </summary>
    internal class MapDataLoader : IMapDataLoader, IConfigurable, IDisposable
    {
        private const string TraceCategory = "mapdata.loader";
        private readonly object _lockObj = new object();

        private readonly IPathResolver _pathResolver;
        private string _mapDataServerUri;
        private string _mapDataServerQuery;
        private string _mapDataFormatExtension;
        private string _cachePath;
        private readonly IFileSystemService _fileSystemService;
        private readonly INetworkService _networkService;

        [Dependency]
        public ITrace Trace { get; set; }

        [Dependency]
        public MapDataLoader(IFileSystemService fileSystemService, INetworkService networkService, IPathResolver pathResolver)
        {
            _fileSystemService = fileSystemService;
            _networkService = networkService;
            _pathResolver = pathResolver;
        }

        /// <inheritdoc />
        public void AddToStore(MapStorageType storageType, string dataPath, Stylesheet stylesheet, Range<int> levelOfDetails)
        {
            var dataPathResolved = _pathResolver.Resolve(dataPath);
            var stylesheetPathResolved = _pathResolver.Resolve(stylesheet.Path);

            Trace.Info(TraceCategory, String.Format("add to {0} storage: data:{1} using style: {2}",
                storageType, dataPathResolved, stylesheetPathResolved));

            string errorMsg = null;
            CoreLibrary.AddToStore(storageType, stylesheetPathResolved, dataPathResolved, levelOfDetails,
                error => errorMsg = error);

            if (errorMsg != null)
                throw new MapDataException(errorMsg);
        }

        /// <inheritdoc />
        public IObservable<Union<Element, Mesh>> Load(Tile tile)
        {
            return CreateDownloadSequence(tile)
                .SelectMany(CreateLoadSequence);
        }

        /// <inheritdoc />
        public void Configure(IConfigSection configSection)
        {
            _mapDataServerUri = configSection.GetString(@"data/remote/server", null);
            _mapDataServerQuery = configSection.GetString(@"data/remote/query", null);
            _mapDataFormatExtension = "." + configSection.GetString(@"data/remote/format", "xml");
            _cachePath = configSection.GetString(@"data/cache", null);

            var stringPath = _pathResolver.Resolve(configSection.GetString("data/index/strings"));
            var mapDataPath = _pathResolver.Resolve(configSection.GetString("data/index/spatial"));
            var elePath = _pathResolver.Resolve(configSection.GetString("data/elevation/local"));

            string errorMsg = null;
            CoreLibrary.Configure(stringPath, mapDataPath, elePath, error => errorMsg = error);
            if (errorMsg != null)
                throw new MapDataException(errorMsg);
        }

        /// <inheritdoc />
        public void Dispose()
        {
            CoreLibrary.Dispose();
        }

        #region Private methods

        /// <summary> Downloads map data for given tile. </summary>
        private IObservable<Tile> CreateDownloadSequence(Tile tile)
        {
            // data exists in store
            if (CoreLibrary.HasData(tile.QuadKey))
                return Observable.Return(tile);

            // data exists in cache
            var filePath = GetCacheFilePath(tile);
            if (_fileSystemService.Exists(filePath))
            {
                var errorMsg = SaveTileDataInMemory(tile, filePath);
                return errorMsg == null
                    ? Observable.Return(tile)
                    : Observable.Throw<Tile>(new MapDataException(Strings.CannotAddDataToInMemoryStore, errorMsg));
            }

            // need to download from remote server
            return Observable.Create<Tile>(observer =>
            {
                double padding = 0.001;
                BoundingBox query = tile.BoundingBox;
                var queryString = String.Format(_mapDataServerQuery,
                    query.MinPoint.Latitude - padding, query.MinPoint.Longitude - padding,
                    query.MaxPoint.Latitude + padding, query.MaxPoint.Longitude + padding);
                var uri = String.Format("{0}{1}", _mapDataServerUri, Uri.EscapeDataString(queryString));
                Trace.Warn(TraceCategory, Strings.NoPresistentElementSourceFound, tile.QuadKey.ToString(), uri);
                _networkService.GetAndGetBytes(uri)
                    .ObserveOn(Scheduler.ThreadPool)
                    .Subscribe(bytes =>
                    {
                        Trace.Debug(TraceCategory, "saving bytes: {0}", bytes.Length.ToString());
                        lock (_lockObj)
                        {
                            if (!_fileSystemService.Exists(filePath))
                                using (var stream = _fileSystemService.WriteStream(filePath))
                                    stream.Write(bytes, 0, bytes.Length);
                        }

                        // try to add in memory store
                        var errorMsg = SaveTileDataInMemory(tile, filePath);
                        if (errorMsg != null)
                            observer.OnError(new MapDataException(String.Format(Strings.CannotAddDataToInMemoryStore, errorMsg)));
                        else
                        {
                            observer.OnNext(tile);
                            observer.OnCompleted();
                        }
                    });

                return Disposable.Empty;
            });
        }

        /// <summary> Creates <see cref="IObservable{T}"/> for loading element of given tile. </summary>
        private IObservable<Union<Element, Mesh>> CreateLoadSequence(Tile tile)
        {
            return Observable.Create<Union<Element, Mesh>>(observer =>
            {
                var stylesheetPathResolved = _pathResolver.Resolve(tile.Stylesheet.Path);

                Trace.Info(TraceCategory, "loading tile: {0} using style: {1}", tile.ToString(), stylesheetPathResolved);
                var adapter = new MapTileAdapter(tile, observer, Trace);

                CoreLibrary.LoadQuadKey(
                    stylesheetPathResolved, 
                    tile.QuadKey,
                    adapter.AdaptMesh,
                    adapter.AdaptElement,
                    adapter.AdaptError);

                Trace.Info(TraceCategory, "tile loaded: {0}", tile.ToString());
                observer.OnCompleted();

                return Disposable.Empty;
            });
        }

        /// <summary> Adds tile data into in memory storage. </summary>
        private string SaveTileDataInMemory(Tile tile, string filePath)
        {
            var filePathResolved = _pathResolver.Resolve(filePath);
            var stylesheetPathResolved = _pathResolver.Resolve(tile.Stylesheet.Path);
            
            Trace.Info(TraceCategory, String.Format("save tile data {0} from {1} using style: {2}", 
                tile, filePathResolved, stylesheetPathResolved));

            string errorMsg = null;
            CoreLibrary.AddToStore(MapStorageType.Persistent,
                stylesheetPathResolved,
                filePathResolved,
                tile.QuadKey,
                error => errorMsg = error);

            return errorMsg;
        }

        /// <summary> Returns cache file name for given tile. </summary>
        private string GetCacheFilePath(Tile tile)
        {
            var cacheFileName = tile.QuadKey + _mapDataFormatExtension;
            return Path.Combine(_cachePath, cacheFileName);
        }

        #endregion
    }
}

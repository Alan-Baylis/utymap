﻿using System;
using System.Collections.Generic;
using UtyDepend;
using UtyDepend.Config;
using UtyMap.Unity.Data.Providers;
using UtyMap.Unity.Infrastructure.Diagnostic;
using UtyMap.Unity.Infrastructure.IO;
using UtyMap.Unity.Infrastructure.Primitives;
using UtyRx;
using Return = UtyRx.Tuple<UtyMap.Unity.Tile, UtyMap.Unity.Infrastructure.Primitives.Union<UtyMap.Unity.Element, UtyMap.Unity.Mesh>>;

namespace UtyMap.Unity.Data
{
    /// <summary> Defines behavior of class responsible of mapdata processing. </summary>
    public interface IMapDataStore : ISubject<Tile, Return>
    {
        /// <summary> Adds mapdata to the specific dataStorage. </summary>
        /// <param name="dataStorage"> Storage type. </param>
        /// <param name="dataPath"> Path to mapdata. </param>
        /// <param name="stylesheet"> Stylesheet which to use during import. </param>
        /// <param name="levelOfDetails"> Which level of details to use. </param>
        /// <returns> Returns progress status. </returns>
        IObservable<int> Add(MapDataStorageType dataStorage, string dataPath, Stylesheet stylesheet, Range<int> levelOfDetails);

        /// <summary> Adds mapdata to the specific dataStorage. </summary>
        /// <param name="dataStorage"> Storage type. </param>
        /// <param name="dataPath"> Path to mapdata. </param>
        /// <param name="stylesheet"> Stylesheet which to use during import. </param>
        /// <param name="quadKey"> QuadKey to add. </param>
        /// <returns> Returns progress status. </returns>
        IObservable<int> Add(MapDataStorageType dataStorage, string dataPath, Stylesheet stylesheet, QuadKey quadKey);
    }

    /// <summary> Default implementation of map data store. </summary>
    internal class MapDataStore : IMapDataStore, IDisposable, IConfigurable
    {
        private readonly object _lockGuard = new object();

        private const string TraceCategory = "mapdata.store";

        private readonly IMapDataProvider _mapDataProvider;
        private readonly MapDataLoader _mapDataLoader;

        private readonly IPathResolver _pathResolver;
        private readonly ITrace _trace;
        
        private MapDataStorageType _mapDataStorageType;

        private readonly List<IObserver<Return>> _observers = new List<IObserver<Return>>();

        [Dependency]
        public MapDataStore(IMapDataProvider mapDataProvider, IPathResolver pathResolver, ITrace trace)
        {
            _mapDataProvider = mapDataProvider;
            _mapDataLoader = new MapDataLoader(pathResolver, trace);
            _pathResolver = pathResolver;
            _trace = trace;

            _mapDataProvider
                .ObserveOn(Scheduler.ThreadPool)
                .Subscribe(value =>
                {
                    // we have map data in store.
                    if (String.IsNullOrEmpty(value.Item2))
                        _mapDataLoader.OnNext(value.Item1);
                    else
                        Add(_mapDataStorageType, value.Item2, value.Item1.Stylesheet, value.Item1.QuadKey)
                            .Subscribe(progress => { }, () => _mapDataLoader.OnNext(value.Item1));
                });

            _mapDataLoader.Subscribe(u => _observers.ForEach(o => o.OnNext(u)));
        }

        #region Interface implementations

        /// <inheritdoc />
        public IObservable<int> Add(MapDataStorageType dataStorageType, string dataPath, Stylesheet stylesheet, Range<int> levelOfDetails)
        {
            return Add(dataStorageType, dataPath, stylesheet, (style, data) =>
            {
                string errorMsg = null;
                CoreLibrary.AddToStore(dataStorageType, style, data, levelOfDetails, error => errorMsg = error);
                return errorMsg;
            });
        }

        /// <inheritdoc />
        public IObservable<int> Add(MapDataStorageType dataStorageType, string dataPath, Stylesheet stylesheet, QuadKey quadKey)
        {
            return Add(dataStorageType, dataPath, stylesheet, (style, data) =>
            {
                string errorMsg = null;
                // NOTE checks data in all registered stores
                // TODO add API to check in specific store.
                if (!CoreLibrary.HasData(quadKey))
                    CoreLibrary.AddToStore(dataStorageType, style, data, quadKey, error => errorMsg = error);
                return errorMsg;
            });
        }

        /// <inheritdoc />
        public virtual void OnCompleted()
        {
            _observers.ForEach(o => o.OnCompleted());
        }

        /// <inheritdoc />
        public virtual void OnError(Exception error)
        {
            _observers.ForEach(o => o.OnError(error));
        }

        /// <inheritdoc />
        public void OnNext(Tile tile)
        {
            _mapDataProvider.OnNext(tile);
        }

        /// <inheritdoc />
        public IDisposable Subscribe(IObserver<Return> observer)
        {
            // TODO handle unsubscribe
            _observers.Add(observer);
            return Disposable.Empty;
        }

        /// <inheritdoc />
        public void Configure(IConfigSection configSection)
        {
            _mapDataLoader.Configure(configSection);

            var stringPath = _pathResolver.Resolve(configSection.GetString("data/index/strings"));
            var mapDataPath = _pathResolver.Resolve(configSection.GetString("data/index/spatial"));
            
            _mapDataStorageType = MapDataStorageType.Persistent;

            string errorMsg = null;
            CoreLibrary.Configure(stringPath, mapDataPath, error => errorMsg = error);
            if (errorMsg != null)
                throw new MapDataException(errorMsg);
        }

        /// <inheritdoc />
        public void Dispose()
        {
            CoreLibrary.Dispose();
        }

        #endregion

        #region Private methods

        private IObservable<int> Add(MapDataStorageType dataStorageType, string dataPath, Stylesheet stylesheet,
            Func<string, string, string> addFunc)
        {
            var dataPathResolved = _pathResolver.Resolve(dataPath);
            var stylePathResolved = _pathResolver.Resolve(stylesheet.Path);

            _trace.Info(TraceCategory, String.Format("add to {0} dataStorage: data:{1} using style: {2}",
                dataStorageType, dataPathResolved, stylePathResolved));

            string errorMsg;
            lock (_lockGuard)
                errorMsg = addFunc(stylePathResolved, dataPathResolved);

            if (errorMsg != null)
                return Observable.Throw<int>(new MapDataException(errorMsg));

            return Observable.Return(100);
        }

        #endregion
    }
}

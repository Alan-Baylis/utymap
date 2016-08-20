﻿using System;
using UnityEngine;
using UtyMap.Unity;
using UtyMap.Unity.Core;
using UtyMap.Unity.Core.Tiling;
using UtyMap.Unity.Infrastructure;
using UtyMap.Unity.Infrastructure.Config;
using UtyMap.Unity.Infrastructure.Diagnostic;
using UtyMap.Unity.Maps.Geocoding;
using UtyRx;
using Component = UtyDepend.Component;

namespace Assets.Scripts
{
    /// <summary> Performs some initialization and listens for position changes of character.  </summary>
    sealed class StreetLevelBehaviour : MonoBehaviour
    {
        private ApplicationManager _appManager;

        // Current character position.
        private Vector3 _position = new Vector3(float.MinValue, float.MinValue, float.MinValue);

        /// <summary>
        ///     Place name. Will be resolved to the certain GeoCoordinate via performing reverse
        ///     geocoding request to reverse geocoding server.
        /// </summary>
        public string PlaceName;

        /// <summary> Start latitude. Used if PlaceName is empty. </summary>
        public double StartLatitude = 52.53149;

        /// <summary> Start longitude. Used if PlaceName is empty. </summary>
        public double StartLongitude = 13.38787;

        #region Unity lifecycle events

        /// <summary> Performs framework initialization once, before any Start() is called. </summary>
        void Awake()
        {
            _appManager = ApplicationManager.Instance;
            _appManager.InitializeFramework(ConfigBuilder.GetDefault(),
                (compositionRoot) =>
                {
                    compositionRoot.RegisterAction((c, _) => 
                        c.Register(UtyDepend.Component.For<IProjection>().Use<CartesianProjection>(GetWorldZeroPoint())));
                });
            _appManager.SetZoomLevel(16);
        }

        void Start()
        {
            // Need to wrap by conditional compilation symbols due to issues with compilation
            // on CI withoud Unity3d: ThirdPersonController is javascript class.
#if !CONSOLE
            // set gravity to zero on start to prevent free fall as terrain loading takes some time.
            // restore it afterwards.
            gameObject.GetComponent<Rigidbody>().isKinematic = true;

            // restore gravity and adjust character y-position once first tile is loaded
            _appManager.GetService<IMessageBus>().AsObservable<TileLoadFinishMessage>()
                .Take(1)
                .ObserveOn(Scheduler.MainThread)
                .Subscribe(_ =>
                {
                    // TODO expose elevation logic from native or use old managed implementation?
                    // NOTE in second case, we will consume additional memory

                    //var position = transform.position;
                    //var elevation = AppManager.GetService<IElevationProvider>()
                    //    .GetElevation(new Vector2(position.x, position.z));
                    //transform.position = new Vector3(position.x, elevation + 90, position.z);
                    gameObject.GetComponent<Rigidbody>().isKinematic = false;
                });
#endif
        }

        /// <summary> Runs game after all Start() methods are called. </summary>
        void OnEnable()
        {
            // utymap is better to start on non-UI thread
            Observable.Start(() => _appManager.RunGame(), Scheduler.ThreadPool).Subscribe();
        }

        /// <summary> Listens for position changes to notify library. </summary>
        void Update()
        {
            if (_appManager.IsInitialized && _position != transform.position)
            {
                _position = transform.position;
                _appManager.SetPosition(new Vector2(_position.x, _position.z));
            }
        }

        #endregion

        #region Private methods

        /// <summary>
        ///     Gets start geocoordinate using desired method: direct latitude/longitude or
        ///     via reverse geocoding request for given place name.
        /// </summary>
        private GeoCoordinate GetWorldZeroPoint()
        {
            var coordinate = new GeoCoordinate(StartLatitude, StartLongitude);
            if (!String.IsNullOrEmpty(PlaceName))
            {
                // NOTE this will freeze UI thread as we're making web request and should wait for its result
                // TODO improve it
                var place = _appManager.GetService<IGeocoder>().Search(PlaceName)
                    .Wait();

                if (place != null)
                {
                    StartLatitude = place.Coordinate.Latitude;
                    StartLongitude = place.Coordinate.Longitude;
                    // NOTE this prevents name resolution to be done more than once
                    PlaceName = null;
                    return place.Coordinate;
                }

                _appManager.GetService<ITrace>()
                    .Warn("init", "Cannot resolve '{0}', will use default latitude/longitude", PlaceName);
            }
            return coordinate;
        }

        #endregion
    }
}

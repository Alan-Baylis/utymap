﻿using UnityEngine;
using Utymap.UnityLib.Core;
using Utymap.UnityLib.Infrastructure.Reactive;

namespace Utymap.UnityLib.Maps.Elevation
{
    /// <summary> Defines behavior of elevation provider. </summary>
    public interface IElevationProvider
    {
        /// <summary> Checks whether elevation data for given bounding box is present in map data. </summary>
        /// <returns>True, if data is already there.</returns>
        bool HasElevation(BoundingBox bbox);

        /// <summary> Download elevation data from server. </summary>
        IObservable<Unit> Download(BoundingBox bbox);

        /// <summary> Gets elevation for given geocoordinate. </summary>
        float GetElevation(GeoCoordinate coordinate);

        /// <summary> Gets elevation for given map point. </summary>
        float GetElevation(Vector2 point);

        /// <summary> Gets elevation for given x and y of map point. </summary>
        float GetElevation(float x, float y);
    }
}

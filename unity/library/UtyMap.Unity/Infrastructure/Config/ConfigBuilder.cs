﻿using UtyDepend.Config;
using UtyMap.Unity.Maps.Data;

namespace UtyMap.Unity.Infrastructure.Config
{
    /// <summary> Represents a builder responsible for application configuration creation. </summary>
    public class ConfigBuilder
    {
        private readonly CodeConfigSection _configSection = new CodeConfigSection();

        /// <summary> Adds value to configuration with path provided. </summary>
        /// <remarks> Use this method to extend application with your custom settings. </remarks>
        /// <typeparam name="T">Type of value.</typeparam>
        /// <param name="path">Path.</param>
        /// <param name="value">Value.</param>
        public void Add<T>(string path, T value)
        {
            _configSection.Add(path, value);
        }

        /// <summary> Builds application specific configuration. </summary>
        public IConfigSection Build()
        {
            return _configSection;
        }

        #region Application specific

        /// <summary> Sets settings to get elevation data from remote server. </summary>
        public ConfigBuilder SetSrtmEleData(string url, string schema)
        {
            Add<string>("data/srtm/server", url);
            Add<string>("data/srtm/schema", schema);
            return this;
        }

        /// <summary> Sets geocoding server's url. </summary>
        public ConfigBuilder SetGeocodingServer(string url)
        {
            Add<string>("geocoding", url);
            return this;
        }

        /// <summary> Sets local path to elevation data. </summary>
        public ConfigBuilder SetLocalElevationData(string path)
        {
            Add<string>("data/elevation/local", path);
            return this;
        }

        /// <summary> Sets string index. </summary>
        public ConfigBuilder SetStringIndex(string path)
        {
            Add<string>("data/index/strings", path);
            return this;
        }

        /// <summary> Sets string index. </summary>
        public ConfigBuilder SetSpatialIndex(string path)
        {
            Add<string>("data/index/spatial", path);
            return this;
        }

        /// <summary> Sets osm map data provider. </summary>
        public ConfigBuilder SetOsmMapData(string url, string schema, string format)
        {
            Add<string>("data/osm/server", url);
            Add<string>("data/osm/query", schema);
            Add<string>("data/osm/format", format);
            return this;
        }

        /// <summary> Sets mapzen map data provider. </summary>
        public ConfigBuilder SetMapzenMapData(string url, string layers, string format, string apiKey)
        {
            Add<string>("data/mapzen/server", url);
            Add<string>("data/mapzen/layers", layers);
            Add<string>("data/mapzen/format", format);
            Add<string>("data/mapzen/apikey", apiKey);
            return this;
        }

        public ConfigBuilder SetMapzenEleData(string url, int gridSize, string format, string apiKey)
        {
            Add<string>("data/mapzen/ele_server", url);
            Add<int>("data/mapzen/ele_grid", gridSize);
            Add<string>("data/mapzen/ele_format", format);
            Add<string>("data/mapzen/api_key", apiKey);
            
            return this;
        }

        /// <summary> Sets cache data path. </summary>
        public ConfigBuilder SetCache(string cache)
        {
            Add<string>("data/cache", cache);
            return this;
        }

        /// <summary> Sets distance (in tile count) between current and tile to be unloaded. </summary>
        public ConfigBuilder SetTileDistance(int distance)
        {
            Add<int>(@"tile/max_tile_distance", distance);
            return this;
        }

        public ConfigBuilder SetElevationType(ElevationDataType type)
        {
            Add<int>(@"data/elevation/type", (int)type);
            return this;
        }

        #endregion

        #region Default instance

        /// <summary> Gets ConfigBuilder with default settings. </summary>
        /// <remarks> You can call methods to override settings with custom ones. </remarks>
        public static ConfigBuilder GetDefault()
        {
            return new ConfigBuilder()
                .SetLocalElevationData("Index/")
                .SetSrtmEleData("http://dds.cr.usgs.gov/srtm/version2_1/SRTM3", "Config/srtm.schema.txt")
                //.SetOsmMapData("http://overpass-api.de/api/interpreter?data=", "(node({0},{1},{2},{3}); <; >;);out body;", "xml")
                .SetOsmMapData("http://api.openstreetmap.org/api/0.6/map?bbox=", "{1},{0},{3},{2}", "xml")
                .SetMapzenMapData("http://tile.mapzen.com/mapzen/vector/v1/{0}/{1}/{2}/{3}.json?api_key={4}", "all", "json", "")
                .SetMapzenEleData("http://elevation.mapzen.com/height?json={0}&api_key={1}", 4, "ele", "")
                .SetCache("Cache")
                .SetGeocodingServer("http://nominatim.openstreetmap.org/search?")
                .SetElevationType(ElevationDataType.Flat);
        }

        #endregion
    }
}

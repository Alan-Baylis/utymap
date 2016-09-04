﻿namespace Assets.Scripts.Environment
{
    internal static class EnvironmentApi
    {
        /// <summary> Returns directory where UtyMap stores files. </summary>
        /// <remarks> Read/write access is required. </remarks>
        public static string ExternalDataPath
        {
            get
            {
#if UNITY_EDITOR
                return @"Assets/StreamingAssets/";
#else
                return UnityEngine.Application.persistentDataPath;
#endif
            }
        }
    }
}

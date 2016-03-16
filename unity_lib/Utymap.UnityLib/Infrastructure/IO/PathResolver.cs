﻿using System.IO;

namespace Utymap.UnityLib.Infrastructure.IO
{
    /// <summary> Default implementation of path resolver simply returns original path. </summary>
    public class PathResolver : IPathResolver
    {
        /// <inheritdoc />
        public string Resolve(string path)
        {
            return path;
        }
    }
}
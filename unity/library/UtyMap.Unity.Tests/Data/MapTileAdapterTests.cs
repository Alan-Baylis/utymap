﻿using System.Collections.Generic;
using Moq;
using NUnit.Framework;
using UtyMap.Unity.Data;
using UtyMap.Unity.Infrastructure.Diagnostic;
using UtyRx;

using Return = UtyRx.Tuple<UtyMap.Unity.Tile, UtyMap.Unity.Infrastructure.Primitives.Union<UtyMap.Unity.Element, UtyMap.Unity.Mesh>>;

namespace UtyMap.Unity.Tests.Data
{
    [TestFixture]
    public class MapTileAdapterTests
    {
        private Mock<IObserver<Return>> _observer;
        private MapDataAdapter _adapter;
            
        [TestFixtureSetUp]
        public void SetUp()
        {
            var tile = new Tile(new QuadKey(), new Mock<Stylesheet>("").Object, new Mock<IProjection>().Object);
            _observer = new Mock<IObserver<Return>>();
            _adapter = new MapDataAdapter(tile, new List<IObserver<Return>>() { _observer.Object }, new DefaultTrace());
        }

        [TestCase("building")]
        public void CanAdaptTheSameNonTerrainMeshOnlyOnce(string name)
        {
            name += ":42";

            for (int i = 0; i < 2; ++i)
                _adapter.AdaptMesh(name, new[] {.0, 0, 0}, 3, new[] {0, 0, 0}, 3, new[] {0, 0, 0}, 3, 
                    new[] {.0, 0, 0, .0, 0, 0}, 6, new int[0], 0);

            _observer.Verify(o => o.OnNext(It.IsAny<Return>()), Times.Once);
        }
    }
}

﻿using Assets.Scripts.Scene.Animations;
using Assets.Scripts.Scene.Gestures;
using Assets.Scripts.Scene.Tiling;
using UnityEngine;
using UtyMap.Unity;

namespace Assets.Scripts.Scene.Spaces
{
    internal sealed class SphereSpace : Space
    {
        /// <inheritdoc />
        public override SpaceAnimator Animator { get; protected set; }

        public SphereSpace(SphereTileController tileController, SphereGestureStrategy gestureStrategy, Transform planet) :
            base(tileController, gestureStrategy, planet)
        {
            Animator = new SphereAnimator(tileController);
        }

        protected override void OnEnter(GeoCoordinate coordinate, bool isFromTop)
        {
            Pivot.rotation = Quaternion.Euler(new Vector3((float)coordinate.Latitude, 270 - (float)coordinate.Longitude, 0));
            Camera.transform.localPosition = new Vector3(0, 0, isFromTop
                ? -TileController.HeightRange.Maximum
                : -TileController.HeightRange.Minimum);
        }

        protected override void OnExit()
        {
        }
    }
}

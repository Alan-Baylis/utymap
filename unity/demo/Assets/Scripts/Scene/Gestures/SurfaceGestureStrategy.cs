﻿using Assets.Scripts.Scene.Tiling;
using TouchScript.Gestures.TransformGestures;
using UnityEngine;

namespace Assets.Scripts.Scene.Gestures
{
    internal class SurfaceGestureStrategy : GestureStrategy
    {
        private float _panSpeed = 1f;
        private float _zoomSpeed = 100f;

        public SurfaceGestureStrategy(TileController tileController, 
                                      ScreenTransformGesture twoFingerMoveGesture,
                                      ScreenTransformGesture manipulationGesture) :
            base(tileController, twoFingerMoveGesture, manipulationGesture)
        {
        }

        public override void OnManipulationTransform(Transform pivot, Transform camera)
        {
            pivot.localPosition += new Vector3(ManipulationGesture.DeltaPosition.x, 0, ManipulationGesture.DeltaPosition.y) * -_panSpeed;
        }

        public override void OnTwoFingerTransform(Transform pivot, Transform camera)
        {
            pivot.localPosition += Vector3.up * (1 -TwoFingerMoveGesture.DeltaScale) * _zoomSpeed;
        }
    }
}

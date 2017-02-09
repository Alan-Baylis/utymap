﻿using System;
using TouchScript.Gestures.TransformGestures;
using UnityEngine;

namespace Assets.Scenes.Surface.Scripts
{
    class SurfaceTouchController : MonoBehaviour
    {
        public ScreenTransformGesture TwoFingerMoveGesture;
        public ScreenTransformGesture ManipulationGesture;

        public float InitialPanSpeed = 50f;
        public float InitialZoomSpeed = 1000f;
        public float InitialRotationSpeed = 1f;
        
        private float _panSpeed;
        private float _zoomSpeed;

        private Transform _pivot;
        private Transform _cam;

        private int _lastLevelOfDetails = -1;

        void Awake()
        {
            _pivot = transform.Find("Pivot");
            _cam = transform.Find("Pivot/Camera");
        }

        void OnEnable()
        {
            TwoFingerMoveGesture.Transformed += twoFingerTransformHandler;
            ManipulationGesture.Transformed += manipulationTransformedHandler;
        }

        void OnDisable()
        {
            TwoFingerMoveGesture.Transformed -= twoFingerTransformHandler;
            ManipulationGesture.Transformed -= manipulationTransformedHandler;
        }

        void Update()
        {
            // update gesture speed based on current LOD.
            if (_lastLevelOfDetails != SurfaceCalculator.CurrentLevelOfDetails)
            {
                _panSpeed = InitialPanSpeed * SurfaceCalculator.GetPanSpeedRatio(SurfaceCalculator.CurrentLevelOfDetails);
                _zoomSpeed = InitialZoomSpeed * SurfaceCalculator.GetZoomSpeedRatio(SurfaceCalculator.CurrentLevelOfDetails);

                _lastLevelOfDetails = SurfaceCalculator.CurrentLevelOfDetails;
            }
        }

        private void manipulationTransformedHandler(object sender, EventArgs e)
        {
            /*var rotation = Quaternion.Euler(
                ManipulationGesture.DeltaPosition.y / Screen.height * InitialRotationSpeed,
                -ManipulationGesture.DeltaPosition.x / Screen.width * InitialRotationSpeed,
                ManipulationGesture.DeltaRotation);
            _pivot.localRotation *= rotation;*/
            _cam.transform.localPosition += Vector3.up * (ManipulationGesture.DeltaScale - 1f) * _zoomSpeed;
        }

        private void twoFingerTransformHandler(object sender, EventArgs e)
        {
            _pivot.localPosition += new Vector3(TwoFingerMoveGesture.DeltaPosition.x, 0, TwoFingerMoveGesture.DeltaPosition.y) * _panSpeed;
        }
    }
}

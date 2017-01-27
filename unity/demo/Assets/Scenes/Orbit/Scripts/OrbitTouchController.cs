﻿using System;
using TouchScript.Gestures.TransformGestures;
using UnityEngine;

namespace Assets.Scenes.Orbit.Scripts
{
    class OrbitTouchController : MonoBehaviour
    {
        public ScreenTransformGesture TwoFingerMoveGesture;
        public ScreenTransformGesture ManipulationGesture;

        public float PanSpeed = 20f;
        public float RotationSpeed = 100f;
        public float ZoomSpeed = 20f;

        private Transform _pivot;
        private Transform _cam;
        private Transform _light;

        private void Awake()
        {
            _pivot = transform.Find("Pivot");
            _cam = transform.Find("Pivot/Camera");
            _light = transform.Find("Directional Light");
        }

        private void OnEnable()
        {
            TwoFingerMoveGesture.Transformed += twoFingerTransformHandler;
            ManipulationGesture.Transformed += manipulationTransformedHandler;
        }

        private void OnDisable()
        {
            TwoFingerMoveGesture.Transformed -= twoFingerTransformHandler;
            ManipulationGesture.Transformed -= manipulationTransformedHandler;
        }

        private void manipulationTransformedHandler(object sender, EventArgs e)
        {
            var rotation = Quaternion.Euler(
                          ManipulationGesture.DeltaPosition.y / Screen.height * RotationSpeed,
                          -ManipulationGesture.DeltaPosition.x / Screen.width * RotationSpeed,
                          ManipulationGesture.DeltaRotation);

            _pivot.localRotation *= rotation;
            // NOTE cannot limit angle: limit values are different for different lods
            // TODO try logic which checks distance to poles?
            //_pivot.localEulerAngles = new Vector3(LimitAngle(_pivot.eulerAngles.x, 45),
            //                                      _pivot.eulerAngles.y,
            //                                      LimitAngle(_pivot.eulerAngles.z, 10));
            _light.localRotation = _pivot.localRotation;
            _cam.transform.localPosition += Vector3.forward * (ManipulationGesture.DeltaScale - 1f) * PanSpeed;
        }

        private void twoFingerTransformHandler(object sender, EventArgs e)
        {
            var rotation = Quaternion.Euler(
              TwoFingerMoveGesture.DeltaPosition.y / Screen.height * RotationSpeed,
               -TwoFingerMoveGesture.DeltaPosition.x / Screen.width * RotationSpeed,
               TwoFingerMoveGesture.DeltaRotation);

            _pivot.localRotation *= rotation;
            _cam.transform.localPosition += Vector3.forward * (TwoFingerMoveGesture.DeltaScale - 1f) * ZoomSpeed;
        }

        private static float LimitAngle(float angle, float limit)
        {
            angle = angle > 180 ? angle - 360 : angle;
            var sign = angle < 0 ? -1 : 1;
            return limit - sign * angle > 0 ? angle : sign * limit;
        }
    }
}

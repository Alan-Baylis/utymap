﻿using System;
using NUnit.Framework;
using Utymap.UnityLib.Infrastructure.Diagnostic;

namespace Utymap.UnityLib.Tests.Infrastructure.Diagnostic
{
    [TestFixture]
    public class DiagnosticTest
    {
        [Description("Dummy test shows that we can use methods without any exceptions")]
        [Test]
        public void CanUseTraceMethods()
        {
            using (var trace = new DefaultTrace())
            {
                trace.Debug("category", "Normal");
                trace.Info("category", "Input");
                trace.Warn("category", "Warn");
                trace.Error("category", new ArgumentException(), "Error");
            }
        }
    }
}
﻿using System.Text;
using Assets.UtymapLib.Infrastructure.Dependencies;
using Assets.UtymapLib.Infrastructure.Dependencies.Interception.Behaviors;
using NUnit.Framework;
using UtymapLib.Tests.Infrastructure.Dependencies.Stubs;

namespace UtymapLib.Tests.Infrastructure.Dependencies
{
    [TestFixture]
    public class InterceptionTests
    {
        [Test]
        public void CanInterceptWithSingleton()
        {
            // ARRANGE
            using (IContainer container = new Container())
            {
                container.Register(Component.For<IClassA>()
                                            .Use<ClassA1>()
                                            .WithProxy<ClassAProxy>()
                                            .AddBehavior(new ExecuteBehavior())
                                            .Singleton());

                // ACT
                var classA = container.Resolve<IClassA>();
                var classA2 = container.Resolve<IClassA>();

                // ASSERT
                Assert.AreSame(classA, classA2);
            }
        }

        [Test]
        public void CanUseProxy()
        {
            // ARRANGE
            using (IContainer container = new Container())
            {
                container.Register(Component.For<IClassA>()
                                            .Use<ClassA1>()
                                            .WithProxy<ClassAProxy>()
                                            .AddBehavior(new ExecuteBehavior())
                                            .Singleton());

                // ACT
                var classA = container.Resolve<IClassA>();
                var result = classA.SayHello("Ilya");

                // ASSERT
                Assert.IsInstanceOf(typeof(ClassAProxy), classA);
                Assert.AreEqual("Hello from A1, Ilya", result);
            }
        }

        [Test]
        [Ignore ("This functionality is not used, but fails on CI with old mono version (2.10.8). ")]
        public void CanGenerateProxy()
        {
            // ARRANGE
            using (IContainer container = new Container())
            {
                container.Register(Component.For<IClassA>()
                                            .Use<ClassA1>()
                                            .WithProxy(ProxyGen.Generate(typeof(IClassA)))
                                            .AddBehavior(new ExecuteBehavior())
                                            .Singleton());
                // ACT
                var classA = container.Resolve<IClassA>();
                var result = classA.SayHello("Ilya");

                // ASSERT
                Assert.AreEqual("ActionStreetMap.Dynamics.IClassAProxy", classA.GetType().FullName);
                Assert.AreEqual("Hello from A1, Ilya", result);
            }
        }

        [Test]
        public void CanUseGlobalBehavior()
        {
            // ARRANGE
            using (IContainer container = new Container())
            {
                container.Register(Component.For<IClassA>()
                                            .Use<ClassA1>()
                                            .WithProxy<ClassAProxy>()
                                            .Singleton());
                container.AddGlobalBehavior(new ExecuteBehavior());

                // ACT
                var classA = container.Resolve<IClassA>();
                var result = classA.SayHello("Ilya");

                // ASSERT
                Assert.AreEqual("Hello from A1, Ilya", result);
            }
        }

        [Test]
        [Ignore("This feature was disabled after moving ProxyGen class to different package in order to incapsulate conditional compilation symbols in single package.")]
        public void CanAutogenerateProxy()
        {
            // ARRANGE
            using (IContainer container = new Container())
            {
                container.Register(Component.For<IClassA>()
                                            .Use<ClassA1>()
                                            .Singleton());
                container.AllowProxy = true;
                container.AutoGenerateProxy = true;
                container.AddGlobalBehavior(new ExecuteBehavior());

                // ACT
                var classA = container.Resolve<IClassA>();
                var result = classA.SayHello("Ilya");

                // ASSERT
                Assert.AreEqual("ActionStreetMap.Dynamics.IClassAProxy", classA.GetType().FullName);
                Assert.AreEqual("Hello from A1, Ilya", result);
            }
        }

        [Test]
        public void CanUseTraceBehavior()
        {
            // ARRANGE
            using (IContainer container = new Container())
            {
                var sb = new StringBuilder();
                container.Register(Component.For<IClassA>()
                                            .Use<ClassA1>()
                                            .WithProxy<ClassAProxy>()
                                            .AddBehavior(new ExecuteBehavior())
                                            .AddBehavior(new TraceBehavior(new TestStringBuilderTrace(sb)))
                                            .Singleton());

                // ACT
                var classA = container.Resolve<IClassA>();
                classA.SayHello("Ilya");
                classA.Add(10, 20);

                // ASSERT
                Assert.IsNotEmpty(sb.ToString());
            }
        }
    }
}
using System;
using Xamarin.Forms;

namespace MqttClient.Ios
{
	public static class Bootstrapper 
	{
		public static void Setup()
		{
			DependencyService.Register<INetworkStream,IosNetworkStream> ();
			DependencyService.Register<ITcpClient,IosTcpClient> ();
		}
	}
}


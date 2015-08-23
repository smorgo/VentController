using System;
using System.Collections.Generic;
using System.Linq;

using Foundation;
using UIKit;
using MqttClient;
using MqttClient.Core;

using System.Diagnostics;

namespace VentControllerClient.iOS
{
	[Register ("AppDelegate")]
	public partial class AppDelegate : global::Xamarin.Forms.Platform.iOS.FormsApplicationDelegate
	{
		private IMqtt _client;

		public override bool FinishedLaunching (UIApplication app, NSDictionary options)
		{
			global::Xamarin.Forms.Forms.Init ();

			// Code for starting up the Xamarin Test Cloud Agent
			#if ENABLE_TEST_CLOUD
			Xamarin.Calabash.Start();
			#endif

			MqttClient.Ios.Bootstrapper.Setup ();

			LoadApplication (new App ());

			var connectionString = Keys.MQTT_BROKER_CONNECTION;
			var clientId = "iphone";

			// Instantiate client using MqttClientFactory
			_client = MqttClientFactory.CreateClient(connectionString, clientId);

			// Setup some useful client delegate callbacks
			_client.Connected += new ConnectionDelegate(client_Connected);
			_client.ConnectionLost += new ConnectionDelegate(client_ConnectionLost);
			_client.PublishArrived += new PublishArrivedDelegate (client_PublishArrived);

			_client.Connect ();
			_client.Subscribe (
				new Subscription[] { 
					new Subscription ("/status", QoS.BestEfforts),
					new Subscription ("/cmd", QoS.BestEfforts),
					new Subscription ("/event", QoS.BestEfforts),
				
				});
			
			return base.FinishedLaunching (app, options);
		}

		private void client_Connected(object sender, EventArgs e)
		{
		}

		private void client_ConnectionLost(object sender, EventArgs e)
		{
		}

		private bool client_PublishArrived(object sender, PublishArrivedArgs e)
		{
			Debug.WriteLine("Received Message");
			Debug.WriteLine("Topic: " + e.Topic);
			Debug.WriteLine("Payload: " + e.Payload);
			return true;
		}

	}
}

